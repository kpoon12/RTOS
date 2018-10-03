#ifndef _FAULTLIST_H
#define _FAULTLIST_H

/* 
SERVO FAULT SWITCHES

D7  = Fault Type 1 = Servo Fault, 0 = Other Faults
D6	= Servo Number bit 3
D5	= Servo Number bit 2
D4	= Servo Number bit 1
D3	= Servo Number bit 0
D2	= Switch number bit 1
D1	= Switch number bit 0
D0  = Fault / clear 1= Fault, 0 = Clear

Example:
0x85 = 10000101 = Servo 0,  switch 2, set
0xE6 = 11100110 = Servo 12, switch 3, clear
*/

#define FAULT_SET				0x01
#define SERVO_TYPE_FAULT		0x80

#define SERVO_IS_ENABLE			0x8000
#define SERVO_SWITCH1			0x4000
#define SERVO_SWITCH2			0x2000
#define SERVO_FAULT				0x1000
#define TECHN_FAULT				0x0800


// Support Faults
#define ARC_GAS_FAULT_CLR		0x02
#define ARC_GAS_FAULT_SET		(ARC_GAS_FAULT_CLR + FAULT_SET)

//#define PRICOOL_INSTALLED		0x08
//#define PRICOOL_NOTINSTALLED	(PRICOOL_INSTALLED + FAULT_SET)

//#define PRICOOL_FLOW_CLR		0x0C
//#define PRICOOL_FLOW_SET		(PRICOOL_FLOW_CLR + FAULT_SET)

#define PS_TEMP_CLR				0x12
#define PS_TEMP_SET				(PS_TEMP_CLR + FAULT_SET)

#define GROUND_FLT_CLR			0x14
#define GROUND_FLT_SET			(GROUND_FLT_CLR + FAULT_SET)

#define STUBOUT_FLT_CLR			0x18
#define STUBOUT_FLT_SET			(STUBOUT_FLT_CLR + FAULT_SET)

#define HI_ARC_VOLT_CLR			0x1A
#define HI_ARC_VOLT_SET			(HI_ARC_VOLT_CLR + FAULT_SET)

#define USER_FLT1_CLR			0x1C
#define USER_FLT1_SET			(USER_FLT1_CLR + FAULT_SET)

#define USER_FLT2_CLR			0x1E
#define USER_FLT2_SET			(USER_FLT2_CLR + FAULT_SET)

#define INPUT_AC_CLR			0x38
#define INPUT_AC_SET			(INPUT_AC_CLR + FAULT_SET)

#define EXT_SEQ_START			0x3A
#define EXT_SEQ_STOP			(EXT_SEQ_START + FAULT_SET)

//#define EXT_MAN_ADVANCE_CLR		0x3C
//#define EXT_MAN_ADVANCE			(EXT_MAN_ADVANCE_CLR + FAULT_SET)

#endif // _FAULTLIST_H
