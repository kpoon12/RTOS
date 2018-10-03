//
// M327.h
//

#ifndef __AMI_M327_H
#define __AMI_M327_H

#include "typedefs.h"

#define M_PI        3.14279265358979323846

// Data Aqi Event defines
#define SYSTEM_EVENT 0x0F

#define INTS_PER_SECOND			((float)100.0)				 // Interrupts per second

// English to Metric conversion constants
#define MM_PER_CM		((float)10.0) // Millimeters to Centimeters
#define MM_PER_INCH		((float)25.4) // Millimeters to Inches
#define CM_PER_INCH		((float)2.54) // Centimeters to Inches
#define LPM_PER_CFH		((float)2.1186) // Liters/Minute to CubicFeet/Hour

#define SET_TO_ENGLISH		0
#define SET_TO_METRIC		1
#define CONVERT_MM_TO_CM	2

#define MAX_STANDARD_SERVOS     (1 + 4) // One current servo + 4 motor servos
#define MAX_OPTIONAL_SERVOS     4 
#define NUM_SERVO_SWITCHES		4


#define MAX_NUMBER_SERVOS   	(MAX_STANDARD_SERVOS + MAX_OPTIONAL_SERVOS)
//#define MAX_NUMBER_SERVOS   	MAX_STANDARD_SERVOS
#define MAX_NUMBER_OF_MANIPS	8
#define MAX_NUMBER_OF_LEVELS	20	
#define MAX_NUMBER_OF_PASSES	20
#define MAX_MANIP_BUTTONS		4  // Number of Manipulator jog buttons on the operator pendant

#define STANDARD_AVERAGE_AMPS	250
#define STANDARD_MAXIMUM_AMPS	300
#define DUMA_AVERAGE_AMPS		500

////////////////////////////////////////////////////////////////////////////////////////////
// Start modes
#define LIT_STARTMODE_RF		1	// RF
#define LIT_STARTMODE_TOUCH		2	// TCH
#define LIT_STARTMODE_DEFAULT	(LIT_STARTMODE_RF * (-1))

#define LIT_STUBOUTMODE_ON 		1	// ON
#define LIT_STUBOUTMODE_OFF		2	// OFF

#define LIT_USE_MULTILEVEL_MODE 0   // Read the value from MultiLevel
#define LIT_PULSEMODE_ON		1	// ON
#define LIT_PULSEMODE_OFF		2	// OFF
#define LIT_PULSEMODE_SYNC		3	// SYNC
#define LIT_PULSEMODE_EXTERNAL	4	// EXT
#define LIT_PULSEMODE_DEFAULT	(LIT_PULSEMODE_ON * (-1))

#define OSC_MODE_UNDEFINED		0
#define OSC_MODE_ON				1	// ON
#define OSC_MODE_OFF			2	// OFF
#define OSC_MODE_DEFAULT		(OSC_MODE_ON * (-1))

// Wirefeed Pulse mode
#define WFD_PULSE_ON			1
#define WFD_PULSE_OFF			2
#define WFD_PULSE_EXTERNAL		3
#define WFD_PULSE_DEFAULT		(WFD_PULSE_ON * (-1))

// Wirefeed motor selects
#define WFD_MOTOR_LEADING		1	// Leading
#define WFD_MOTOR_TRAILING		2	// Trailing
#define WFD_MOTOR_DEFAULT		(WFD_MOTOR_LEADING * (-1))

// Travel step literals
#define LIT_TVL_STEP_UNDEFINED	0	// Cont
#define LIT_TVL_STEP_CONT		1	// Cont
#define LIT_TVL_STEP_STEP		2	// Step
#define LIT_TVL_STEP_OFF		3	// Off
#define LIT_TVL_STEP_EXTERNAL	4	// Ext
#define LIT_TVL_STEP_DEFAULT	(LIT_TVL_STEP_CONT * (-1))

#define LIT_AVCMODE_CONT		1	// CONT
#define LIT_AVCMODE_OFF			2	// OFF
#define LIT_AVCMODE_SAMP_PRI	3	// SAMP_PRI
#define LIT_AVCMODE_SAMP_BAC	4	// SAMP_BAC
#define LIT_AVCMODE_EXTERNAL    5	// EXT
#define LIT_AVCMODE_DEFAULT		(LIT_AVCMODE_CONT * (-1))

// Travel literals
#define LIT_TVL_UNDEFINED		0
#define LIT_TVL_CW				1	// CW
#define LIT_TVL_CCW				2	// CCW
#define LIT_TVL_OFF				3	// OFF
#define LIT_TVL_DIR_DEFAULT		(LIT_TVL_CW * (-1))
#define MAX_TVL_DIRS			3

#define LOC_MODE_MASK			0x80
#define LOC_ONELEVEL			0x80

// WeldSchedule Values
#define SC_OPENLOOP				0x01
#define SC_CLOSEDLOOP			0x02
#define SC_ULPHEAD              0x03

// Used in Avc/Osc interchange calculations
#define AVC_COMPONENT			0
#define OSC_COMPONENT			1

#define INACTIVE				FALSE
#define ACTIVE					(!INACTIVE)

// Start modes
#define SM_RF					1
#define SM_TOUCH				2
#define SM_BOTH					3

////////////////////////////////////////////////////////////////////////////////////////////
// Servo functions
#define SF_CURRENT			0
#define SF_WIREFEED			1
#define SF_OSC				2
#define SF_TRAVEL			3
#define SF_AVC				4
#define SF_JOG_ONLY_VEL		5
#define SF_JOG_ONLY_POS		6
#define SF_COUNT			7

typedef enum tagServoFunction_t
{
    ServoFunction_Invalid		   = (-1),
    ServoFunction_Current		   = SF_CURRENT,
    ServoFunction_Wirefeed		   = SF_WIREFEED,
    ServoFunction_Osc			   = SF_OSC,
    ServoFunction_Travel		   = SF_TRAVEL,
    ServoFunction_Avc			   = SF_AVC,
    ServoFunction_JogOnly_Velocity = SF_JOG_ONLY_VEL,
    ServoFunction_JogOnly_Position = SF_JOG_ONLY_POS,
    ServoFunction_Count			   = SF_COUNT
} ServoFunction_t;

// Servo Types
#define ST_VELOCITY				0
#define ST_POSITION				1

// Averaged ACQ literals
#define NUM_AVG_CHANNELS		4
#define PRI_BACK_COUNT			2

// Standard Servo slot literals
#define STD_AMP_SLOT			0
#define STD_AVC_SLOT			3 	/////////////// TEST ONLY /////////////////////
#define STD_OSC_SLOT			4 	/////////////// TEST ONLY /////////////////////

// System weld literals
#define LIT_WELDMODE			1
#define LIT_TESTMODE			2

// Wire direction literals
#define WIRE_DIR_FEED			1
#define WIRE_DIR_RETR			2

// Wirefeed mode literals
#define LIT_WIREFEED_AUTO		1
#define LIT_WIREFEED_MANUAL		2

#define LEVEL_ADVANCE_MANUAL	1	// Manual
#define LEVEL_ADVANCE_TIME_10	2	// Time in whole seconds
#define LEVEL_ADVANCE_POSITION	3	// Position
#define LEVEL_ADVANCE_TIME_01	4	// Time in tenths seconds
#define LEVEL_ADVANCE_TACK		5	// Tack
#define LEVEL_ADVANCE_EXTERNAL  6   // Ext
#define LEVEL_ADVANCE_DEFAULT	(LEVEL_ADVANCE_MANUAL * (-1))


// Aux Line Defines
#define AUX_INE_1
#define AUX_USER_DEF1		0x00 // AUX LINE #1
#define AUX_SEQ_STOP		0x01 // AUX LINE #1

#define AUX_INE_2
#define AUX_USER_DEF2		0x00 // AUX LINE #2
#define AUX_JOG_REV 		0x01 // AUX LINE #2

#define AUX_INE_3
#define AUX_AMP_SYNC		0x00 // AUX LINE #3
#define AUX_HOME	 		0x01 // AUX LINE #3

#define AUX_INE_4
#define AUX_JOG_FWD 		0x01 // AUX LINE #4
#define AUX_OSCSLEW 		0x02 // AUX LINE #4


// Osc Special Mode literals
#define	OSC_NOSPECIAL		0x0000	// No Spiral or SlewTo functions
#define	SOL_OSC_SPIRAL		0x0001	// Single Level Osc Spiral
#define	MUL_OSC_SPIRAL		0x0002	// Multi-Level Osc Spiral
#define	SOL_OSC_SLEWTO		0x0004	// Single Level Osc SlewTo (EXTERNAL TRIGGERED)
#define	MUL_OSC_SLEWTO		0x0008	// Multi-Level Osc SlewTo

// Feedback device types
#define FEEDBACK_ENCODER				0	// Encoder
#define FEEDBACK_BACKEMF				1	// Back EMF feedback
#define FEEDBACK_ANALOG					2	// Analog Tachometer
#define FEEDBACK_POSITIONFROMVELOCITY	3	// Encoder Position type data derived from Analog Tachometer Velocity info

// Unit of measure types
#define UMT_ENGLISH			0	// English
#define UMT_METRIC			1	// Metric
#define UMT_RPM			    2	// RPM
#define UMT_USER			3	// Other

//	manipulator limits
#define MANIP_CURMIN		 0.0f // Current min
#define	MANIP_CURMAX		 1.0f // Current max
#define	MANIP_VMIN			 6.0f // Voltage min
#define	MANIP_VMAX			24.0f // Voltage max

// Servo Current limits
#define	AMI_WHL_SL_CURMIN		0.0f	// Current min	
#define	AMI_WHL_SL_CURMAX		4.0f	// Current max

// Default Gas Flow Limit Values
#define STD_GAS_LO_LIMIT		 0.0f	
#define STD_GAS_HI_LIMIT		40.0f

// Gas Fault Enable defines
#define GAS_FAULTDISABLE    0x00 
#define GAS_HILOFAULTENABLE 0x01 
#define GAS_HIFAULTENABLE   0x02 
#define GAS_LOFAULTENABLE   0x03 


// ADC Value
#define MAX_ADC_VALUE			4096.0f
#define ADC_ZERO_OFFSET		    0x07FF
#define MIN_ADC_VALUE			(ADC_ZERO_OFFSET - (MAX_ADC_VALUE - ADC_ZERO_OFFSET))
#define ADC_RANGE				(MAX_ADC_VALUE - MIN_ADC_VALUE)
#define ADC_POSITIVE_SWING      (MAX_ADC_VALUE - ADC_ZERO_OFFSET)

// Velocity Servos with Encoder Open Loop constant
#define DESIRED_POS_DIVISOR		512

// (80 counts / interrupt) * DESIRED_POS_DIVISOR
#define OPENLOOP_JOG_CONSTANT	0xA000

// Offset & Gain Scaler value
#define OG_SCALER	0x1000

typedef struct tagDAC_CAL_STRUCT {
	short nCmdOffset;
	short nCmdGain;
	short nFdbkOffset;
	short nFdbkGain;
} DAC_CAL_STRUCT;

/*lint -restore */
#endif // __AMI_M327_H
