/*
 * CAN.h
 *
 *  Created on: Oct 18, 2016
 *      Author: kpoon
 */

#ifndef CAN_H_
#define CAN_H_

/* Includes ------------------------------------------------------------------*/
#include "typedefs.h"
#include "global_includes.h"
#include "Support.h"
#include "DAC.h"
#include "ADC.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#define USE_CAN1
//#define USE_CAN2

#ifdef  USE_CAN1
  #define CANx                       CAN1
  #define CAN_CLK                    RCC_APB1Periph_CAN1
  #define CAN_RX_PIN                 GPIO_Pin_8
  #define CAN_TX_PIN                 GPIO_Pin_9
  #define CAN_GPIO_PORT              GPIOB
  #define CAN_GPIO_CLK               RCC_AHB1Periph_GPIOB
  #define CAN_AF_PORT                GPIO_AF_CAN1
  #define CAN_RX_SOURCE              GPIO_PinSource8
  #define CAN_TX_SOURCE              GPIO_PinSource9 /*
  #define CANx                       CAN1
  #define CAN_CLK                    RCC_APB1Periph_CAN1
  #define CAN_RX_PIN                 GPIO_Pin_0
  #define CAN_TX_PIN                 GPIO_Pin_1
  #define CAN_GPIO_PORT              GPIOD
  #define CAN_GPIO_CLK               RCC_AHB1Periph_GPIOD
  #define CAN_AF_PORT                GPIO_AF_CAN1
  #define CAN_RX_SOURCE              GPIO_PinSource0
  #define CAN_TX_SOURCE              GPIO_PinSource1*/

#else /*USE_CAN2*/
  #define CANx                       CAN2
  #define CAN_CLK                    (RCC_APB1Periph_CAN1 | RCC_APB1Periph_CAN2)
  #define CAN_RX_PIN                 GPIO_Pin_5
  #define CAN_TX_PIN                 GPIO_Pin_13
  #define CAN_GPIO_PORT              GPIOB
  #define CAN_GPIO_CLK               RCC_AHB1Periph_GPIOB
  #define CAN_AF_PORT                GPIO_AF_CAN2
  #define CAN_RX_SOURCE              GPIO_PinSource5
  #define CAN_TX_SOURCE              GPIO_PinSource13
#endif  /* USE_CAN1 */

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void LED_Display(uint8_t Ledstatus);

#endif /* __MAIN_H */
/*-----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define KEY_PRESSED     0x00
#define KEY_NOT_PRESSED 0x01

#define GET_COMMAND 0x00
#define GET_TACH 	0x01
#define GET_AD2	 	0x02
#define GET_AD5		0x03
#define GET_APOS	0x04
#define GET_TIME0 	0x05
#define GET_LIMITSW	0x06
#define GET_GOHOME 	0x07

#define CMDReset	0x00800002
#define	CMDAxison	0x00000102
#define CMDAxisoff	0x00000002

#define SetCommand	0x01
#define SetSpeed	0x02
#define SetAvcJog	0x03
#define SetGoHome   0x04

//ADIO CAN BUS COMMAND
#define ADIODigitalOutput 	0x00
#define ADIODigitalInput 	0x01
#define ADIOAnalogOutput 	0x02
#define ADIOAnalogInput		0x03
/*-----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
CAN_InitTypeDef        CAN_InitStructure;
CAN_FilterInitTypeDef  CAN_FilterInitStructure;
CanTxMsg TxMessage;
CanRxMsg RxMessage;



/* Private function prototypes -----------------------------------------------*/
void NVIC_Config(void);
void CAN_Config(void);
void Init_RxMes(CanRxMsg *RxMessage);
void InputToggle(void);

void CANinit(void);
void TechSendCMD(uint8_t axis, uint32_t cmd);
void TechWriteCommand(uint8_t axis, uint16_t location, uint16_t cmd);
short TechReadFeedback(uint8_t axis, uint8_t ID);
long TechReadLongFeedback(uint8_t axis, uint8_t ID);
long TestIn();

//short IOReadDigital(void);
//short IOReadAnalog(uint8_t location);
//void IOWriteDigital(uint16_t data);
//void IODout(uint8_t location, uint16_t data);
//void IOWriteAnalog(uint8_t location, int16_t cmd);
BOOL GetServoEncoder(BYTE uServoSlot);

/* Private functions ---------------------------------------------------------*/


