/* Includes ------------------------------------------------------------------*/
#include "typedefs.h"
#include "GPIO.h"
#include "CAN.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
void GPIO_LowLevel_DeInit(void);
void GPIO_LowLevel_Init(void);

/* Private functions ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/
volatile DWORD _wDigitalInput;
volatile uint16_t CMD[5];
volatile uint16_t TACH[5];
volatile uint32_t APOS[5];

void Init_GPIO(void)
{
	GPIO_LowLevel_Init();
}

/**
  * @brief  DeInitializes GPIO.
  * @param  None
  * @retval None
  */
void DeInit_GPIO(void)
{
	GPIO_LowLevel_DeInit();
}

/**
  * @brief  Initializes the peripherals used by GPIO in/out.
  * @param  None
  * @retval None
  */
void GPIO_LowLevel_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/*!< Enable GPIO clocks */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB |
	RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD |
	RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOF |
	RCC_AHB1Periph_GPIOG | RCC_AHB1Periph_GPIOH |
	RCC_AHB1Periph_GPIOI, ENABLE);

	/*!< SPI pins configuration *************************************************/

	/*!< Configure CS pins in input pushpull mode ********************/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;

	GPIO_InitStructure.GPIO_Pin = M1CURRENT_Pin;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = CUR_DETECTED_Pin;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = TEMP_FAULT_Pin;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = M1FAULT_Pin;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = AUX_IN1_Pin | AUX_IN2_Pin
	        | FOOT_CON_INSTALLED_Pin | EMERG_STOP_Pin | SEQ_START_STOP_Pin;
	GPIO_Init(GPIOG, &GPIO_InitStructure);

	/*!< Configure CS pins in output pushpull mode ********************/
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;

	GPIO_InitStructure.GPIO_Pin = M1MODE1_Pin;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = BOOST_ENA_Pin | CURRENT_ENA_Pin;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = MAN_AS0_Pin | MAN_AS1_Pin | M1DIR_Pin
	        | M1EN_Pin;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GAS_SOLE_ENA1_Pin | GAS_SOLE_ENA2_Pin
	        | EMERG_STOP_OUT_Pin | FAN12_Pin | FAN56_Pin |
	        GNDFLT_RELAY_Pin | ILLUM_Pin | WIRE_SELECT_Pin;
	GPIO_Init(GPIOF, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = AUX_OUT1_Pin | AUX_OUT2_Pin | TRAVEL_ENOUT_Pin
	        | WIRE_ENOUT_Pin | IN_SEQ_Pin;
	GPIO_Init(GPIOG, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = RF_ENABLE_Pin | M1SLEEP_Pin | M1MODE2_Pin;
	GPIO_Init(GPIOH, &GPIO_InitStructure);

}

/**
  * @brief  DeInitializes the peripherals used by the SPI FLASH driver.
  * @param  None
  * @retval None
  */
void GPIO_LowLevel_DeInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/*!< Configure all pins used by the SPI as input floating *******************/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

	GPIO_InitStructure.GPIO_Pin = 0xFF;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = 0xFF;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
}

void DriveBit(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, BOOL status)
{
	if (status)
	{
		GPIO_SetBits(GPIOx, GPIO_Pin);
	}
	else
	{
		GPIO_ResetBits(GPIOx, GPIO_Pin);
	}
}

/*******************************************
  Output all GPIO status
  ******************************************/
void IO_DigitalUpdate(uint32_t wSetbit)
{
	DriveBit(GPIOF, ILLUM_Pin, 			(wSetbit & 0x00000001));	// VERIFIED GOOD
	DriveBit(GPIOF, GAS_SOLE_ENA1_Pin, 	(wSetbit & 0x00000002));	// VERIFIED GOOD
	DriveBit(GPIOF, WIRE_SELECT_Pin, 	(wSetbit & 0x00000004));	// VERIFIED GOOD

	DriveBit(GPIOD, CURRENT_ENA_Pin, 	(wSetbit & 0x00000010)); 	// VERIFIED GOOD
	DriveBit(GPIOG, IN_SEQ_Pin, 		(wSetbit & 0x00000020));	// VERIFIED GOOD
	DriveBit(GPIOG, AUX_OUT1_Pin, 		(wSetbit & 0x00000040));	// VERIFIED GOOD
	DriveBit(GPIOG, AUX_OUT2_Pin, 		(wSetbit & 0x00000080));	// VERIFIED GOOD

	DriveBit(GPIOH, RF_ENABLE_Pin, 		(wSetbit & 0x00000100));	// VERIFIED GOOD
	DriveBit(GPIOD, BOOST_ENA_Pin, 		(wSetbit & 0x00000200));	// VERIFIED GOOD
	DriveBit(GPIOF, EMERG_STOP_OUT_Pin, (wSetbit & 0x00000800));	// VERIFIED GOOD

	DriveBit(GPIOF, FAN12_Pin, 			(wSetbit & 0x00001000));	// VERIFIED GOOD
	DriveBit(GPIOF, FAN56_Pin, 			(wSetbit & 0x00002000));	// VERIFIED GOOD
	DriveBit(GPIOF, GNDFLT_RELAY_Pin, 	(wSetbit & 0x00008000));	// VERIFIED GOOD

	DriveBit(GPIOA, M1MODE1_Pin, 		(wSetbit & 0x00010000));	// VERIFIED GOOD
	DriveBit(GPIOE, MAN_AS0_Pin, 		(wSetbit & 0x00020000));	// VERIFIED GOOD
	DriveBit(GPIOE, MAN_AS1_Pin, 		(wSetbit & 0x00040000));	// VERIFIED GOOD
	DriveBit(GPIOE, M1DIR_Pin, 			(wSetbit & 0x00080000));	// VERIFIED GOOD

	DriveBit(GPIOE, M1EN_Pin, 			(wSetbit & 0x00200000));	// VERIFIED GOOD
	DriveBit(GPIOF, GAS_SOLE_ENA2_Pin, 	(wSetbit & 0x00400000));	// VERIFIED GOOD
	DriveBit(GPIOG, TRAVEL_ENOUT_Pin, 	(wSetbit & 0x00800000));	// VERIFIED GOOD

	DriveBit(GPIOG, WIRE_ENOUT_Pin,		(wSetbit & 0x01000000));	// VERIFIED GOOD
	DriveBit(GPIOH, M1SLEEP_Pin, 		(wSetbit & 0x02000000));	// VERIFIED GOOD
	DriveBit(GPIOH, M1MODE2_Pin, 		(wSetbit & 0x04000000));	// VERIFIED GOOD
}

/*******************************************
  Read all GPIO Input
  ******************************************/
DWORD IO_DigitalInput(void)
{
	DWORD DigitalInput  = 0;


	DWORD isActive = 0;

	//TEST DIGITAL INPUT
	DigitalInput |= GPIO_ReadInputDataBit(GPIOG, FOOT_CON_INSTALLED_Pin) << 0; // TESTED GOOD
	DigitalInput |= GPIO_ReadInputDataBit(GPIOG, AUX_IN2_Pin)            << 1; // TESTED GOOD
	DigitalInput |= GPIO_ReadInputDataBit(GPIOG, AUX_IN1_Pin)            << 2; // TESTED GOOD
	DigitalInput |= GPIO_ReadInputDataBit(GPIOC, M1CURRENT_Pin)    		 << 3; // VERIFIED GOOD

	DigitalInput |= GPIO_ReadInputDataBit(GPIOE, M1FAULT_Pin) 	     	 << 4; // TESTED GOOD
	DigitalInput |= GPIO_ReadInputDataBit(GPIOD, TEMP_FAULT_Pin)   		 << 5; // TESTED GOOD
	DigitalInput |= GPIO_ReadInputDataBit(GPIOC, CUR_DETECTED_Pin) 	     << 6; // TESTED GOOD
	DigitalInput |= GPIO_ReadInputDataBit(GPIOG, SEQ_START_STOP_Pin) 	 << 7;	// TESTED GOOD

	//DigitalInput |= GPIO_ReadInputDataBit(GPIOG, EMERG_STOP_Pin)     	 << 4; // TESTED GOOD
	// Servo 1 Limit Switch 1											 << 17
	// Servo 1 Limit Switch 2											 << 18

	// Servo 2 Limit Switch 1											 << 19
	// Servo 2 Limit Switch 2											 << 20

	// Servo 3 Limit Switch 1											 << 21
	// Servo 3 Limit Switch 2											 << 22

	// Servo 4 Limit Switch 1											 << 23
	// Servo 4 Limit Switch 2											 << 24

	for (uint8_t nAxis = 1; nAxis < 5; ++nAxis)
	{
		isActive = (TechReadFeedback(nAxis, GET_LIMITSW) & 0x000C) >> 2;
		DigitalInput += isActive << (14 + (nAxis *2));
		  TACH[nAxis-1]  = (uint16_t) TechReadFeedback(nAxis, GET_TACH);
		  CMD[nAxis-1] = (uint16_t) TechReadFeedback(nAxis, GET_COMMAND);
		  APOS[nAxis-1] = (uint32_t) TechReadLongFeedback(nAxis, GET_APOS);
		  //outdata.AD2[nAxis-1]  = (uint16_t) TechReadFeedback(s, GET_AD2);
		  //outdata.AD5[nAxis-1]  = (uint16_t) TechReadFeedback(s, GET_LIMITSW);
		  //outdata.AD5[nAxis-1] = (uint16_t) TechReadFeedback(s, GET_AD5);
		  //outdata.TIME0[nAxis-1] = (uint32_t) TechReadLongFeedback(s, GET_TIME0);

	}

	return DigitalInput;
}

void GetDigitalInput(void)
{
	_wDigitalInput = (DWORD)IO_DigitalInput();
}

BOOL GetCurrentState(DWORD dwAddressData)
{
	BOOL bState = __FALSE;

	switch (dwAddressData)
	{
		case FOOT_CON_INSTALLED:    if((_wDigitalInput & 0x00000001) == 0x00000001) bState = __TRUE; break;
		case AUX_IN2:			    if((_wDigitalInput & 0x00000002) == 0x00000002) bState = __TRUE; break;
		case AUX_IN1: 			    if((_wDigitalInput & 0x00000004) == 0x00000004) bState = __TRUE; break;
		case M1CURRENT:				if((_wDigitalInput & 0x00000008) == 0x00000008)	bState = __TRUE; break;

		case M1FAULT:				if((_wDigitalInput & 0x00000010) == 0x00000010)	bState = __TRUE; break;
		case TEMP_FAULT:			if((_wDigitalInput & 0x00000020) == 0x00000020)	bState = __TRUE; break;
		case CUR_DETECTED:			if((_wDigitalInput & 0x00000040) == 0x00000040)	bState = __TRUE; break;
		case SEQ_START_STOP:		if((_wDigitalInput & 0x00000080) == 0x00000080)	bState = __TRUE; break;

		case EMERG_STOP_IN:			if((_wDigitalInput & 0x00000100) == 0x00000100)	bState = __TRUE; break;

		case AXIS1_LIMIT1:			if((_wDigitalInput & 0x00010000) == 0x00010000)	bState = __TRUE; break;
		case AXIS1_LIMIT2:			if((_wDigitalInput & 0x00020000) == 0x00020000)	bState = __TRUE; break;

		case AXIS2_LIMIT1:			if((_wDigitalInput & 0x00040000) == 0x00040000)	bState = __TRUE; break;
		case AXIS2_LIMIT2:			if((_wDigitalInput & 0x00080000) == 0x00080000)	bState = __TRUE; break;

		case AXIS3_LIMIT1:			if((_wDigitalInput & 0x00100000) == 0x00100000)	bState = __TRUE; break;
		case AXIS3_LIMIT2:			if((_wDigitalInput & 0x00200000) == 0x00200000)	bState = __TRUE; break;

		case AXIS4_LIMIT1:			if((_wDigitalInput & 0x00400000) == 0x00400000)	bState = __TRUE; break;
		case AXIS4_LIMIT2:			if((_wDigitalInput & 0x00800000) == 0x00800000)	bState = __TRUE; break;
		default:																			 		 break;
	}

	return bState;
}

WORD GetServoFaults(BYTE uAxis)
{
	DWORD dwLocal;
	BYTE uBitCount;

	dwLocal = _wDigitalInput;

	uBitCount = 14 + (uAxis *2);
	dwLocal = dwLocal >> uBitCount; // Shift right based on Servo Number

	dwLocal &= 0x03; // Mask on the last two bits

	return (WORD)dwLocal;
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
