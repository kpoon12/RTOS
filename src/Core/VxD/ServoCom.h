#ifndef __SERVO_COMLIT_H__
#define __SERVO_COMLIT_H__

#include "TypeDefs.h"
//////////////////////////////
// Contains Structures and defines for LIVE AD/IO to Servo communication


// wModeCommand BIT DEFINES
#define USE_ANALOG		(0x00 << 1)
#define USE_DIGITAL		(0x01 << 1)
#define USE_BEMF		(0x02 << 1)
#define USE_FUTURE		(0x03 << 1)
#define USE_CLEARMASK	~USE_FUTURE

#define SLOT_ENABLE				BIT_0 // 1 = Enable 0 = DISABLE
#define SLOT_USE_DIGITAL1		BIT_1 // ANALOG, DIGITAL, BEMF
#define SLOT_USE_DIGITAL2		BIT_2 // ANALOG = 00, DIGITAL = 01, BEMF = 10, FUTURE_EPANSION = 11
#define SLOT_IS_VEL_SERVO		BIT_3 // 1 = Velocity Servo 0 = Position Servo
#define SLOT_FWD_DIRECTION		BIT_4 // Direction 1 = FORWARD 0 = REVERSE 
#define SLOT_AVC_JOG			BIT_5 // AVC Jog / Weld 1 = JOG, 0 = WELD
#define SERVO_COMMAND_BIT		BIT_F // 0 = Normal Update, 1 = SPECIAL COMMAND STRUCTURE FLAG
// 	BIT_8 thru BIT_E Represents the command number
// Note: When BIT 15 is set, bits 0 thru 14 represent the COMMAND NUMBER
// Normal Update Commands, and special servo commands use the same length structure

// Define one per servo
// Dumped Every 10milliSeconds
typedef struct tagCAN_SERVOCMD 
{
	BYTE  uServoSlot;			// Servo ID
	WORD  wModeCommand;			// Use bits 0 - 15
	short nCommandPort;			// Can be Velocity or Position Command
	short nCurrentLimitPort;	// Current Limt
	short nGainPort;			// Gain Scales Encoder feedback into an Analog Voltage (May have to be eliminated)
	short nResponsePort;		// Used only for AVC Response, may be enhanced for all servos
} CAN_SERVOCMD;

/*
// Definitons for wUpdateRequired
#define SERVO_ENABLE		BIT_0
#define SERVO_DIRECTION		BIT_1
#define SERVO_FUNCTION		BIT_2
#define AVC_SPECIAL			BIT_3
#define SPEED_UPDATE		BIT_4
#define CRNTLIMIT_UPDATE	BIT_5
#define GAIN_UPDATE			BIT_6
#define RESP_UPDATE			BIT_7
*/

// uFaults BIT DEFINES
#define LIMIT_SWITCH1			BIT_7 // Switch #1
#define LIMIT_SWITCH2			BIT_6 // Switch #2
#define CAN_BUS_FAIL			BIT_5 // Can Buss Failure
#define TECHNOSOFT_FAULT		BIT_4 // Technosoft faults
#define SMARTFAULT1				BIT_3 // Reserved for Technosoft faults
#define SMARTFAULT2				BIT_2 // Reserved for Technosoft faults
#define SMARTFAULT3				BIT_1 // Reserved for Technosoft faults
#define SMARTFAULT4				BIT_0 // Reserved for Technosoft faults
// BIT_8 thru BIT_F reserved for future expansion

// Define one per servo
// Dumped Every 10milliSeconds
typedef struct tagCAN_SERVOFDBK {
	WORD wFaults;			// Use bits 0 - 4
	WORD wPastServoFault;
	WORD wVelocityFeedback;	// Can be Analog Velocity or Position Feedback
	int  nPositionFeedback;	// Current Limt
	WORD wSumCheck;			// Sum Check for structure
} CAN_SERVOFDBK;

#endif // __SERVO_COMLIT_H__

