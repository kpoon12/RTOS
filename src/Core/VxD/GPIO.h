/*
 * GPIO.h
 *
 *  Created on: Aug 2, 2018
 *      Author: edwardp
 */
/* Includes ------------------------------------------------------------------*/
#ifndef CORE_VXD_GPIO_H_
#define CORE_VXD_GPIO_H_

#include "global_includes.h"
#include "typedefs.h"
#include "CAN.h"


// Port A
#define M1CURRENT_Pin				GPIO_Pin_0		//In  PA0
#define M1MODE1_Pin					GPIO_Pin_4		//Out PA4

// Port C
#define CUR_DETECTED_Pin			GPIO_Pin_8 		//In  PC8

// Port D
#define BOOST_ENA_Pin				GPIO_Pin_1 		//Out PD1
#define TEMP_FAULT_Pin   			GPIO_Pin_2 		//In  PD2
#define CURRENT_ENA_Pin				GPIO_Pin_3		//Out PD3

// Port E
#define MAN_AS0_Pin					GPIO_Pin_11		//Out PE11  // Manipulator Address Selection
#define MAN_AS1_Pin					GPIO_Pin_12  	//Out PE12  // Manipulator Address Selection
#define M1DIR_Pin					GPIO_Pin_13		//Out PE13
#define M1FAULT_Pin					GPIO_Pin_14		//in  PE14
#define M1EN_Pin					GPIO_Pin_15		//OUt PE15

// Port F
#define GAS_SOLE_ENA1_Pin		    GPIO_Pin_0		//Out PF0
#define GAS_SOLE_ENA2_Pin			GPIO_Pin_1 		//Out PF1
#define EMERG_STOP_OUT_Pin			GPIO_Pin_2		//Out PF2
#define FAN12_Pin        			GPIO_Pin_3		//Out PF3
#define FAN56_Pin		      		GPIO_Pin_4	 	//Out PF4
#define GNDFLT_RELAY_Pin			GPIO_Pin_5		//Out PF5
#define ILLUM_Pin            		GPIO_Pin_6		//Out PF6
#define WIRE_SELECT_Pin				GPIO_Pin_7		//OUt PF7

// Port G
#define AUX_OUT1_Pin    			GPIO_Pin_0		//Out PG0
#define AUX_OUT2_Pin    			GPIO_Pin_1		//Out PG1
#define TRAVEL_ENOUT_Pin   			GPIO_Pin_2		//Out PG2
#define WIRE_ENOUT_Pin    			GPIO_Pin_3		//Out PG3
#define IN_SEQ_Pin  				GPIO_Pin_4		//Out PG4
#define AUX_IN1_Pin			    	GPIO_Pin_5 		//In  PG5
#define AUX_IN2_Pin			    	GPIO_Pin_6 		//In  PG6
#define FOOT_CON_INSTALLED_Pin		GPIO_Pin_7		//In  PG7
#define EMERG_STOP_Pin				GPIO_Pin_8		//In  PG8
#define SEQ_START_STOP_Pin			GPIO_Pin_9 		//In  PG9

// Port H
#define RF_ENABLE_Pin				GPIO_Pin_7		//Out PH7
#define M1SLEEP_Pin					GPIO_Pin_8		//Out PH8
#define M1MODE2_Pin					GPIO_Pin_9		//Out PH9


//Digital input address
#define FOOT_CON_INSTALLED		0x01
#define AUX_IN2					0x02
#define AUX_IN1					0x03
#define TEMP_FAULT				0x04
#define CUR_DETECTED			0x05
#define M1CURRENT				0x06
#define M1FAULT					0x07
#define EMERG_STOP_IN			0x08
#define SEQ_START_STOP			0x09

#define AXIS1_LIMIT1			0x0A
#define AXIS1_LIMIT2			0x0B
#define AXIS2_LIMIT1			0x0C
#define AXIS2_LIMIT2			0x0D
#define AXIS3_LIMIT1			0x0E
#define AXIS3_LIMIT2			0x0F
#define AXIS4_LIMIT1			0x10
#define AXIS4_LIMIT2			0x11

//Digital output address
#define ILLUM					0x01
#define GAS_SOLE_ENA1			0x02
#define WIRE_SELECT				0x03
#define WHEAD2_SEL				0x04
#define CURRENT_ENA				0x05
#define IN_SEQ					0x06
#define AUX_OUT1				0x07
#define AUX_OUT2				0x08
#define RF_ENABLE				0x09
#define BOOST_ENA				0x0A
#define BLEED_DISABLE			0x0B
#define EMERG_STOP_OUT			0x0C
#define FAN12					0x0D
#define FAN56					0x0E
#define FAN34					0x0F
#define GNDFLT_RELAY			0x10
#define M1MODE1					0x11
#define	MANIP_AD0				0x12
#define	MANIP_AD1				0x13
#define MANIP_DIR				0x14
#define MANIP_ENABLE			0x16
#define	GAS_SOLE_ENA2			0x17
#define TRAVEL_ENOUT			0x18
#define WIRE_ENOUT				0x19
#define	M1SLEEP					0x1A
#define	M1MODE2					0x1B

/* Public function prototypes -----------------------------------------------*/
void  Init_GPIO(void);
void  DeInit_GPIO(void);

void  DriveBit(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, BOOL status);
void  IO_DigitalUpdate(DWORD wSetbit);
DWORD IO_DigitalInput(void);
void  GetDigitalInput(void);
BOOL  GetCurrentState(DWORD dwAddressData);
WORD  GetServoFaults(BYTE uAxis);


#endif /* CORE_VXD_GPIO_H_ */

