/**
  ******************************************************************************
  * @file    app_ethernet.c
  * @author  MCD Application Team
  * @version V1.3.0
  * @date    14-January-2013
  * @brief   This file contains the Ethernet applications process.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "fs.h"
#include "string.h"
#include "app_ethernet.h"
#include "mod_ethernet.h"
#include "mod_ethernet_config.h"
#include "mod_system.h"

#include "typedefs.h"
#include "pkeymap.h"
#include "InitWeldStruct.h"
#include "Support.h"
#include "Weldloop.h"
#include "CAN.h"
#include "ADC.h"
#include "DAC.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define MAX_DHCP_TRIES 5
#define IPCAM_TIME_WAITING_FOR_INPUT	((portTickType) 100)
#define LPBYTE BYTE *
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/***************************** Netconn variables ******************************/
struct netif xnetif;
extern ETHERNET_SETTINGS_TypeDef  EthCfg;
extern xTaskHandle  DHCP_Task_Handle;
extern __IO uint8_t EthLinkStatus;
extern xTaskHandle Task_Handle;
extern IW_INITWELD		stInitWeldSched;
extern BOOL				_isAppLoaded;
uint16_t ao  = 0x800;
extern uint16_t IO_DigitalOutput;
extern uint16_t CMD[5];
extern uint16_t TACH[5];
extern uint32_t APOS[5];

extern uint16_t CMD[5];
extern DWORD IO_DigitalInput(void);
extern void IO_DigitalUpdate(uint32_t wSetbit);
extern void ProcessMessage(LPBYTE pInbound, WORD wBytesReceived);

extern void SendQueueData( struct netconn *conn );
extern void QueuePendantKey(BYTE uKey);
extern void fnPendantServoJog(BYTE uKey, int nJogSpeed);
extern void SendPendantSelection(void);
extern BYTE  uPendantScrollIndex;
//extern BYTE uPendantMode;
extern GRAPH_INFO		GraphInfo[MAX_GRAPH_PORTS];
extern IW_INITWELD		stInitWeldSched;				// in Support.c
extern PD_PENDANTINFO	_stPendantInfo;					// in Support.c

//extern WORD _wDigialInput;
//extern void QueueEventMessage(BYTE uServoNum, BYTE uEventMsg);
//extern WORD GetFeedbackADIO(BYTE uChannel);
//extern SERVO_VALUES			 ServoStruct[MAX_NUMBER_SERVOS];
//extern BYTE uNumGraphPorts;
//extern BYTE uPendantSelectionStr[20];
//extern WORD wCurrentFeedback[8];
extern SYSTEM_STRUCT  WeldSystem;
extern DWORD _wDigitalInput;

/**************************** Web server variables ****************************/
uint32_t nPageHits = 0;
uint32_t IPaddress;
extern ETHERNET_SettingsTypeDef  EthernetSettings;
extern xTaskHandle HTTP_Task_Handle;
extern xTaskHandle GUI_Task_Handle;


int rxdata1;
int rxdata2;
int rxdata3;
int rxdata4;

SYSTEM_CONFIG_TypeDef DC_Global_Config;
/* Private function prototypes -----------------------------------------------*/

static void DynAPIsingle(struct netconn *conn);
static void DynAPImulti(struct netconn *conn);
static void DynData(struct netconn *conn);
static void DynLib(struct netconn *conn);
static void http_ipcam_serve(struct netconn *conn);
static void http_ipcam_thread(void *arg);


/* Private functions ---------------------------------------------------------*/
typedef struct datastr{
	uint8_t inputcmd;
	uint16_t inputlen;
	uint16_t inputsum;

    uint16_t TACH[4];
	uint16_t CMD[4];
	uint16_t AD2[4];
	uint16_t AD5[4];

	uint32_t APOS[4];
	uint32_t TIME0[4];

	uint16_t AnalogInput[8];
	uint16_t AnalogOutput[8];
	uint32_t DigitalInput;
	uint32_t DigitalOutput;

}IN_DATA;
static IN_DATA outdata;

typedef struct testd{
	uint16_t current;
	uint16_t travel;
	uint16_t wirefeed;
	uint16_t avc;
	uint16_t osc;
	uint16_t time;
}OUT_DATA;
static OUT_DATA testdata;

BOOL socketOpen = TRUE;
BOOL socketOpenIn = TRUE;
/*******************************************************************************
                            Web Server Section 
*******************************************************************************/
/**
  * @brief serve tcp connection  
  * @param conn: pointer on connection structure 
  * @retval None
  */
static void http_ipcam_serve(struct netconn *conn)
{
  struct netbuf *inbuf;
  char* buf;
  uint16_t buflen;
  struct fs_file * file;
  uint8_t setpage = 0;

 /* Read the data from the port, blocking if nothing yet there. 
   We assume the request (the part we care about) is in one netbuf */
  inbuf = netconn_recv(conn);

  if (inbuf != NULL)
  {
    if (netconn_err(conn) == ERR_OK)
    {
      netbuf_data(inbuf, (void**)&buf, &buflen);

      /* Is this an HTTP GET command? (only check the first 5 chars, since
      there are other formats for GET, and we're keeping it very simple )*/
      if ((buflen >=5) && (strncmp(buf, "GET /", 5) == 0))
      {
          setpage = 0;
          if((strncmp(buf, "GET /index.html", 15) == 0) || (strncmp(buf, "GET / ", 6) == 0))
          {
              file = fs_open("/index.html");
              netconn_write(conn, (const unsigned char*)(file->data), (size_t)file->len, NETCONN_NOCOPY);
              fs_close(file);
          }
          else if((strncmp(buf, "GET /PB_SEQ_START", 17) == 0))
       	  {
       		  QueuePendantKey(PB_SEQ_START);
       		  setpage = 1;
       	  }
       	  else if((strncmp(buf, "GET /PB_ALL_STOP", 16) == 0))
       	  {
       		  QueuePendantKey(PB_ALL_STOP);
       		  setpage = 1;
       	  }
       	  else if((strncmp(buf, "GET /PB_SEQ_STOP", 16) == 0))
       	  {
       		  QueuePendantKey(PB_SEQ_STOP);
       		  setpage = 1;
       	  }
          else if((strncmp(buf, "GET /PB_WELD_TEST", 17) == 0))
          {
         	  QueuePendantKey(PB_WELD_TEST);
              setpage = 1;
          }
          else if((strncmp(buf, "GET /PB_MAN_PURGE", 17) == 0))
          {
        	  QueuePendantKey(PB_MAN_PURGE);
        	  setpage = 1;
          }
          else if((strncmp(buf, "GET /PB_LAMP", 12) == 0))
          {
        	  QueuePendantKey(PB_LAMP);
        	  setpage = 1;
          }
          else if((strncmp(buf, "GET /PB_MAN_ADVI", 16) == 0))
          {
        	  QueuePendantKey(PB_MAN_ADV);
          	  QueuePendantKey(PB_INCREMENT);
        	  setpage = 1;
          }
          else if((strncmp(buf, "GET /PB_MAN_ADVD", 16) == 0))
          {
        	  QueuePendantKey(PB_MAN_ADV);
        	  QueuePendantKey(PB_DECREMENT);
        	  setpage = 1;
          }
          else if((strncmp(buf, "GET /PB_OSC_MAN", 15) == 0))
          {
        	  QueuePendantKey(PB_OSC_MAN);
        	  setpage = 1;
          }
          else if((strncmp(buf, "GET /PB_TVL_MODE", 16) == 0))
          {
         	  QueuePendantKey(PB_TVL_MODE);
         	  setpage = 1;
          }
          else if((strncmp(buf, "GET /PB_WIRE_ON_OFF", 19) == 0))
          {
         	  QueuePendantKey(PB_WIRE_ON_OFF);
         	  setpage = 1;
          }
          else if((strncmp(buf, "GET /PB_PREWRAP", 15) == 0))
          {
        	  QueuePendantKey(PB_PREWRAP);
           	  setpage = 1;
          }

          else if((strncmp(buf, "GET /PB_TVL_CW_JOG", 18) == 0))
          {
              fnPendantServoJog(PB_TVL_CW_JOG, (-100));
           	  setpage = 3;
          }
          else if((strncmp(buf, "GET /PB_TVL_CCW_JOG", 19) == 0))
          {
         	  fnPendantServoJog(PB_TVL_CW_JOG, (100));
         	  setpage = 3;
          }
          else if((strncmp(buf, "GET /PB_TVL_CW_ZERO", 19) == 0))
          {
         	  fnPendantServoJog(PB_TVL_CW_JOG, 0);
         	  setpage = 3;
          }

          else if((strncmp(buf, "GET /PB_WFD_FD_JOG", 18) == 0))
          {
        	  fnPendantServoJog(PB_WFD_FD_JOG, (100));
        	  setpage = 3;
          }
          else if((strncmp(buf, "GET /PB_WFD_RT_JOG", 18) == 0))
          {
         	  fnPendantServoJog(PB_WFD_FD_JOG, (-100));
         	  setpage = 3;
          }
          else if((strncmp(buf, "GET /PB_WFD_FD_ZERO", 19) == 0))
          {
        	  fnPendantServoJog(PB_WFD_FD_JOG, 0);
           	  setpage = 3;
          }

          else if((strncmp(buf, "GET /PB_AVC_UP_JOG", 18) == 0))
          {
        	  fnPendantServoJog(PB_AVC_UP_JOG, (100));
          	  setpage = 3;
          }
          else if((strncmp(buf, "GET /PB_AVC_DN_JOG", 18) == 0))
          {
        	  fnPendantServoJog(PB_AVC_UP_JOG, (-100));
          	  setpage = 3;
          }
          else if((strncmp(buf, "GET /PB_AVC_ZERO", 16) == 0))
          {
        	  fnPendantServoJog(PB_AVC_UP_JOG, 0);
        	  setpage = 3;
          }

       	  else if((strncmp(buf, "GET /PB_AVC_RESP", 16) == 0))
      	  {
      		  QueuePendantKey(PB_AVC_RESP);
       		  setpage = 2;
    	  }
       	  else if((strncmp(buf, "GET /PB_PLS_TIME", 16) == 0))
          {
       		  QueuePendantKey(PB_PLS_TIME);
       		  setpage = 2;
          }
          else if((strncmp(buf, "GET /PB_AMPS", 12) == 0))
          {
          	  QueuePendantKey(PB_AMPS);
          	  setpage = 2;
          }
          else if((strncmp(buf, "GET /PB_AVC", 11) == 0))
          {
          	  QueuePendantKey(PB_AVC);
          	  setpage = 2;
          }
         else if((strncmp(buf, "GET /PB_EXC", 11) == 0))
         {
        	 QueuePendantKey(PB_EXC);
        	 setpage = 2;
         }
         else if((strncmp(buf, "GET /PB_DWL", 11) == 0))
         {
         	  QueuePendantKey(PB_DWL);
         	  setpage = 2;
         }
         else if((strncmp(buf, "GET /PB_WIRE", 12) == 0))
         {
         	 QueuePendantKey(PB_WIRE);
         	 setpage = 2;
          }
          else if((strncmp(buf, "GET /PB_TVL", 11) == 0))
          {
        	  QueuePendantKey(PB_TVL);
           	  setpage = 2;
       	  }
      	  else if((strncmp(buf, "GET /PB_DECREMENT", 17) == 0))
       	  {
       		  QueuePendantKey(PB_DECREMENT);
       		  setpage = 2;
       	  }
      	  else if((strncmp(buf, "GET /PB_INCREMENT", 17) == 0))
       	  {
       		  QueuePendantKey(PB_INCREMENT);
       		  setpage = 2;
      	  }
          else if((strncmp(buf, "GET /f_jog.html", 15) == 0))
          {
              setpage = 3;
          }
          else if((strncmp(buf, "GET /f_update.html", 18) == 0))
          {
              setpage = 2;
          }
          else if((strncmp(buf, "GET /f_weld.html", 16) == 0))
          {
              setpage = 1;
          }
          else if((strncmp(buf, "GET /graph.html", 15) == 0) )
          {
              file = fs_open("/graph.html");
              netconn_write(conn, (const unsigned char*)(file->data), (size_t)file->len, NETCONN_NOCOPY);
              fs_close(file);
          }
          else if((strncmp(buf, "GET /f_parameter.html", 21) == 0) )
          {
              file = fs_open("/f_parameter.html");
              netconn_write(conn, (const unsigned char*)(file->data), (size_t)file->len, NETCONN_NOCOPY);
              fs_close(file);
          }
          else if((strncmp(buf, "GET /get_progressgraph.html", 27) == 0))
          {
        	  setpage = 6;
          }
          else if((strncmp(buf, "GET /get_status.html", 20) == 0))
          {
              file = fs_open("/get_status.html");
              netconn_write(conn, (const unsigned char*)(file->data), (size_t)file->len, NETCONN_NOCOPY);
              fs_close(file);
          }
          else if((strncmp(buf, "GET /get_parameter.html", 23) == 0) )
          {
              //setpage = 4;
          }
          else if((strncmp(buf, "GET /keypad.html", 16) == 0))
          {
              //file = fs_open("/keypad.html");
              //netconn_write(conn, (const unsigned char*)(file->data), (size_t)file->len, NETCONN_NOCOPY);
              //fs_close(file);
              setpage = 4;
          }
          else if((strncmp(buf, "GET /library.html?select=", 25) == 0))
          {
        	  int x = buf[25] - '0';
    		  int y = buf[26] - '0';
    		  if(y > -1)
    		  {
    			  x = x*10 + y;
    		  }
    		  if(x > -1)
    		  {
    			  uPendantScrollIndex = x;
    			  //uPendantMode = PENDANT_MODE_LIB;
    			  //SendPendantSelection();
           		  file = fs_open("/library.html");
           		  netconn_write(conn, (const unsigned char*)(file->data), (size_t)file->len, NETCONN_NOCOPY);
           		  fs_close(file);
           		  setpage = 5;
    		  }
          }
          else if((strncmp(buf, "GET /library.html", 17) == 0))
          {
       		  file = fs_open("/library.html");
       		  netconn_write(conn, (const unsigned char*)(file->data), (size_t)file->len, NETCONN_NOCOPY);
       		  fs_close(file);
       		  setpage = 5;
          }
          else if((strncmp(buf, "GET /meter.html", 15) == 0))
          {
              file = fs_open("/meter.html");
              netconn_write(conn, (const unsigned char*)(file->data), (size_t)file->len, NETCONN_NOCOPY);
              fs_close(file);
              setpage = 4;
          }

          else if((strncmp(buf, "GET /APIsingle", 14) == 0))
          {
              setpage = 6;
          }

          else if((strncmp(buf, "GET /APImulti", 13) == 0))
          {
              setpage = 7;
          }

     	  //vTaskDelay(200);
     	  	  switch(setpage)
         	  {
         	  	  case 1:
         	  	  {
                      file = fs_open("/f_weld.html");
                      netconn_write(conn, (const unsigned char*)(file->data), (size_t)file->len, NETCONN_NOCOPY);
                      fs_close(file);
         	  		  break;
         	  	  }
         	  	  case 2:
         	  	  {
                      file = fs_open("/f_update.html");
                      netconn_write(conn, (const unsigned char*)(file->data), (size_t)file->len, NETCONN_NOCOPY);
                      fs_close(file);
         	  		  break;
         	  	  }
         	  	  case 3:
         	  	  {
                      file = fs_open("/f_jog.html");
                      netconn_write(conn, (const unsigned char*)(file->data), (size_t)file->len, NETCONN_NOCOPY);
                      fs_close(file);
                      break;
         	  	  }
         	  	  case 4:
             	  	  DynData(conn);
         	  		  break;
         	  	  case 5:
             	  	  DynLib(conn);
         	  		  break;
         	  	  case 6:
             		  DynAPIsingle(conn);
         	  		  break;
         	  	  case 7:
             		  DynAPImulti(conn);
         	  		  break;
         	  	  default:
         	  		  break;
         	  }
        }
      }
    }

  netconn_close(conn);
  netconn_delete(conn);
  netbuf_delete(inbuf);
}

static void GuiRequest( struct netconn *conn ) {
	  struct netbuf *inbuf;
	  uint8_t* buf;
	  uint8_t connerr;
	  uint16_t buflen;
	  uint8_t tmpInfo5[20];
	  connerr = 0;

      inbuf = netconn_recv(conn);


	  if (inbuf != NULL)
	  {
		connerr = netconn_err(conn);
	    if (connerr == ERR_OK)
	    {
	    	  buflen = 0;
			  netbuf_data(inbuf, (void**)&buf, &buflen);
			  if(buflen > 0)
			  {
			      STM_EVAL_LEDToggle(LED2);
				  LPBYTE pData = (LPBYTE)& buf[0];

				  if(pData[0] == 0xAA)//TEST PYTHON CODE
				  {
					  ao += 0x1;
					  if(ao > 0xFFFF)
						  ao = 0x0;
					  outdata.DigitalInput = ao;
					  testdata.current = ao;
					  testdata.travel = ao;
					  testdata.wirefeed = ao;
					  testdata.avc = ao;
					  testdata.osc = ao;
					  testdata.time = ao;

					  netconn_write( conn, &testdata, (uint16_t) sizeof(testdata), NETCONN_COPY );
				  }
				  else if(pData[0] == 0xBB)//TEST WPF CODE
				  {
					  memcpy(&outdata, pData, sizeof(IN_DATA));
					  ao += 0x10;
					  if(ao > 0xFFF)
						  ao = 0x0;
					  outdata.TIME0[0]++;

/*
					  outdata.DigitalInput = _wDigitalInput;
					  outdata.DigitalOutput = IO_DigitalOutput;
*/

 					  outdata.DigitalInput = IO_DigitalInput();
					  IO_DigitalUpdate(outdata.DigitalOutput);

 					  WriteDacData(PA_AMP_COMMAND,  0x800 + (outdata.AnalogOutput[0] * 20));
					  WriteDacData(PA_GAS_MFC_CMD,  0x800 + (outdata.AnalogOutput[1] * 20));
					  WriteDacData(PA_BOOSTER_VOLT,  0x800 + (outdata.AnalogOutput[2] * 20));
					  WriteDacData(PA_SPARE_OUT1,  0x800 + (outdata.AnalogOutput[3] * 20));

					  WriteDacData(PA_CURRENT_CMD_HIGH,  0x800 + (outdata.AnalogOutput[4] * 20));
					  WriteDacData(PA_CURRENT_CMD_LOW, 0x800 + (outdata.AnalogOutput[5] * 20));
					  WriteDacData(PA_EXTERNAL_TVL_CMD, 0x800 + (outdata.AnalogOutput[6] * 20));
					  WriteDacData(PA_EXTERNAL_WIRE_CMD, 0x800 + (outdata.AnalogOutput[7] * 20));

					  outdata.AnalogInput[4] = (uint16_t) GetFeedbackADIO(FB_FOOTCONTROL);
					  outdata.AnalogInput[5] = (uint16_t) GetFeedbackADIO(FB_EXT_STEERING_IN);
					  outdata.AnalogInput[6] = (uint16_t) GetFeedbackADIO(FB_SPARE_IN1);
					  outdata.AnalogInput[7] = (uint16_t) GetFeedbackADIO(FB_SPARE_IN2);

					  outdata.AnalogInput[0] = rxdata1 = (uint16_t) GetFeedbackADIO(FB_CURRENT);
					  outdata.AnalogInput[1] = (uint16_t) GetFeedbackADIO(FB_GAS_MFC);
					  outdata.AnalogInput[2] = (uint16_t) GetFeedbackADIO(FB_SETPOT);
					  outdata.AnalogInput[3] = rxdata2 = (uint16_t) GetFeedbackADIO(FB_PS_VOLTAGE);

					  //for(BYTE s = 1; s < stInitWeldSched.uNumberOfServos; s ++)
					  for(BYTE s = 1; s < 5; s ++)
					  {
						  if(outdata.CMD[s-1] == 0)
						  {
							TechSendCMD(s, CMDAxisoff);
						  }
						  else
						  {
							TechSendCMD(s, CMDAxison);
						  }
						  if(s == 4)//OSC
						  {
							  TechWriteCommand(s, SetCommand, (outdata.CMD[s-1]+4000)*40);
						  }
						  else
						  {
							  TechWriteCommand(s, SetCommand, outdata.CMD[s-1]*40);
						  }

						  outdata.CMD[s-1] = CMD[s-1];
						  outdata.TACH[s-1]  = TACH[s-1];
						  outdata.APOS[s-1] = APOS[s-1];



						  //outdata.TACH[s-1]  = (uint16_t) TechReadFeedback(s, GET_TACH);
						  //outdata.CMD[s-1] = (uint16_t) TechReadFeedback(s, GET_COMMAND);
						  //outdata.AD2[s-1]  = (uint16_t) TechReadFeedback(s, GET_AD2);
						  //outdata.AD5[s-1]  = (uint16_t) TechReadFeedback(s, GET_LIMITSW);
						  //outdata.AD5[s-1] = (uint16_t) TechReadFeedback(s, GET_AD5);
						  //outdata.APOS[s-1] = (uint32_t) TechReadLongFeedback(s, GET_APOS);
						  //outdata.TIME0[s-1] = (uint32_t) TechReadLongFeedback(s, GET_TIME0);
					  }

					  netconn_write( conn, &outdata, (uint16_t) sizeof(outdata), NETCONN_COPY );
				  }
				  else if(pData[0] >= WELD_XMIT_START && pData[0] <= LAST_COMMAND ){
				      STM_EVAL_LEDToggle(LED4);
					  ProcessMessage(buf, buflen);
					  //SendQueueData(conn);
					  netconn_write( conn, &outdata, (uint16_t) sizeof(outdata), NETCONN_COPY );
				  }
			  }
			  //vTaskDelay(100);
			  netbuf_delete(inbuf);

	    }
	    else
	    {
		    netconn_close(conn);
			netconn_delete(conn);
	    	socketOpen = FALSE;
	    	outdata.DigitalInput =0xFFFF;
	    	outdata.DigitalOutput = 0x0000;
 		    netconn_write( conn, &outdata, (uint16_t) sizeof(outdata), NETCONN_COPY );
 	        sprintf((char *)tmpInfo5, "Socket: %d", (int)connerr);
	    }
	  }
	  else
	  {
	    	socketOpen = FALSE;
	    	outdata.DigitalInput = 0x0000;
	    	outdata.DigitalOutput = 0xFFFF;
 		    netconn_write( conn, &outdata, (uint16_t) sizeof(outdata), NETCONN_COPY );
 		    if(inbuf == NULL)
 		    {
 			    netconn_close(conn);
 				netconn_delete(conn);
 		    	socketOpen = FALSE;
 		    }
 		    else
 		    {
 		    }
	  }
}

static void gui_thread(void *arg)
{
	  struct netconn *conn, *newconn;
	  static __IO uint8_t connflag = 0;


	  conn = NULL;
	  if(connflag == 0)
	  {
	    /* Create a new TCP connection handle */
	    conn = netconn_new(NETCONN_TCP);

	    /* Bind to port 80 (HTTP) with default IP address */
	    netconn_bind(conn, NULL, 700);

	    /* Put the connection into LISTEN state */
	    netconn_listen(conn);
	    connflag = 1;
	  }


	  /* Infinite loop */
	  for ( ;;)
	  {
	    /* Accept any icoming connection */
	    newconn = netconn_accept(conn);

	    socketOpen = TRUE;
	    if(newconn != NULL)
	    {
	    	while(socketOpen){
	    		GuiRequest(newconn);
	    	}
	    }
	  }

}


/**
  * @brief  http server thread
  * @param arg: pointer on argument(not used here)
  * @retval None
  */

static void http_ipcam_thread(void *arg)
{
  struct netconn *conn, *newconn;
  //static __IO uint8_t connflag = 0;

  //if(connflag == 0)
  //{
    /* Create a new TCP connection handle */
    conn = netconn_new(NETCONN_TCP);

    /* Bind to port 80 (HTTP) with default IP address */
    netconn_bind(conn, NULL, 180);

    /* Put the connection into LISTEN state */
    netconn_listen(conn);
  //  connflag = 1;
  //}

  /* Infinite loop */
  for ( ;;)
  {
    /* Accept any icoming connection */
    newconn = netconn_accept(conn);

    if(newconn != NULL)
    {

    	/* Serve the icoming connection */
    	http_ipcam_serve(newconn);
    }
  }
}

/**
  * @brief  Initialize the HTTP server (start its thread) 
  * @param  None
  * @retval None
  */
void http_init()
{
  nPageHits = 0;

  if(GUI_Task_Handle == NULL)
  {
	GUI_Task_Handle = sys_thread_new("HTTP1", gui_thread, NULL, HTTP_THREAD_STACK_SIZE, HTTP_THREAD_PRIO);
  }

  if(HTTP_Task_Handle == NULL)
  {
    HTTP_Task_Handle = sys_thread_new("HTTP0", http_ipcam_thread, NULL, 5 * HTTP_THREAD_STACK_SIZE, HTTP_THREAD_PRIO + 2);
  }
}

///**
//  * @brief  Create and send a dynamic Web Page. This page contains the list of
//  *         running tasks and the number of page hits.
//  * @param  conn pointer on connection structure
//  * @retval None
//  */
//static void DynDataPage(struct netconn *conn)
//{
//	  portCHAR PAGE_BODY[50];
//	  portCHAR pagehits[10] = {0};
//	  nPageHits++;
//	  memset(PAGE_BODY, 0, 50);
//
//	  strcat((char *)PAGE_BODY, "<input type='text' id='txtSpeed' name='txtSpeed' value='");
//	  sprintf(pagehits, "%d' />", (int)nPageHits);
//	  strcat(PAGE_BODY, pagehits);
//	  netconn_write(conn, PAGE_BODY, strlen(PAGE_BODY), NETCONN_COPY);
//
////  strcat((char *)PAGE_BODY, "<pre><br>Name          State  Priority  Stack   Num" );
////  strcat((char *)PAGE_BODY, "<br>---------------------------------------------<br>");
////
////  /* The list of tasks and their status */
////  vTaskList((signed char *)PAGE_BODY + strlen(PAGE_BODY));
////  strcat((char *)PAGE_BODY, "<br><br>---------------------------------------------");
////  strcat((char *)PAGE_BODY, "<br>B : Blocked, R : Ready, D : Deleted, S : Suspended<br>");
//}

static char* GetLabel(int ix)
{
	GRAPH_INFO *pGraphInfo;
	char* label = "P1";

	pGraphInfo = &GraphInfo[ix];
	switch(pGraphInfo->wPort)
	{
		case FB_SLOT0:
			label = "CURRENT";
			break;
		case FB_SLOT1:
			label = "TRAVEL";
			break;
		case FB_SLOT4:
			label = "OSC";
			break;
		case FB_SLOT3:
			label = "AVC";
			break;
		case FB_SLOT2:
			label = "WIREFEED";
			break;
		case FB_SLOT5:
			label = "JOG VELOCITY";
			break;
		case FB_SLOT6:
			label = "JOG POSITION";
			break;
		case FB_PS_VOLTAGE:
			label = "VOLTAGE";
			break;
		case FB_GAS_MFC:
			label = "GAS MFC";
			break;
		default:
			label = "NO LABEL";
			break;
	}
	return label;
}

static void DynAPIsingle(struct netconn *conn)
{
  uint8_t nServos;
  char* label2;
  portCHAR PAGE_BODY[1000];
  portCHAR temp_string[30] = {0};
  portCHAR json[1000];
  memset(json, 0, 1000);
  memset(PAGE_BODY, 0, 1000);
  nPageHits++;

  //Create JSON string
  sprintf(temp_string, "{\"NumPages\": %d", (int)nPageHits);
  strcat(json, temp_string);
  sprintf(temp_string, ",\"NumServos\": %d", (int)mNumServos);
  strcat(json, temp_string);
  sprintf(temp_string, ",\"NumOfLevels\": %d", (int)mNumberOfLevels);
  strcat(json, temp_string);
  sprintf(temp_string, ",\"LastLevel\": %d", (int)mLastLevel);
  strcat(json, temp_string);
  sprintf(temp_string, ",\"Prepurge\": %d", (int)stInitWeldSched.stOneLevel.wPrepurge);
  strcat(json, temp_string);
  sprintf(temp_string, ",\"Posturge\": %d", (int)stInitWeldSched.stOneLevel.wPostpurge);
  strcat(json, temp_string);
  sprintf(temp_string, ",\"Upslope\": %d", (int)stInitWeldSched.stOneLevel.wUpslope);
  strcat(json, temp_string);
  sprintf(temp_string, ",\"Downslope\": %d", (int)stInitWeldSched.stOneLevel.wDownslope);
  strcat(json, temp_string);
  sprintf(temp_string, ",\"StartLevel\": %d", (int)stInitWeldSched.stOneLevel.wStartLevel);
  strcat(json, temp_string);
  sprintf(temp_string, ",\"TouchStartCurrent\": %d", (int)stInitWeldSched.stOneLevel.wTouchStartCurrent);
  strcat(json, temp_string);
  sprintf(temp_string, ",\"StartMode\": %d", (int)stInitWeldSched.stOneLevel.uStartMode);
  strcat(json, temp_string);
  sprintf(temp_string, ",\"LevelAdvanceMode\": %d", (int)stInitWeldSched.stOneLevel.uLevelAdvanceMode);
  strcat(json, temp_string);
  sprintf(temp_string, ",\"StuboutMode\": %d", (int)stInitWeldSched.stOneLevel.uStuboutMode);
  strcat(json, temp_string);
  sprintf(temp_string, ",\"StartAttemtps\": %d", (int)stInitWeldSched.stOneLevel.uStartAttemtps);
  strcat(json, temp_string);
  for (nServos = 0; nServos < stInitWeldSched.uNumberOfServos; ++nServos)
  {
	  label2 = GetLabel(nServos);
	  sprintf(temp_string, ",\"%s_StartDelay\": %d", label2, (int)stInitWeldSched.stOneLevel.stServoOneLevel[nServos].wStartDelay);
	  strcat(json, temp_string);
	  sprintf(temp_string, ",\"%s_StopDelay\": %d",  label2, (int)stInitWeldSched.stOneLevel.stServoOneLevel[nServos].nStopDelay);
	  strcat(json, temp_string);
	  sprintf(temp_string, ",\"%s_Retract\": %d",  label2, (int)stInitWeldSched.stOneLevel.stServoOneLevel[nServos].wRetract);
	  strcat(json, temp_string);
	  sprintf(temp_string, ",\"%s_MaxSteeringDac\": %d", label2,  (int)stInitWeldSched.stOneLevel.stServoOneLevel[nServos].wMaxSteeringDac);
	  strcat(json, temp_string);
	  sprintf(temp_string, ",\"%s_MinSteeringDac\": %d",  label2, (int)stInitWeldSched.stOneLevel.stServoOneLevel[nServos].wMinSteeringDac);
	  strcat(json, temp_string);
  }

  strcat((char *)json, "}\r\n" );


  //Create Reple string
  strcat((char *)PAGE_BODY, "HTTP/1.1 200 OK\r\n");
  strcat((char *)PAGE_BODY, "Content-Type: text/html; charset=utf-8\r\n");

  sprintf(temp_string, "Content-Length: %d\r\n", (int)strlen(json));
  strcat(PAGE_BODY, temp_string);

  //strcat((char *)PAGE_BODY, "Content-Length: 45\r\n");// 105\r\n");
  strcat((char *)PAGE_BODY, "Access-Control-Allow-Origin: *\r\n\r\n");

  strcat(PAGE_BODY, json);

  netconn_write(conn, PAGE_BODY, strlen(PAGE_BODY), NETCONN_COPY);
}

static void DynAPImulti(struct netconn *conn)
{
  uint8_t nServos, nLevels;
  char* label2;
  portCHAR PAGE_BODY[3000];
  portCHAR temp_string[30] = {0};
  portCHAR json[3000];
  memset(json, 0, 3000);
  memset(PAGE_BODY, 0, 3000);
  nPageHits++;

  nLevels = (int)WeldSystem.uWeldLevel;

  //Create JSON string
  sprintf(temp_string, "{\"LevelTime\": %d", (int)stInitWeldSched.stMultiLevel[nLevels].dwLevelTime);
  strcat(json, temp_string);
  sprintf(temp_string, ",\"SlopeStartTime\": %d", (int)stInitWeldSched.stMultiLevel[nLevels].dwSlopeStartTime);
  strcat(json, temp_string);
  sprintf(temp_string, ",\"SlopeTime\": %d", (int)stInitWeldSched.stMultiLevel[nLevels].dwSlopeTime);
  strcat(json, temp_string);
  sprintf(temp_string, ",\"LevelPosition\": %d", (int)stInitWeldSched.stMultiLevel[nLevels].nLevelPosition);
  strcat(json, temp_string);
  sprintf(temp_string, ",\"BckPulseTime\": %d", (int)stInitWeldSched.stMultiLevel[nLevels].wBckPulseTime);
  strcat(json, temp_string);
  sprintf(temp_string, ",\"PriPulseTime\": %d", (int)stInitWeldSched.stMultiLevel[nLevels].wPriPulseTime);
  strcat(json, temp_string);
  sprintf(temp_string, ",\"uWeldLevel\": %d", (int)WeldSystem.uWeldLevel);
  strcat(json, temp_string);

  for (nServos = 0; nServos < stInitWeldSched.uNumberOfServos; ++nServos)
  {
	  label2 = GetLabel(nServos);
	  sprintf(temp_string, ",\"%s_BacExternalDac\": %d", label2, (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].dwBacExternalDac);
	  strcat(json, temp_string);
	  sprintf(temp_string,  ",\"%s_OwgInc\": %d", label2, (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].dwOwgInc);
	  strcat(json, temp_string);
	  sprintf(temp_string,  ",\"%s_PriExternalDac\": %d", label2, (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].dwPriExternalDac);
	  strcat(json, temp_string);
	  sprintf(temp_string, ",\"%s_OscSpiralInc\": %d", label2, (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].lOscSpiralInc);
	  strcat(json, temp_string);
	  sprintf(temp_string, ",\"%s_BackgroundValue\": %d", label2, (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].nBackgroundValue);
	  strcat(json, temp_string);
	  sprintf(temp_string, ",\"%s_Direction\": %d", label2, (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].uDirection);
	  strcat(json, temp_string);
	  sprintf(temp_string, ",\"%s_MotorSelect\": %d", label2, (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].uMotorSelect);
	  strcat(json, temp_string);
	  sprintf(temp_string, ",\"%s_OscSpecialMode\": %d", label2, (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].uOscSpecialMode);
	  strcat(json, temp_string);
	  sprintf(temp_string, ",\"%s_PulseMode\": %d", label2, (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].uPulseMode);
	  strcat(json, temp_string);
	  sprintf(temp_string, ",\"%s_ExcursionTime\": %d", label2, (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].wExcursionTime);
	  strcat(json, temp_string);
	  sprintf(temp_string, ",\"%s_InDwellTime\": %d", label2, (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].wInDwellTime);
	  strcat(json, temp_string);
	  sprintf(temp_string, ",\"%s_OutDwellTime\": %d", label2, (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].wOutDwellTime);
	  strcat(json, temp_string);
	  sprintf(temp_string, ",\"%s_PriPulseDelay\": %d", label2, (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].wPriPulseDelay);
	  strcat(json, temp_string);
	  sprintf(temp_string, ",\"%s_PrimaryValue\": %d", label2, (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].wPrimaryValue);
	  strcat(json, temp_string);
	  sprintf(temp_string, ",\"%s_Response\": %d", label2, (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].wResponse);
	  strcat(json, temp_string);
  }


  strcat((char *)json, "}\r\n" );


  //Create Reple string
  strcat((char *)PAGE_BODY, "HTTP/1.1 200 OK\r\n");
  strcat((char *)PAGE_BODY, "Content-Type: text/html; charset=utf-8\r\n");

  sprintf(temp_string, "Content-Length: %d\r\n", (int)strlen(json));
  strcat(PAGE_BODY, temp_string);

  //strcat((char *)PAGE_BODY, "Content-Length: 45\r\n");// 105\r\n");
  strcat((char *)PAGE_BODY, "Access-Control-Allow-Origin: *\r\n\r\n");

  strcat(PAGE_BODY, json);

  netconn_write(conn, PAGE_BODY, strlen(PAGE_BODY), NETCONN_COPY);
}

static void DynLib(struct netconn *conn)
{
  int ix;
  portCHAR PAGE_BODY[1000];
  portCHAR pagehits[10] = {0};
  memset(PAGE_BODY, 0, 1000);

  strcat((char *)PAGE_BODY, "<select id=lib size=5 onchange=SelectSchedule()>" );
  for(ix=0; ix < 16; ix++)
  {
	  if((strncmp(_stPendantInfo.LibStrings[ix], "               ", 15) != 0))
	  {
		  strcat((char *)PAGE_BODY, "<option value=");
		  sprintf(pagehits, "%d >", (int)ix);
		  strcat(PAGE_BODY, pagehits);
		  BufferLcdDisplay(_stPendantInfo.LibStrings[ix]);
		  strcat((char *)PAGE_BODY, GetLedDisplay());
		  strcat((char *)PAGE_BODY, "---");
		  BufferLcdDisplay(_stPendantInfo.IDStrings[ix]);
		  strcat((char *)PAGE_BODY, GetLedDisplay());
		  strcat((char *)PAGE_BODY, "</option>" );
	  }
  }
  strcat((char *)PAGE_BODY, "</select>" );

  netconn_write(conn, PAGE_BODY, strlen(PAGE_BODY), NETCONN_COPY);
}

static void DynData(struct netconn *conn)
{
  uint8_t nServos, nLevels ;
  char* label2;
  portCHAR PAGE_BODY[15000];
  portCHAR pagehits[50] = {0};
  nPageHits++;
  memset(PAGE_BODY, 0, 15000);

  strcat((char *)PAGE_BODY, "<table><tr><td>");
  strcat((char *)PAGE_BODY, "<div id='txtSpeed' style='visibitity:hidden'>");
  sprintf(pagehits, "%d </div> %d </br> %d </br> %d </br> %d </br>", (int)rxdata1, (int)rxdata2, (int)mLastLevel, (int)mNumberOfLevels, (int)mNumServos);
  strcat(PAGE_BODY, pagehits);
  sprintf(pagehits, "<br/>Prepurge: %d", (int)stInitWeldSched.stOneLevel.wPrepurge);
  strcat(PAGE_BODY, pagehits);
  sprintf(pagehits, "<br/>Posturge: %d", (int)stInitWeldSched.stOneLevel.wPostpurge);
  strcat(PAGE_BODY, pagehits);
  sprintf(pagehits, "<br/>Upslope: %d", (int)stInitWeldSched.stOneLevel.wUpslope);
  strcat(PAGE_BODY, pagehits);
  sprintf(pagehits, "<br/>Downslope: %d", (int)stInitWeldSched.stOneLevel.wDownslope);
  strcat(PAGE_BODY, pagehits);
  sprintf(pagehits, "<br/>StartLevel: %d", (int)stInitWeldSched.stOneLevel.wStartLevel);
  strcat(PAGE_BODY, pagehits);
  sprintf(pagehits, "<br/>TouchStartCurrent: %d", (int)stInitWeldSched.stOneLevel.wTouchStartCurrent);
  strcat(PAGE_BODY, pagehits);

  sprintf(pagehits, "<br/>StartMode: %d", (int)stInitWeldSched.stOneLevel.uStartMode);
  strcat(PAGE_BODY, pagehits);
  sprintf(pagehits, "<br/>LevelAdvanceMode: %d", (int)stInitWeldSched.stOneLevel.uLevelAdvanceMode);
  strcat(PAGE_BODY, pagehits);
  sprintf(pagehits, "<br/>StuboutMode: %d", (int)stInitWeldSched.stOneLevel.uStuboutMode);
  strcat(PAGE_BODY, pagehits);
  sprintf(pagehits, "<br/>StartAttemtps: %d", (int)stInitWeldSched.stOneLevel.uStartAttemtps);
  strcat(PAGE_BODY, pagehits);
  for (nServos = 0; nServos < stInitWeldSched.uNumberOfServos; ++nServos)
  {
	  label2 = GetLabel(nServos);
	  sprintf(pagehits, "<br/>%s", label2);
	  strcat(PAGE_BODY, pagehits);
	  sprintf(pagehits, "<br/>StartDelay: %d", (int)stInitWeldSched.stOneLevel.stServoOneLevel[nServos].wStartDelay);
	  strcat(PAGE_BODY, pagehits);
	  sprintf(pagehits, "<br/>StopDelay: %d", (int)stInitWeldSched.stOneLevel.stServoOneLevel[nServos].nStopDelay);
	  strcat(PAGE_BODY, pagehits);
	  sprintf(pagehits, "<br/>Retract: %d", (int)stInitWeldSched.stOneLevel.stServoOneLevel[nServos].wRetract);
	  strcat(PAGE_BODY, pagehits);
	  sprintf(pagehits, "<br/>MaxSteeringDac: %d", (int)stInitWeldSched.stOneLevel.stServoOneLevel[nServos].wMaxSteeringDac);
	  strcat(PAGE_BODY, pagehits);
	  sprintf(pagehits, "<br/>MinSteeringDac: %d", (int)stInitWeldSched.stOneLevel.stServoOneLevel[nServos].wMinSteeringDac);
	  strcat(PAGE_BODY, pagehits);
  }
  strcat((char *)PAGE_BODY, "</td>");

  //Write Label
  strcat((char *)PAGE_BODY, "<td>");
  strcat((char *)PAGE_BODY, "<br/><div>Level:");
  strcat((char *)PAGE_BODY, "<br/>LevelTime:");
  strcat((char *)PAGE_BODY, "<br/>SlopeStartTime:");
  strcat((char *)PAGE_BODY, "<br/>SlopeTime:");
  strcat((char *)PAGE_BODY, "<br/>LevelPosition:");
  strcat((char *)PAGE_BODY, "<br/>BckPulseTime:");
  strcat((char *)PAGE_BODY, "<br/>PriPulseTime:");

  for (nServos = 0; nServos < stInitWeldSched.uNumberOfServos; ++nServos)
  {
	  label2 = GetLabel(nServos);
	  sprintf(pagehits, "<br/>%s", label2);
	  strcat(PAGE_BODY, pagehits);

	  strcat((char *)PAGE_BODY, "<br/>BacExternalDac:");
	  strcat((char *)PAGE_BODY, "<br/>OwgInc:");
	  strcat((char *)PAGE_BODY, "<br/>PriExternalDac:");
	  strcat((char *)PAGE_BODY, "<br/>OscSpiralInc:");
	  strcat((char *)PAGE_BODY, "<br/>BackgroundValue:");
	  strcat((char *)PAGE_BODY, "<br/>Direction:");
	  strcat((char *)PAGE_BODY, "<br/>MotorSelect:");
	  strcat((char *)PAGE_BODY, "<br/>OscSpecialMode:");
	  strcat((char *)PAGE_BODY, "<br/>PulseMode:");
	  strcat((char *)PAGE_BODY, "<br/>ExcursionTime:");
	  strcat((char *)PAGE_BODY, "<br/>InDwellTime:");
	  strcat((char *)PAGE_BODY, "<br/>OutDwellTime:");
	  strcat((char *)PAGE_BODY, "<br/>PriPulseDelay:");
	  strcat((char *)PAGE_BODY, "<br/>PrimaryValue:");
	  strcat((char *)PAGE_BODY, "<br/>Response:");
  }
  strcat((char *)PAGE_BODY, "</td>");

  for (nLevels = 0; nLevels < stInitWeldSched.uNumberOfLevels; ++nLevels)
  {
	  strcat((char *)PAGE_BODY, "<td>");
	  sprintf(pagehits, "<br/>%d", nLevels);
	  strcat(PAGE_BODY, pagehits);
	  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].dwLevelTime);
	  strcat(PAGE_BODY, pagehits);
	  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].dwSlopeStartTime);
	  strcat(PAGE_BODY, pagehits);
	  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].dwSlopeTime);
	  strcat(PAGE_BODY, pagehits);
	  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].nLevelPosition);
	  strcat(PAGE_BODY, pagehits);
	  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].wBckPulseTime);
	  strcat(PAGE_BODY, pagehits);
	  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].wPriPulseTime);
	  strcat(PAGE_BODY, pagehits);
	  for (nServos = 0; nServos < stInitWeldSched.uNumberOfServos; ++nServos)
	  {
		  sprintf(pagehits, "<br/><br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].dwBacExternalDac);
		  strcat(PAGE_BODY, pagehits);
		  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].dwOwgInc);
		  strcat(PAGE_BODY, pagehits);
		  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].dwPriExternalDac);
		  strcat(PAGE_BODY, pagehits);
		  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].lOscSpiralInc);
		  strcat(PAGE_BODY, pagehits);
		  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].nBackgroundValue);
		  strcat(PAGE_BODY, pagehits);
		  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].uDirection);
		  strcat(PAGE_BODY, pagehits);
		  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].uMotorSelect);
		  strcat(PAGE_BODY, pagehits);
		  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].uOscSpecialMode);
		  strcat(PAGE_BODY, pagehits);
		  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].uPulseMode);
		  strcat(PAGE_BODY, pagehits);
		  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].wExcursionTime);
		  strcat(PAGE_BODY, pagehits);
		  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].wInDwellTime);
		  strcat(PAGE_BODY, pagehits);
		  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].wOutDwellTime);
		  strcat(PAGE_BODY, pagehits);
		  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].wPriPulseDelay);
		  strcat(PAGE_BODY, pagehits);
		  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].wPrimaryValue);
		  strcat(PAGE_BODY, pagehits);
		  sprintf(pagehits, "<br/>%d", (int)stInitWeldSched.stMultiLevel[nLevels].stServoMultiLevel[nServos].wResponse);
		  strcat(PAGE_BODY, pagehits);
	  }
	  strcat((char *)PAGE_BODY, "</td>");
  }

  strcat((char *)PAGE_BODY, "</tr></table>");

  netconn_write(conn, PAGE_BODY, strlen(PAGE_BODY), NETCONN_COPY);
}


/*******************************************************************************
                            Netconn Section 
*******************************************************************************/
/**
  * @brief  Initializes the lwIP stack
  * @param  None
  * @retval None
  */
void LwIP_Init(void)
{
  struct ip_addr ipaddr;
  struct ip_addr netmask;
  struct ip_addr gw;
  uint8_t iptxt[20];

  /* Create tcp_ip stack thread */
  tcpip_init(NULL, NULL);

  /* IP address setting */
  MOD_GetParam(3, &EthCfg.d32);

  if(EthCfg.b.DHCPEnable == 1)
  {
    ipaddr.addr = 0;
    netmask.addr = 0;
    gw.addr = 0;
  }
  else
  {
    IP4_ADDR(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
    IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
    IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);

    sprintf((char*)iptxt, "%d.%d.%d.%d", IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
    ETHERNET_UpdateDHCPState(6,iptxt);
  }

  low_level_MAC_init();

  netif_add(&xnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

  /*  Registers the default network interface. */
  netif_set_default(&xnetif);

  /* When the netif is fully configured this function must be called.*/
  netif_set_up(&xnetif);
}

/**
  * @brief  LwIP_DHCP_Process_Handle
  * @param  None
  * @retval None
  */
void LwIP_DHCP_task(void * pvParameters)
{
  struct ip_addr ipaddr;
  struct ip_addr netmask;
  struct ip_addr gw;

  uint8_t iptab[4];
  uint8_t iptxt[20];
  uint8_t index = 0;
  uint8_t DHCP_state;  
  DHCP_state = DHCP_START;

  for (;;)
  {
    switch (DHCP_state)
    {
      case DHCP_START:
      {
        dhcp_start(&xnetif);
        IPaddress = 0;
        DHCP_state = DHCP_WAIT_ADDRESS;
        ETHERNET_UpdateDHCPState(0,NULL);
        ETHERNET_UpdateIcon(ethernet_dhcp_icon);
      }
      break;

      case DHCP_WAIT_ADDRESS:
      {
        /* Read the new IP address */
        IPaddress = xnetif.ip_addr.addr;
        index++;
        if(index == 4)
        {
          index = 0;
        }
        ETHERNET_UpdateDHCPState(index,NULL);

        if (IPaddress!=0)
        {
          DHCP_state = DHCP_ADDRESS_ASSIGNED;	

          /* Stop DHCP */
          dhcp_stop(&xnetif);

          iptab[0] = (uint8_t)(IPaddress >> 24);
          iptab[1] = (uint8_t)(IPaddress >> 16);
          iptab[2] = (uint8_t)(IPaddress >> 8);
          iptab[3] = (uint8_t)(IPaddress);

          sprintf((char*)iptxt, "%d.%d.%d.%d", iptab[3], iptab[2], iptab[1], iptab[0]);
          ETHERNET_UpdateDHCPState(4,iptxt);
          ETHERNET_UpdateIcon(ethernet_conn_icon);

          vTaskPrioritySet(Task_Handle, (configMAX_PRIORITIES - 7));//5//7
          /* End of DHCP process */
          DHCP_Task_Handle = NULL;
          vTaskDelete(NULL);
        }
        else
        {
          /* DHCP timeout */
          if (xnetif.dhcp->tries > MAX_DHCP_TRIES)
          {
            DHCP_state = DHCP_TIMEOUT;

            /* Stop DHCP */
            dhcp_stop(&xnetif);

            /* Static address used */
            IP4_ADDR(&ipaddr, IP_ADDR0 ,IP_ADDR1 , IP_ADDR2 , IP_ADDR3 );
            IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
            IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
            netif_set_addr(&xnetif, &ipaddr , &netmask, &gw);

            sprintf((char*)iptxt, "%d.%d.%d.%d", IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
            ETHERNET_UpdateDHCPState(5,iptxt);
            ETHERNET_UpdateIcon(ethernet_conn_icon);

            vTaskPrioritySet(Task_Handle, (configMAX_PRIORITIES - 7));//5
            /* End of DHCP process */
            DHCP_Task_Handle = NULL;
            vTaskDelete(NULL);
          }
        }
        if(EthLinkStatus != 0)
        {
          ETHERNET_DHCPFailState();
          /* End of DHCP process */
          DHCP_Task_Handle = NULL;
          vTaskDelete(NULL);
        }
      }
      break;
      default: break;
    }

    /* Wait 250 ms */
    vTaskDelay(250);
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
