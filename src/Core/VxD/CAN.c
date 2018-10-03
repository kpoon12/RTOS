/*
 * CAN.c
 *
 *  Created on: Oct 18, 2016
 *      Author: Edward Poon
 */

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
#include "CAN.h"

uint8_t KeyNumber = 0x0;

WORD IO_AnalogInput(int ix);
extern WORD IO_DigitalInput(void);

void CANinit(void)
{
  /*!< At this stage the microcontroller clock setting is already configured,
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f4xx.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f4xx.c file
     */

  /* NVIC configuration */
  NVIC_Config();

  /* Configure Push button key */
  //STM_EVAL_PBInit(BUTTON_KEY, BUTTON_MODE_GPIO);

  /* CAN configuration */
  CAN_Config();
}

void InputToggle(void)
{
	if (KeyNumber == 0x4)
	{
		KeyNumber = 0x00;
	}
	else
	{
		LED_Display(++KeyNumber);
	}
}



/*
void IO_AnalogOutput(WORD wChannel, WORD wTempValue)
{
	//WORD wOutDac = 0x800 + (wTempValue << 4);
	WORD wOutDac = wTempValue;

	switch(wChannel)
	{
		case 0:
			WriteDacData(PA_AMP_COMMAND, wOutDac);
			break;

		case 1:
			WriteDacData(PA_EXTERNAL_TVL_CMD, wOutDac);
			break;

//		case 2:
//			WriteDacData(PA_TRAV_ENOUT, wOutDac);
//			break;

		case 3:
			WriteDacData(PA_EXTERNAL_WIRE_CMD, wOutDac);
			break;

//		case 4:
//			WriteDacData(PA_WIRE_ENOUT, wOutDac);
//			break;

		default:
			// SHOULD NOT HIT HERE
			break;
	}
}

WORD IO_AnalogInput(int nChannel)
{
	switch(nChannel)
	{
		case FB_CURRENT:
		case FB_PS_VOLTAGE:
		case FB_EXT_STEERING_IN:
		case FB_FOOTCONTROL:
		case FB_SETPOT:
			return ReadADC(nChannel) & 0x0FFF;
			break;

		default:
			break;
	}

	return 0x0FFF;
}

void IOWriteDigital(uint16_t data)
{
	//Remove canbus IO
	uint32_t i = 0;
	uint8_t TransmitMailbox = 0;
	int16_t setdata = data;

	TxMessage.StdId = 0x33;
	TxMessage.ExtId = 0x00000033;
	TxMessage.RTR = CAN_RTR_Data;
	TxMessage.IDE = CAN_ID_EXT;
    TxMessage.DLC = 4;
    TxMessage.Data[0] = ADIODigitalOutput;
    TxMessage.Data[1] = setdata>>8;
    TxMessage.Data[2] = data;
    TxMessage.Data[3] = 0x00;
    TxMessage.Data[4] = 0x00;
    TxMessage.Data[5] = 0x00;
    TxMessage.Data[6] = 0x00;
    TxMessage.Data[7] = 0x00;

    CAN_Transmit(CANx, &TxMessage);
	  i = 0;
	  while((CAN_TransmitStatus(CANx, TransmitMailbox)  !=  CANTXOK) && (i  !=  0xFFFF))
	  {
	    i++;
	  }

}
*/

/*
void IOWriteAnalog(uint8_t location, int16_t cmd)
{
	//Remove canbus IO
	int16_t setcmd = cmd;
	  uint32_t i = 0;
	  uint8_t TransmitMailbox = 0;

	  //Disable CAN IO commnuication
	  return 0;

	TxMessage.StdId = 0x33;
	TxMessage.ExtId = 0x00000033;
	TxMessage.RTR = CAN_RTR_Data;
	TxMessage.IDE = CAN_ID_EXT;
    TxMessage.DLC = 4;
    TxMessage.Data[0] = ADIOAnalogOutput;
    TxMessage.Data[1] = location;
    TxMessage.Data[2] = cmd;
    TxMessage.Data[3] = setcmd>>8;
    TxMessage.Data[4] = 0x00;
    TxMessage.Data[5] = 0x00;
    TxMessage.Data[6] = 0x00;
    TxMessage.Data[7] = 0x00;

    CAN_Transmit(CANx, &TxMessage);
	  i = 0;
	  while((CAN_TransmitStatus(CANx, TransmitMailbox)  !=  CANTXOK) && (i  !=  0xFFFF))
	  {
	    i++;
	  }
}
*/


/*
short IOReadDigital()
{
	//Remove canbus IO
	uint32_t i = 0;
	  uint8_t TransmitMailbox = 0;
	  short* pShortData;
	  short nVelocity = 0;

	  //Disable CAN IO commnuication
	  return 0;

	  TxMessage.StdId = 0x33;
	  TxMessage.ExtId = 0x00000033;

	  TxMessage.RTR = CAN_RTR_Data;
	  TxMessage.IDE = CAN_Id_Extended;
	  TxMessage.DLC = 1;

	  TxMessage.Data[0] = ADIODigitalInput;
	  TxMessage.Data[1] = 0x00;
	  TxMessage.Data[2] = 0x00;
	  TxMessage.Data[3] = 0x00;

	  TxMessage.Data[4] = 0x00;
	  TxMessage.Data[5] = 0x00;
	  TxMessage.Data[6] = 0x00;
	  TxMessage.Data[7] = 0x00;
	  TransmitMailbox = CAN_Transmit(CANx, &TxMessage);

	  i = 0;
	  while((CAN_TransmitStatus(CANx, TransmitMailbox)  !=  CANTXOK) && (i  !=  0xFFFF))
	  {
	    i++;
	  }

	  i = 0;
	  while((CAN_MessagePending(CANx, CAN_FIFO0) < 1) && (i  !=  0xFFFF))
	  {
	    i++;
	  }
      //Delay();

      CAN_Receive(CANx, CAN_FIFO0, &RxMessage);
      if(RxMessage.ExtId == 0x00000227){
   		  pShortData = (short*)&RxMessage.Data[0];
   		  nVelocity = (short)*pShortData;
      }
      return nVelocity;

}
*/
/*
short IOReadAnalog(uint8_t location)
{
	return IO_AnalogInput(location);


	//Remove canbus IO

	uint32_t i = 0;
	  uint8_t TransmitMailbox = 0;
	  short* pShortData;
	  short nVelocity = 0;

  	  TxMessage.StdId = 0x33;
	  TxMessage.ExtId = 0x00000033;

	  TxMessage.RTR = CAN_RTR_Data;
	  TxMessage.IDE = CAN_Id_Extended;
	  TxMessage.DLC = 2;

	  TxMessage.Data[0] = ADIOAnalogInput;
	  TxMessage.Data[1] = location;
	  TxMessage.Data[2] = 0x00;
	  TxMessage.Data[3] = 0x00;

	  TxMessage.Data[4] = 0x00;
	  TxMessage.Data[5] = 0x00;
	  TxMessage.Data[6] = 0x00;
	  TxMessage.Data[7] = 0x00;
	  TransmitMailbox = CAN_Transmit(CANx, &TxMessage);

	  i = 0;
	  while((CAN_TransmitStatus(CANx, TransmitMailbox)  !=  CANTXOK) && (i  !=  0xFFFF))
	  {
	    i++;
	  }

	  i = 0;
	  while((CAN_MessagePending(CANx, CAN_FIFO0) < 1) && (i  !=  0xFFFF))
	  {
	    i++;
	  }
      //Delay();

      CAN_Receive(CANx, CAN_FIFO0, &RxMessage);
      if(RxMessage.ExtId == 0x00000227){
   		  pShortData = (short*)&RxMessage.Data[0];
   		  nVelocity = (short)*pShortData;
      }

      return nVelocity;

}
*/

long TechReadLongFeedback(uint8_t axis, uint8_t location)
{
#ifdef DESKTOP_RUN
	return 0L;
#endif // DESKTOP_RUN

	  uint32_t i = 0;
	  uint8_t TransmitMailbox = 0;
	  long* pLongData;
	  long nPosition = 0;

	  /* transmit */
	  TxMessage.StdId = 0x00;
	  TxMessage.ExtId = 0x16000004 | (uint32_t)(axis<<13);

	  TxMessage.RTR = CAN_RTR_Data;
	  TxMessage.IDE = CAN_ID_EXT;
	  TxMessage.DLC = 4;

	  switch(location)
	  {
		  case GET_APOS:
			  //?APOS
			  TxMessage.ExtId = 0x16000005 | (uint32_t)(axis<<13);
			  TxMessage.Data[0] = 0x00;
			  TxMessage.Data[1] = 0x11;
			  TxMessage.Data[2] = 0x28;
			  TxMessage.Data[3] = 0x02;
			  break;
		  case GET_TIME0:
			  //?APOS
			  TxMessage.ExtId = 0x16000005 | (uint32_t)(axis<<13);
			  TxMessage.Data[0] = 0x00;
			  TxMessage.Data[1] = 0x11;
			  TxMessage.Data[2] = 0xC0;
			  TxMessage.Data[3] = 0x02;
			  break;
	  }

	  TxMessage.Data[4] = 0x00;
	  TxMessage.Data[5] = 0x00;
	  TxMessage.Data[6] = 0x00;
	  TxMessage.Data[7] = 0x00;
	  TransmitMailbox = CAN_Transmit(CANx, &TxMessage);
	  /* receive */
	  i = 0;
	  while((CAN_TransmitStatus(CANx, TransmitMailbox)  !=  CANTXOK) && (i  !=  0xFFFF))
	  {
	    i++;
	  }

	  i = 0;
	  while((CAN_MessagePending(CANx, CAN_FIFO0) < 1) && (i  !=  0xFFFF))
	  {
	    i++;
	  }
    //Delay();

    CAN_Receive(CANx, CAN_FIFO0, &RxMessage);
    if(RxMessage.ExtId == 0x16A20005){
    	pLongData = (long*)&RxMessage.Data[4];
    	nPosition = (long) *pLongData;
    }

    return nPosition;
}

short TechReadFeedback(uint8_t axis, uint8_t location)
{
#ifdef DESKTOP_RUN
	return 0;
#endif // DESKTOP_RUN

	  uint32_t i = 0;
	  uint8_t TransmitMailbox = 0;
	  short* pShortData;
	  short nVelocity = 0;

	  /* transmit */
	  TxMessage.StdId = 0x00;
	  TxMessage.ExtId = 0x16000004 | (uint32_t)(axis<<13);

	  TxMessage.RTR = CAN_RTR_Data;
	  TxMessage.IDE = CAN_ID_EXT;
	  TxMessage.DLC = 4;

	  TxMessage.Data[0] = 0x00;
	  TxMessage.Data[1] = 0x11;
	  switch(location)
	  {
		  case GET_COMMAND:
			  //?COMMAND
			  TxMessage.Data[2] = 0xB0;
			  TxMessage.Data[3] = 0x03;
			  break;

		  case GET_AD5:
			  //?AD5
			  TxMessage.Data[2] = 0x41;
			  TxMessage.Data[3] = 0x02;
			  break;

		  case GET_AD2:
			  //?AD5
			  TxMessage.Data[2] = 0x3E;
			  TxMessage.Data[3] = 0x02;
			  break;

		  case GET_TACH:
			  //?TACH
			  TxMessage.Data[2] = 0xB2;
			  TxMessage.Data[3] = 0x03;
			  break;

		  case GET_LIMITSW:
			  TxMessage.Data[2] = 0xB1;
			  TxMessage.Data[3] = 0x03;
			  break;

		  case GET_GOHOME:
			  TxMessage.Data[2] = 0xB5;
			  TxMessage.Data[3] = 0x03;
			  break;

		  default:
			  break;
	  }

	  TxMessage.Data[4] = 0x00;
	  TxMessage.Data[5] = 0x00;
	  TxMessage.Data[6] = 0x00;
	  TxMessage.Data[7] = 0x00;
	  TransmitMailbox = CAN_Transmit(CANx, &TxMessage);
	/* receive */
	i = 0;
	while ((CAN_TransmitStatus(CANx, TransmitMailbox) != CANTXOK)
	        && (i != 0xFFFF))
	{
		i++;
	}

	i = 0;
	while ((CAN_MessagePending(CANx, CAN_FIFO0) < 1) && (i != 0xFFFF))
	{
		i++;
	}
	//Delay();

      CAN_Receive(CANx, CAN_FIFO0, &RxMessage);
	if (RxMessage.ExtId == 0x16A20004)
	{
		pShortData = (short*) &RxMessage.Data[4];
		nVelocity = (short) *pShortData;
	}

      return nVelocity;
}

void TechWriteCommand(uint8_t axis, uint16_t location, uint16_t cmd)
{
#ifdef DESKTOP_RUN
	return;
#endif // DESKTOP_RUN

	int16_t setcmd = cmd;
	uint32_t i = 0;
	uint8_t TransmitMailbox = 0;

	switch(location)
	{
		  case SetCommand:
				TxMessage.ExtId = 0x040001B0 | (uint32_t)(axis<<13);
			  break;
		  case SetSpeed:
				TxMessage.ExtId = 0x040001B3 | (uint32_t)(axis<<13);
			  break;
		  case SetAvcJog:
				TxMessage.ExtId = 0x040001B4 | (uint32_t)(axis<<13);
			  break;
		  case SetGoHome:
			    TxMessage.ExtId = 0x040001B5 | (uint32_t)(axis<<13);
			  break;
		  default:
			  break;
	}
	TxMessage.RTR = CAN_RTR_Data;
	TxMessage.IDE = CAN_ID_EXT;
    TxMessage.DLC = 4;
    TxMessage.Data[1] = setcmd>>8;
    TxMessage.Data[0] = cmd;
    TxMessage.Data[2] = 0x00;
    TxMessage.Data[3] = 0x00;
    TxMessage.Data[4] = 0x00;
    TxMessage.Data[5] = 0x00;
    TxMessage.Data[6] = 0x00;
    TxMessage.Data[7] = 0x00;

    CAN_Transmit(CANx, &TxMessage);
	  /* receive */
	  i = 0;
	  while((CAN_TransmitStatus(CANx, TransmitMailbox)  !=  CANTXOK) && (i  !=  0xFFFF))
	  {
	    i++;
	  }
}

void TechSendCMD(uint8_t axis, uint32_t cmd)
{
#ifdef DESKTOP_RUN
	return;
#endif // DESKTOP_RUN

	uint32_t i = 0;
	uint8_t TransmitMailbox = 0;

	TxMessage.ExtId = cmd | (uint32_t)(axis << 13);
	TxMessage.RTR = CAN_RTR_Data;
	TxMessage.IDE = CAN_ID_EXT;
	TxMessage.DLC = 0;
	TxMessage.Data[0] = 0x00;
	TxMessage.Data[1] = 0x00;
	TxMessage.Data[2] = 0x00;
	TxMessage.Data[3] = 0x00;
	TxMessage.Data[4] = 0x00;
	TxMessage.Data[5] = 0x00;
	TxMessage.Data[6] = 0x00;
	TxMessage.Data[7] = 0x00;

	CAN_Transmit(CANx, &TxMessage);
	/* receive */
	i = 0;
	while ((CAN_TransmitStatus(CANx, TransmitMailbox) != CANTXOK) && (i != 0xFFFF))
	{
		i++;
	}
}

/**
  * @brief  Configures the CAN.
  * @param  None
  * @retval None
  */
void CAN_Config(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;

  /* CAN GPIOs configuration **************************************************/

  /* Enable GPIO clock */
  RCC_AHB1PeriphClockCmd(CAN_GPIO_CLK, ENABLE);

  /* Connect CAN pins to AF9 */
  GPIO_PinAFConfig(CAN_GPIO_PORT, CAN_RX_SOURCE, CAN_AF_PORT);
  GPIO_PinAFConfig(CAN_GPIO_PORT, CAN_TX_SOURCE, CAN_AF_PORT);

  /* Configure CAN RX and TX pins */
  GPIO_InitStructure.GPIO_Pin = CAN_RX_PIN | CAN_TX_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_Init(CAN_GPIO_PORT, &GPIO_InitStructure);

  /* CAN configuration ********************************************************/
  /* Enable CAN clock */
  RCC_APB1PeriphClockCmd(CAN_CLK, ENABLE);

  /* CAN register init */
  CAN_DeInit(CANx);

  /* CAN cell init */
  CAN_InitStructure.CAN_TTCM = DISABLE;
  CAN_InitStructure.CAN_ABOM = DISABLE;
  CAN_InitStructure.CAN_AWUM = DISABLE;
  CAN_InitStructure.CAN_NART = DISABLE;
  CAN_InitStructure.CAN_RFLM = DISABLE;
  CAN_InitStructure.CAN_TXFP = DISABLE;
  CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
  CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;

  /* CAN Baudrate = 1 MBps (CAN clocked at 30 MHz) */
  CAN_InitStructure.CAN_BS1 = CAN_BS1_6tq;
  CAN_InitStructure.CAN_BS2 = CAN_BS2_8tq;
  CAN_InitStructure.CAN_Prescaler = 4;
  CAN_Init(CANx, &CAN_InitStructure);

  /* CAN filter init */
#ifdef  USE_CAN1
  CAN_FilterInitStructure.CAN_FilterNumber = 0;
#else /* USE_CAN2 */
  CAN_FilterInitStructure.CAN_FilterNumber = 14;
#endif  /* USE_CAN1 */
  CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
  CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
  CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
  CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
  CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
  CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
  CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
  CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
  CAN_FilterInit(&CAN_FilterInitStructure);

  /* Transmit Structure preparation */
  TxMessage.StdId = 0x00;
  TxMessage.ExtId = 0x040021B0;
  TxMessage.RTR = CAN_RTR_Data;
  TxMessage.IDE = CAN_ID_EXT;
  TxMessage.DLC = 8;

  /* Enable FIFO 0 message pending Interrupt */
  CAN_ITConfig(CANx, CAN_IT_FMP0, DISABLE);
}

/**
  * @brief  Configures the NVIC for CAN.
  * @param  None
  * @retval None
  */
void NVIC_Config(void)
{
  NVIC_InitTypeDef  NVIC_InitStructure;

#ifdef  USE_CAN1
  NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
#else  /* USE_CAN2 */
  NVIC_InitStructure.NVIC_IRQChannel = CAN2_RX0_IRQn;
#endif /* USE_CAN1 */

  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

/**
  * @brief  Initializes the Rx Message.
  * @param  RxMessage: pointer to the message to initialize
  * @retval None
  */
void Init_RxMes(CanRxMsg *RxMessage)
{
	uint8_t i = 0;

	RxMessage->StdId = 0x00;
	RxMessage->ExtId = 0x00;
	RxMessage->IDE = CAN_ID_STD;
	RxMessage->DLC = 0;
	RxMessage->FMI = 0;
	for (i = 0; i < 8; i++)
	{
		RxMessage->Data[i] = 0x00;
	}
}

/**
  * @brief  Turn ON/OFF the dedicated led
  * @param  Ledstatus: Led number from 0 to 3.
  * @retval None
  */
void LED_Display(uint8_t Ledstatus)
{
	/* Turn off all leds */
	STM_EVAL_LEDOff(LED1);
	STM_EVAL_LEDOff(LED2);
	STM_EVAL_LEDOff(LED3);
	STM_EVAL_LEDOff(LED4);

	switch (Ledstatus)
	{
		case (1):
			STM_EVAL_LEDOn(LED1);
			break;

		case (2):
			STM_EVAL_LEDOn(LED2);
			break;

		case (3):
			STM_EVAL_LEDOn(LED3);
			break;

		case (4):
			STM_EVAL_LEDOn(LED4);
			break;
		default:
			break;
	}
}

// TODO: This needs to be coded
// Return the Encoder value for the uServoSlot passed in
BOOL GetServoEncoder(BYTE uServoSlot)
{
/*	CAN_msg DiMsg = { 0x16000005,
					{ 0xA1, 0x00, 0x28, 0x02, 0x00, 0x00, 0x00, 0x00 },
						4, 1, EXTENDED_FORMAT, DATA_FRAME };

	BOOL IsSuccess = __FALSE;
	CAN_msg ReceivedMsg;
	CAN_SERVOFDBK* pServoFeedback;
	SERVO_POSITION_STRUCT *sps;

	pServoFeedback = &ServoFeedback[uServoSlot];
	sps = &ServoPosition[uServoSlot];

	if (sps->nEncoderType != (-1))
	{
		ASSERT(uServoSlot < MAX_NUMBER_SERVOS);
		ReceivedMsg.data[0] = 0x00;
		//Get data from Technosoft
		DiMsg.id = DiMsg.id | (uServoSlot << 13);
		CAN_rx_object(1, 0, RX_ID, EXTENDED_FORMAT);
		if (CAN_OK == ExecuteCanMsg(&DiMsg, uServoSlot))
		{
			if (CAN_OK == ReceiveCanMsg(&ReceivedMsg, uServoSlot))
			{
				if (ReceivedMsg.data[0] == (uServoSlot << 4))
				{
					int* pData;
					pData = (int*) &ReceivedMsg.data[4];
					pServoFeedback->nPositionFeedback = *pData;

					IsSuccess = __TRUE;
				}
			}
		}
	}

	return IsSuccess;
*/

	return __FALSE;
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}

#endif

/**
  * @}
  */

/**
  * @}
  */



