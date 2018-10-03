//
// InitWeldStruct.h
#ifndef __AMI_INITWELDSTRUCT_H
#define __AMI_INITWELDSTRUCT_H

#include <inttypes.h>

#include "ComLit.h"
#include "M327.h"

#define ZERO_VOLT_OFFSET	    (WORD)0x1000 
#define DAC_VOLT_OFFSET			(WORD)0x0800 
#define POS_DAC_SWING			(int)8000	// This is decimal
#define NEG_DAC_SWING			(int)0	// This is decimal
#define FULL_DAC_SWING			(int)(POS_DAC_SWING + NEG_DAC_SWING) // Positive + Negative swing
#define MAX_CURRENT_LIMIT_DAC	POS_DAC_SWING

// These definitions are for the OSCILLATOR DAC ONLY!!!
// Do not use these for any other DAC
//#define MAX_INNER_POSITION      (0 - NEG_DAC_SWING)
//#define MAX_OUTER_POSITION      (0 + POS_DAC_SWING)
#define MAX_INNER_POSITION      (NEG_DAC_SWING)
#define MAX_OUTER_POSITION      (POS_DAC_SWING)

#define MAX_OSCDAC_RANGE		((float)8000.0) // MAX_OUTER_POSITION - MAX_INNER_POSITION


// #define MAX_POS_AXIALSPEED      ZERO_VOLT_OFFSET + POS_DAC_SWING
// #define MAX_NEG_AXIALSPEED      ZERO_VOLT_OFFSET - NEG_DAC_SWING
// Changed on 7/23/97
#define MAX_POS_AXIALSPEED      (POS_DAC_SWING - ZERO_VOLT_OFFSET)
#define MAX_NEG_AXIALSPEED      (NEG_DAC_SWING + ZERO_VOLT_OFFSET)

// AVC Positive Jog is UP; Neg is Down
// These are magnitudes only....The zero offset will be added by
// the output function
#define AVC_POS_JOG				(short)(0x00FF *   1)
#define AVC_NEG_JOG				(short)(0x00FF * (-1))

#define MAX_SERVO_CURRENT_LIMIT	((float)4.0)
#define MAX_BOOSTER_VOLTAGE		277 // 0x7FF in DAC will cause Booster to charge to 277 Volts

#define RESPONSE_MAX_VALUE	(float)40.0

// 10.0 Volts = (2048 + ADC_ZERO_OFFSET) = 4095
#define ADC_PER_VOLT		204 
//#define ADC_ZERO_OFFSET		2047
#define INC_GUIDERING		1000

#define TOUCH_START_CURRENT	((float)26.0) // 26 Amps

#define OWG_MAX				((DWORD)((DWORD)1000 << 16))

#define NUM_TVL_DIRS			3
#define NUM_MANIP_JOG_BUTTONS	4

// Defines used for Data Aqi
#define SAMPLE_SLOT0		(1 << 0)
#define SAMPLE_SLOT1		(1 << 1)
#define SAMPLE_SLOT2		(1 << 2)
#define SAMPLE_SLOT3		(1 << 3)
#define SAMPLE_SLOT4		(1 << 4)
#define SAMPLE_SLOT5		(1 << 5)
#define SAMPLE_SLOT6		(1 << 6)
#define SAMPLE_SLOT7		(1 << 7)
#define SAMPLE_SLOT8		(1 << 8)
#define SAMPLE_GAS_FLOW		(1 << 9)
#define SAMPLE_LEVEL_TIME	(1 << 10)
#define SAMPLE_EVENTS		(1 << 11)
#define SAMPLE_HEAD_POS		(1 << 12)

#define	DA_STARTAT_PPGE			0	// prepurge start
#define DA_STARTAT_ARC			1	// arc start
#define DA_STARTAT_UPSLOPE		2	// end of upslope

#define DA_STOPAT_DSLOPE_START	0	// downslope start
#define DA_STOPAT_DSLOPE_STOP	1	// downslope end
#define DA_STOPAT_DSLOPE_PPGE	2	// postpurge end

// Switch Type Defines
#define NOT_DEFINED		(-1)
#define HOME_SWITCH		00
#define SYNC_LINE		01
#define LIMIT_SWITCH	02

// Switch Actions
#define SA_WARNING_ONLY		00
#define SA_DOWNSLOPE		01
#define SA_ALLSTOP			02

/////////////////////////////////////////////////////////
#pragma pack (push, 1)

typedef struct tagIW_SERVOONELEVEL 
{
	WORD  wStartDelay;
	short nStopDelay;
	WORD  wRetract;
	WORD  wMinSteeringDac;
	WORD  wMaxSteeringDac;
	BYTE  bPadding[2];
} IW_SERVOONELEVEL, *LPIW_SERVOONELEVEL;

// Additional Structure used to hold IPM Travel speed
// to Rotor RPM values
typedef struct tagIW_PROGIPM2ROTATORIPM 
{
	WORD  wTiedToServoIndex;
	float fMaxRotatorRPM;
	float fMinRotatorRPM;
	float fPostion2Diameter;	// Slaved Servo Encoder Distance / Encoder Counts 
	float fTravelInchesPerDac;	// Converts InitWeld Program value to float Inches/Minute
	float fRotorDacPerRPM;	    // Max DAC Value / Rotor Speed (RPM)
	float fRotorCountsPerRev;	// EncoderCounts / 360 degrees(1Rev)
	float fCircum;				// Circumfrence of the Weld (Calculated on the fly)

	// Used to convert from Real IPM into a scaled value simialr to ADC
	float fAdcScaler;			
	float fMinRange;
    WORD  wRotatorIPM;
} IW_PROGIPM2ROTATORIPM;

typedef struct tagIW_PROGIPM2TRAVELIPM 
{
	WORD  wTiedToServoIndex;
	float fMaxTravelRPM;
	float fMinTravelRPM;
	float fPostion2Diameter;
	float fTravelInchesPerDac;	// Converts InitWeld Program value to float Inches/Minute
	float fRotorDacPerRPM;	    // Max DAC Value / Rotor Speed (RPM)
	float fRotorCountsPerRev;	// EncoderCounts / 360 degrees(1Rev)
	float fCircum;				// Circumfrence of the Weld (Calculated on the fly)

	// Used to convert from Real IPM into a scaled value simialr to ADC
	float fAdcScaler;			
	float fMinRange;
    WORD  wRotatorIPM;
} IW_PROGIPM2TRAVELIPM;

typedef struct tagIW_SERVOMULTILEVEL 
{
	WORD  wPrimaryValue;
	short nBackgroundValue;
	DWORD dwPriExternalDac;
	DWORD dwBacExternalDac;
	WORD  wResponse;
	WORD  wInDwellTime;
	WORD  wOutDwellTime;
	WORD  wExcursionTime;
	DWORD dwOwgInc;
	DWORD lOscSpiralInc;
	WORD  wPriPulseDelay;
	BYTE  uPulseMode;
	BYTE  uDirection;
	BYTE  uOscSpecialMode;
	BYTE  uMotorSelect;
	BYTE  uPadding[2];
} IW_SERVOMULTILEVEL, *LPIW_SERVOMULTILEVEL;
enum {IW_SERVOMULTILEVELsz = sizeof(IW_SERVOMULTILEVEL)};

typedef struct tagIW_SERVO_SWITCH 
{
	BYTE uSwitchActive;
	char uSwitchType;   // Must hold (-1) to show inactive
	char uFaultAction;  // Must hold (-1) to show inactive
	BYTE uPadding;
} IW_SERVO_SWITCH, *LPIW_SERVO_SWITCH;

typedef struct tagIW_SERVODEF 
{
	BYTE uServoSlot;
	BYTE uServoType;
	BYTE uServoFunction;
	BYTE uServoControl;

	WORD wCurrentLimit;
	WORD wJogSpeedForward;

	WORD wJogSpeedReverse;
	WORD wTouchSpeed;

	WORD wRetractSpeed;
	WORD wExtDacJogSpeedFwd;

	WORD wExtDacJogSpeedRev;
	char cFeedbackDeviceType;  // Must be able to reflect -1 for not used
	char cPositionDeviceType;  // Must be able to reflect -1 for not used

	BYTE uReverseCommands;
	BYTE uRevEncoderCnts;
    BYTE uFootControlNeeded;
	BYTE uHomeSwitchUsed;

	IW_SERVO_SWITCH stServoSwitch[4];

	BYTE uSetFunctionUsed;
	BYTE uPrewrapUsed;
	BYTE uExternalDAC;
	BYTE uExtOscSteerEnable;	// Enable for external osc steering input

	WORD wExtOscSteerChannel;	// Analog input channel number
	BYTE uPadding[2];

} IW_SERVODEF, *LPIW_SERVODEF;

typedef struct tagIW_HEADINFO 
{
	BYTE bCoolantUsed;
	BYTE bGNDFaultUsed;
	BYTE bPositionIndicator;
	BYTE bPositionServoSlot;
	BYTE bPositionFeedbackType;
	BYTE uRevEncoderCnts;		 // CHanges direction encoder counts should be accumlated (USED AS A BOOL)
	BYTE bStartOptions;
	BYTE uPadding[1];
} IW_HEADINFO, *LPIW_HEADINFO;

typedef struct tagIW_MANIPDEF 
{
	WORD wJogCurrent;
	WORD wJogVoltage;
	BYTE uManipSlot;
	BYTE uPadding[3];
} IW_MANIPDEF;

typedef struct tagIW_MANIPJOGARRAY 
{
	JOG_STRUCT stManipJogCommand[MAX_MANIP_BUTTONS]; // sizeof(JOG_STRUCT) = sizeof(WORD)
} IW_MANIPJOGARRAY;

typedef struct tagIW_WELDHEADTYPEDEF 
{
	IW_HEADINFO		 stHeadInfo;
	IW_SERVODEF		 stServoDef[MAX_NUMBER_SERVOS];
	//IW_PROGIPM2ROTATORIPM   stIpmRpm[MAX_NUMBER_SERVOS];
	IW_MANIPDEF		 stManipDef[MAX_NUMBER_OF_MANIPS];
	IW_MANIPJOGARRAY stManipJogArray[NUM_TVL_DIRS];
} IW_WELDHEADTYPEDEF, *LPIW_WELDHEADTYPEDEF;

typedef struct tagIW_DATA_AQI 
{
	WORD wSampleMask;
	WORD wSampleRate;
	BYTE uDataAqiEnabled;
	BYTE uStartAt;
	BYTE uStopAt;
	BYTE uPadding;
} IW_DATA_AQI;

typedef struct tagIW_MULTILEVEL 
{
	DWORD    dwLevelTime;
	DWORD    dwSlopeTime;
	DWORD    dwSlopeStartTime;
	uint64_t nLevelPosition;
	WORD     wPriPulseTime;
	WORD     wBckPulseTime;
	IW_SERVOMULTILEVEL stServoMultiLevel[MAX_NUMBER_SERVOS];
} IW_MULTILEVEL, *LPIW_MULTILEVEL;

typedef struct tagIW_ONELEVEL 
{
	WORD  wPrepurge;
	WORD  wPostpurge;
	WORD  wUpslope;
	WORD  wDownslope;
	WORD  wStartLevel;
	WORD  wTouchStartCurrent;
	WORD  wTorchGasRate;
	BYTE  uStartMode;
	BYTE  uLevelAdvanceMode;
	BYTE  uStuboutMode;
	BYTE  uStartAttemtps;
	BYTE  uPadding[2];
	IW_SERVOONELEVEL stServoOneLevel[MAX_NUMBER_SERVOS];
} IW_ONELEVEL, *LPIW_ONELEVEL;


typedef struct tagIW_SCHEDULEPARMS 
{
	WORD  wLowerGasFlowLimit;
	WORD  wUpperGasFlowLimit;
	short nSinAngle[2];  // Holder for both Avc and Osc
	short nCosAngle[2];  // Holder for both Avc and Osc
} IW_SCHEDULEPARMS, *LPIW_SCHEDULEPARMS;

typedef struct tagIW_INITWELD 
{
	BYTE                uNumberOfLevels;
	BYTE                uNumberOfServos;
	WORD                wPadding;
	IW_SCHEDULEPARMS	stScheduleParms;
	IW_ONELEVEL         stOneLevel;
	IW_MULTILEVEL       stMultiLevel[MAX_NUMBER_OF_LEVELS];
	IW_DATA_AQI			stDataAqi;
	IW_WELDHEADTYPEDEF	stWeldHeadTypeDef;
    BYTE				uTechnosoftBuffer[0x1000];
} IW_INITWELD, *LPIW_INITWELD;

typedef struct tagIW_INITWELDLEVEL 
{
	DWORD				dwLevelNum;
	IW_MULTILEVEL		stLevelData;
} IW_INITWELDLEVEL;

typedef struct tagIW_STEERSTRUCT 
{
	WORD  wMinSteeringDac;
	WORD  wMaxSteeringDac;
} IW_STEERSTRUCT;

typedef struct tagIW_OSCLIMIT 
{
	IW_STEERSTRUCT stSteeringStruct[MAX_NUMBER_SERVOS];
} IW_OSCLIMIT;

#pragma pack (pop)

#endif // __AMI_INITWELDSTRUCT_H






