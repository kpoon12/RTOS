/* W32INTF.C */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"

// Global include files
#include "typedefs.h"
#include "pkeymap.h"
#include "InitWeldStruct.h"

// Local include files
#include "Support.h"
#include "weldloop.h"
#include "Queue.h"
#include "util.h"
#include "gl_mgr.h"
//
// This is the data that is shared between the app and the vxd
//
DWORD		dwVxdFlags = 0; // !W32IF_MSG_FLAG_LIB_STR || !W32IF_MSG_FLAG_ARC_ON
MESSAGE_QUEUE    MessageQueue;
BYTE		_isGuiConnected = 0;
BOOL        _bIsArcOn    = __FALSE;

extern void fnWeldCommands(LPCWELDCOMMAND);	// in Weldloop.c
extern IW_INITWELD		stInitWeldSched;				// in Support.c
extern PD_PENDANTINFO	_stPendantInfo;					// in Support.c

// Local Variables
TS_FILEBUFFER _stTechnoServoArray;
extern WORD _wServoInitDelay;  // Delay enabling the OSC

void BumpLeds(void)
{
	static WORD wTickCount = 0;

	++wTickCount;

	if (wTickCount & 0x40)
	{
		//GPIO_WriteBit(GPIO4, BLINK,  Bit_SET);
	}
	else
	{
		//GPIO_WriteBit(GPIO4, BLINK,  Bit_RESET);
	}
}

BOOL Toggle = __FALSE;
void MasterInterruptServiceRoutine( void )
{                     
	if(Toggle)
	{  
		Toggle = __FALSE;
	}
	else
	{
		Toggle = __TRUE;
	}
	
	BumpLeds();
	//os_evt_set(FIRE_HANDLER, _gHandler);
}



/////////////////////////////////////////////////////////////////////////////////
//
BOOL IsArcOn(void)
{    
	return _bIsArcOn;
}

void SetArcOn(BOOL bState)
{    
	_bIsArcOn = bState;
}

static void WeldCommand(WORD nCommand, WORD wLength, void *pDataArray)
{
    WELDCOMMAND wc;

	ASSERT(wLength < sizeof(wc.uByteData));

	wc.uCommand  = (BYTE)nCommand;
	wc.wLength   = wLength;
	if (wLength <= 64)
	{
		if (pDataArray != NULL)
		{
			memcpy(wc.uByteData, pDataArray, wLength);
		}
		fnWeldCommands((LPCWELDCOMMAND)&wc);
	}
}

BOOL ExecuteClientMessage(LPCBYTE uInboundBuffer)
{
	WORD   uCommand = *((LPWORD)&uInboundBuffer[0]);
	WORD   nLen     = *((LPWORD)&uInboundBuffer[2]);
	LPBYTE pData	= (LPBYTE)&uInboundBuffer[4];

//	printf("ExecuteClientMessage::uCommand %04X\n", uCommand);

	switch(uCommand)
	{
		case WELD_XMIT_START:
			// Clear Memory
			//It cause osc moving.........
			memset(&stInitWeldSched, 0, sizeof(IW_INITWELD));
			break;

		case SCHED_PARMS_DATA:
			ASSERT(nLen == sizeof(IW_SCHEDULEPARMS));
			memcpy(&stInitWeldSched.stScheduleParms, pData, sizeof(IW_SCHEDULEPARMS));
			break;
		
		case ONE_LEVEL_DATA:
			ASSERT(nLen == sizeof(IW_ONELEVEL));
			memcpy(&stInitWeldSched.stOneLevel, pData, nLen );
			break;
		
		case LEVEL_DATA:
			{
				IW_INITWELDLEVEL *pWL;
				ASSERT(nLen == sizeof(IW_INITWELDLEVEL));
				pWL = (IW_INITWELDLEVEL *)pData;
				memcpy(&stInitWeldSched.stMultiLevel[ pWL->dwLevelNum ], &pWL->stLevelData, sizeof(IW_MULTILEVEL));
			}
			break;
		
		case SCHED_DATAAQI_DATA:
			ASSERT(nLen == sizeof(IW_DATA_AQI));
			memcpy(&stInitWeldSched.stDataAqi, pData, nLen );
			break;
		
		case WELD_HEAD_DATA:
			ASSERT(nLen == sizeof(IW_WELDHEADTYPEDEF));
			memcpy(&stInitWeldSched.stWeldHeadTypeDef, pData, nLen );
			break;
		
		case WELD_XMIT_DONE:
			WeldCommand(AMI_SET_WELDSCHED, 0, (LPBYTE)NULL);
			break;
		
		case NUM_LEVELS:
			ASSERT(nLen == 2);
			stInitWeldSched.uNumberOfLevels = (BYTE)*pData;
 			stInitWeldSched.uNumberOfServos = (BYTE)*(pData+1);
			break;
		
		case WRITE_LIB_STRINGS:
			memcpy(&_stPendantInfo, pData, sizeof(PD_PENDANTINFO));
			break;

		case WRITE_LED_STRING:
			BufferLcdDisplay((const char*)pData);
			break;
				
		case WRITE_PDBITMAP:
			ASSERT(nLen == sizeof(PD_BITMAP));
			BufferBitmapDisplay((const char*)pData);
			break;

		case SINGLE_MULTILEVEL_DATA:
			ASSERT(nLen == sizeof(IW_INITWELDLEVEL));
			{
				IW_INITWELDLEVEL *pWL = (IW_INITWELDLEVEL *)pData;
				DWORD dwLevel            = pWL->dwLevelNum;
				printf("Received: %d, Expected: %d\n", nLen, sizeof(IW_INITWELDLEVEL));
				IW_MULTILEVEL *pLevelSrc =  &pWL->stLevelData;
				IW_MULTILEVEL *pLevelDst =  &stInitWeldSched.stMultiLevel[dwLevel];

				memcpy(pLevelDst, pLevelSrc, sizeof(IW_MULTILEVEL) );
				WeldCommand(ONE_LEVEL_UPDATE, 0, (LPBYTE)NULL);
			}
			break;

		case XMIT_SERVODEF_START:
			{
				const TS_HEADERINFO* pHeaderInfo;
				memset((void*)&_stTechnoServoArray, 0, sizeof(_stTechnoServoArray));  // Clear Memory
	
				// Move File Header Information
				pHeaderInfo = (const TS_HEADERINFO*)pData;
				_stTechnoServoArray.uServoIndex = pHeaderInfo->uServoIndex;
				_stTechnoServoArray.uServoSlot  = pHeaderInfo->uServoSlot;
				_stTechnoServoArray.wFileLength = pHeaderInfo->wFileLength;
				
				memset((void*)&_stTechnoServoArray.uBuffer, 0x0C, _stTechnoServoArray.wFileLength+5);  // Clear Buffer
			}
			break;

		case XMIT_ONESERVODEF:
			{
				const TS_TRANSFERPACKET* pPacketData;
				BYTE *pTo;
				const BYTE *pFrom;
				WORD wAddress;

				pPacketData = (const TS_TRANSFERPACKET*)pData;
				wAddress = pPacketData->uThisBlock * TS_BUFFER_SIZE;
				pTo      = &_stTechnoServoArray.uBuffer[wAddress];
				pFrom    =  pPacketData->uBuffer;

				memcpy(pTo, pFrom, TS_BUFFER_SIZE);
			}
			break;

		case XMIT_SERVODEF_DONE:
			{
				//os_evt_set(BURN_SERVO, _gProgramServo);
			}
			break;


		default:
			WeldCommand(uCommand, (WORD)nLen, (LPBYTE)pData);
			//ETHERNET_Print("Default           ");
			break;
	}

	return __TRUE;
}

BOOL TestCommandBuffer(LPCBYTE pCommandBuffer)
{
	WORD wCommand;
	int nLenght;
	LPCWORD pWordValue;
	
	pWordValue = (LPCWORD)&pCommandBuffer[0];
	wCommand = *pWordValue;

	if ((wCommand < WELD_XMIT_START) || 
		(wCommand > LAST_COMMAND))
	{
//		printf("TestCommandBuffer:: Bad Command 0x%04X\n", wCommand);
		return __FALSE;
	}

	pWordValue = (LPCWORD)&pCommandBuffer[2];
	nLenght = (int)*pWordValue;
	if (nLenght > 0x220)
	{
//		printf("TestCommandBuffer:: Bad Data Length 0x%04X\n", nLenght);
		return __FALSE;
	}

	return __TRUE;  // ALL TESTS PASSED
}

void ProcessMessage(LPCBYTE pInbound, WORD wBytesReceived)
{
	// [ COMMAND ]  [ LENGTH  ]  [ Data Buffer ]
	//  [0] - [1],   [2] - [3],   [4] - [0x206]

	if (TestCommandBuffer(pInbound))
	{
		ExecuteClientMessage(pInbound);
	}
}


