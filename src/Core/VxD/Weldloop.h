//
// weldloop.h
//

#ifndef _WELDLOOP_H_
#define _WELDLOOP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>

#include "typedefs.h"
#include "adc.h"
#include "InitWeldStruct.h"
#include "M327.h"
#include "ServoCom.h"
#include "util.h"
#include "Support.h"
#include "FaultList.h"
#include "sloping.h"
#include "CAN.h"
#include "Queue.h"
#include "FaultList.h"

// Arc Start Phases
#define AS_PRESTRIKE    0
#define AS_RFSTARTING	1
#define AS_DRIVEDOWN    2
#define AS_DRIVEUP		3
#define AS_EVALUATE		4
#define AS_RETRY_PHASE	5
#define AS_COMPLETE		6

// Weld Phase Definitions
#define WP_INITUPSLOPE     0
#define WP_PREPURGE        1
#define WP_UPSLOPE         2
#define WP_LEVELWELD       3
#define WP_DOWNSLOPE       4
#define WP_POSTPURGE       5
#define WP_EXIT            6
#define WP_COMPLETE        7

// Return To Home literals
#define HS_PRETEST			0
#define HS_FIRST_CLOSE		1
#define HS_BACKUP			2
#define HS_CLOSEUP			3
#define HS_EXITPHASE		4
#define HS_OPEN				5
#define HS_GO_ON			6
#define HS_ABORT			7

#define HD_REVERSE			0
#define HD_FORWARD			1

// OSC Enable defines
#define SERVOENABLE_READ			0
#define SERVOENABLE_ENABLE			1

// Slope Literals
#define SLOPE_DAC_MAX	0x7FFF0000L
#define SLOPE_DAC_MIN	0x00000000L

// FULL Foot controller = 8.0 Volts
#define FOOT_CONTROL_MAX  1638 // This is decimal, after the zero offset is removed

// Osc Steering 
#define MAX_ENCODER_CHANGE_ALLOWED 64 // this is 8 bits / 4

// Osc Literals
#define OSC_SLEW_OUT    0
#define OSC_SLEW_IN		1

#define OWG_OUT_POINT   0x00000000L //    0 << 16
#define OWG_MID_POINT	0x01F40000L //  500 << 16
#define OWG_IN_POINT	0x03E80000L // 1000 << 16

// Pulse Phase Literals
#define IN_PRIMARY			1
#define PRIMARY2BACKGND		2
#define IN_BACKGND			3
#define BACKGND2PRIMARY		4

// AVC response value for set function
#define SET_POT_RESPONSE (POS_DAC_SWING / 4) // Set Response to 1/4 max

typedef struct tagFAULT_STRUCT
{
	BOOL bHighArcVoltFault;
	BYTE uArcOutCount;
	
	BYTE uStuboutDly;
	BOOL bStubOutFault;
	BYTE uStuboutCount;
	
	BOOL bInputAcFault;
	BYTE uInputACCount;
	BYTE uAmpFlowCount;
} FAULT_STRUCT;

typedef struct tagSLOPE_STRUCT
{
	DWORD dwPurgeTime;
	DWORD dwSlopeTimeCount;
	DWORD dwSlopeDac;
	DWORD dwSlopeDacInc;
} SLOPE_STRUCT;

typedef struct tagSYSTEM_STRUCT
{	
	BYTE  uWeldMode;
	BYTE  uWeldLevel;
	BOOL  bBadStart;
	BOOL  bArcEstablished;
	BYTE  uArcStartPhase;
	BYTE  uStartAttempts;
	WORD  wAbortTime;
	BYTE  uRfTime;
	BYTE  uPulseMode;
	BYTE  uPulsePhase;
	BYTE  uInTransistion;
	DWORD dwLevelTimeCount;
	DWORD dwLevelTime;
	int64_t   nLevelPosition;	
	BOOL  bHeadDirectionFwd;

	DWORD dwSlopeTime;
	DWORD dwSlopeStartTime;

	WORD  wPriPulseTimer;
	WORD  wBacPulseTimer;
	WORD  wPulseDuration;
	BYTE  uCurrentFaultDelay;
	
	BYTE  uInNegStopDelay;
	BYTE  uWireFeedModeAuto[MAX_NUMBER_SERVOS];

	BYTE  uGndFaultPhase;
	BYTE  uSetPotPhase;

	BOOL  uPurgeOn;
	BOOL  uLampOn;
	
	FAULT_STRUCT Fault;
	SLOPE_STRUCT Slope;

	BYTE uDataAqiOutEnable;
	BYTE uDataAqiStart;
	BYTE uDataAqiStop;
	WORD wDataAqiRate;
} SYSTEM_STRUCT;

typedef struct tagHOME_STRUCT
{
	BYTE		 uPhase;
	BYTE		 uTime;
	SERVO_VALUES* ss;
} HOME_STRUCT;

#define mNumServos	    ((stInitWeldSched.uNumberOfServos))
#define mNumberOfLevels ((stInitWeldSched.uNumberOfLevels))
#define mLastLevel      ((mNumberOfLevels) - 1)

// Macros for MultiLevel Parameters
#define LEVEL			WeldSystem.uWeldLevel
#define ML_ADDR			(&stInitWeldSched.stMultiLevel[(LEVEL)])
#define mPriPulseTime	((ML_ADDR->wPriPulseTime))
#define mBacPulseTime	((ML_ADDR->wBckPulseTime))
#define mLevelTime		((ML_ADDR->dwLevelTime))
#define mLevelPosition	((int)(ML_ADDR->nLevelPosition))
#define mSlopeTime				((ML_ADDR->dwSlopeTime))
#define mSlopeStartTime			((ML_ADDR->dwSlopeStartTime))

// Macros for ServoMultiLevel Parameters
#define SML_ADDR(S)				(&ML_ADDR->stServoMultiLevel[(S)])
#define mPrimaryValue(S)        ((SML_ADDR((S))->wPrimaryValue))
#define mBackgroundValue(S)     ((short)(SML_ADDR((S))->nBackgroundValue))
#define mExternDacPri(S)        ((SML_ADDR((S))->dwPriExternalDac))
#define mExternDacBac(S)        ((SML_ADDR((S))->dwBacExternalDac))
#define mInDwellTime(S)			((SML_ADDR((S))->wInDwellTime))
#define mExcursionTime(S)		((SML_ADDR((S))->wExcursionTime))
#define mOutDwellTime(S)		((SML_ADDR((S))->wOutDwellTime))
#define mOwgInc(S)				((SML_ADDR((S))->dwOwgInc))
#define mOscSpiralInc(S)		((long)(SML_ADDR((S))->lOscSpiralInc))
#define mOscSpecialMode(S)		((SML_ADDR((S))->uOscSpecialMode))
#define mPriPulseDelay(S)       ((SML_ADDR((S))->wPriPulseDelay))
#define mPulseMode(S)           ((SML_ADDR((S))->uPulseMode))
#define mDirection(S)           ((SML_ADDR((S))->uDirection))
#define mResponse(S)            ((SML_ADDR((S))->wResponse))
#define mMotorSelect(S)         ((SML_ADDR((S))->uMotorSelect))


// Macros for OneLevel Parameters
#define OL_ADDR				(&stInitWeldSched.stOneLevel)
#define mPrePurgeTime       ((OL_ADDR->wPrepurge))
#define mUpslopeTime        ((OL_ADDR->wUpslope))
#define mDownslopeTime      ((OL_ADDR->wDownslope))
#define mPostPurgeTime      ((OL_ADDR->wPostpurge))
#define mStartLevel         ((short)(OL_ADDR->wStartLevel))
#define mTouchStartCurrent	((OL_ADDR->wTouchStartCurrent))
#define mStartMode          ((OL_ADDR->uStartMode))
#define mLevelAdvanceMode   ((OL_ADDR->uLevelAdvanceMode))
#define mStuboutMode        ((OL_ADDR->uStuboutMode))
#define mStartAttempts		((OL_ADDR->uStartAttemtps))
#define mTorchGasRate       ((OL_ADDR->wTorchGasRate))

// Macros for ServoOneLevel Parameters
#define SOL_ADDR(S)			(&OL_ADDR->stServoOneLevel[(S)])
#define mWireRetractTime(S)	((SOL_ADDR((S))->wRetract))
#define mStopDelay(S)       ((short)(SOL_ADDR((S))->nStopDelay))
#define mStartDelay(S)      ((SOL_ADDR((S))->wStartDelay))
#define mMinSteeringDac(S)  ((SOL_ADDR((S))->wMinSteeringDac))
#define mMaxSteeringDac(S)  ((SOL_ADDR((S))->wMaxSteeringDac))

// Macros for ServoDef Parameters
#define WHT_ADDR				(&stInitWeldSched.stWeldHeadTypeDef)
#define SD_ADDR(S)				(&WHT_ADDR->stServoDef[(S)])
#define mServoSlot(S)           ((SD_ADDR((S))->uServoSlot))
#define mServoType(S)           ((SD_ADDR((S))->uServoType))
#define mServoFunction(S)       ((SD_ADDR((S))->uServoFunction))
#define mServoControl(S)        ((SD_ADDR((S))->uServoControl))
#define mCurrentLimit(S)        ((SD_ADDR((S))->wCurrentLimit))
#define mJogSpeedForward(S)     ((SD_ADDR((S))->wJogSpeedForward))
#define mJogSpeedReverse(S)     ((SD_ADDR((S))->wJogSpeedReverse))
#define mTouchSpeed(S)          ((SD_ADDR((S))->wTouchSpeed))
#define mRetractSpeed(S)        ((SD_ADDR((S))->wRetractSpeed))
#define mExtDacJogSpeedFwd(S)   ((SD_ADDR((S))->wExtDacJogSpeedFwd))
#define mExtDacJogSpeedRev(S)   ((SD_ADDR((S))->wExtDacJogSpeedRev))
#define mFeedbackDeviceType(S)	((SD_ADDR((S))->cFeedbackDeviceType))
#define mServoRevEncoderCnts(S) ((SD_ADDR((S))->uRevEncoderCnts))
#define mPositionDeviceType(S)	((SD_ADDR((S))->cPositionDeviceType))
#define mActiveFaults(S)        ((SD_ADDR((S))->wActiveFaults))
#define mDirectionReversed(S)	((SD_ADDR((S))->uReverseCommands))
#define mRequiresFootControl(S) ((SD_ADDR((S))->uFootControlNeeded))
#define mHomeSwitchUsed(S)		((SD_ADDR((S))->uHomeSwitchUsed))
#define mPreWrapUsed(S)			((SD_ADDR((S))->uPrewrapUsed))
#define mSetFunctionUsed(S)		((SD_ADDR((S))->uSetFunctionUsed))
#define mExternalDac(S)		    ((SD_ADDR((S))->uExternalDAC))

// Macros for ManipDef Parameters
//#define WHT_ADDR				(&stInitWeldSched.stWeldHeadTypeDef)
#define MD_ADDR(S)		(&WHT_ADDR->stManipDef[(S)])
#define mJogCurrent(S)	((MD_ADDR((S))->wJogCurrent))
#define mJogVoltage(S)  ((MD_ADDR((S))->wJogVoltage))
#define mManipSlot(S)	((MD_ADDR((S))->uManipSlot))

// Macros for HeadInfo Parameters
#define HI_ADDR						(&WHT_ADDR->stHeadInfo)
#define mCoolantUsed				((HI_ADDR->bCoolantUsed))
#define mGNDFaultUsed				((HI_ADDR->bGNDFaultUsed))
#define mHeadPositionIndicator		((HI_ADDR->bPositionIndicator))
#define mHeadPositionFeedbackType	((HI_ADDR->bPositionFeedbackType))
#define mHeadRevEncoderCnts			((HI_ADDR->uRevEncoderCnts))
#define mStartOptions				((HI_ADDR->wStartOptions))
#define mPositionServoSlot			((HI_ADDR->bPositionServoSlot))

void fnHandler(void);

#endif // _WELDLOOP_H_

