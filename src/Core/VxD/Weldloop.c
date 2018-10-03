//
// weldloop.c
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "Task.h"

// Global include files
#include "Weldloop.h"
#include "GPIO.h"
#include "Manip.h"

#define SLOT_ENABLE				BIT_0 // 1 = Enable 0 = DISABLE
#define SLOT_USE_DIGITAL1		BIT_1 // ANALOG, DIGITAL, BEMF
#define SLOT_USE_DIGITAL2		BIT_2 // ANALOG = 00, DIGITAL = 01, BEMF = 10, FUTURE_EPANSION = 11
#define SLOT_IS_VEL_SERVO		BIT_3 // 1 = Velocity Servo 0 = Position Servo
#define SLOT_FWD_DIRECTION		BIT_4 // Direction 1 = FORWARD 0 = REVERSE
#define SLOT_AVC_JOG			BIT_5 // AVC Jog / Weld 1 = JOG, 0 = WELD
#define SERVO_COMMAND_BIT		BIT_F // 0 = Normal Update, 1 = SPECIAL COMMAND STRUCTURE FLAG

typedef struct tagSTANDARD_SETUP_STRUCT {
	int	nStubOut;
	int nHighArc;
	int nPriCoolant;
	int nAuxCoolant;
	int	nUMT;
	int	nGasFault;
}	STANDARD_SETUP_STRUCT;
#define LPC_STANDARD_SETUP_STRUCT const STANDARD_SETUP_STRUCT *

extern void fnDumpAllDacs(void);
extern void StopAvcDrive(SERVO_VALUES* ss);
extern void UpdateGraphInfo(GRAPH_INFO* pGraphInfo, WORD wCommandData, WORD wFeedbackData);
extern void fnCommandServo(SERVO_VALUES* ss);
extern void JogVelocityServo(SERVO_VALUES* ss, int nServoSpeed);
//////////////////////////////////////////////////////////////////////
//
// Global variables defined outside this source module.
//
//////////////////////////////////////////////////////////////////////
extern IW_INITWELD		stInitWeldSched;				// in Support.c
extern GRAPH_INFO		GraphInfo[MAX_GRAPH_PORTS]; // These are defined in support.c
extern DAC_CAL_STRUCT   DacCalMap[MAX_NUMBER_SERVOS];
extern DWORD		    dwElapsedTime;
extern int			    nLampTimer;
extern int			    nBoosterVoltage;

// These are defined in w32intf.c
//RM extern MESSAGE_QUEUE    MessageQueue;
extern BOOL			    _isAvcCardPresent;
extern BYTE             _uAuxLineFunction[4];
extern BYTE			    bManipCurrentLimitFault[MAX_NUMBER_OF_MANIPS];
extern BOOL             _isManipEnabled;
extern BYTE			    uActiveManip;
extern SERVO_VALUES*    pMasterAvcServo;
extern OSC_VALUES*      pMasterOscServo;

extern SERVO_VALUES			 ServoStruct[MAX_NUMBER_SERVOS];
extern SERVO_POSITION_STRUCT ServoPosition[MAX_NUMBER_SERVOS];
extern SYSTEM_STRUCT	     WeldSystem;
extern SERVO_CONST_STRUCT    ServoConstant[MAX_NUMBER_SERVOS];

extern BOOL bStubOutEnable;
extern int  nUlpVoltFaultState;
extern BOOL bHighArcEnable;
extern BYTE uGasFaultEnable;
extern BOOL bPriCoolantEnable;
extern BOOL bAuxCoolantEnable;

extern BOOL bWeldSequence;
extern BYTE uSystemWeldPhase;
extern BYTE uLockFaults;
extern BYTE uNumGraphPorts;
extern int	PendantKeyServoMap[SF_COUNT];

extern int	nHB_CheckCount;
extern int	nHB_FailCount;
extern int  nHomeDirection;
extern int  nHomeDuringPostpurge;
extern BYTE bCheckGroundFault;

extern BOOL bResetAvcOscPositions;
extern int	nMasterTvlSlot;
extern int	nMasterTravelDirection;

extern WORD _wServoInitDelay;  // Delay enabling the OSC
extern BYTE _isGuiConnected;
extern BOOL _bUpdateDas;
extern BOOL g_bForceBargraphUpdate;
extern MESSAGE_QUEUE    MessageQueue;

//////////////////////////////////////////////////////////////////////
//
// Local variables
//
//////////////////////////////////////////////////////////////////////
static HOME_STRUCT	Home;
static BOOL	bOscManual = __FALSE;
static BYTE	bReturnToHome[MAX_NUMBER_SERVOS];
static BYTE	bPreWrap[MAX_NUMBER_SERVOS];
static BYTE	bPerformSetPot[MAX_NUMBER_SERVOS];
static BYTE	uDesiredLevel = 0;
static BOOL uStoppedOnFault = __FALSE;
static int	nManipFaultCount = 0;
static char	uManipSpeed;

//static int  nOscSpeedSteer = 10;  // OSC Speed Multiplier
//static WORD wSteeringWasPosition = 0;
//static WORD wSteeringIsPosition  = 0;

// Global function prototyps
void fnColdShutdown(void);
void fnWeldCommands(LPCWELDCOMMAND pWeldCommand);
void QueueFaultMessage(BYTE uFaultMsg);

//////////////////////////////////////////////////////////////////////
// Weld Phase Function Prototypes
static void fnInitPostPurge(void);
static void fnPostPurgeLoop(void);
static void fnExitPhase(void);
static void MoveToNewLevel(void);

//////////////////////////////////////////////////////////////////////
// Servo function prototypes
static void AvcPriCmd(SERVO_VALUES* );
static void AvcPriToBacCmd(SERVO_VALUES* );
static void AvcBacCmd(SERVO_VALUES* );
static void AvcBacToPriCmd(SERVO_VALUES* );

static void AmpPriCmd(SERVO_VALUES* );
static void AmpBacCmd(SERVO_VALUES* );
static void AmpCommonCmd(SERVO_VALUES* ss, WORD wDacValue);
static void TvlPriCmd(SERVO_VALUES* );
static void TvlBacCmd(SERVO_VALUES* );
static void WfdPriCmd(SERVO_VALUES* );
static void WfdBacCmd(SERVO_VALUES* );
static void WfdBacToPriCmd(SERVO_VALUES* );

static void EvalFaultMessage(BYTE);
static void InitStopDelays(void);


//static void fnProcessOscSteerCommands(void);
static void GetZeroOscCentering(LPOSC_VALUES ss, int64_t nDesiredPosition);
static void SetPositionEncoderValue(const SET_POSITION* pPosInfo);
static void fnWarmShutdown(void);


//////////////////////////////////////////////////////////////////////
//
// BOOL ActiveServoFault(BYTE uFaultMsg, LPBYTE pAction)
//
//////////////////////////////////////////////////////////////////////
BOOL ActiveServoFault(BYTE uFaultMsg, LPBYTE pAction)
{
	BYTE uServo;
	BYTE uServoSlot;
	BYTE uSwitch;

	IW_SERVODEF     *pServoDef;
	IW_SERVO_SWITCH *pServoSwitch;

	// D7 = Fault Type 1 = Servo Fault, 0 = Other Faults
	// D6 = Servo Number bit 3
	// D5 = Servo Number bit 2
	// D4 = Servo Number bit 1
	// D3 = Servo Number bit 0
	// D2 = Switch number bit 1
	// D1 = Switch number bit 0
	// D0 = Fault / clear 1= Fault, 0 = Clear

	if ((uFaultMsg & SERVO_TYPE_FAULT) == 0)
		return __FALSE; // Not a servo fault

	uServoSlot = (uFaultMsg & 0x78) >> 0x03;
	uSwitch    = (uFaultMsg & 0x06) >> 0x01;

//	if (uFaultMsg & 0x01)
//		printf("ActiveServoFault::Slot %d Switch %d SET\n", uServoSlot, uSwitch); 
//	else
//		printf("ActiveServoFault::Slot %d Switch %d CLR\n", uServoSlot, uSwitch); 

	for (uServo = 0; uServo < mNumServos; ++uServo) 
	{
		if (uServoSlot == mServoSlot(uServo))
		{
			pServoDef    = &stInitWeldSched.stWeldHeadTypeDef.stServoDef[uServo];
			pServoSwitch = &pServoDef->stServoSwitch[uSwitch];
				
			if (pServoSwitch->uSwitchActive)
			{
				if (pServoSwitch->uSwitchType == LIMIT_SWITCH)
				{
					// Return the Action required
					*pAction = (BYTE)pServoSwitch->uFaultAction;
					return __TRUE;
				}
			}
		}
	}
	
	return __FALSE; // Not a fault switch
}


//////////////////////////////////////////////////////////////////////
//
// void QueCommandDacValue(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
static void QueCommandDacValue(SERVO_VALUES* ss)
{
	if (!ss->bIsJogging) 
	{
		ss->wBuffDacValue = ss->wSeqDacValue;
		ss->bDacUpdate = __TRUE;
	}
}

//////////////////////////////////////////////////////////////////////
//
// void AvcPriCmd(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
void AvcPriCmd(SERVO_VALUES* ss)
{
	switch (ss->uPulseMode)
	{
		case LIT_AVCMODE_OFF:
			ss->wSeqDacValue = (WORD)0;
			break;
		case LIT_AVCMODE_SAMP_PRI:
			ss->wSeqDacValue = (WORD)mPrimaryValue(ss->uServoIndex);
			break;
		case LIT_AVCMODE_SAMP_BAC:
			ss->wSeqDacValue = (WORD)mBackgroundValue(ss->uServoIndex);
			break;
		default:
			ss->wSeqDacValue = mPrimaryValue(ss->uServoIndex);
			break;
	}

//	ETHERNET_Print("AvcPriCmd:: %d\n", ss->wSeqDacValue);
	QueCommandDacValue(ss);
	CommandExternalDacs(mExternDacPri(ss->uServoIndex), ss);
}

//////////////////////////////////////////////////////////////////////
//
// void fnEnableServo(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
static void fnEnableServo(SERVO_VALUES* ss)
{
	SetEnableBit(__TRUE, ss);
	ss->bEnabledFlag = __TRUE;
//	ETHERNET_Print("fnEnableServo:: Servo slot %d enabled\n", ss->uServoSlot);
}

//////////////////////////////////////////////////////////////////////
//
// WORD GetOscPosition(const OSC_VALUES *)
//
// Comments:
//       Returns the present position of the Oscillator
//       tranlated into DAC values
//
//////////////////////////////////////////////////////////////////////
static WORD GetOscPosition(const OSC_VALUES *os)
{
	WORD wCurrentOscPos;

	wCurrentOscPos = GetFeedback(os->uServoSlot + FB_SLOT_OFFSET);
	//ETHERNET_Print("GetOscPosition:: Feedback: %d\n", wCurrentOscPos);
	return wCurrentOscPos;
}

//////////////////////////////////////////////////////////////////////
//
// void fnAvcModeFixup(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
static void fnAvcModeFixup(SERVO_VALUES* ss)
{
	if (WeldSystem.uPulseMode == LIT_PULSEMODE_OFF)
	{
		if ((ss->uPulseMode == LIT_AVCMODE_SAMP_PRI) || (ss->uPulseMode == LIT_AVCMODE_SAMP_BAC))
		{
			ss->uPulseMode = LIT_AVCMODE_CONT;
		}
	}
}

//////////////////////////////////////////////////////////////////////
//
// void CalculateAvcEnableDly(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
void CalculateAvcEnableDly(SERVO_VALUES* ss)
{
	fnAvcModeFixup(ss);
	ss->wEnableDelay = 5;
	if (WeldSystem.wPulseDuration < 10)
	{
		ss->wEnableDelay = (WeldSystem.wPulseDuration / 2);
		if (ss->wEnableDelay == 0)
		{
			ss->wEnableDelay = 1;
		}
	}
	
	switch (ss->uPulseMode) 
	{
		case LIT_AVCMODE_OFF:
			ss->wEnableDelay = 0x0FFFF;
			break;

		case LIT_AVCMODE_SAMP_PRI:
			if ((WeldSystem.uPulsePhase == PRIMARY2BACKGND) ||
				(WeldSystem.uPulsePhase == IN_BACKGND)) 
			{
				ss->wEnableDelay = 0x0FFFF;    
			}
			break;

		case LIT_AVCMODE_SAMP_BAC:
			if ((WeldSystem.uPulsePhase == BACKGND2PRIMARY) ||
				(WeldSystem.uPulsePhase == IN_PRIMARY)) 
			{
				ss->wEnableDelay = 0x0FFFF;
			}
			break;
	}
}             

//////////////////////////////////////////////////////////////////////
//
// void fnSetSystemPulseMode(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////

// Set new value of WeldSystem.uPulseMode
static void fnSetSystemPulseMode(SERVO_VALUES* ss)
{
	BYTE uServo = ss->uServoIndex;

	///////////////////////////////////////////////////////
	// WeldSystem.uPulseMode is equal to the "was" value
	// mPulseMode(uServo) is equal to the "is" value
	///////////////////////////////////////////////////////


	///////////////////////////////////////////////////////
	// TESTS on Previous Mode
	///////////////////////////////////////////////////////
	if (WeldSystem.uPulseMode != LIT_PULSEMODE_ON)
	{
		// Turn on Pulse timers due to last pulse mode = OFF or SYNC
		switch (mPulseMode(uServo))
		{
			case LIT_PULSEMODE_ON:
				WeldSystem.wPriPulseTimer = 0;
				WeldSystem.wBacPulseTimer = mBacPulseTime;
				break;
				
			case LIT_PULSEMODE_OFF:
				WeldSystem.wPriPulseTimer = 1;
				WeldSystem.wBacPulseTimer = 0;
				break;
		}
	}

	///////////////////////////////////////////////////////
	// TESTS on Current Mode
	///////////////////////////////////////////////////////
	switch (mPulseMode(uServo))
	{
		// Check for Pulse Mode = ON, with no pulse timers
		case LIT_PULSEMODE_ON:
			if ((mPriPulseTime | mBacPulseTime) == 0)
			{
				WeldSystem.wPriPulseTimer = 0;
				WeldSystem.wBacPulseTimer = 0;
				mPulseMode(uServo) = LIT_PULSEMODE_OFF;
			}
			break;
	}

	ss->uPulseMode = mPulseMode(uServo);
	// Set new value of Pulse Mode
	WeldSystem.uPulseMode = mPulseMode(uServo);
}

//////////////////////////////////////////////////////////////////////
//
// void InvalidModeFixup(void)
//
//////////////////////////////////////////////////////////////////////
static void InvalidModeFixup(void)
{
	int uServo;
	int uAmpServo = (-1);
	int uAvcServo = (-1);
	int uOscServo = (-1);
	SERVO_VALUES* ss;

	for (uServo = 0; uServo < mNumServos; ++uServo) {
		ss = &ServoStruct[uServo]; // Save pointer to this servos struct

		switch(ss->uServoFunction) {
			case SF_CURRENT: uAmpServo = uServo; break;
			case SF_AVC:	 uAvcServo = uServo; break;
			case SF_OSC:	 uOscServo = uServo; break;
		}
	}

	// Pulse Mode Setting
	if (uAmpServo != (-1)) {
		if (mPulseMode(uAmpServo) == LIT_PULSEMODE_ON) {
			// Check for Pulse Mode = ON, with no pulse timers
			if ((mPriPulseTime == 0) || (mBacPulseTime == 0)) {
				// No Pulse Times to Sync to 
				mPulseMode(uAmpServo) = LIT_PULSEMODE_OFF;
			}
		}
	}

	if (uAmpServo != (-1)) {
		if (mPulseMode(uAmpServo) == LIT_PULSEMODE_SYNC) {
			if (uOscServo == (-1)) {
				// No OSC to Sync to 
				mPulseMode(uAmpServo) = LIT_PULSEMODE_OFF;
			}
			else {
				if ((mInDwellTime(uOscServo)   == 0) || 
					(mExcursionTime(uOscServo) == 0) ||
					(mOutDwellTime(uOscServo)  == 0)) {
					// No OSC Times to Sync to 
					mPulseMode(uAmpServo) = LIT_PULSEMODE_OFF;
				}
			}
		}
	}

	// Avc Mode fixup
	if (uAmpServo != (-1)) {
		if (mPulseMode(uAmpServo) == LIT_PULSEMODE_OFF)	{
			if (uAvcServo != (-1)) {
				if (mPulseMode(uAvcServo) == LIT_AVCMODE_SAMP_PRI) 
					mPulseMode(uAvcServo) = LIT_AVCMODE_CONT;  // Not Pulse, therefore Pri Only, therefore enabled

				if (mPulseMode(uAvcServo) == LIT_AVCMODE_SAMP_BAC) {
					mPulseMode(uAvcServo) = LIT_AVCMODE_OFF;   // Not Pulse, therefore never backgnd, therefore off
				}
			}
		}
	}
}

void CheckStartMode(void)
{
	if (!_isAvcCardPresent) {
		if (mStartMode == LIT_STARTMODE_TOUCH) {
			mStartMode = LIT_STARTMODE_RF;
		}
	}
}


//////////////////////////////////////////////////////////////////////
//
// void InitServoStructs(BOOL bServoConfigChange)
//
//////////////////////////////////////////////////////////////////////
void InitServoStructs(BOOL bServoConfigChange)
{
	BYTE uServo;
	SERVO_VALUES* ss;
	OSC_VALUES   *os;
//	SERVO_POSITION_STRUCT *sps;
	
	EraseServoPostionStruct();

	if (WeldSystem.uWeldLevel > mLastLevel) 
	{
		// Level Reset
		WeldSystem.uWeldLevel = 0;
		QueueEventMessage(LEVEL_RST, EV_LEVELCHANGE);
//		dprintf("InitServoStructs::Level Reset\n");
	}

//ETHERNET_Print("InitServoStructs\n");
	_isAvcCardPresent = __FALSE;
	nMasterTvlSlot = (-1); // Reset Master Travel Slot 
	nMasterTravelDirection = LIT_TVL_CCW;  // Setup Default

	// Output something in case there is no AVC servo
	DefaultAvcServoInit();
	SetDesiredAvcResponse(0);
	InvalidModeFixup();

	for (uServo = 0; uServo < mNumServos; ++uServo) {
		ss = &ServoStruct[uServo]; // Save pointer to this servos struct
		ss->uServoIndex    = uServo;
//		ss->bPendantControl = __FALSE;
		ss->bEnabledFlag    = __FALSE;
		ss->bSpiralEnable	= __FALSE;
		ss->uServoSlot      = mServoSlot(uServo);
		ss->uServoControl   = mServoControl(uServo);
		ss->uServoFunction  = mServoFunction(uServo);
		ss->uDirection      = mDirection(uServo);
		SetForwardDirectionBit((BOOL)ss->uDirection, ss);
		ss->bIsJogging      = __FALSE;
		ss->bIsJogging2     = __FALSE;
		ss->wBuffDacValue   = 0; // Start at 0, unless overwritten later
		ss->bDacUpdate      = __TRUE;
		ss->uPulseMode      = mPulseMode(uServo);
//		SetCurrentLimit(mCurrentLimit(uServo), ss->uServoSlot);
//		sps = &ServoPosition[ss->uServoSlot];
		ss->nUnlockTime     = mStartDelay(uServo) + 1;
//		dprintf("InitServoStructs:: Servo: %d nUnlockTime: %d\n", uServo, ss->nUnlockTime);

		// Setup External Dac 1 and 2
		ss->uExternalDac = mExternalDac(uServo);
//		dprintf("InitServoStructs:: Slot: %d Dac %d\n", ss->uServoSlot, ss->uExternalDac);

		switch(ss->uServoFunction) 
        {
		    case SF_CURRENT:
			    fnSetSystemPulseMode(ss);
			    break;
		    
		    case SF_TRAVEL:
			    if (nMasterTvlSlot == (-1))
				{
			    	nMasterTvlSlot = ss->uServoSlot;
    //			dprintf("InitServoStructs:: TRAVEL: Index %d Slot %d\n", uServo, ss->uServoSlot);
				}
			    // Falls through
		    case SF_JOG_ONLY_VEL:
			    SetServoType(ST_VELOCITY, ss->uServoSlot);
			    fnTravelDirectionCommand(ss);
                break;
		    
		    case SF_WIREFEED:
			    SetServoType(ST_VELOCITY, ss->uServoSlot);
			    ss->bRetracting = __FALSE;
    //			dprintf("InitServoStructs:: WIREFEED: Index %d Slot %d\n", uServo, ss->uServoSlot);
			    break;
		    
		    case SF_AVC:
			    _isAvcCardPresent = __TRUE;
    		    nUlpVoltFaultState = ULP_VOLTNOTUSED;
			    switch(mFeedbackDeviceType(uServo)) 
			    {
				    case FEEDBACK_ENCODER:	// Encoder
					    // Encoder feedback
					    SetServoType(ST_VELOCITY, ss->uServoSlot); // Encoder feedback must use the servo as a velocity servo
    //					sps->nCurrentEncoder = sps->nPreviousEncoder = ReadEncoderFeedback(ss->uServoSlot); // Reset Current & Previous encoder position
					    break;
				    case FEEDBACK_BACKEMF: 	// Back EMF feedback
				    case FEEDBACK_ANALOG:     // Analog Tachometer
					    // Analog feedback
					    if (ss->uServoControl == SC_ULPHEAD)
					    {
    //						sps->nCurrentEncoder = sps->nPreviousEncoder = ReadEncoderFeedback(ss->uServoSlot); // Reset Current & Previous encoder position
						    nUlpVoltFaultState = ULP_VOLTDISABLED;
					    }
					    SetServoType(ST_POSITION, ss->uServoSlot); 
					    SetAvcResponseDacValue(mResponse(uServo));
					    //RM SetServoCmdBit(ss->uServoSlot, SLOT_AVC_JOG, __FALSE);
					    break;
			    }

			    if (WeldSystem.uPulseMode == LIT_PULSEMODE_ON)
				    AvcPriCmd(ss);
			    else	
				    AvcBacCmd(ss);

			    CalculateAvcEnableDly(ss); // CALCULATE AVC ENABLE DELAY 
    //			dprintf("InitServoStructs:: AVC: Index %d Slot %d\n", uServo, ss->uServoSlot);
			    StopAvcDrive(ss);
			    break;
		    
		    case SF_OSC:
		    case SF_JOG_ONLY_POS:
			    switch(mFeedbackDeviceType(uServo)) 
				{
				    case FEEDBACK_ENCODER:	// Encoder
					    // Encoder feedback must use the servo as a velocity servo
					    SetServoType(ST_VELOCITY, ss->uServoSlot); 
					    break;
				    case FEEDBACK_BACKEMF: 	// Back EMF feedback
				    case FEEDBACK_ANALOG:     // Analog Tachometer
					    SetServoType(ST_POSITION, ss->uServoSlot);
					    break;
			    }

			    os = (OSC_VALUES *)ss;
			    os->wInDwellTimer = 0;
			    os->wOtDwellTimer = 0;
                os->wExcTimer = mExcursionTime(uServo);
			    if (os->wExcTimer > 1) 
				    os->wExcTimer /= 2;			// Start with 1/2 the excursion time
			    os->dwOwgDac		= OWG_MID_POINT;	// SET OSC WAVEFORM GENERATOR TO MID POINT 
			    os->uDirection		= OSC_SLEW_OUT;		// SET OSC SLEW DIRECTION TO OUT 
			    os->wOscAmp			= 0;
                os->nOscUpperLim	= mMaxSteeringDac(uServo);
                os->nOscLowerLim    = mMinSteeringDac(uServo);
			    os->nOscSlewCount   = 0;				// Set slew count to zero
			    os->lOscSpiralCount = 0;
			    os->uOscSpecialMode	= mOscSpecialMode(uServo);
			    break;
		}
	}
	SetFeedbackType();
	CheckStartMode();
}

//////////////////////////////////////////////////////////////////////
//
// void InitSlopeStruct(DWORD dwSlopeTime)
//
//////////////////////////////////////////////////////////////////////
void InitSlopeStruct(DWORD dwSlopeTime)
{
	WeldSystem.dwSlopeStartTime		    = mSlopeStartTime;
	WeldSystem.Slope.dwSlopeDac			= SLOPE_DAC_MIN;
	WeldSystem.Slope.dwSlopeTimeCount	= dwSlopeTime;

	if (dwSlopeTime != 0)
		WeldSystem.Slope.dwSlopeDacInc = SLOPE_DAC_MAX / dwSlopeTime;
	else
		WeldSystem.Slope.dwSlopeDacInc = SLOPE_DAC_MAX;

//	printf("InitSlopeStruct:: dwSlopeTimeCount %08X\n", WeldSystem.Slope.dwSlopeTimeCount);
//	printf("InitSlopeStruct:: StartTime %08X LevelTimeCnt %08X LevelTime %08X\n\n", WeldSystem.dwSlopeStartTime, WeldSystem.dwLevelTimeCount, mLevelTime);
}

//////////////////////////////////////////////////////////////////////
//
// void InitFaultStruct(void)
//
//////////////////////////////////////////////////////////////////////
void InitFaultStruct(void)
{
	WeldSystem.Fault.uAmpFlowCount       = 0;
	WeldSystem.Fault.uArcOutCount        = 0;
	WeldSystem.Fault.bHighArcVoltFault   = __FALSE;
	WeldSystem.Fault.uInputACCount       = 0;        
	WeldSystem.Fault.bInputAcFault       = __FALSE;
	WeldSystem.Fault.uStuboutCount       = 0;
	WeldSystem.Fault.bStubOutFault       = __FALSE;
	WeldSystem.Fault.uStuboutDly         = 100;
}

//////////////////////////////////////////////////////////////////////
//
// void InitSystemStruct(void)
//
//////////////////////////////////////////////////////////////////////
void InitSystemStruct(void)
{
	WeldSystem.uPulsePhase = IN_PRIMARY;
	WeldSystem.uInTransistion = 5;
	if (WeldSystem.uPulseMode == LIT_PULSEMODE_SYNC)
		WeldSystem.uPulsePhase = IN_BACKGND;
	
//	uDesiredLevel					= 0;
	WeldSystem.uWeldLevel           = uDesiredLevel;
	WeldSystem.wPulseDuration		= mPriPulseTime;
	WeldSystem.wPriPulseTimer		= mPriPulseTime;
	WeldSystem.wBacPulseTimer		= 0;
	WeldSystem.uCurrentFaultDelay	= 100;
	WeldSystem.bArcEstablished		= __FALSE;
	SetArcOnBit(WeldSystem.bArcEstablished);
	WeldSystem.bBadStart			= __FALSE;
	WeldSystem.uArcStartPhase		= AS_PRESTRIKE;
	WeldSystem.dwLevelTimeCount		= 0;
	SetLevelTime(mLevelTime);
	WeldSystem.dwSlopeTime			= 0;
	WeldSystem.dwSlopeStartTime		= 1;  // Need a non-zero value to start sloping
	WeldSystem.uInNegStopDelay		= __FALSE;

	WeldSystem.uDataAqiOutEnable    = __FALSE;
	QueueEventMessage(SYSTEM_EVENT, EV_DASTREAM_OFF);

	WeldSystem.wDataAqiRate			= stInitWeldSched.stDataAqi.wSampleRate;
	if (WeldSystem.wDataAqiRate == 0)
		WeldSystem.wDataAqiRate = 100;
	WeldSystem.uDataAqiStart		= stInitWeldSched.stDataAqi.uStartAt;
	WeldSystem.uDataAqiStop			= stInitWeldSched.stDataAqi.uStopAt;

	InitFaultStruct();
	if (mUpslopeTime) 
	{
		InitSlopeStruct(mUpslopeTime);
		WeldSystem.dwSlopeStartTime	=  1;  // Need a non-zero value to start sloping
	}
	else {
		InitSlopeStruct(1);
		WeldSystem.dwSlopeStartTime	=  0;  // Need a non-zero value to start sloping
	}
}

//////////////////////////////////////////////////////////////////////
//
// void SetStartLevel(void)
//
//////////////////////////////////////////////////////////////////////
static void SetStartLevel(void)
{
	SetCommandDacValue(mStartLevel, &ServoStruct[STD_AMP_SLOT]);
////ETHERNET_Print("SetStartLevel:: wValue %0X\n", wStartLevel);
}

//////////////////////////////////////////////////////////////////////
//
// void SetTouchStartLevel(void)
//
//////////////////////////////////////////////////////////////////////
void SetTouchStartLevel(void)
{
	SetCommandDacValue((short)mTouchStartCurrent, &ServoStruct[STD_AMP_SLOT]);
}

//////////////////////////////////////////////////////////////////////
//
// BYTE PreStrike(void)
//
//////////////////////////////////////////////////////////////////////
static BYTE PreStrike(void)
{
	if (uSystemWeldPhase != WP_PREPURGE) 
	{
		uSystemWeldPhase = WP_EXIT;
		SetCurrentEnableBit(__FALSE);
		SetBoosterEnableBit(__FALSE);
		SetBleederDisableBit(__FALSE); // Enable the bleeder
		WeldSystem.bBadStart = __TRUE;
		uStoppedOnFault = __TRUE;
//		printf("Prestrike was failure\n");
		return __FALSE; // Failure of the PreStrike Phase
	}
   
	SetBoosterEnableBit(__FALSE);
	CommandBoosterVoltage(0);
	SetStartLevel();
	SetCurrentEnableBit(__TRUE);
//ETHERNET_Print("Prestrike was good\n");
	return __TRUE; // PreStrike was sucessfull
}

//////////////////////////////////////////////////////////////////////
//
// void RfStart(void)
//
//////////////////////////////////////////////////////////////////////
static void RfStart(void)
{   
	switch(WeldSystem.uArcStartPhase) 
	{
	case AS_PRESTRIKE:   // PreStrike
//		dprintf("RF Start: PRESTRIKE\n");
		if (!PreStrike())
			return; // Failure during PreStrike
		
		SetRfStartBit(__TRUE);
		WeldSystem.uRfTime = 50; // 500 mSeconds
		WeldSystem.uArcStartPhase = AS_RFSTARTING;
//		dprintf("RF On!!!\n");
		break;
	
	case AS_RFSTARTING: // RF'ing
		if (PsInRegulation()) 
		{
			// Current is now flowing; we can stop RFing
			WeldSystem.uRfTime = 1; // decrement to zero on next line
		}

		if (--WeldSystem.uRfTime)
			return; // RF not done

//		dprintf("RF Off!!\n");
		SetRfStartBit(__FALSE);
		SetBoosterEnableBit(__FALSE);
		WeldSystem.wAbortTime = 100;
		WeldSystem.uArcStartPhase = AS_EVALUATE;
		break;
	
	case AS_RETRY_PHASE:
		SetBoosterEnableBit(__TRUE);
		CommandBoosterVoltage(nBoosterVoltage);
		if ((WeldSystem.wAbortTime < 300) &&
			(WeldSystem.wAbortTime > 5)) 
		{
			SetStartLevel();							
			SetCurrentEnableBit(__TRUE);	
		}
		if (--WeldSystem.wAbortTime)
			return; // Still waiting
		WeldSystem.uArcStartPhase = AS_PRESTRIKE; 
		break;
	}
}

void DriveAvcInSequence(const SERVO_VALUES* ss, int nServoSpeed)
{
	if (nServoSpeed < 0) 
	{
		SetCommandDacValue((short)0, ss);
	}
	else 
	{
		SetCommandDacValue((short)POS_DAC_SWING, ss);
	}
}


void DriveAvcOutSequence(const SERVO_VALUES* ss, int nServoSpeed)
{
	WORD wResponseDacValue;

	SetServoCmdBit(ss->uServoSlot, SLOT_AVC_JOG, __TRUE);
	if (nServoSpeed < 0)
	{
		wResponseDacValue = (WORD)((mJogSpeedReverse(ss->uServoIndex) * abs(nServoSpeed)) / 100);
		SetDesiredAvcResponse(wResponseDacValue); // Allow it to Ramp
		SetCommandDacValue((short)AVC_NEG_JOG, ss);
	}
	else 
	{
		wResponseDacValue = (WORD)((mJogSpeedForward(ss->uServoIndex) * abs(nServoSpeed)) / 100);
		SetDesiredAvcResponse(wResponseDacValue); // Allow it to Ramp
		SetCommandDacValue(AVC_POS_JOG, ss);
	}
    // printf("DriveAvcOutSequence: wResponseDacValue = %X\n", wResponseDacValue);
}

// StartAvcDrive is used during closed loop operations
static void StartAvcDrive(SERVO_VALUES* ss, int nServoSpeed)
{
	if (WeldSystem.bArcEstablished)
	{
		DriveAvcInSequence(ss, nServoSpeed);
	}
	else
	{
		DriveAvcOutSequence(ss, nServoSpeed);
	}
	SetEnableBit(__TRUE, ss);
	ss->bIsJogging = __TRUE;
}

// StopAvcDrive is used during closed loop operations
void StopAvcDrive(SERVO_VALUES* ss)
{
	// Jog completed, Return to normal operation
	SetServoCmdBit(ss->uServoSlot, SLOT_AVC_JOG, __FALSE);
	SetAvcResponseDacValue(mResponse(ss->uServoIndex));
	SetCommandDacValue((short)mPrimaryValue(ss->uServoIndex), ss); // RESTORE AVC COMMAND 

	if ((ss->bEnabledFlag) && (ss->wEnableDelay == 0))
		SetEnableBit(__TRUE, ss);
	else
		SetEnableBit(__FALSE, ss);
	ss->bIsJogging = __FALSE;
}


// MoveAvcServo is used during closed loop operations
void MoveAvcServo(SERVO_VALUES* ss, int nServoSpeed)
{
	if (nServoSpeed != 0)
		StartAvcDrive(ss, nServoSpeed);
	else 
		StopAvcDrive(ss);
}

// JogAvcServo is used for all jog commands
// For driving the avc into position use MoveAvcServo
void JogAvcServo(SERVO_VALUES* ss, int nServoSpeed)
{
	int nProgramJogValue;

	switch(ss->uServoControl)
	{
		case SC_ULPHEAD:
			if (nServoSpeed < 0) 
			{
				nProgramJogValue = (int)mJogSpeedReverse(ss->uServoIndex);
			}
			else 
			{
				nProgramJogValue = (int)mJogSpeedForward(ss->uServoIndex);
			}
			ss->nJogSpeed2 = (nProgramJogValue * nServoSpeed) / 100;

			ss->bIsJogging2 = __FALSE;
			if (ss->nJogSpeed2 != 0) 
			{
				ss->bIsJogging2 = __TRUE;
			}
			break;

		case SC_OPENLOOP:
			// For open loop operations
			if (nServoSpeed < 0) 
			{
				nProgramJogValue = (int)mJogSpeedReverse(ss->uServoIndex);
			}
			else 
			{
				nProgramJogValue = (int)mJogSpeedForward(ss->uServoIndex);
			}
			ss->nJogSpeed = (nProgramJogValue * nServoSpeed) / 100;

			if (ss->nJogSpeed != 0) 
			{
				ss->bIsJogging = __TRUE;
			}
			else 
			{
				ss->bIsJogging = __FALSE;
			}
			break;

		case SC_CLOSEDLOOP:
			// For closed loop operations
			MoveAvcServo(ss, nServoSpeed);
			break;
	}

}

static void JogOscServo(OSC_VALUES *ss, int nServoSpeed)
{
	if (nServoSpeed >= 0)
		ss->nJogSpeed = ((int)mJogSpeedForward(ss->uServoIndex) *	nServoSpeed) / (int)100;
	else
		ss->nJogSpeed = ((int)mJogSpeedReverse(ss->uServoIndex) *	nServoSpeed) / (int)100;

	ss->bIsJogging = __FALSE;
	if (ss->nJogSpeed != 0)
		ss->bIsJogging = __TRUE;
}

// defines are: JOG_COMMAND for __TRUE jog functions and 
//              DRIVE_COMMAND for positioning commands
void  JogServo(JOG_STRUCT const *pJogStruct)
{
	SERVO_VALUES* ss;
	int  nServoSpeed;
	short nExtDacCommand;

	nServoSpeed = pJogStruct->uSpeed;
	ss = &ServoStruct[pJogStruct->uServo];
	//ETHERNET_Print("JogServo:: Servo: %X Speed: %x\n",pJogStruct->uServo, nServoSpeed);

	switch(ss->uServoFunction) 
	{
		case SF_TRAVEL:
			JogVelocityServo(ss, -nServoSpeed);
			break;
		case SF_WIREFEED:
		case SF_JOG_ONLY_VEL:
			JogVelocityServo(ss, nServoSpeed);
			break;
		
		case SF_AVC:
			JogAvcServo(ss, nServoSpeed);
			break;

		case SF_OSC:
		case SF_JOG_ONLY_POS:
			JogOscServo((OSC_VALUES *)ss, nServoSpeed);
			break;
	}

	if (nServoSpeed == 0) {
		nExtDacCommand = 0;
	}
	else {
		if (nServoSpeed > 0) {
			nExtDacCommand = ((mExtDacJogSpeedFwd(pJogStruct->uServo) * nServoSpeed) / 100);
		}
		else {
			nExtDacCommand = ((mExtDacJogSpeedRev(pJogStruct->uServo) * nServoSpeed) / 100);
		}
	}

	JogExternalDacs(nExtDacCommand, ss);
}

//////////////////////////////////////////////////////////////////////
//
// void TouchStartLoop(SERVO_VALUES* ss, bUseJogCommands)
//
//////////////////////////////////////////////////////////////////////
static void TouchStartLoop(SERVO_VALUES* ss)
{
	int nSpeed;

	switch(WeldSystem.uArcStartPhase) 
	{
		case AS_PRESTRIKE:   // PreStrike
			if (!PreStrike())
			{
				return;        // Failure during PreStrike
			}
			
			SetTouchStartLevel();
			if ((ss->uServoControl == SC_OPENLOOP) ||
				(ss->uServoControl == SC_ULPHEAD))
			{
				// Convert exiting jog speeds to percent jog commands
				nSpeed = mTouchSpeed(ss->uServoIndex);
				nSpeed *= (-1);  // Make TOUCH a jog down
				JogAvcServo(ss, nSpeed);
			}
			else 
			{
				SetServoCmdBit(ss->uServoSlot, SLOT_AVC_JOG, __TRUE);
				SetAvcResponseDacValue(mTouchSpeed(ss->uServoIndex));
				SetCommandDacValue((short)AVC_NEG_JOG, ss);
				SetEnableBit(__TRUE, ss);
			}

			WeldSystem.wAbortTime = 1000; // Wait for 10 Seconds for torch to touch
			WeldSystem.uArcStartPhase = AS_DRIVEDOWN; // Driving Down
//			dprintf("TouchStartLoop:: Executing AS_DRIVEDOWN\n");
			// Falls through
			
		case AS_DRIVEDOWN:   // Driving Down
			// Waiting for Arc Volts to go to 0
			if (!PsInRegulation() && (WeldSystem.wAbortTime))
			{
				--WeldSystem.wAbortTime;
				return;
			}
			
//			dprintf("TouchStartLoop:: Executing AS_DRIVEUP  wAbortTime:%d\n",WeldSystem.wAbortTime);
			if ((ss->uServoControl == SC_OPENLOOP) ||
				(ss->uServoControl == SC_ULPHEAD))
			{
				nSpeed = mRetractSpeed(ss->uServoIndex);
				JogAvcServo(ss, nSpeed);
			}
			else 
			{
				SetCommandDacValue(AVC_POS_JOG, ss);
				//SetAvcResponseDacValue((WORD)-4000);//mRetractSpeed(ss->uServoIndex)); // Set retract speed
				nSpeed = mRetractSpeed(ss->uServoIndex);

				SetAvcResponseDacValue((WORD)mRetractSpeed(ss->uServoIndex)); // Set retract speed
			}
			WeldSystem.wAbortTime = 30; // Drive up for 300 mSeconds
			WeldSystem.uArcStartPhase = AS_DRIVEUP;
			break;
			
		case AS_DRIVEUP: // Driving Up
			if (--WeldSystem.wAbortTime)
				return; // Still driving up
			
			if ((ss->uServoControl == SC_OPENLOOP) ||
				(ss->uServoControl == SC_ULPHEAD))
			{
				JogAvcServo(ss, 0);
			}
			else 
			{
				SetEnableBit(__FALSE, ss);
				SetAvcResponseDacValue(mResponse(ss->uServoIndex));
				SetServoCmdBit(ss->uServoSlot, SLOT_AVC_JOG, __FALSE);
				SetCommandDacValue((short)mPrimaryValue(ss->uServoIndex), ss); // RESTORE AVC COMMAND 
			}
			SetStartLevel();
			SetBoosterEnableBit(__FALSE);
			WeldSystem.wAbortTime = 100; // Wait for 1 second for current to flow
			WeldSystem.uArcStartPhase = AS_EVALUATE;
			break;
			
		case AS_RETRY_PHASE: // Driving Up
			if (--WeldSystem.wAbortTime)
				return; // Still waiting
			WeldSystem.uArcStartPhase = AS_PRESTRIKE; 
			break;
	}
}

//////////////////////////////////////////////////////////////////////
//
// void TouchStart(void)
//
//////////////////////////////////////////////////////////////////////
static void TouchStart(void)
{
	SERVO_VALUES* ss;
	BYTE uServo;

	for (uServo = 0; uServo < mNumServos; ++uServo) {
		ss = &ServoStruct[uServo];
		if (ss->uServoFunction == SF_AVC) {
			TouchStartLoop(ss);
			return; // Get out now, we can only handle one servo doing a touch start
		}
	}

}

void AvcJogBackLoop(SERVO_VALUES* ss)
{
	int nSpeed;

	switch(WeldSystem.uArcStartPhase) {
		case AS_PRESTRIKE:   // PreStrike
			WeldSystem.uArcStartPhase = AS_DRIVEDOWN; // Driving Down
			// Falls through
			
		case AS_DRIVEDOWN:   // Driving Down
//			dprintf("AvcJogBackLoop:: Executing AS_DRIVEUP  wAbortTime:%d\n",WeldSystem.wAbortTime);
			if ((ss->uServoControl == SC_OPENLOOP) ||
				(ss->uServoControl == SC_ULPHEAD))
			{
				nSpeed = mRetractSpeed(ss->uServoIndex);
				JogAvcServo(ss, nSpeed);
			}
			else 
			{
				SetServoCmdBit(ss->uServoSlot, SLOT_AVC_JOG, __TRUE);
				SetCommandDacValue(AVC_POS_JOG, ss);
				SetAvcResponseDacValue(mRetractSpeed(ss->uServoIndex)); // Set retract speed
				SetEnableBit(__TRUE, ss);
			}
			WeldSystem.wAbortTime = 30; // Drive up for 300 mSeconds
			WeldSystem.uArcStartPhase = AS_DRIVEUP;
			break;
			
		case AS_DRIVEUP: // Driving Up
			if (--WeldSystem.wAbortTime)
				return; // Still driving up
			
			if ((ss->uServoControl == SC_OPENLOOP) ||
				(ss->uServoControl == SC_ULPHEAD))
			{
				JogAvcServo(ss, 0); // Stop Jogging
			}
			else {
				SetEnableBit(__FALSE, ss);
				SetAvcResponseDacValue(mResponse(ss->uServoIndex));
				SetServoCmdBit(ss->uServoSlot, SLOT_AVC_JOG, __FALSE);
				SetCommandDacValue((short)mPrimaryValue(ss->uServoIndex), ss); // RESTORE AVC COMMAND 
			}
			SetBoosterEnableBit(__FALSE);
			WeldSystem.wAbortTime = 100; // Wait for 1 second for current to flow
			WeldSystem.uArcStartPhase = AS_EVALUATE;
			break;
			
		case AS_RETRY_PHASE: // Driving Up
			if (--WeldSystem.wAbortTime)
				return; // Still waiting
			WeldSystem.uArcStartPhase = AS_PRESTRIKE; 
			break;
		}
}


void AvcJogBack(void)
{
	SERVO_VALUES* ss;
	BYTE uServo;

	for (uServo = 0; uServo < mNumServos; ++uServo) {
		ss = &ServoStruct[uServo];
		if (ss->uServoFunction == SF_AVC) {
			AvcJogBackLoop(ss);
			return;  // Get out now, we can only handle one servo doing a Jog Back
		}
	}
}


//////////////////////////////////////////////////////////////////////
//
// BOOL GotArc(void)
//
//////////////////////////////////////////////////////////////////////
static BOOL GotArc(void) 
{
	BOOL bStatus = __TRUE;

	if (!PsInRegulation()) {
//		dprintf("GotArc:: Current Failed\n");
		bStatus = __FALSE;
	}

	if (ArcBelowMin()) {
//		dprintf("GotArc:: Voltage Failed\n");
		bStatus = __FALSE;
	}

	return bStatus;

}

//////////////////////////////////////////////////////////////////////
//
// void PostStrike(void)
//
//////////////////////////////////////////////////////////////////////
void PostStrike(void)
{
	if (!PsInRegulation() && WeldSystem.wAbortTime) {
		--WeldSystem.wAbortTime;
		return;
	}
	
	SetBoosterEnableBit(__FALSE);
	
/*
	if (mGNDFaultUsed) 
	{
		dwElapsedTime += GROUND_FAULT_TIME;
	}
*/

	if (!GotArc())
	{
//		printf("PostStrike:: Current + Voltage Failed\n");
		WeldSystem.bBadStart = __TRUE;
		if ((uSystemWeldPhase != WP_EXIT) && (WeldSystem.uStartAttempts))
		{
//			printf("PostStrike: Start Attempts = %d\n", WeldSystem.uStartAttempts);
			--WeldSystem.uStartAttempts;
			if (WeldSystem.uStartAttempts)
			{
				if (mStartMode == LIT_STARTMODE_RF)
				{
					SetCurrentEnableBit(__FALSE);
					SetBoosterEnableBit(__TRUE);
					WeldSystem.wAbortTime = 500; // Wait 5000 milliseconds for Booster to charge
				}
				else
				{
					WeldSystem.wAbortTime = 2; // Wait 20 milliseconds before starting again
				}
				WeldSystem.uArcStartPhase = AS_RETRY_PHASE;
				return;
			}
			WeldSystem.bBadStart = __TRUE;
			WeldSystem.bArcEstablished = __FALSE;
			SetArcOnBit(WeldSystem.bArcEstablished);
//			printf("PostStrike:: Bad Start!!\n");
			QueueEventMessage(SYSTEM_EVENT, EV_BADSTART);
			uSystemWeldPhase = WP_EXIT; // it was exit, or out of attempts
		}
		return;
	}

	WeldSystem.bBadStart = __FALSE;
	WeldSystem.bArcEstablished = __TRUE;
	SetArcOnBit(WeldSystem.bArcEstablished);
	uSystemWeldPhase = WP_UPSLOPE;
	if (WeldSystem.Slope.dwSlopeTimeCount == 0)
	{
		--dwElapsedTime;
	}

	if (WeldSystem.uDataAqiStart == DA_STARTAT_ARC)
	{
		WeldSystem.uDataAqiOutEnable = __TRUE;
		QueueEventMessage(SYSTEM_EVENT, EV_DASTREAM_ON);
	}

	QueueEventMessage(SYSTEM_EVENT, EV_ARCSTART_COMPLETE);
//	printf("Arc start COMPLETE!!\n");
}

//////////////////////////////////////////////////////////////////////
//
// void ArcStart(void)
//
//////////////////////////////////////////////////////////////////////
void ArcStart(void)
{
	if (!WeldSystem.uWeldMode)
	{
//		printf("ArcStart:: Not in weld mode\n");
		if (mStartMode == LIT_STARTMODE_RF)
		{
//			dprintf("ArcStart:: Not in weld mode, RF START mode\n");
			uSystemWeldPhase = WP_UPSLOPE;
			return;
		}

		if (WeldSystem.uArcStartPhase != AS_EVALUATE)
		{
//			dprintf("ArcStart:: Not in weld mode, AvcJogBack()\n");
			AvcJogBack();
			return;
		}

//		printf("ArcStart:: Not in weld mode, GO TO UPSLOPE\n");
		uSystemWeldPhase = WP_UPSLOPE;
		return;
	}

	//ETHERNET_Print("ArcStart:: In weld mode\n");
	if (WeldSystem.uArcStartPhase != AS_EVALUATE)
	{
		switch (mStartMode)
		{
			case LIT_STARTMODE_TOUCH:
				TouchStart();
				break;
				
			case LIT_STARTMODE_RF:
			default:
				RfStart();
				break;
		}
		return;
	}
	
	PostStrike();
}

static BOOL SetPotInProgress(void)
{
	BYTE uServo;

	for(uServo = 0; uServo < mNumServos; ++uServo)
	{
		if(bPerformSetPot[uServo])
		{
			return __TRUE;
		}
	}

	return __FALSE;

}

//////////////////////////////////////////////////////////////////////
//
// void DoSetPot(void)
//
//////////////////////////////////////////////////////////////////////
static void DoSetPot(void)
{
	BYTE uServo;

	for (uServo = 0; uServo < mNumServos; ++uServo) 
	{
		if (mSetFunctionUsed(uServo)) 
		{
			if (mStartMode == LIT_STARTMODE_RF)
			{
				WeldSystem.uSetPotPhase = 0;
				WeldSystem.uArcStartPhase = AS_PRESTRIKE;
				bPerformSetPot[uServo] = __TRUE;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
//
// void fnInitUpslope(void)
//
//////////////////////////////////////////////////////////////////////
static void fnInitUpslope(void)
{
	if (_wServoInitDelay != 0)
		return; // Wait for Osc Enable

	if (SetPotInProgress())
		return; // Set Pot Function in progress

//	printf("First line of InitUpslope\r\n");
	SendServoPosition(__TRUE); // Ensure Data Acq. has correct data
//	QueueLevelTime();
	uStoppedOnFault = __FALSE;
	uLockFaults = __TRUE;
	InitSystemStruct();
	InitServoStructs(__FALSE);
	
	if (WeldSystem.uDataAqiStart == DA_STARTAT_PPGE)
	{
		WeldSystem.uDataAqiOutEnable = __TRUE;
		QueueEventMessage(SYSTEM_EVENT, EV_DASTREAM_ON);
	}

	if (mUpslopeTime) 
	{
		InitSlopeStruct(mUpslopeTime);
		WeldSystem.dwSlopeStartTime	=  1;  // Need a non-zero value to start sloping
	}
	else {
		InitSlopeStruct(1);
		WeldSystem.dwSlopeStartTime	=  0;  // Need a non-zero value to start sloping
	}

	if (!WeldSystem.uWeldMode) {
		WeldSystem.uArcStartPhase = AS_PRESTRIKE;
		WeldSystem.Slope.dwPurgeTime = 0; // zero arc start time to go on to arc start
		uSystemWeldPhase = WP_PREPURGE;
		QueueEventMessage(SYSTEM_EVENT, EV_WELDPHASE_PREPURGE);
		if (WeldSystem.uDataAqiStart == DA_STARTAT_ARC)
		{
			WeldSystem.uDataAqiOutEnable = __TRUE;
			QueueEventMessage(SYSTEM_EVENT, EV_DASTREAM_ON);
		}
//		printf("Now to Upslope-Not in Weld Mode\r\n");
		return;
	}

	///////////////////////
	// IN WELD MODE ONLY //
	///////////////////////
	
	SetBleederDisableBit(__TRUE);
	if (mStartMode == LIT_STARTMODE_RF) {
		CommandBoosterVoltage(nBoosterVoltage);
		SetBoosterEnableBit(__TRUE);
//		dprintf("fnInitUpslope:: Booster Active-RF Mode\n");
	}
	
	SetGasValveOpen(__TRUE);
//	printf("Gas valve open\n");

	WeldSystem.Slope.dwPurgeTime = mPrePurgeTime;
	if (WeldSystem.Slope.dwPurgeTime == 0)
	{
		WeldSystem.Slope.dwPurgeTime = 1;  // Set to minimum time
	}

/*
	if (mGNDFaultUsed) 
	{
		if ((WeldSystem.Slope.dwPurgeTime + 1) > GROUND_FAULT_TIME)
		{
			WeldSystem.Slope.dwPurgeTime -= GROUND_FAULT_TIME;
		}
	}
*/

	if (WeldSystem.Slope.dwPurgeTime < MINIMUM_PREPURGE_TIME)
    {
		WeldSystem.Slope.dwPurgeTime = (DWORD)MINIMUM_PREPURGE_TIME;
    }

	uSystemWeldPhase = WP_PREPURGE;
	dwElapsedTime = 0;
	QueueEventMessage(SYSTEM_EVENT, EV_WELDPHASE_PREPURGE);

//	printf("Now to Prepurge-Weld Mode\n");
}  

//////////////////////////////////////////////////////////////////////
//
// BYTE DecrementTimer(SERVO_VALUES* ss)
//
// Decrement a timer and return __TRUE if it did time out
//////////////////////////////////////////////////////////////////////
static BYTE DecrementTimer(SERVO_VALUES* ss)
{
	if (ss->nUnlockTime < 0)
		return __FALSE; // Do not decrement the negitive delays

	if (!ss->nUnlockTime)
		return __FALSE; // Timer has alread expired

	--ss->nUnlockTime;

	if (ss->nUnlockTime)
		return __FALSE; // Timer is still running

	return __TRUE; // Timer has just timed out;
}

//////////////////////////////////////////////////////////////////////
//
// void fnDisableServo(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
void fnDisableServo(SERVO_VALUES* ss)
{
	SetEnableBit(__FALSE, ss);
	ss->bEnabledFlag = __FALSE;
//ETHERNET_Print("fnDisableServo:: Servo slot %d disabled\n", ss->uServoSlot);
}

//////////////////////////////////////////////////////////////////////
//
// void DecAvcUnlock(SERVO_VALUES* ss)
//
// This functions is used for both start & stop delays            
//////////////////////////////////////////////////////////////////////
static void DecAvcUnlock(SERVO_VALUES* ss)
{
	if (!WeldSystem.uWeldMode) 
		return;
	
	if (!DecrementTimer(ss))
		return;

	if (uSystemWeldPhase == WP_DOWNSLOPE) 
	{
		fnDisableServo(ss);
		QueueEventMessage(ss->uServoIndex, SERVO_LOCK);
	}
	else {
		ss->bEnabledFlag = __TRUE; // Set enable flag, turn on servo if allowed
		QueueEventMessage(ss->uServoIndex, SERVO_UNLOCK);
		if (ss->wEnableDelay != 0)
		{
			return; // Enable delay is still running
		}
		fnEnableServo(ss);
	}
}
            
//////////////////////////////////////////////////////////////////////
//
// void DecTvlUnlock(SERVO_VALUES* ss)
//
// This functions is used for both start & stop delays            
//////////////////////////////////////////////////////////////////////
void DecTvlUnlock(SERVO_VALUES* ss)
{
	if (!DecrementTimer(ss))				
		return;

	if (uSystemWeldPhase == WP_DOWNSLOPE) {
		fnDisableServo(ss);
		QueueEventMessage(ss->uServoIndex, SERVO_LOCK);
	}
	else {
		fnEnableServo(ss);
		QueueEventMessage(ss->uServoIndex, SERVO_UNLOCK);
	}
}

void InitWireInTime(LPSERVO_VALUES ss)
{
	ss->bRetracting = __TRUE;
	ss->wWireInTime = mWireRetractTime(ss->uServoIndex);
	if (ss->wWireInTime == 0)
		ss->wWireInTime = 1;
}

//////////////////////////////////////////////////////////////////////
//
// void DecWfdUnlock(LPSERVO_VALUES ss)
//
// This functions is used for both start & stop delays            
//////////////////////////////////////////////////////////////////////
void DecWfdUnlock(LPSERVO_VALUES ss)
{
	if (!DecrementTimer(ss))
		return;
	
	if (!WeldSystem.uWireFeedModeAuto[ss->uServoIndex])
		return;
	
	if (uSystemWeldPhase != WP_DOWNSLOPE) {
		fnEnableServo(ss);
//		printf("WFD Servo Unlock\n");
		QueueEventMessage(ss->uServoIndex, SERVO_UNLOCK);
		return;
	}

	if (ss->bEnabledFlag)
	{
		InitWireInTime(ss);
		QueueEventMessage(ss->uServoIndex, SERVO_LOCK);
		return;
	}

	fnDisableServo(ss);
	QueueEventMessage(ss->uServoIndex, SERVO_LOCK);
}
            
//////////////////////////////////////////////////////////////////////
//
// void DecSpiralUnlock(SERVO_VALUES* ss)
//
// This functions is used for both start & stop delays            
//////////////////////////////////////////////////////////////////////
static void DecSpiralUnlock(SERVO_VALUES* ss)
{
	if (!DecrementTimer(ss))				
		return;

//	ETHERNET_Print("DecSpiralUnlock: TIMEOUT\n");
	if (uSystemWeldPhase == WP_DOWNSLOPE) {
		ss->bSpiralEnable = __FALSE; // Stop Spiraling
//		dprintf("DecSpiralUnlock: LOCKED\n");
		QueueEventMessage(ss->uServoIndex, SERVO_LOCK);
	}
	else {
		ss->bSpiralEnable = __TRUE; // Start Spiraling
//		dprintf("DecSpiralUnlock: UNLOCK\n");
		QueueEventMessage(ss->uServoIndex, SERVO_UNLOCK);
	}
}

//////////////////////////////////////////////////////////////////////
//
// void fnDecrementUnlockTimers(void)
//
//////////////////////////////////////////////////////////////////////
static void fnDecrementUnlockTimers(void)
{
	SERVO_VALUES* ss;
	BYTE         uServo;
	
	for (uServo = 0; uServo < mNumServos; ++uServo) 
	{
		ss = &ServoStruct[uServo];
		
		switch(ss->uServoFunction) 
		{
			case SF_TRAVEL:		DecTvlUnlock(ss);	 break;
			case SF_WIREFEED:	DecWfdUnlock(ss);	 break;
			case SF_AVC:		DecAvcUnlock(ss);	 break;
			case SF_OSC:		DecSpiralUnlock(ss); break;
			default:								 break;
		}
	}
}

//////////////////////////////////////////////////////////////////////
//
// void fnUpdatePrimaryValues(void)
//
//////////////////////////////////////////////////////////////////////
static void fnUpdatePrimaryValues(void)
{
	SERVO_VALUES* ss;
	BYTE         uServo;
	for (uServo = 0; uServo < mNumServos; ++uServo) 
	{
		ss = &ServoStruct[uServo];

		switch(ss->uServoFunction) 
		{
			case SF_CURRENT:	AmpPriCmd(ss);	break;
			case SF_TRAVEL:		TvlPriCmd(ss);	break;
			case SF_WIREFEED:	WfdPriCmd(ss);	break;
			case SF_AVC:		AvcPriCmd(ss);	break;
		}
	}
}

//////////////////////////////////////////////////////////////////////
//
// void fnUpdatePrimary2BackgndValues(void)
//
//////////////////////////////////////////////////////////////////////
static void fnUpdatePrimary2BackgndValues(void)
{
	SERVO_VALUES* ss;
	BYTE         uServo;

	WeldSystem.uCurrentFaultDelay = 5;
	for (uServo = 0; uServo < mNumServos; ++uServo) 
	{
		ss = &ServoStruct[uServo];
		switch(ss->uServoFunction) 
		{
			case SF_CURRENT:	AmpBacCmd(ss);		break;
			case SF_TRAVEL:		TvlBacCmd(ss);		break;
			case SF_WIREFEED:	WfdBacCmd(ss);		break;
			case SF_AVC:		AvcPriToBacCmd(ss);	break;
		}
	}
}

//////////////////////////////////////////////////////////////////////
//
// void fnUpdateBackgndValues(void)
//
//////////////////////////////////////////////////////////////////////
static void fnUpdateBackgndValues(void)
{
	SERVO_VALUES* ss;
	BYTE         uServo;

	for (uServo = 0; uServo < mNumServos; ++uServo) 
	{
		ss = &ServoStruct[uServo];
		switch(ss->uServoFunction) 
		{
			case SF_CURRENT:	AmpBacCmd(ss);	break;
			case SF_TRAVEL:		TvlBacCmd(ss);	break;
			case SF_WIREFEED:	WfdBacCmd(ss);	break;
			case SF_AVC:		AvcBacCmd(ss);	break;
		}
	}
}


//////////////////////////////////////////////////////////////////////
//
// void fnUpdateBackgnd2PrimaryValues(void)
//
//////////////////////////////////////////////////////////////////////
static void fnUpdateBackgnd2PrimaryValues(void)
{
	SERVO_VALUES* ss;
	BYTE         uServo;

	WeldSystem.uCurrentFaultDelay = 5;
	for (uServo = 0; uServo < mNumServos; ++uServo) 
	{
		ss = &ServoStruct[uServo];
		switch(ss->uServoFunction) 
		{
			case SF_CURRENT:	AmpPriCmd(ss);		break;
			case SF_TRAVEL:		TvlPriCmd(ss);		break;
			case SF_WIREFEED:	WfdBacToPriCmd(ss); break;
			case SF_AVC:		AvcBacToPriCmd(ss); break;
		}
	}
}

//////////////////////////////////////////////////////////////////////
//
// void fnExecutePriPulseTimer(void)
//
//////////////////////////////////////////////////////////////////////
void fnExecutePriPulseTimer(void)
{
	if (WeldSystem.wPriPulseTimer != 0) 
	{
		--WeldSystem.wPriPulseTimer;
		
		if (WeldSystem.wPriPulseTimer == 0) 
		{
			if (WeldSystem.uPulseMode == LIT_PULSEMODE_ON) 
			{
				WeldSystem.uPulsePhase = PRIMARY2BACKGND;
				WeldSystem.wPulseDuration = mBacPulseTime;
				WeldSystem.uInTransistion = 5;
			}

			WeldSystem.wBacPulseTimer = mBacPulseTime;
			++WeldSystem.wBacPulseTimer; // add 1 due to pending decrement;
		}
	}
}

//////////////////////////////////////////////////////////////////////
//
// void fnExecuteBacPulseTimer(void)
//
//////////////////////////////////////////////////////////////////////
void fnExecuteBacPulseTimer(void)
{
	if (WeldSystem.wBacPulseTimer != 0) 
	{
		--WeldSystem.wBacPulseTimer;
		if (WeldSystem.wBacPulseTimer == 0) 
		{
			if (WeldSystem.uPulseMode == LIT_PULSEMODE_ON) 
			{
				WeldSystem.uPulsePhase = BACKGND2PRIMARY;
				WeldSystem.wPulseDuration = mPriPulseTime;
				WeldSystem.uInTransistion = 5;
			}

			WeldSystem.wPriPulseTimer = mPriPulseTime;
		}
	}
}

//////////////////////////////////////////////////////////////////////
//
// void fnExecutePulseTimers(void)
//
//////////////////////////////////////////////////////////////////////
void fnExecutePulseTimers(void)
{
	fnExecutePriPulseTimer();
	fnExecuteBacPulseTimer();
}

//////////////////////////////////////////////////////////////////////
//
// void fnExecuteInDwellTimer(OSC_VALUES *ss)
//
//////////////////////////////////////////////////////////////////////
void fnExecuteInDwellTimer(OSC_VALUES *ss)
{
	if (ss->wInDwellTimer != 0) 
	{
		--ss->wInDwellTimer;

		if (ss->wInDwellTimer) 
		{
			if (ss->uPulseMode == OSC_MODE_ON)
			{
				ss->dwOwgDac = OWG_IN_POINT; 
			}
			return;
		}

		ss->wExcTimer = mExcursionTime(ss->uServoIndex);
		if (WeldSystem.uPulseMode == LIT_PULSEMODE_SYNC) 
		{
			WeldSystem.uPulsePhase = PRIMARY2BACKGND;
			WeldSystem.wPulseDuration = ss->wExcTimer;
			WeldSystem.uInTransistion = 5;
		}

		ss->uDirection = OSC_SLEW_OUT;
	}
}

//////////////////////////////////////////////////////////////////////
//
// void ExcursionSlew(OSC_VALUES *ss)
//
//////////////////////////////////////////////////////////////////////
void ExcursionSlew(OSC_VALUES *ss)
{   
	if (ss->uPulseMode == OSC_MODE_ON)  {
		if (ss->uDirection == OSC_SLEW_OUT) {
			ss->dwOwgDac -= mOwgInc(ss->uServoIndex);
			if (ss->dwOwgDac > OWG_IN_POINT)  
				ss->dwOwgDac = OWG_OUT_POINT;
		}
		else {
			ss->dwOwgDac += mOwgInc(ss->uServoIndex);
			if (ss->dwOwgDac > OWG_IN_POINT)  
				ss->dwOwgDac = OWG_IN_POINT;
		}
	}
}

//////////////////////////////////////////////////////////////////////
//
// void fnExecuteExcursionTimer(OSC_VALUES *ss)
//
//////////////////////////////////////////////////////////////////////
void fnExecuteExcursionTimer(OSC_VALUES *ss)
{
	if (ss->wExcTimer != 0)
	{
		--ss->wExcTimer;
		if (ss->wExcTimer) 
		{
			if (ss->uPulseMode == OSC_MODE_ON)
			{
				ExcursionSlew(ss);
			}
			return;
		}

		if (WeldSystem.uPulseMode == LIT_PULSEMODE_SYNC) 
		{
			WeldSystem.uPulsePhase = BACKGND2PRIMARY;
			WeldSystem.uInTransistion = 5;
		}

		if (ss->uDirection == OSC_SLEW_IN) 
		{
			if (ss->uPulseMode == OSC_MODE_ON)  
			{
				ss->dwOwgDac = OWG_IN_POINT;
			}
			ss->wInDwellTimer = mInDwellTime(ss->uServoIndex);
			if (WeldSystem.uPulseMode == LIT_PULSEMODE_SYNC)
			{
				WeldSystem.wPulseDuration = ss->wInDwellTimer;
			}
			++ss->wInDwellTimer; // add 1 due to pending decrement;
		}
		else 
		{      // ss->uDirection == OSC_SLEW_OUT
			if (ss->uPulseMode == OSC_MODE_ON)  
			{
				ss->dwOwgDac = OWG_OUT_POINT;
			}
			ss->wOtDwellTimer = mOutDwellTime(ss->uServoIndex);
			if (WeldSystem.uPulseMode == LIT_PULSEMODE_SYNC)
			{
				WeldSystem.wPulseDuration = ss->wOtDwellTimer;
			}
			++ss->wOtDwellTimer; // add 1 due to pending decrement;
		}
	}
}

//////////////////////////////////////////////////////////////////////
//
// void fnExecuteOutDwellTimer(OSC_VALUES *ss)
//
//////////////////////////////////////////////////////////////////////
void fnExecuteOutDwellTimer(OSC_VALUES *ss)
{
	if (ss->wOtDwellTimer != 0) 
	{
		--ss->wOtDwellTimer;
		
		if (ss->wOtDwellTimer) 
		{
			if (ss->uPulseMode == OSC_MODE_ON)  
			{
				ss->dwOwgDac = OWG_OUT_POINT; 
			}
			return;
		}

		ss->wExcTimer = mExcursionTime(ss->uServoIndex);
		if (WeldSystem.uPulseMode == LIT_PULSEMODE_SYNC) 
		{
			WeldSystem.uPulsePhase = PRIMARY2BACKGND;
			WeldSystem.wPulseDuration = ss->wExcTimer;
			WeldSystem.uInTransistion = 5;
		}

		ss->uDirection = OSC_SLEW_IN;
	}
}

//////////////////////////////////////////////////////////////////////
//
// void fnExecuteSlewTo(OSC_VALUES *ss)
//
//////////////////////////////////////////////////////////////////////
void fnExecuteSlewTo(OSC_VALUES *os)
{
	int nJogSpeed;
	
	if (os->nOscSlewCount == 0) 
		return;

	
	if (os->nOscSlewCount > 0) {
		// nOscSlewCount is positive
		nJogSpeed = mJogSpeedForward(os->uServoIndex);
		if (nJogSpeed > os->nOscSlewCount) {
			nJogSpeed = os->nOscSlewCount;
		}
	}
	else {
		// nOscSlewCount is negative
		nJogSpeed = mJogSpeedReverse(os->uServoIndex) * (-1);  // Jog Speed is now negative
		if (nJogSpeed < os->nOscSlewCount) {
			nJogSpeed = os->nOscSlewCount;
		}
	}

	os->nOscSlewCount -= (short)nJogSpeed;
	os->nOscCentering -= nJogSpeed;
//ETHERNET_Print("fnExecuteSlewTo: Centering: %d nJogSpeed: %X\n", os->nOscCentering, nJogSpeed);
	LimitOscSteering(os);

}

//////////////////////////////////////////////////////////////////////
//
// void fnExecuteDwellTimers(void)
//
//////////////////////////////////////////////////////////////////////
void fnExecuteDwellTimers(void)
{
	OSC_VALUES *ss;
	BYTE       uServo;
	
	for (uServo = 0; uServo < mNumServos; ++uServo) 
	{
		ss = (OSC_VALUES *)&ServoStruct[uServo];
		
		if (ss->uServoFunction != SF_OSC)
			continue;
		
		fnExecuteExcursionTimer(ss);
		fnExecuteInDwellTimer(ss);
		fnExecuteOutDwellTimer(ss);
		ss->wOscAmp = fnSlopeValuePrimary(ss->uServoIndex);

		switch(mOscSpecialMode(uServo)) 
		{
			case SOL_OSC_SLEWTO:	// Single Level Osc SlewTo (EXTERNAL TRIGGERED)
			case MUL_OSC_SLEWTO:	// Multi-Level Osc SlewTo
				fnExecuteSlewTo(ss);  	
				break;

			case SOL_OSC_SPIRAL:	// Single Level Osc Spiral
			case MUL_OSC_SPIRAL:	// Multi-Level Osc Spiral
//				dprintf("fnExecuteDwellTimers: MUL_OSC_SPIRAL\n");
				OscSpiralFunction(ss);	
				break;

			case OSC_NOSPECIAL:	// No Spiral or SlewTo functions
				// No functions needed
				break;
		}

		ss->bDacUpdate = __TRUE;
	}
}

//////////////////////////////////////////////////////////////////////
//
// void fnExecutePulsePhase(void) 
//
//////////////////////////////////////////////////////////////////////
void fnExecutePulsePhase(void) 
{
	if (WeldSystem.uPulseMode == LIT_PULSEMODE_OFF)  {
		WeldSystem.uPulsePhase = IN_PRIMARY;
		WeldSystem.uInTransistion = 0;
	}

	if (WeldSystem.uInTransistion)
		--WeldSystem.uInTransistion;

//ETHERNET_Print("fnExecutePulsePhase: ");
	
	switch(WeldSystem.uPulsePhase) 
    {
	    case IN_PRIMARY:
		    fnUpdatePrimaryValues();
    //		dprintf("IN_PRIMARY\n");
		    break;

	    case PRIMARY2BACKGND:
		    fnUpdatePrimary2BackgndValues();
		    WeldSystem.uPulsePhase = IN_BACKGND;
            g_bForceBargraphUpdate |= __TRUE;
    //		dprintf("PRIMARY2BACKGND\n");
		    break;

	    case IN_BACKGND:
		    fnUpdateBackgndValues();
    //		dprintf("IN_BACKGND\n");
		    break;

	    case BACKGND2PRIMARY:
		    fnUpdateBackgnd2PrimaryValues();
		    WeldSystem.uPulsePhase = IN_PRIMARY;
            g_bForceBargraphUpdate |= __TRUE;
    //		dprintf("BACKGND2PRIMARY\n");
		    break;
	}
}

//////////////////////////////////////////////////////////////////////
//
// void fnWireRetract(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
void fnWireRetract(SERVO_VALUES* ss)
{
	if ((ss->wWireInTime != 0) && (ss->bEnabledFlag)) {
		--ss->wWireInTime;
		
		ss->wSeqDacValue = -mJogSpeedReverse(ss->uServoIndex);
		if (!ss->bIsJogging)    
			ss->bDacUpdate = __TRUE;
		
		SetForwardDirectionBit(__FALSE, ss);
		ss->uDirection = WIRE_DIR_RETR;
		ss->bEnabledFlag = __TRUE;
		SetEnableBit(__TRUE, ss);
		return;
	}
	
	// Retract is completed
	SetEnableBit(__FALSE, ss);
	ss->bRetracting  = __FALSE;
	ss->bEnabledFlag = __FALSE;
	ss->uDirection   = WIRE_DIR_FEED;
	SetForwardDirectionBit(__TRUE, ss);
}        

//////////////////////////////////////////////////////////////////////
//
// void fnExecuteRetracts(void)
//
//////////////////////////////////////////////////////////////////////
void fnExecuteRetracts(void)
{
	SERVO_VALUES* ss;
	BYTE         uServo;

	for (uServo = 0; uServo < mNumServos; ++uServo) 
	{
		ss = &ServoStruct[uServo];
		if (ss->uServoFunction != SF_WIREFEED)
			continue; // Not a wirefeed
		
		if (!ss->bRetracting)
			continue; // We're not retracting this one

		fnWireRetract(ss);
	}
}

//////////////////////////////////////////////////////////////////////
//
// BOOL fnStartRetract(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
static BOOL fnStartRetract(SERVO_VALUES* ss)
{
	if (GetStopDelay(ss->uServoIndex) >= 0) {
//		printf("fnStartRetract:: Unlock Time %d\n", ss->nUnlockTime);
		return __FALSE;
	}
	
	if (WeldSystem.uInNegStopDelay) {
//		printf("fnStartRetract:: Stop Delay already active\n");
		return __FALSE;
	}
	
	if (!ss->bEnabledFlag) {
//		printf("fnStartRetract:: Servo Not Enabled\n");
		return __FALSE;
	}
	
	// This servo caused a negitive stop delay.
	// NOTE: These lines must be modified when multiple servos have a neg stop delay.

	// Set this level's time equal to accumulated time count
	SetLevelTime(WeldSystem.dwLevelTimeCount);
	if ((mLevelAdvanceMode != LEVEL_ADVANCE_TIME_10) && 
		(mLevelAdvanceMode != LEVEL_ADVANCE_TIME_01)) {
		// Level advance must be set to time to advance to downslope
		mLevelAdvanceMode = LEVEL_ADVANCE_TIME_10;
	}

	// Bump this Level's time by the amount of negative stop dalay
//	printf("fnStartRetract:: Level Time before: %d\n", WeldSystem.dwLevelTime);
	WeldSystem.dwLevelTime += (GetStopDelay(ss->uServoIndex) * (-1));
//	printf("fnStartRetract:: Level Time after: %d\n", WeldSystem.dwLevelTime);

	mNumberOfLevels = WeldSystem.uWeldLevel + 1;  // Make this level the last level

	InitWireInTime(ss);
	WeldSystem.uInNegStopDelay = __TRUE;
//	printf("fnStartRetract:: Start Retract Complete\n");
	return __TRUE;
}

//////////////////////////////////////////////////////////////////////
//
// BOOL fnStartAllRetracts(void)
// Returns: __TRUE if any servo has a negative stop delay programmed
//
//////////////////////////////////////////////////////////////////////
static BOOL fnStartAllRetracts(void)
{
	SERVO_VALUES* ss;
	BYTE         uServo;
	BOOL         bReturnVal = __FALSE;

	for (uServo = 0; uServo < mNumServos; ++uServo) 
	{
		ss = &ServoStruct[uServo];

		if (ss->uServoFunction == SF_WIREFEED)
			bReturnVal |= fnStartRetract(ss);
	}

//	printf("fnStartAllRetracts:: Return Value: %d\n", bReturnVal);

	return bReturnVal;
}

//////////////////////////////////////////////////////////////////////
//
// void DecAvcEnableDly(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
static void DecAvcEnableDly(SERVO_VALUES* ss)
{
	if (ss->wEnableDelay == 0x0FFFF)
		return;

	if (ss->wEnableDelay)
		--ss->wEnableDelay;

	if ((ss->wEnableDelay) || (!ss->bEnabledFlag) || (!WeldSystem.uWeldMode))
		return;
	
	if (ss->uServoControl == SC_CLOSEDLOOP)
		SetEnableBit(__TRUE, ss);
}

//////////////////////////////////////////////////////////////////////
//
// void fnDecrementServoDelays(void)
//
//////////////////////////////////////////////////////////////////////
static void fnDecrementServoDelays(void)
{
	SERVO_VALUES *ss;
	BYTE         uServo;
	
	for (uServo = 0; uServo < mNumServos; ++uServo) 
	{
		ss = &ServoStruct[uServo];

		switch(ss->uServoFunction) 
		{
			case SF_WIREFEED:
				if (ss->wEnableDelay) 
					--ss->wEnableDelay;
				break;
				
			case SF_AVC:
				DecAvcEnableDly(ss);
				break;
		}
	}
}

//////////////////////////////////////////////////////////////////////
//
// void LowVoltCheck(void)
//
//////////////////////////////////////////////////////////////////////
static void LowVoltCheck(void)
{   
	if (mStuboutMode == LIT_STUBOUTMODE_OFF)
		return;

	if (!bStubOutEnable || (nUlpVoltFaultState == ULP_VOLTDISABLED)) 
		return;

	if (!ArcBelowMin()) 
	{
		WeldSystem.Fault.uStuboutCount = 0;
		return;
	}
	
	// A Lo arc volt condition exists 
	++WeldSystem.Fault.uStuboutCount;
//ETHERNET_Print("LowVoltCheck:: %1d-Times\n", WeldSystem.Fault.uStuboutCount);
	if (WeldSystem.Fault.uStuboutCount <= 5) 
		return;
	
	// A Lo arc volt condition existed 5 consecutive times 
//ETHERNET_Print("LowVoltCheck:: FAULT!!!\n");
	WeldSystem.Fault.bStubOutFault = __TRUE;
	QueueFaultMessage(STUBOUT_FLT_SET);
}
   
//////////////////////////////////////////////////////////////////////
//
// void HighVoltCheck(void)
//
//////////////////////////////////////////////////////////////////////
void HighVoltCheck(void)
{
	if (!bHighArcEnable || (nUlpVoltFaultState == ULP_VOLTDISABLED)) 
	{
//		dprintf("HighArc Disabled\n");
		return;
	}

	if (!ArcAboveMax()) 
	{
		WeldSystem.Fault.uArcOutCount = 0;
		return;
	}
	
	// A Hi arc volt condition exists 
	// Greater than 25.0 Volts
	++WeldSystem.Fault.uArcOutCount;
//ETHERNET_Print("HighVoltCheck:: %1d-Times\n", WeldSystem.Fault.uArcOutCount);
	if (WeldSystem.Fault.uArcOutCount <= 5) 
		return;
	
	// A hi arc volt condition existed 5 consecutive times 
//ETHERNET_Print("HighVoltCheck:: FAULT!!!\n");
	WeldSystem.Fault.bHighArcVoltFault = __TRUE;
	QueueFaultMessage(HI_ARC_VOLT_SET);
}
               
//////////////////////////////////////////////////////////////////////
//
// void CheckVoltage(void)
//
//////////////////////////////////////////////////////////////////////
void CheckVoltage(void)
{
	if (!WeldSystem.uWeldMode)
		return; // Weld Mode only fault

	if (uSystemWeldPhase >= WP_DOWNSLOPE)
		return; // Do not check for stubout in downslope

	if (WeldSystem.Fault.uStuboutDly) {
		--WeldSystem.Fault.uStuboutDly;
		return;
	}
	
	LowVoltCheck();

	if (uSystemWeldPhase == WP_LEVELWELD) 
		HighVoltCheck(); // Do not check high voltage in upslope or downslope

	if (WeldSystem.Fault.bStubOutFault || WeldSystem.Fault.bHighArcVoltFault) {
		// An Arc fault is active
		if (uSystemWeldPhase >= WP_POSTPURGE)
			return; // Already in postpurge
		fnInitPostPurge();
	}
}

//////////////////////////////////////////////////////////////////////
//
// void EvaluateCurrent(void)
//
//////////////////////////////////////////////////////////////////////
static void EvaluateCurrent(void)
{
	if (WeldSystem.uWeldMode)
	{
		// Weld Mode only fault
		if (PsInRegulation()) 
		{
			WeldSystem.Fault.uInputACCount = 0;
		}
		else
		{
			// dprintf("EvaluateCurrent:: InputAC Count %d\n", WeldSystem.Fault.uInputACCount);
			if ((++WeldSystem.Fault.uInputACCount) > 20) 
			{
				if (!WeldSystem.Fault.bInputAcFault)
				{
//					printf("EvaluateCurrent:: InputAC FAULT\n");
					QueueFaultMessage(INPUT_AC_SET);
					WeldSystem.Fault.bInputAcFault = __TRUE;
				}
			}
		}

	}
	else
	{
		WeldSystem.Fault.uInputACCount = 0;
	}
}         

//////////////////////////////////////////////////////////////////////
//
// void CheckCurrent(void)
//
//////////////////////////////////////////////////////////////////////
void CheckCurrent(void)
{
	if (WeldSystem.uCurrentFaultDelay) {
		--WeldSystem.uCurrentFaultDelay;
		return;
	}

	if (uSystemWeldPhase != WP_LEVELWELD)
		return;

	EvaluateCurrent();
}            

//////////////////////////////////////////////////////////////////////
//
// void UpdateServoStructs(void)
//
//////////////////////////////////////////////////////////////////////
void UpdateServoStructs(void)
{
	SERVO_VALUES* ss;
	OSC_VALUES   *os;
	BYTE uServo;
	
	if (WeldSystem.uWeldLevel > mLastLevel) {
		// Level Reset
		WeldSystem.uWeldLevel = 0;
		QueueEventMessage(LEVEL_RST, EV_LEVELCHANGE);
//		dprintf("UpdateServoStructs::Level Reset\n");
	}

	InvalidModeFixup();
	for (uServo = 0; uServo < mNumServos; ++uServo) 
	{
		ss = &ServoStruct[uServo]; // Save pointer to this servos struct
		
		ss->uPulseMode = mPulseMode(uServo);
		switch(ss->uServoFunction) 
		{
			case SF_CURRENT:
				fnSetSystemPulseMode(ss);
				break;
				
			case SF_TRAVEL:    
				ss->uDirection = mDirection(uServo);
				fnTravelDirectionCommand(ss);
				ss->uPulseMode = mPulseMode(uServo);
				if (ss->uPulseMode == LIT_TVL_STEP_OFF) 
					ZeroCommandDac(ss->uServoSlot);
				break;
				
			case SF_AVC:
				ss->uPulseMode = mPulseMode(uServo);
				if (ss->uPulseMode == LIT_AVCMODE_OFF)
                {
					AvcBacToPriCmd(ss); // Diables servo & set AVC Enable Delay
                }
				else 
                {
					CalculateAvcEnableDly(ss); // CALCULATE AVC ENABLE DELAY 
                }
				
                if (ss->uServoControl == SC_CLOSEDLOOP)
                {
					SetAvcResponseDacValue(mResponse(uServo));
                }
				break;
				
			case SF_OSC:
			case SF_JOG_ONLY_POS:
				os = (OSC_VALUES *)ss;
				os->uPulseMode = mPulseMode(uServo);
				if (os->uPulseMode == OSC_MODE_OFF)
					os->dwOwgDac = OWG_MID_POINT;	// SET OSC WAVEFORM GENERATOR TO MID POINT 
	//			os->wOscAmp = mPrimaryValue(uServo);
				if ((os->wInDwellTimer	== 0) &&
					(os->wExcTimer		== 0) &&
					(os->wOtDwellTimer	== 0)) {
					os->wExcTimer = mExcursionTime(uServo);
					if (os->wExcTimer > 1)
						os->wExcTimer /= 2;
					os->uDirection = OSC_SLEW_OUT;
				}
				break;
		}
	}
}

//////////////////////////////////////////////////////////////////////
//
// void LevelWeldLevelChange(void)
//
//////////////////////////////////////////////////////////////////////
static void LevelWeldLevelChange(void)
{
//ETHERNET_Print("LevelWeldLevelChange::Start LevelChange\n");
	MoveToNewLevel();
	OscSlewToFunction(MUL_OSC_SLEWTO);  // Cause all OSC servos to SLEW_TO do to a Level Change
	WeldSystem.uCurrentFaultDelay = 5;
//ETHERNET_Print("LevelWeldLevelChange::End LevelChange\n");
}

//////////////////////////////////////////////////////////////////////
//
// void fnInitDownSlope(void)
//
//////////////////////////////////////////////////////////////////////
static void fnInitDownSlope(void)
{
//	printf("fnInitDownSlope\n");

	if (mDownslopeTime)
		InitSlopeStruct(mDownslopeTime);
	else
		InitSlopeStruct(1);  // Can not transistion out of downslope without a single countdown.
	WeldSystem.dwSlopeStartTime	= 1;  // Need a non-zero value to start sloping

	if (WeldSystem.uDataAqiStop == DA_STOPAT_DSLOPE_START)
	{
		WeldSystem.uDataAqiOutEnable = __FALSE;
		QueueEventMessage(SYSTEM_EVENT, EV_DASTREAM_OFF);
	}


	InitStopDelays();
	uSystemWeldPhase = WP_DOWNSLOPE;
	QueueEventMessage(SYSTEM_EVENT, EV_WELDPHASE_DOWNSLOPE);
}

static void CommenceDownslope(void)
{
//	printf("CommenceDownslope\r\n");
	if (!bWeldSequence || (uSystemWeldPhase >= WP_DOWNSLOPE))
	{
//		printf("CommenceDownslope::Already in Downslope\r\n");
		return;
	}

	if (fnStartAllRetracts() == __FALSE) 
    {   
        // No negative stop delay
		fnInitDownSlope();
	}
}

//////////////////////////////////////////////////////////////////////
//
// static void fnEvalLevelTime(void)
//
//////////////////////////////////////////////////////////////////////
static void fnEvalLevelTime(void)
{
	if (uSystemWeldPhase >= WP_DOWNSLOPE)
		return;

	if ((mLevelAdvanceMode != LEVEL_ADVANCE_TIME_10) && 
		(mLevelAdvanceMode != LEVEL_ADVANCE_TIME_01))
		return; // Do not advance

	if (WeldSystem.dwLevelTime > WeldSystem.dwLevelTimeCount) {
		return; // Not time yet
	}

	if (WeldSystem.uWeldLevel < mLastLevel) {
		++uDesiredLevel;  // Go to next level
		WeldSystem.dwLevelTimeCount = 0;
		SetLevelTime(mLevelTime);
		InitSlopeStruct(mSlopeTime);
//		dprintf("fnEvalLevelTime: Bump DesiredLevel\n");
		return;
	}

	// All levels have been completed
//	printf("fnEvalLevelTime::CommenceDownslope\r\n");
	CommenceDownslope();
}

//////////////////////////////////////////////////////////////////////
//
// void fnEvalLevelPosition(void)
//
//////////////////////////////////////////////////////////////////////
static void fnEvalLevelPosition(void)
{
	if (uSystemWeldPhase >= WP_DOWNSLOPE)
		return;

	if (mLevelAdvanceMode != LEVEL_ADVANCE_POSITION)
		return; // Do not advance

	if (WeldSystem.uWeldLevel != uDesiredLevel)
		return; // level change in progress

	if (WeldSystem.bHeadDirectionFwd) {
		// Encoder counts DOWN, for forward travel
		if (WeldSystem.nLevelPosition < mLevelPosition) 
		{
			return; 
		}
	}
	else {
		// Encoder counts UP, for reverse travel
		if (WeldSystem.nLevelPosition > mLevelPosition) 
		{
			return; 
		}
	}

//ETHERNET_Print("fnEvalLevelPosition:: bHeadDirectionFwd: %d\n", WeldSystem.bHeadDirectionFwd);
//ETHERNET_Print("fnEvalLevelPosition:: nLevelPosition: %d\n", WeldSystem.nLevelPosition);
//ETHERNET_Print("fnEvalLevelPosition:: mLevelPosition: %d\n", mLevelPosition);
	SendServoPosition(__TRUE); // Ensure Data Acq. has correct data
	if (WeldSystem.uWeldLevel < mLastLevel) 
	{
		++uDesiredLevel;  // Go to next level
//		dprintf("Level Advance from fnEvalLevelPosition\n");
		return;
	}

	// All levels have been completed
//	printf("fnEvalLevelPosition::CommenceDownslope\r\n");
	CommenceDownslope();
}

//////////////////////////////////////////////////////////////////////
//
// void fnEvalLevelAdvance(void)
//
//////////////////////////////////////////////////////////////////////
static void fnEvalLevelAdvance(void)
{
	if (uSystemWeldPhase >= WP_DOWNSLOPE)
		return;

	switch(mLevelAdvanceMode) {
		case LEVEL_ADVANCE_TIME_10:
		case LEVEL_ADVANCE_TIME_01:
			fnEvalLevelTime();
			break;

		case LEVEL_ADVANCE_POSITION:
			fnEvalLevelPosition();
			break;
	}

}

//////////////////////////////////////////////////////////////////////
//
// void InitStopDelays(void)
//
//////////////////////////////////////////////////////////////////////
static void InitStopDelays(void)
{
	SERVO_VALUES* ss;
	BYTE         uServo;
	
	for (uServo = 0; uServo < mNumServos; ++uServo) 
	{
		ss = &ServoStruct[uServo]; // Save pointer to this servos struct
		
		switch(ss->uServoFunction) 
		{
			case SF_TRAVEL:
			case SF_WIREFEED:
			case SF_AVC:
			case SF_OSC:
				ss->nUnlockTime = GetStopDelay(uServo);
				break;
		}
	}
}

//////////////////////////////////////////////////////////////////////
//
// void StartReturnToHome(void)
//
//////////////////////////////////////////////////////////////////////
void StartReturnToHome(void)
{
	BYTE uServo;
	SERVO_VALUES* ss;
	
	for (uServo = 0; uServo < mNumServos; ++uServo) 
	{
		ss = &ServoStruct[uServo]; // Save pointer to this servos struct
		if (ss->uServoFunction != SF_TRAVEL)
			continue;

		if (!mHomeSwitchUsed(uServo)) 
		{
//			dprintf("StartReturnToHome:: Home function not required for servo %d\n", uServo); 
			continue;
		}

//		dprintf("StartReturnToHome:: bReturnToHome = __TRUE\n"); 
		bReturnToHome[uServo] = __TRUE;
	}
}

//////////////////////////////////////////////////////////////////////
//
// void fnInitPostPurge(void)
//
//////////////////////////////////////////////////////////////////////
static void fnInitPostPurge(void)
{
	if ((WeldSystem.uDataAqiStop == DA_STOPAT_DSLOPE_START) ||
		(WeldSystem.uDataAqiStop == DA_STOPAT_DSLOPE_STOP))
	{
		WeldSystem.uDataAqiOutEnable = __FALSE;
		QueueEventMessage(SYSTEM_EVENT, EV_DASTREAM_OFF);
	}

	if ((!WeldSystem.uWeldMode) || (!WeldSystem.bArcEstablished)) {
//		printf("fnInitPostPurge::Go to WP_EXIT\n");
		uSystemWeldPhase = WP_EXIT; // ON NEXT INTERRUPT
		return;
	}
	
	fnWarmShutdown();
//	ReEnableOsc();

//	if ((WeldSystem.uWeldMode) && (WeldSystem.bArcEstablished))
//		SetFanDelay();

	uSystemWeldPhase = WP_POSTPURGE;
//	printf("fnInitPostPurge::Go to WP_POSTPURGE\n");
	SetGasValveOpen(__TRUE);
	WeldSystem.Slope.dwPurgeTime = mPostPurgeTime;
	if (WeldSystem.Slope.dwPurgeTime == 0)
	{
		WeldSystem.Slope.dwPurgeTime = 1;  // Set to minimum time
	}
	
	InitServoStructs(__FALSE);
	if (nHomeDuringPostpurge) {
		StartReturnToHome();
	}
}

//////////////////////////////////////////////////////////////////////
//
// BYTE InitPreWrapServo(void)
//
//////////////////////////////////////////////////////////////////////
BYTE InitPreWrapServo(BYTE uServo)
{
	SERVO_VALUES* ss;

	ss = &ServoStruct[uServo]; // Save pointer to this servos struct
	Home.ss = ss;

	if (uServo == mNumServos) 
	{
//		dprintf("InitPreWrapServo:: Failed BAD SERVO NUMBER\n");
		return __FALSE; // No travel servo
	}

	if (!mHomeSwitchUsed(uServo)) 
	{
//		dprintf("InitPreWrapServo:: Failed NO HOME SWITCH\n");
		return __FALSE; // No home switch
	}
	
	Home.ss->bEnabledFlag = __FALSE;
//	JogVelocityServo(Home.ss, (100 * nHomeDirection));
//ETHERNET_Print("InitPreWrapServo:: JogVelocityServo 100\n");

	return __TRUE;
}

//////////////////////////////////////////////////////////////////////
//
// BYTE InitHomeServo(void)
//
//////////////////////////////////////////////////////////////////////
BYTE InitHomeServo(BYTE uServo)
{
	SERVO_VALUES* ss;

	ss = &ServoStruct[uServo]; // Save pointer to this servos struct
	Home.ss = ss;

	if (uServo == mNumServos)
		return __FALSE; // No travel servo

	if (!mHomeSwitchUsed(uServo))
		return __FALSE; // No home switch
	
	if (HomeSwitchShut(ss->uServoSlot)) 
		return __FALSE; // Home switch already shut
	
	if (WeldSystem.bBadStart)
		return __FALSE; // Don't return to home on after a Bad Start
	
	if (WeldSystem.Fault.bHighArcVoltFault || WeldSystem.Fault.bStubOutFault) 
		return __FALSE;  // Don't return to home on after an arc type fault
	
	return __TRUE;
}

//////////////////////////////////////////////////////////////////////
//
// void ReturnToHome(BYTE uServoIndex)
//
//////////////////////////////////////////////////////////////////////
void ReturnToHome(BYTE uServoIndex)
{
	SERVO_VALUES* ss;

	if (!bReturnToHome[uServoIndex])		
		return;
	
	ss = &ServoStruct[uServoIndex]; // Save pointer to this servos struct
	Home.ss = ss;
	TechWriteCommand(uServoIndex, SetGoHome, 1);

	switch(Home.uPhase) 
    {
	case HS_PRETEST:
//		dprintf("ReturnToHome:: HS_PRETEST\n");
		if (!InitHomeServo(uServoIndex)) 
        {
			Home.uPhase = HS_EXITPHASE;
//			dprintf("ReturnToHome:: Pretest Failed\n");
			break;
		}
		Home.ss->bEnabledFlag = __FALSE;
		Home.uPhase = HS_CLOSEUP;
		break;
		
	case HS_CLOSEUP:
		if(TechReadFeedback(uServoIndex, GET_GOHOME) > 0)
			break; // Wait for switch to shut

		SetEnableBit(__FALSE, Home.ss);
		Home.ss->bEnabledFlag = __FALSE;
		Home.uPhase = HS_EXITPHASE;
		// Falls through
		break;

	case HS_EXITPHASE:
//		dprintf("ReturnToHome:: COMPLETE\n");
		//JogVelocityServo(Home.ss, 0);
		QueueEventMessage(Home.ss->uServoIndex, EV_HOME_COMPLETE);
		bReturnToHome[Home.ss->uServoIndex] = __FALSE;
		Home.uPhase   = HS_PRETEST;
		break;
	}
//	//Get Servo status
//	//GetServoFeedback(Home.ss->uServoSlot);
//
//
//	switch(Home.uPhase)
//    {
//	case HS_PRETEST:
////		dprintf("ReturnToHome:: HS_PRETEST\n");
//		if (!InitHomeServo(uServoIndex))
//        {
//			Home.uPhase = HS_EXITPHASE;
////			dprintf("ReturnToHome:: Pretest Failed\n");
//			break;
//		}
//		Home.ss->bEnabledFlag = __FALSE;
//		//JogVelocityServo(Home.ss, (0 * nHomeDirection));
//		//JogVelocityServo(Home.ss, (100 * nHomeDirection));
//		Home.uPhase = HS_FIRST_CLOSE;
//		// Falls through
//
//	case HS_FIRST_CLOSE:
//		// Driving @ +100%
//		//JogVelocityServo(Home.ss, (100 * nHomeDirection));
//		if (!HomeSwitchShut(Home.ss->uServoSlot))
//			break; // Wait for switch to shut
//		//JogVelocityServo(Home.ss, (0 * nHomeDirection));
//		//JogVelocityServo(Home.ss, (-50 * nHomeDirection));
//		Home.uTime = 24;
//		Home.uPhase = HS_BACKUP;
//		break;
//
//	case HS_BACKUP:
//		//JogVelocityServo(Home.ss, (-100 * nHomeDirection));
//		if (--Home.uTime)
//			break; // Still backing up
//
//		//JogVelocityServo(Home.ss, (0 * nHomeDirection));
//		//JogVelocityServo(Home.ss, (25 * nHomeDirection));
//		Home.uPhase = HS_CLOSEUP;
//		// Falls through
//
//	case HS_CLOSEUP:
//		//JogVelocityServo(Home.ss, (25 * nHomeDirection));
//		if (!HomeSwitchShut(Home.ss->uServoSlot))
//			break; // Wait for switch to shut
//
//		SetEnableBit(__FALSE, Home.ss);
//		Home.ss->bEnabledFlag = __FALSE;
//		Home.uPhase = HS_EXITPHASE;
//		// Falls through
//
//	case HS_EXITPHASE:
////		dprintf("ReturnToHome:: COMPLETE\n");
//		//JogVelocityServo(Home.ss, 0);
//		QueueEventMessage(Home.ss->uServoIndex, EV_HOME_COMPLETE);
//		bReturnToHome[Home.ss->uServoIndex] = __FALSE;
//		Home.uPhase   = HS_PRETEST;
//		break;
//	}
}

//////////////////////////////////////////////////////////////////////
//
// void PreWrapFunction(void)
//
//////////////////////////////////////////////////////////////////////
static void PreWrapFunction(BYTE uServoIndex)
{  
	if (!bPreWrap[uServoIndex])			
		return; // Function not active

	TechWriteCommand(uServoIndex, SetGoHome, 3);
	Home.uPhase = HS_PRETEST;
	bPreWrap[uServoIndex] = __FALSE;
	bReturnToHome[uServoIndex] = __FALSE;
	//Get Servo status
//	GetServoFeedback(Home.ss->uServoSlot);

//	switch(Home.uPhase)
//    {
//	case HS_PRETEST:
//		if (!InitPreWrapServo(uServoIndex))
//        {
////			dprintf("InitPreWrapServo:: JogVelocityServo 100\n");
//			Home.uPhase = HS_EXITPHASE;
//			return;
//		}
//		Home.uPhase = HS_OPEN;
//		Home.uTime = 100;  // Allow 1 second for the switch to open
//		// Falls through
//
//	case HS_OPEN:
////		JogVelocityServo(Home.ss, (100 * nHomeDirection));
//		if (HomeSwitchShut(Home.ss->uServoSlot)) {
//			if (--Home.uTime)
//            {
//				return; // Keep going; Home switch still SHUT for LESS than 1 second
//			}
//
//			// Home Swtich did NOT open in 1 second
//			Home.uPhase = HS_ABORT;
//			return; // Wait for switch to open
//		}
//
////		dprintf("PreWrapFunction:: HS_OPEN: Switch OPEN\n");
//		Home.uPhase = HS_GO_ON;
//		Home.uTime = 100;
//		break;
//
//	case HS_GO_ON:
////		JogVelocityServo(Home.ss, (100 * nHomeDirection));
//		if (--Home.uTime)
//        {
//			return; // Keep going
//        }
////		dprintf("PreWrapFunction:: HS_GO_ON: Time Done\n");
//		Home.uPhase = HS_EXITPHASE;
//		// Falls through
//
//	case HS_EXITPHASE:
////		dprintf("PreWrapFunction:: HS_EXITPHASE\n");
////		JogVelocityServo(Home.ss, 0);
//		Home.uPhase = HS_PRETEST;
//		bPreWrap[uServoIndex] = __FALSE;
//		bReturnToHome[uServoIndex] = __TRUE;
//		break;
//
//	case HS_ABORT:
////		dprintf("PreWrapFunction:: HS_ABORT\n");
////		JogVelocityServo(Home.ss, 0);
//		Home.uPhase = HS_PRETEST;
//		bPreWrap[uServoIndex] = __FALSE;
//		bReturnToHome[uServoIndex] = __FALSE;
//		break;
//	}
}               

//////////////////////////////////////////////////////////////////////
//
// void fnDumpAllDacs(void)
//
//////////////////////////////////////////////////////////////////////
void fnDumpAllDacs(void)
{
	SERVO_VALUES* ss;
	BYTE         uServoIndex;

	for (uServoIndex = 0; uServoIndex < stInitWeldSched.uNumberOfServos; ++uServoIndex)
	{
		ss = &ServoStruct[uServoIndex];
		fnCommandServo(ss);
	}

	BumpAvcResponseDacValue();
}      

/*
//////////////////////////////////////////////////////////////////////
//
// void SetFunction(void)
//
//////////////////////////////////////////////////////////////////////
void SetFunction(BYTE uServoIndex)
{
	static SERVO_VALUES* pstSetPot;
//	static WORD wSetPotTime;
	WORD wVoltage;
	int nSpeed;

	if (!bPerformSetPot[uServoIndex])		
		return;  // Function not active

	switch(WeldSystem.uSetPotPhase) 
	{
		case 0:   // init
			pstSetPot = &ServoStruct[uServoIndex];
			WeldSystem.uSetPotPhase = 1; // Go to next phase
			break;

		case 1:
			wVoltage = GetFeedback(FB_SETPOT);
			if((wVoltage > SP_VOLTAGEP25) || (wVoltage < SP_VOLTAGENEGP25))
			{
				if(wVoltage < SP_VOLTAGE0)
				{
					// Jog AVC UP
                    if ((pstSetPot->uServoControl == SC_OPENLOOP) ||
                        (pstSetPot->uServoControl == SC_ULPHEAD))
                    {
                        nSpeed = mRetractSpeed(pstSetPot->uServoIndex);
                        JogAvcServo(pstSetPot, nSpeed);
                    }
                    else 
                    {
                        SetCommandDacValue(AVC_POS_JOG, pstSetPot);
                        SetAvcResponseDacValue((WORD)mRetractSpeed(pstSetPot->uServoIndex)); // Set retract speed
                    }
				}
				else
				{
					// Jog AVC DOWN
                    if ((pstSetPot->uServoControl == SC_OPENLOOP) ||
                        (pstSetPot->uServoControl == SC_ULPHEAD))
                    {
                        // Convert exiting jog speeds to percent jog commands
                        nSpeed = mTouchSpeed(pstSetPot->uServoIndex);
                        nSpeed *= (-1);  // Make TOUCH a jog down
                        JogAvcServo(pstSetPot, nSpeed);
                    }
                    else 
                    {
                        SetServoCmdBit(pstSetPot->uServoSlot, SLOT_AVC_JOG, __TRUE);
                        SetAvcResponseDacValue(mTouchSpeed(pstSetPot->uServoIndex));
                        SetCommandDacValue((short)AVC_NEG_JOG, pstSetPot);
                        SetEnableBit(__TRUE, pstSetPot);
                    }
				}

			}
			else
			{
                // Stop Jogging
                if ((pstSetPot->uServoControl == SC_OPENLOOP) ||
                    (pstSetPot->uServoControl == SC_ULPHEAD))
                {
                    JogAvcServo(pstSetPot, 0);
                }
                else 
                {
                    SetEnableBit(__FALSE, pstSetPot);
                    SetAvcResponseDacValue(mResponse(pstSetPot->uServoIndex));
                    SetServoCmdBit(pstSetPot->uServoSlot, SLOT_AVC_JOG, __FALSE);
                    SetCommandDacValue((short)mPrimaryValue(pstSetPot->uServoIndex), pstSetPot); // RESTORE AVC COMMAND 
                }
				WeldSystem.uSetPotPhase = 2;
//				dprintf("SetFunction:: Phase 1 complete\n");
			}
			break;
			
		case 2:
            // Stop Jogging
            if ((pstSetPot->uServoControl == SC_OPENLOOP) ||
                (pstSetPot->uServoControl == SC_ULPHEAD))
            {
                JogAvcServo(pstSetPot, 0);
            }
            else 
            {
                SetEnableBit(__FALSE, pstSetPot);
                SetAvcResponseDacValue(mResponse(pstSetPot->uServoIndex));
                SetServoCmdBit(pstSetPot->uServoSlot, SLOT_AVC_JOG, __FALSE);
                SetCommandDacValue((short)mPrimaryValue(pstSetPot->uServoIndex), pstSetPot); // RESTORE AVC COMMAND 
            }
			bPerformSetPot[pstSetPot->uServoIndex] = __FALSE;
			QueueEventMessage(pstSetPot->uServoIndex, EV_SET_POT_COMPLETE);
//			dprintf("SetFunction:: DONE\n");
			break;
	}
}      
*/
   
//////////////////////////////////////////////////////////////////////
//
// void SetFunction(void)
//
//////////////////////////////////////////////////////////////////////
void SetFunction(BYTE uServoIndex)
{
	static SERVO_VALUES* pstSetPot;
//	static WORD wSetPotTime;
	WORD wVoltage;

	if (!bPerformSetPot[uServoIndex])		
		return;  // Function not active

	pstSetPot = &ServoStruct[uServoIndex];
	switch(WeldSystem.uSetPotPhase) 
	{
		case 0:   // init
			WeldSystem.uSetPotPhase = 1; // Go to next phase
			break;

		case 1:
			wVoltage = GetFeedback(FB_SETPOT);
			if((wVoltage > SP_VOLTAGEP25) || (wVoltage < SP_VOLTAGENEGP25))
			{
				if(wVoltage < SP_VOLTAGE0)
				{
					// Jog AVC UP
                    JogAvcServo(pstSetPot, -100);
				}
				else
				{
					// Jog AVC DOWN
                    JogAvcServo(pstSetPot, 100);
				}

			}
			else
			{
                // Stop Jogging
                JogAvcServo(pstSetPot, 0);
				WeldSystem.uSetPotPhase = 2;
//				dprintf("SetFunction:: Phase 1 complete\n");
			}
			break;
			
		case 2:
            // Stop Jogging
            JogAvcServo(pstSetPot, 0);

			bPerformSetPot[pstSetPot->uServoIndex] = __FALSE;
			QueueEventMessage(pstSetPot->uServoIndex, EV_SET_POT_COMPLETE);
//			dprintf("SetFunction:: DONE\n");
			break;
	}
}      
   
//////////////////////////////////////////////////////////////////////
//
// void AvcBacCmd(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
static void AvcBacCmd(SERVO_VALUES* ss)
{
	switch (ss->uPulseMode)
	{
		case LIT_AVCMODE_OFF:
			ss->wSeqDacValue = (WORD)0;
			break;
		case LIT_AVCMODE_SAMP_PRI:
			ss->wSeqDacValue = (WORD)mPrimaryValue(ss->uServoIndex);
			break;
		case LIT_AVCMODE_SAMP_BAC:
			ss->wSeqDacValue = (WORD)mBackgroundValue(ss->uServoIndex);
			break;
		default:
			ss->wSeqDacValue = (WORD)mBackgroundValue(ss->uServoIndex);
			break;
	}
//ETHERNET_Print("AvcBacCmd:: %d\n", ss->wSeqDacValue);
	QueCommandDacValue(ss);
	CommandExternalDacs(mExternDacBac(ss->uServoIndex), ss);
}

//////////////////////////////////////////////////////////////////////
//
// void AvcPriToBacCmd(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
void AvcPriToBacCmd(SERVO_VALUES* ss)
{
	switch (ss->uPulseMode)
	{
		case LIT_AVCMODE_OFF:
			ss->wSeqDacValue = (WORD)0;
			break;
		case LIT_AVCMODE_SAMP_PRI:
			ss->wSeqDacValue = (WORD)mPrimaryValue(ss->uServoIndex);
			break;
		case LIT_AVCMODE_SAMP_BAC:
			ss->wSeqDacValue = (WORD)mBackgroundValue(ss->uServoIndex);
			break;
		default:
			ss->wSeqDacValue = (WORD)mBackgroundValue(ss->uServoIndex);
			break;
	}
	if (!ss->bIsJogging)
		SetEnableBit(__FALSE, ss);
	QueCommandDacValue(ss);
	CommandExternalDacs(mExternDacBac(ss->uServoIndex), ss);
	CalculateAvcEnableDly(ss); // CALCULATE AVC ENABLE DELAY 
}   

//////////////////////////////////////////////////////////////////////
//
// void AvcBacToPriCmd(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
static void AvcBacToPriCmd(SERVO_VALUES* ss)
{
	switch (ss->uPulseMode)
	{
		case LIT_AVCMODE_OFF:
			ss->wSeqDacValue = (WORD)0;
			break;
		case LIT_AVCMODE_SAMP_PRI:
			ss->wSeqDacValue = (WORD)mPrimaryValue(ss->uServoIndex);
			break;
		case LIT_AVCMODE_SAMP_BAC:
			ss->wSeqDacValue = (WORD)mBackgroundValue(ss->uServoIndex);
			break;
		default:
			ss->wSeqDacValue = mPrimaryValue(ss->uServoIndex);
			break;
	}
	if (!ss->bIsJogging)
		SetEnableBit(__FALSE, ss);
	QueCommandDacValue(ss);
	CommandExternalDacs(mExternDacPri(ss->uServoIndex), ss);
	CalculateAvcEnableDly(ss); // CALCULATE AVC ENABLE DELAY 
}   

//////////////////////////////////////////////////////////////////////
//
// void fnZeroTvlDac(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
void fnZeroTvlDac(SERVO_VALUES* ss)
{
	ss->wSeqDacValue = 0;
	if (!ss->bIsJogging)
    {
		ss->wBuffDacValue = ss->wSeqDacValue;
    }
	QueCommandDacValue(ss);
	CommandExternalDacs(0, ss);
}

//////////////////////////////////////////////////////////////////////
//
// WORD fnAdjustByFoot(WORD wDacValue)
//
//////////////////////////////////////////////////////////////////////
static WORD fnAdjustByFoot(WORD wDacValue)
{
	DWORD dwTemp = (DWORD)wDacValue * ReadFootController(); 
	return (WORD)(dwTemp / FOOT_CONTROL_MAX);
}

//////////////////////////////////////////////////////////////////////
//
// void AmpPriCmd(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
static void AmpPriCmd(SERVO_VALUES* ss)
{
	WORD wDacValue;

	if (WeldSystem.uWeldMode)
    {
	    wDacValue = fnSlopeValuePrimary(ss->uServoIndex);

	    if (ss->uPulseMode == LIT_PULSEMODE_EXTERNAL) 
        {
		    BOOL bExternalState;
		    if (ExternalLine(&bExternalState, ss)) 
            {
			    if (bExternalState == __FALSE) 
                {
				    wDacValue = fnSlopeValueBackgnd(ss->uServoIndex); // Special case for External Sync
			    }
		    }
	    }
	    
	    AmpCommonCmd(ss, wDacValue);
    }
}         

//////////////////////////////////////////////////////////////////////
//
// void AmpBacCmd(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
void AmpBacCmd(SERVO_VALUES* ss)
{
	WORD wDacValue;

	if (WeldSystem.uWeldMode)
    {
	    wDacValue = fnSlopeValueBackgnd(ss->uServoIndex);  // Normal case

	    if (ss->uPulseMode == LIT_PULSEMODE_EXTERNAL) 
        {
		    BOOL bExternalState;
		    if (ExternalLine(&bExternalState, ss)) 
            {
			    if (bExternalState == __TRUE) 
                {
				    wDacValue = fnSlopeValuePrimary(ss->uServoIndex); // Special case for External Sync
			    }
		    }
	    }
	    
	    AmpCommonCmd(ss, wDacValue);
    }
}         

//////////////////////////////////////////////////////////////////////
//
// void AmpCommonCmd(SERVO_VALUES* ss, WORD wDacValue)
//
//////////////////////////////////////////////////////////////////////
void AmpCommonCmd(SERVO_VALUES* ss, WORD wDacValue)
{
	ss->wSeqDacValue = wDacValue;

	if (mRequiresFootControl(ss->uServoIndex))
    {
		ss->wSeqDacValue = fnAdjustByFoot(ss->wSeqDacValue);
    }

	if (uSystemWeldPhase > WP_UPSLOPE) 
    {
		ss->wBuffDacValue = ss->wSeqDacValue;
		ss->bDacUpdate = __TRUE;
	}
    else
    {
    
	    if (ss->wSeqDacValue > mStartLevel)
        {
		    ss->wBuffDacValue  = ss->wSeqDacValue;
        }
	    else
        {
		    ss->wBuffDacValue  = mStartLevel;
        }
	    
	    ss->bDacUpdate = __TRUE;
    }
}         

//////////////////////////////////////////////////////////////////////
//
// void TvlPriCmd(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
void TvlPriCmd(SERVO_VALUES* ss)
{
	if (ss->uPulseMode == LIT_TVL_STEP_EXTERNAL) 
    {
		BOOL bExternalState;
		if (ExternalLine(&bExternalState, ss)) 
        {
			if (bExternalState == __FALSE) 
            {
				TvlBacCmd(ss);
				return;
			}
		}
	}
	
	if ((ss->uPulseMode == LIT_TVL_STEP_OFF) ||
		(mDirection(ss->uServoIndex) == LIT_TVL_OFF)) 
    {
		fnZeroTvlDac(ss);
		return;
	}
   
	ss->wSeqDacValue = mPrimaryValue(ss->uServoIndex);
//ETHERNET_Print("TvlPriCmd: %04X\n", ss->wSeqDacValue);
	QueCommandDacValue(ss);
	CommandExternalDacs(mExternDacPri(ss->uServoIndex), ss);
}

//////////////////////////////////////////////////////////////////////
//
// void TvlBacCmd(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
void TvlBacCmd(SERVO_VALUES* ss)
{  
	if (ss->uPulseMode == LIT_TVL_STEP_EXTERNAL) 
    {
		BOOL bExternalState;
		if (ExternalLine(&bExternalState, ss)) 
        {
			if (bExternalState == __TRUE) 
            {
				TvlPriCmd(ss);
				return;
			}
		}
	}

	if ((ss->uPulseMode == LIT_TVL_STEP_OFF) ||
		(mDirection(ss->uServoIndex) == LIT_TVL_OFF)) 
    {
		fnZeroTvlDac(ss);
		return;
	}
	
	if (ss->uPulseMode != LIT_TVL_STEP_STEP) 
    {
		TvlPriCmd(ss);
		return;
	}
	
	ss->wSeqDacValue = (WORD)mBackgroundValue(ss->uServoIndex);
//ETHERNET_Print("TvlBacCmd: %X\n", ss->wSeqDacValue);
	QueCommandDacValue(ss);
	CommandExternalDacs(mExternDacBac(ss->uServoIndex), ss);
}

//////////////////////////////////////////////////////////////////////
//
// void WfdPriCmd(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
void WfdPriCmd(SERVO_VALUES* ss)
{     
	WORD wExtDacValue;

	if (ss->bRetracting) 
    {
		ss->wSeqDacValue = mJogSpeedForward(ss->uServoIndex);
		QueCommandDacValue(ss);
		CommandExternalDacs(mExtDacJogSpeedFwd(ss->uServoIndex), ss);
		return; // Retract in progress
	}

	if (ss->uPulseMode == WFD_PULSE_EXTERNAL) 
    {
		BOOL bExternalState;
		if (ExternalLine(&bExternalState, ss)) 
        {
			if (bExternalState == __FALSE) 
            {
				WfdBacCmd(ss);
				return;
			}
		}
	}

	if (ss->wEnableDelay != 0) 
    {
		// Enable Delay is running, Output Background
		if (mPulseMode(ss->uServoIndex) == WFD_PULSE_ON) 
        {
			ss->wSeqDacValue = fnSlopeValueBackgnd(ss->uServoIndex);
			wExtDacValue     = fnSlopeValueBackgndExt(ss->uServoIndex);
//			printf("WfdPriCmd::BacWfd: %04X\n", ss->wSeqDacValue);
//			printf("WfdPriCmd::BacExt: %04X\n", wExtDacValue);
		}
		else 
        {
			ss->wSeqDacValue = fnSlopeValuePrimary(ss->uServoIndex);
			wExtDacValue     = fnSlopeValuePrimaryExt(ss->uServoIndex);
//			printf("WfdPriCmd::PriWfd: %04X\n", ss->wSeqDacValue);
//			printf("WfdPriCmd::PriExt: %04X\n", wExtDacValue);
		}
	}
	else 
    {
		// Enable Delay is timed out, Output Primary
		ss->wSeqDacValue = fnSlopeValuePrimary(ss->uServoIndex);
		wExtDacValue     = fnSlopeValuePrimaryExt(ss->uServoIndex);
//		printf("WfdPriCmd::PriWfd: %04X\n", ss->wSeqDacValue);
//		printf("WfdPriCmd::PriExt: %04X\n", wExtDacValue);
	}

//ETHERNET_Print("WfdPriCmd: Dac:%04X\n", ss->wSeqDacValue);

	QueCommandDacValue(ss);
	CommandExternalDacs(wExtDacValue, ss);
}
  
//////////////////////////////////////////////////////////////////////
//
// void WfdBacToPriCmd(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
void WfdBacToPriCmd(SERVO_VALUES* ss)
{      
	ss->wEnableDelay = mPriPulseDelay(ss->uServoIndex);
	WfdPriCmd(ss);
}
  
//////////////////////////////////////////////////////////////////////
//
// void WfdBacCmd(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
void WfdBacCmd(SERVO_VALUES* ss)
{
	WORD wExtDacValue = 0;

	if (ss->bRetracting) 
	{
		ss->wSeqDacValue = mJogSpeedForward(ss->uServoIndex);
		QueCommandDacValue(ss);
		CommandExternalDacs(mExtDacJogSpeedFwd(ss->uServoIndex), ss);
		return; // Retract in progress
	}

	if (ss->uPulseMode == WFD_PULSE_EXTERNAL) 
	{
		BOOL bExternalState;
		if (ExternalLine(&bExternalState, ss)) 
		{
			if (bExternalState == __TRUE) 
			{
				WfdPriCmd(ss);
				return;
			}
		}
	}

	if (ss->uPulseMode == WFD_PULSE_ON) 
	{
		// Pulse Mode ON, Output sloped backgnd values
		ss->wEnableDelay = 0;
		ss->wSeqDacValue = fnSlopeValueBackgnd(ss->uServoIndex);
		wExtDacValue     = fnSlopeValueBackgndExt(ss->uServoIndex);
//		printf("WfdBacCmd::BacWfd: %04X\n", ss->wSeqDacValue);
//		printf("WfdBacCmd::BacExt: %04X\n", wExtDacValue);
	}
	else 
	{
		// Pulse Mode OFF, Output sloped Primary Values
		ss->wSeqDacValue = fnSlopeValuePrimary(ss->uServoIndex);
		wExtDacValue     = fnSlopeValuePrimaryExt(ss->uServoIndex);
//		printf("WfdBacCmd::PriWfd: %04X\n", ss->wSeqDacValue);
//		printf("WfdBacCmd::PriExt: %04X\n", wExtDacValue);
	}

	QueCommandDacValue(ss);
	CommandExternalDacs(wExtDacValue, ss);
}

static short GetGndFaultCurrent(void)
{
    short nDacValue;

    
    // nDacValue = (GROUND_FAULT_CURRENT * FULL_DAC_SWING) / fAmpRange);
    nDacValue = (3 * FULL_DAC_SWING) / 1200;

	return nDacValue;
}

void DisableGroundFault(void)
{
	SERVO_VALUES* ss;
	ss = &ServoStruct[STD_AMP_SLOT]; // Save pointer to this servos struct
	
		//Exit ground fault test
	if (mGNDFaultUsed && WeldSystem.uWeldMode && (WeldSystem.uGndFaultPhase != 0)) 			
	{
		WeldSystem.uGndFaultPhase = 0;
		bCheckGroundFault = __FALSE;
		SetCommandDacValue((short)0, ss); // COMMAND 0 AMPS
		SetCurrentEnableBit(__FALSE);
		DriveCommandBit(GNDFLT_RELAY, __FALSE);
		SetGasValveOpen(__FALSE);
	}
}

//////////////////////////////////////////////////////////////////////
//
// void GndFaultCheck(void)
//
//////////////////////////////////////////////////////////////////////
static void GndFaultCheck(void)
{
	SERVO_VALUES* ss;
	static BYTE uGfTime;
	static BYTE bGroundFault;

	if (!bCheckGroundFault)
		return;  // Function not active

	ss = &ServoStruct[STD_AMP_SLOT]; // Save pointer to this servos struct
	
	switch(WeldSystem.uGndFaultPhase) 
	{
	case 0:
		uLockFaults = __TRUE;
		bGroundFault = __FALSE;
		SetGasValveOpen(__TRUE);
		SetCurrentEnableBit(__FALSE);
		uGfTime = 10; // Delay 100 mSeconds
		WeldSystem.uGndFaultPhase = 1;
		break;
		
	case 1:
		if (--uGfTime) 
			return;  // Exit, Not done
		
		SetBleederDisableBit(__FALSE);
		uGfTime = 60; // Delay 600 mSeconds
		WeldSystem.uGndFaultPhase = 2;
		break;
		
	case 2:
		if (--uGfTime)
			return;  // Exit, Not done
		
		DriveCommandBit(GNDFLT_RELAY, __TRUE);
		uGfTime = 20; // Delay 200 mSeconds
		WeldSystem.uGndFaultPhase = 3;
		break;
		
	case 3:
		if (--uGfTime)
			return;  // Exit, Not done
		
		SetBleederDisableBit(__TRUE);
		uGfTime = 10; // Delay 100 mSeconds
		WeldSystem.uGndFaultPhase = 4;
		break;
		
	case 4:
		if (--uGfTime)
			return;  // Exit, Not done
		
		SetCommandDacValue(GetGndFaultCurrent(), ss); // Command out 3 amps
		SetCurrentEnableBit(__TRUE);
		uGfTime = 200; // Delay 2000 mSeconds
		WeldSystem.uGndFaultPhase = 5;
		break;
		
	case 5:
		if (--uGfTime)
			return;  // Exit, Not done
		
		if (!PsInRegulation()) 
		{
			bGroundFault = __TRUE;
			SetGasValveOpen(__FALSE);
		}

//		EDWARD TEST
/*		Ps voltage is not ready......
		if (GetPowerSupplyVoltage() > GF_VOLTAGE18) 
		{
			bGroundFault = __TRUE;
			SetGasValveOpen(__FALSE);
		}
*/		
		SetCommandDacValue((short)0, ss); // COMMAND 0 AMPS
		SetCurrentEnableBit(__FALSE);
		uGfTime = 100; // Delay 1000 mSeconds
		WeldSystem.uGndFaultPhase = 6;
		break;
		
	case 6:
		if (--uGfTime)
			return;  // Exit, Not done
		
		DriveCommandBit(GNDFLT_RELAY, __FALSE);
		uGfTime = 60; // Delay 600 mSeconds
		WeldSystem.uGndFaultPhase = 7;
		break;
		
	case 7:
		if (--uGfTime)
			return;  // Exit, Not done

		// WEDGE TEST
		//bGroundFault = __FALSE;

		if (bGroundFault) 
		{
			SetBleederDisableBit(__FALSE); // Enable the bleeder
			QueueFaultMessage(GROUND_FLT_SET);
			QueueEventMessage(SYSTEM_EVENT, EV_WELDPHASE_EXIT);
			uStoppedOnFault = __TRUE;
		}
		else 
		{
			// Clear this fault directly because it's not a lock fault
			QueueFaultMessage(GROUND_FLT_CLR);
			bWeldSequence = __TRUE; 
			InSequenceIO(bWeldSequence);
		}
		
		WeldSystem.uGndFaultPhase = 0;
		bCheckGroundFault = __FALSE;
		break;
	}
}

//////////////////////////////////////////////////////////////////////
//
// void AmpFlowCheck(void)
//
//////////////////////////////////////////////////////////////////////
/*
void AmpFlowCheck(void)
{
	if (!PsInRegulation()) 
    {
		WeldSystem.Fault.uAmpFlowCount = 0;
		return;
	}

	if (WeldSystem.Fault.uAmpFlowCount > 2)
		return; // Once is enough...get out

	++WeldSystem.Fault.uAmpFlowCount;
//ETHERNET_Print("AmpFlowCheck: AmpFlowCount: %d\n", WeldSystem.Fault.uAmpFlowCount);
	if (WeldSystem.Fault.uAmpFlowCount > 2) 
    {
		SetBoosterEnableBit(__FALSE);
//		dprintf("AmpFlowCheck: Booster Disable\n");
	}
}
*/

//////////////////////////////////////////////////////////////////////
//
// void fnPrepurgeLoop(void)
//
//////////////////////////////////////////////////////////////////////
void fnPrepurgeLoop(void)
{
	if (WeldSystem.Slope.dwPurgeTime) 
    {
		--WeldSystem.Slope.dwPurgeTime;
//		AmpFlowCheck();
		
		if (!WeldSystem.Slope.dwPurgeTime) 
        {
			WeldSystem.uArcStartPhase = AS_PRESTRIKE;
			WeldSystem.uStartAttempts = mStartAttempts;
//			printf("Prepurge Complete\n");
		}

		if ((WeldSystem.Slope.dwPurgeTime < 300) &&
			(WeldSystem.Slope.dwPurgeTime > 5)) 
        {
			SetStartLevel();
			SetCurrentEnableBit(__TRUE);
//			dprintf("fnPrepurgeLoop:: dwPurgeTime: %d\n", WeldSystem.Slope.dwPurgeTime);
		}
		return;
	}
	
	// If ArcStart Succeeds: Sets uSystemWeldPhase = WP_UPSLOPE.
	// If ArcStart Fails:    Sets uSystemWeldPhase = WP_EXIT.
	ArcStart(); 
}
            
//////////////////////////////////////////////////////////////////////
//
// void BumpSlopeDAC(void)
//
//////////////////////////////////////////////////////////////////////
void BumpSlopeDAC(void)
{
	WeldSystem.Slope.dwSlopeDac += WeldSystem.Slope.dwSlopeDacInc; 
	if (WeldSystem.Slope.dwSlopeDac > SLOPE_DAC_MAX)  
		WeldSystem.Slope.dwSlopeDac = SLOPE_DAC_MAX;

//ETHERNET_Print("BumpSlopeDAC:: SlopeDac: %04X LevelTime %05d\n", (WORD)(WeldSystem.Slope.dwSlopeDac >> 16), WeldSystem.dwLevelTimeCount);
}

//////////////////////////////////////////////////////////////////////
//
// void fnDecSlopeTime(void)
//
//////////////////////////////////////////////////////////////////////
static void fnDecSlopeTime(void)
{
	if (WeldSystem.Slope.dwSlopeTimeCount) 
    {
		--WeldSystem.Slope.dwSlopeTimeCount;
		if (WeldSystem.Slope.dwSlopeTimeCount)
			return;

//		printf("fnDecSlopeTime: uSystemWeldPhase is %d\n",uSystemWeldPhase);

		// dwSlopeTimeCount now is zero
		switch(uSystemWeldPhase) 
        {
		case WP_UPSLOPE:
			uSystemWeldPhase = WP_LEVELWELD;
//			printf("fnDecSlopeTime::Go to WP_LEVELWELD\n");
			if (WeldSystem.uDataAqiStart == DA_STARTAT_UPSLOPE)
			{
				WeldSystem.uDataAqiOutEnable = __TRUE;
				QueueEventMessage(SYSTEM_EVENT, EV_DASTREAM_ON);
			}
			QueueEventMessage(SYSTEM_EVENT, EV_WELDPHASE_LEVELWELD);

			// Reset counters to prepare for Sloping in Level 1
			InitSlopeStruct(mSlopeTime);  
			break;
   
		case WP_DOWNSLOPE:
			fnInitPostPurge(); // Will determine the next phase
			QueueEventMessage(SYSTEM_EVENT, EV_WELDPHASE_POSTPURGE);
			break;
		}
	}

}

//////////////////////////////////////////////////////////////////////
//
// void fnLevelWeldLoop(void)
//
//////////////////////////////////////////////////////////////////////
void fnLevelWeldLoop(void)
{   
	if (WeldSystem.uWeldLevel != uDesiredLevel)
		LevelWeldLevelChange();
   
	fnExecutePulseTimers();
	fnExecuteDwellTimers();
	fnExecutePulsePhase();
	fnDecrementUnlockTimers();
	fnExecuteRetracts();
	
	CheckCurrent();
	//CheckVoltage();
	fnEvalLevelAdvance();
	fnDecrementServoDelays();
	++WeldSystem.dwLevelTimeCount;
}

//////////////////////////////////////////////////////////////////////
//
// void fnSlopeLoop(void)
//
//////////////////////////////////////////////////////////////////////
void fnSlopeLoop(void)
{
	if (WeldSystem.dwSlopeStartTime != 0)
    {
		if (WeldSystem.dwLevelTimeCount > WeldSystem.dwSlopeStartTime)
			BumpSlopeDAC();
	}
	fnLevelWeldLoop();
	fnDecSlopeTime();
}

//////////////////////////////////////////////////////////////////////
//
// void fnPostPurgeLoop(void)
//
//////////////////////////////////////////////////////////////////////
void fnPostPurgeLoop(void)
{
	if (WeldSystem.Slope.dwPurgeTime) 
		--WeldSystem.Slope.dwPurgeTime;

	if (!WeldSystem.Slope.dwPurgeTime)
	{
		uSystemWeldPhase = WP_EXIT; // ON NEXT INTERRUPT
//		printf("fnPostPurgeLoop::Go to WP_EXIT\n");
	}
}

//////////////////////////////////////////////////////////////////////
//
// void fnExitPhase(void)
//
//////////////////////////////////////////////////////////////////////
void fnExitPhase(void)
{
//	printf("fnExitPhase\n");

	fnWarmShutdown();
//	ReEnableOsc();

//	if ((WeldSystem.uWeldMode) && (WeldSystem.bArcEstablished))
//		SetFanDelay();
	bWeldSequence    = __FALSE;
	InSequenceIO(bWeldSequence);
	uSystemWeldPhase = WP_INITUPSLOPE; // Prepare for next sequence
//	printf("fnExitPhase::Go to WP_INITUPSLOPE ???????\n");

	if (uStoppedOnFault) 
    {
		QueueEventMessage(SYSTEM_EVENT, EV_STOPPED_ON_FAULT);
	}
	else 
    {
		uLockFaults = __FALSE;
	}

	StartReturnToHome();

	uDesiredLevel				= 0; // Reset Desired level
	WeldSystem.uWeldLevel		= 0; // Reset Weld Level
	WeldSystem.dwLevelTimeCount = 0; // Reset Level Time
	SetLevelTime(mLevelTime);
	InitSlopeStruct(mSlopeTime);

	WeldSystem.bArcEstablished = __FALSE;
	SetArcOnBit(WeldSystem.bArcEstablished);
	QueueEventMessage(SYSTEM_EVENT, EV_WELDPHASE_EXIT);
	WeldSystem.uDataAqiOutEnable = __FALSE;
	QueueEventMessage(SYSTEM_EVENT, EV_DASTREAM_OFF);
//	printf("fnExitPhase:: END\n");
}


static void fnForceSeqStop(void)
{
	WELDCOMMAND stWeldCommand;
	stWeldCommand.uCommand = WELDCMD_SEQSTOP;
	stWeldCommand.wLength  = 0;
	fnWeldCommands(&stWeldCommand);
// 	printf("fnForceSeqStop\n");
}

static void PerformHeartBeatCheck(void)
{
	if (++nHB_CheckCount > 100)
	{
		nHB_CheckCount = 0;
		QueueEventMessage(SYSTEM_EVENT, EV_HEARTBEAT);
//		printf("Heart Beat Sent\n");
		if (++nHB_FailCount > 25) 
		{
			printf("Heart Beat FAILURE\n");
			fnForceSeqStop();
		}
	}
}

//////////////////////////////////////////////////////////////////////
//
// void WeldLoop(void)
//
//////////////////////////////////////////////////////////////////////
static void WeldLoop(void)
{

	switch(uSystemWeldPhase) 
	{
		case WP_INITUPSLOPE: fnInitUpslope();	break;
		case WP_PREPURGE:	 fnPrepurgeLoop();	break;
		case WP_UPSLOPE:	 fnSlopeLoop();		break;
		case WP_LEVELWELD:	 fnSlopeLoop();		break;  // Modified to slope between levels
		case WP_DOWNSLOPE:	 fnSlopeLoop();		break;
		case WP_POSTPURGE:	 fnPostPurgeLoop(); break;
		case WP_EXIT:		 fnExitPhase();		break;
		default : 
			return;
	}

	//PerformHeartBeatCheck();

    ++dwElapsedTime;
	if (uSystemWeldPhase < WP_PREPURGE) 
    {
		dwElapsedTime = 0;
	}

//    if ((WeldSystem.uDataAqiOutEnable != __FALSE)                   ||
//        ((dwElapsedTime % (DWORD)WeldSystem.wDataAqiRate) == 0)     ||
//         (g_bForceBargraphUpdate != __FALSE))
//    {
//		QueueElapsedTime();
//    }
}

//////////////////////////////////////////////////////////////////////
//
// void InitOscManual(void)
//
//////////////////////////////////////////////////////////////////////
void InitOscManual(void)
{
	InitSystemStruct();
	uSystemWeldPhase = WP_LEVELWELD;
}

//////////////////////////////////////////////////////////////////////
//
// void fnExecuteOscManual(void)
//
//////////////////////////////////////////////////////////////////////
void fnExecuteOscManual(void)
{
	fnExecuteDwellTimers();
}

//////////////////////////////////////////////////////////////////////
//
// void JogManip(JOG_STRUCT *)
//
//////////////////////////////////////////////////////////////////////
static void JogManip(const JOG_STRUCT *pJogStruct)
{
	BYTE uManipNum;

	uManipNum   = pJogStruct->uServo;
	uManipSpeed = pJogStruct->uSpeed;

	SelectManip(uManipNum);
	if (uManipSpeed == 0) 
    {		
		ShutdownManip(uManipNum);
		return;
	}

	//SetManipCurrent(mJogCurrent(uManipNum)); // Jog normally
	//SetManipVolts(mJogVoltage(uManipNum), uManipSpeed);
	EnableManip(uManipNum);
}


//////////////////////////////////////////////////////////////////////
//
// void BuildGraphInfoTable(void)
//
// Comments:
//       Generates a table that is used when reading the graph data.
//		 Must be called before ReadBarGraphData().
//
//////////////////////////////////////////////////////////////////////
static void BuildGraphInfoTable(void)
{
	SERVO_VALUES* ss;
	BYTE uServoIndex;
	BYTE uIndex;

	WORD wSlotMap[MAX_GRAPH_PORTS] = {FB_SLOT0,	    // Slot 0 
									  FB_SLOT1, 	// Slot 1 
									  FB_SLOT2,		// Slot 2 
									  FB_SLOT3,		// Slot 3 
									  FB_SLOT4,		// Slot 4 
									  FB_SLOT5,		// Slot 5 
									  FB_SLOT6,		// Slot 6 
									  FB_SLOT7,		// Slot 7 
									  FB_SLOT8};	// Slot 8

	for (uServoIndex = 0; uServoIndex < mNumServos; ++uServoIndex) 
	{
		ss = &ServoStruct[uServoIndex]; // Save pointer to this servos struct

		GraphInfo[uServoIndex].wPort = wSlotMap[ss->uServoSlot];
		GraphInfo[uServoIndex].dwDataTag = (DWORD)(uServoIndex << 28);
		UpdateGraphInfo(&GraphInfo[uServoIndex], ADC_ZERO_OFFSET,  ADC_ZERO_OFFSET); // Init Command & Feedback values

		// Special Case for Openloop Avc Servos
		if (ss->uServoFunction == SF_AVC) 
        {
			if (wSlotMap[ss->uServoSlot] != STD_AVC_SLOT) 
            {
				// Avc Servo not in slot 3, still read thier voltage from the standard ADC channel
				GraphInfo[uServoIndex].wPort = FB_SLOT3;
			}
		}

		// Special Case for Openloop Velocity Servos
		if ((ss->uServoFunction == SF_TRAVEL)	|| 
			(ss->uServoFunction == SF_WIREFEED)	||
			(ss->uServoFunction == SF_JOG_ONLY_VEL)) 
        {
			if (mFeedbackDeviceType(uServoIndex) == FEEDBACK_ENCODER) 
            {
				// Encoder Data will be read seperately
				GraphInfo[uServoIndex].wPort = 0xFFFF;  // Indcates DO NOT READ
			}
		}
	}

	uIndex = mNumServos;
	if (!_isAvcCardPresent) 
	{
		// mNumServos is used as the index here
		GraphInfo[uIndex].wPort     = FB_PS_VOLTAGE;
		GraphInfo[uIndex].dwDataTag = (DWORD)(uIndex << 28); // AVC Slot, FB_PS_VOLTAGE in this case
    	UpdateGraphInfo(&GraphInfo[uServoIndex], ADC_ZERO_OFFSET,  ADC_ZERO_OFFSET); // Init Command & Feedback values
		++uIndex;
	}

	uNumGraphPorts = uIndex;
}

void SetWeldSchedule(void)
{
	InitServoStructs(__TRUE);
	BuildGraphInfoTable();
}

void SetWirefeedMode(LPCWELDCOMMAND pWeldCommand)
{
	BYTE uWireMode;
	BYTE uServoIndex;
	SERVO_VALUES* ss;

	uWireMode   = pWeldCommand->uByteData[0];
	uServoIndex = pWeldCommand->uByteData[1];

//ETHERNET_Print("SetWirefeedMode:: Mode: %d Servo: %d\n", pWeldCommand->uByteData[0], pWeldCommand->uByteData[1]);
	WeldSystem.uWireFeedModeAuto[uServoIndex] = uWireMode;

	ss = &ServoStruct[uServoIndex]; // Save pointer to this servos struct
	if (ss->uServoFunction == SF_WIREFEED) 
	{
		if (bWeldSequence) 
		{
			// Turn On or Off wirefeed servo
			if (uWireMode) 
            {
				// Wire should be turned on;
				if ((uSystemWeldPhase != WP_DOWNSLOPE) && (!ss->nUnlockTime)) 
                {
					fnEnableServo(ss);
					QueueEventMessage(ss->uServoIndex, SERVO_UNLOCK);
				}
			}
			else 
            {
				// Wire should be turned off;
				// Start retract if it was feeding
				if (ss->bEnabledFlag) 
				{
					InitWireInTime(ss);
					QueueEventMessage(ss->uServoIndex, SERVO_LOCK);
				}
			}
		}
	}
}

// This code is to disable jogging of the OSC servo if the 
// SF_JOG_ONLY_POS is set to be jogged
static void fnServoMapFixup(void)
{
	PendantKeyServoMap[SF_TRAVEL] = 1;
	PendantKeyServoMap[SF_WIREFEED] = 2;
	PendantKeyServoMap[SF_OSC] = 4;
	PendantKeyServoMap[SF_AVC] = 3;
	if (PendantKeyServoMap[SF_OSC] != PendantKeyServoMap[SF_JOG_ONLY_POS])
		PendantKeyServoMap[SF_JOG_ONLY_POS] = (-1);
}

static void SetPendantServoSelection(LPCWELDCOMMAND pWeldCommand)
{
	SERVO_VALUES* ss;
	BYTE uServoIndex;

	memcpy(PendantKeyServoMap, pWeldCommand->uByteData, (sizeof(int) * SF_COUNT));
	fnServoMapFixup();

	for (uServoIndex = 0; uServoIndex < mNumServos; ++uServoIndex) 
	{
		ss = &ServoStruct[uServoIndex];
		ss->bPendantControl = __FALSE;
		if (PendantKeyServoMap[ss->uServoFunction] == uServoIndex) 
        {
			ss->bPendantControl = __TRUE;
			// Reset the Travel LED State here
			if (ss->uServoFunction == SF_TRAVEL) 
            {
				// Will output Travel Direction AND update the LEDs
				fnTravelDirectionCommand(ss);
			}
//			dprintf("SetPendantServoSelection:: Servo: %d\n", uServoIndex);
		}
	}
}

static void ChangeTVL_DirCommandLvl(BYTE uServoIndex, BYTE uDirection, BYTE uAllLevels)
{
	int nLevelIndex;
	LPIW_SERVOMULTILEVEL pServoMultiLevel;

	if (uAllLevels) 
    {
		// Update all weld schedule values, all levels including the last level
		for (nLevelIndex = 0; nLevelIndex < (mLastLevel + 1); ++nLevelIndex) 
        {
			pServoMultiLevel = &stInitWeldSched.stMultiLevel[nLevelIndex].stServoMultiLevel[uServoIndex];
			pServoMultiLevel->uDirection = uDirection;
		}
	}
	else 
    {
		// Change this level only
		nLevelIndex = WeldSystem.uWeldLevel;
		pServoMultiLevel = &stInitWeldSched.stMultiLevel[nLevelIndex].stServoMultiLevel[uServoIndex];
		pServoMultiLevel->uDirection = uDirection;
	}
}

static void ChangeTVL_DirCommand(LPCWELDCOMMAND pWeldCommand)
{
	SERVO_VALUES* ss;

	BYTE uDirection  = pWeldCommand->uByteData[0];
	BYTE uServoIndex = pWeldCommand->uByteData[1];
	BYTE uAllLevels  = pWeldCommand->uByteData[2];

	ChangeTVL_DirCommandLvl(uServoIndex, uDirection, uAllLevels);
//ETHERNET_Print("ChangeTVL_DirCommand:: Servo: %d\n", uServoIndex);
	ss = &ServoStruct[uServoIndex];
	ss->uDirection = uDirection;
	fnTravelDirectionCommand(ss);
}

// NEEDS WORK
static void SetServoConstants(const int *pConstantTable)
{
/*
	BYTE uIndex, uServoSlot;

	for (uIndex = 0; uIndex < 8; ++uIndex) 
    {

		uServoSlot = uIndex + 1;

		ServoConstant[uServoSlot].wPosition = (WORD)pConstantTable[uIndex + 0];
		ServoConstant[uServoSlot].wVelocity = (WORD)pConstantTable[uIndex + 8];

//		printf("SetServoConstants:: Slot: %02X POS: 0x%04X VEL: 0x%04X\n", uServoSlot, ServoConstant[uServoSlot].wPosition, ServoConstant[uServoSlot].wVelocity);
	}
*/
}

//////////////////////////////////////////////////////////////////////
//
// void SetFaultEnables(LPC_STANDARD_SETUP_STRUCT stStandardSetupStruct)
//
//////////////////////////////////////////////////////////////////////
void SetFaultEnables(LPC_STANDARD_SETUP_STRUCT stStandardSetupStruct)
{
	bStubOutEnable    = (BOOL)stStandardSetupStruct->nStubOut;
	bHighArcEnable    = (BOOL)stStandardSetupStruct->nHighArc;
	uGasFaultEnable   = (BYTE)stStandardSetupStruct->nGasFault;
	bPriCoolantEnable = (BOOL)stStandardSetupStruct->nPriCoolant;
	bAuxCoolantEnable = (BOOL)stStandardSetupStruct->nAuxCoolant;
}

//////////////////////////////////////////////////////////////////////
//
// void ExtendLevelTime(void)
// Extends the time of this level by the amount of slope time
//////////////////////////////////////////////////////////////////////
void  ExtendLevelTime(void)
{
//ETHERNET_Print("ExtendLevelTime:: ");
	if (uSystemWeldPhase == WP_UPSLOPE) 
    {
//		dprintf("In Upslope\n");
		SetLevelTime(WeldSystem.dwLevelTimeCount);  // Advance on the next interrupt
		return; 
	} 
	
	if (WeldSystem.dwSlopeStartTime != 0) 
    {
		if (WeldSystem.dwLevelTimeCount > WeldSystem.dwSlopeStartTime) 
        {
//			dprintf("Currently Sloping, stop and advance to next level\n");
			// We're already sloping to the next Level..
			SetLevelTime(WeldSystem.dwLevelTimeCount);  // Advance on the next interrupt
			return;
		} 
	}
	
	// Reset the time for this Level to 
	// Current time in the Level plus the amount of slope time
	// Only if we are currently in-sequence
//ETHERNET_Print("Commencing slope to next level\n");
	WeldSystem.dwSlopeStartTime = WeldSystem.dwLevelTimeCount; // Start SlopeTime now
	SetLevelTime(WeldSystem.dwLevelTimeCount + mSlopeTime);
}

void SetDesiredLevel(BYTE uCommandedLevel)
{
//ETHERNET_Print("SetDesiredLevel::uCommandedLevel %d\n", uCommandedLevel);
	uDesiredLevel = uCommandedLevel;
	if (uDesiredLevel > mLastLevel) 
    {
		uDesiredLevel = mLastLevel; // Went too far
		if (bWeldSequence) 
        {
			// All levels have been completed
//			printf("SetDesiredLevel::CommenceDownslope\r\n");
			CommenceDownslope();
		}
	}
}

void MoveToNewLevel(void)
{
//ETHERNET_Print("MoveToNewLevel::Desired:%d Current:%d\n", uDesiredLevel, WeldSystem.uWeldLevel);
/*
	if ((uDesiredLevel == 0) || (uDesiredLevel > mLastLevel)) 
    {
		// Level Reset
//		dprintf("MoveToNewLevel::Level Reset from Level %d\n", WeldSystem.uWeldLevel);
		WeldSystem.uWeldLevel = 0;
		QueueEventMessage(LEVEL_RST, EV_LEVELCHANGE);
		UpdateServoStructs();
		return;
	}
*/

	if ((uDesiredLevel == 0) && 
		(WeldSystem.uWeldLevel != 1)) 
	{
		// Level Reset
//		dprintf("MoveToNewLevel::Level Reset from Level %d\n", WeldSystem.uWeldLevel);
		WeldSystem.uWeldLevel = 0;
		QueueEventMessage(LEVEL_RST, EV_LEVELCHANGE);
		UpdateServoStructs();
		return;
	}
	else 
	{
		// This is a normal Level De-Advance.....handled later
	}

	if (uDesiredLevel == WeldSystem.uWeldLevel)
		return; // Nothing to do
	
	if (uDesiredLevel > WeldSystem.uWeldLevel) 
	{
		// Level Advance
		while(uDesiredLevel != WeldSystem.uWeldLevel) 
		{
			++WeldSystem.uWeldLevel;
			QueueEventMessage(LEVEL_INC, EV_LEVELCHANGE);
//			dprintf("MoveToNewLevel::Level Advance to Level %d\n", WeldSystem.uWeldLevel);
		}
	}
	else 
	{
		// Level De-advance
		while(uDesiredLevel != WeldSystem.uWeldLevel) 
		{
			--WeldSystem.uWeldLevel;
			QueueEventMessage(LEVEL_DEC, EV_LEVELCHANGE);
//			dprintf("MoveToNewLevel::Level De-Advance to Level %d\n", WeldSystem.uWeldLevel);
		}
	}

	WeldSystem.dwLevelTimeCount = 0;
	SetLevelTime(mLevelTime);
	InitSlopeStruct(mSlopeTime);
	UpdateServoStructs();
}

static void HandleCommandedLevel(BYTE uCommandedLevel)
{
//ETHERNET_Print("HandleCommandedLevel\n");

	if (uDesiredLevel == uCommandedLevel)
		return;

	if (bWeldSequence) 
	{
		// Power Supply is in Sequence
		if (uSystemWeldPhase < WP_DOWNSLOPE) 
		{
			// Weld is NOT in downslope
			if ((uDesiredLevel < uCommandedLevel) && mSlopeTime)
			{
				// Level Advance requested and sloping is used
				// Don't go to the next Level, but extend the time of this level
				ExtendLevelTime();
			}
			else 
			{
				// Level de-advance or Not sloping
				SetDesiredLevel(uCommandedLevel);
				MoveToNewLevel();
			}
		}
		else 
		{
			// Weld IS in downslope, No action required
		}
	}
	else 
	{
		// Not insequence
		SetDesiredLevel(uCommandedLevel);
		MoveToNewLevel();
	}
}

//////////////////////////////////////////////////////////////////////
//
// void InitializeWeldParameters(void)
//
//////////////////////////////////////////////////////////////////////
void InitializeWeldParameters(void)
{
	int i;
	for (i = 0; i < MAX_NUMBER_SERVOS; ++i) 
    {
		bReturnToHome[i]  = __FALSE;
		bPreWrap[i]		  = __FALSE;
		bPerformSetPot[i] = __FALSE;
		WeldSystem.uWireFeedModeAuto[i] = 0;  // Set all to Manual
	}

	bOscManual			  = __FALSE;
	bCheckGroundFault	  = __FALSE;
	WeldSystem.uPurgeOn   = __FALSE;
	WeldSystem.uWeldMode  = __FALSE;
	fnServoMapFixup();
	ClearQueues();
	uDesiredLevel = 0;
}

//////////////////////////////////////////////////////////////////////
//
// void fnWeldCommands(LPCWELDCOMMAND pWeldCommand)
//
//////////////////////////////////////////////////////////////////////
void fnWeldCommands(LPCWELDCOMMAND pWeldCommand)
{
	BYTE uCommand  = pWeldCommand->uCommand;
//	WORD wLength   = pWeldCommand->wLength;
	BYTE uByteData = pWeldCommand->uByteData[0];	// pWeldCommand->uByteData is a 64 byte array
    int nFirstByte = (int) pWeldCommand->uByteData[0];
	int nSecondByte =	(int) pWeldCommand->uByteData[1];
	nFirstByte += (int) nSecondByte<<8;

	switch(uCommand) 
    {
		//////////////////////////////////////////
		// Commands with Data
	case WELDCMD_TESTMODE:
		if (!bWeldSequence) 
        {
			WeldSystem.uWeldMode = (BOOL)uByteData;
        }
		break;
	
	case WELDCMD_MANPRGE:
		WeldSystem.uPurgeOn = (BOOL)uByteData;
		STM_EVAL_LEDToggle(LED3);
		if (bWeldSequence && WeldSystem.uWeldMode)
			break; // Don't shut off the gas now!
		SetGasValveOpen(WeldSystem.uPurgeOn);
		break;
	
	case WELDCMD_WIREOFF:
		SetWirefeedMode(pWeldCommand);
		break;
	
	case SET_WELD_LEVEL:
		HandleCommandedLevel(uByteData);
		break;
	
	case WELDCMD_LAMPOFF:
		SetLampOn((BOOL)uByteData);
		break;
	
	case WELDCMD_SERVOJOG:
		JogServo((JOG_STRUCT const *)pWeldCommand->uByteData);
		break;

	case WELDCMD_MANIPJOG:
		JogManip((LPJOG_STRUCT)pWeldCommand->uByteData);
		break;
		
	case WELDCMD_OSCMAN:
		if (!bWeldSequence) 
		{
			if (bOscManual != uByteData) 
			{
				if (uByteData) 
				{
					InitOscManual();
					bOscManual = __TRUE;
				}
				else 
				{
					fnWarmShutdown();
					bOscManual = __FALSE;
				}
			}
		}
		break;

	case WELDCMD_SET:
		if (mSetFunctionUsed(uByteData) == __FALSE) 
            break;
		
		WeldSystem.uSetPotPhase = 0;
		bPerformSetPot[uByteData] = __TRUE;
		break;
	
	case WELDCMD_PWRAP:
		if (mPreWrapUsed(uByteData) == __FALSE)
            break;
		Home.uPhase = HS_PRETEST;
		bPreWrap[uByteData] = __TRUE;
		break;
	
	// Change which servo is controlled 
	// by the pendant 
	case WELDCMD_SVRCHG:
		SetPendantServoSelection(pWeldCommand);
		break;

	case SERVO_CONSTANTS:
		SetServoConstants((const int *)pWeldCommand->uByteData);
		break;

	case OFFSETGAIN_LOWER:
		memcpy(DacCalMap, pWeldCommand->uByteData, sizeof(DAC_CAL_STRUCT) * MAX_NUMBER_SERVOS);
		break;

	case SET_AUXLINE_FUNCTION:
		memcpy(&_uAuxLineFunction, pWeldCommand->uByteData, sizeof(BYTE) * 4);
		break;

	case ENABLE_OSCILLATOR:
		_wServoInitDelay = 100;
		break;
	
	case OSC_SPEEDSTEER:
		//nOscSpeedSteer = nFirstByte;//*((int *)pWeldCommand->uByteData);
		break;

	case SET_LAMP_TIME:
		nLampTimer = nFirstByte;//*((int *)pWeldCommand->uByteData);
		break;

//	case SET_BOOSTER_VOLTAGE:
//		nBoosterVoltage = nFirstByte;//*((int *)pWeldCommand->uByteData);
//		break;

	case SET_FAULT_ENABLES:
		SetFaultEnables((LPC_STANDARD_SETUP_STRUCT)pWeldCommand->uByteData);
		break;


	case SET_HOME_DIRECTION:
		if (uByteData == 0) // FORWARD
			nHomeDirection = 1; // Forward / DEFAULT direction
		else
			nHomeDirection = (-1); // Reverse direction
		break;

	case SET_POSTPURGEHOME:
		nHomeDuringPostpurge = uByteData; // Return to home during postpurge
		break;

	//////////////////////////////////////////
	// Commands without Data
	case CHECK_GROUND_FAULT:
		WeldSystem.uGndFaultPhase = 0;
		bCheckGroundFault = __TRUE;
		break;
	
	case WELDCMD_SEQSTART:
		if (!bWeldSequence)
		{
			if (mRequiresFootControl(0) && !FootControllerInstalled()) 
			{
				fnExitPhase(); // Perform an ALL Stop
				return;
			}

			DoSetPot();
			if (bOscManual) 
			{
				bOscManual = __FALSE;
				fnWarmShutdown();
			}
			
			uSystemWeldPhase = WP_INITUPSLOPE;
			QueueEventMessage(SYSTEM_EVENT, EV_WELDPHASE_SEQSTART);
			if ((mGNDFaultUsed) && (WeldSystem.uWeldMode)) 
			{
				WeldSystem.uGndFaultPhase = 0;
				bCheckGroundFault = __TRUE;
				// bWeldSequence will be set after completion of Ground Fault
			}
			else 
			{
				bWeldSequence = __TRUE;
				InSequenceIO(bWeldSequence);
			}
		}
		break;
	
	case WELDCMD_SEQSTOP:
		CommenceDownslope();
		break;
	
	case WELDCMD_ALLSTOP:
		DisableGroundFault();
		if ((uSystemWeldPhase >= WP_POSTPURGE) || !bWeldSequence) 
		{
         	QueueEventMessage(SYSTEM_EVENT, EV_WELDPHASE_EXIT);
			return;
		}
		fnInitPostPurge();
		break;
	
	case WELDCMD_TVLDIR:
		ChangeTVL_DirCommand(pWeldCommand);
		break;

	case AMI_CLEAR_FAULTS:
		InitFaultStruct();
		ResetAllFaults();
		break;

	case AMI_SET_WELDSCHED:
		SetWeldSchedule();
		break;

	case ONE_LEVEL_UPDATE:
		UpdateServoStructs();
		break;
	
	case POSITION_RESET:
		SetPositionEncoderValue((const SET_POSITION*)&pWeldCommand->uByteData[0]);
		break;

	case CMD_HEARTBEAT:
		nHB_FailCount = 0;
		break;

	case NO_SCHED_LOADED:
	   	TechSendCMD(1, CMDReset);
	   	TechSendCMD(2, CMDReset);
	   	TechSendCMD(3, CMDReset);
	   	TechSendCMD(4, CMDReset);
	   	StartUp();

/*		InitializeWeldParameters();
		uDesiredLevel				= 0; // Reset Desired level
		WeldSystem.uWeldLevel		= 0; // Reset Weld Level
		WeldSystem.dwLevelTimeCount = 0; // Reset Level Time
		mNumServos = 0;
		uNumGraphPorts = 0;
		bResetAvcOscPositions = __TRUE;  // Reset positions on next entry
		fnColdShutdown();
*/

//		ResetAllServoDrives();
//		DisableAllServoDrives();
//		dprintf("Heart Beat Reset\n");
		break;

	case GUI_CONNECTED:
		_isGuiConnected = 1;
		break;

	case DIGITAL_OUTPUT_TEST:
		nFirstByte += (int) nSecondByte<<8;
		//DigitalOutput_Test(nFirstByte);
		break;
	
	case ANALOG_OUTPUT_TEST:
//		AnalogOutput_Test(nFirstByte, nSecondByte);
		break;
 	
	case RF_TEST:
//		RF_Test();
		break;

	default:
//		ETHERNET_Print_Status(4, "fnWeldCommands:: Unknown command: %X\n", uCommand);
		break;
	}
}

//////////////////////////////////////////////////////////////////////
//
// void fnColdShutdown(void)
//
// Comments:
//       Set all IO lines to inactive
//       Set all DAC values to minimum values
//
//////////////////////////////////////////////////////////////////////
void fnColdShutdown(void)
{
	BYTE uServoIndex;
	SERVO_VALUES* ss;
	OSC_VALUES   *os;

//	ETHERNET_Print("In fnColdShutdown\n");

	ResetAllControlPorts(__TRUE);
//	ETHERNET_Print("fnColdShutdown::ResetAllControlPorts\n");
	SetGasValveOpen(WeldSystem.uPurgeOn);
	ZeroAllDacs();
	for (uServoIndex = 0; uServoIndex < mNumServos; ++uServoIndex) 
	{
		ss = &ServoStruct[uServoIndex]; // Save pointer to this servos struct
		if ((ss->uServoFunction == SF_OSC) || 
			(ss->uServoFunction == SF_JOG_ONLY_POS)) 
        {
			os = (OSC_VALUES *)ss;
			os->dwOwgDac   = OWG_MID_POINT;	// SET OSC WAVEFORM GENERATOR TO MID POINT 
			continue;
		}
		ss->wSeqDacValue = 0;
		QueCommandDacValue(ss);
		DriveExternalDacs(0, ss);
	}

	// Reset the Heart Beat variables
	nHB_CheckCount = 0;
	nHB_FailCount  = 0;

}

//////////////////////////////////////////////////////////////////////
//
// void fnWarmShutdown(void)
//
// Comments:
//       Set all IO lines to inactive
//       Set all DAC values to minimum values
//
//////////////////////////////////////////////////////////////////////
static void fnWarmShutdown(void)
{
	BYTE uServoIndex;
	SERVO_VALUES* ss;
	OSC_VALUES   *os;

//ETHERNET_Print("In fnWarmdown\n");

	ResetAllControlPorts(__FALSE);
	InitServoStructs(__FALSE); // This replaces ResetAllControlPorts();
	SetGasValveOpen(WeldSystem.uPurgeOn);
//	ZeroAllDacs();
	for (uServoIndex = 0; uServoIndex < mNumServos; ++uServoIndex) 
	{
		ss = &ServoStruct[uServoIndex]; // Save pointer to this servos struct
		switch(ss->uServoFunction) 
		{
			case SF_OSC:
			case SF_JOG_ONLY_POS:
				os = (OSC_VALUES *)ss;
				os->dwOwgDac   = OWG_MID_POINT;	// SET OSC WAVEFORM GENERATOR TO MID POINT 
				break;

			case SF_AVC:
			case SF_TRAVEL:
			case SF_WIREFEED:
			case SF_JOG_ONLY_VEL:
				if ((ss->uServoControl == SC_OPENLOOP) ||
					(ss->uServoControl == SC_ULPHEAD))
				{
					SetEnableBit(__TRUE, ss);
				}
				// FALLS THROUGH
			default:
				fnDisableServo(ss);
				ss->wSeqDacValue = 0;
				QueCommandDacValue(ss);
				DriveExternalDacs(0, ss);
				break;
		}
	}

	// Reset the Heart Beat variables
	nHB_CheckCount = 0;
	nHB_FailCount  = 0;
}

void ResetServoPositionStruct(SERVO_VALUES *ss, BOOL bUseDivisor)
{
	SERVO_POSITION_STRUCT *sps = &ServoPosition[ss->uServoSlot];

	sps->nCurrentEncoder = sps->nPreviousEncoder = ReadEncoderFeedback(ss->uServoSlot); // Reset Current & Previous encoder position
	sps->nDesiredPosition  = sps->nCurrentPosition;
	
	if (bUseDivisor)
	{
		sps->nDesiredPosition *= (int64_t)DESIRED_POS_DIVISOR;
	}

	sps->nPreviousPosition = sps->nCurrentPosition;
	sps->nActualVelocityCnt = 0;
	sps->nActualVelocitySum = 0;
    sps->nActualVelocitySumLast = 0;
//	printf("ResetServoPositionStruct::Reset nIndex = %d\n", sps->nServoIndex);
//	DumpData2(sps);
}

void EnableVelServo(SERVO_VALUES *ss, BYTE uEnableServo)
{
	if (mFeedbackDeviceType(ss->uServoIndex) == FEEDBACK_ENCODER)
	{
		if ((ss->uServoControl == SC_OPENLOOP) || (ss->uServoControl == SC_ULPHEAD)) 
		{
			switch (uEnableServo) 
			{
				case SERVOENABLE_READ:
					ResetServoPositionStruct(ss, __TRUE);
//    				printf("EnableVelServo::Reset\n");
					break;
					
				case SERVOENABLE_ENABLE:
					fnEnableServo(ss);
        			//RM SetServoCmdBit(ss->uServoSlot, SLOT_ENABLE, __TRUE);
//    				printf("EnableVelServo::Enabled\n");
					break; // uEnableServo
			}
		}
	}
}

void EnableAvcServo(SERVO_VALUES *ss, BYTE uEnableServo)
{
	if (ss->uServoControl == SC_ULPHEAD) 
	{
		switch (uEnableServo) 
		{
			case SERVOENABLE_READ:
                // Resets Position information
				ResetServoPositionStruct(ss, __TRUE);
//    			printf("EnableAvcServo::Reset\n");
				break;
				
			case SERVOENABLE_ENABLE:
//    			printf("EnableAvcServo::Enabled\n");
				break; // uEnableServo
		}
	}
}

void EnablePosServo(SERVO_VALUES* ss, BYTE uEnableServo)
{
	SERVO_POSITION_STRUCT *sps;
	OSC_VALUES   *os;
	long lDiff;	
	WORD wOscPos;

	os = (OSC_VALUES *)ss;

	switch(uEnableServo) 
	{
		case SERVOENABLE_READ:
			if (mFeedbackDeviceType(ss->uServoIndex) == FEEDBACK_ANALOG) 
			{
				wOscPos = GetOscPosition(os);
				lDiff = 4135 + (long)(2048 - ((int)wOscPos)) * 4;
				os->nOscCentering = (int)lDiff;
			}
			else 
			{
				sps = &ServoPosition[ss->uServoSlot];
				sps->nPreviousPosition = 0;
				sps->nDesiredPosition  = 0;
				sps->nCurrentPosition  = 0;
                sps->nActualVelocitySumLast = 0;
				GetZeroOscCentering(os, sps->nDesiredPosition);
			}
			break;  // switch (uEnableServo) 
			
		case SERVOENABLE_ENABLE:
			fnEnableServo(ss);
			SetServoCmdBit(ss->uServoSlot, SLOT_ENABLE, __TRUE);
			break;
	}
}

void PerformServoInit(BYTE uEnableServo)
{
	BYTE uServoIndex;
	SERVO_VALUES* ss;
	
	for (uServoIndex = 0; uServoIndex < mNumServos; ++uServoIndex) 
	{
		ss = &ServoStruct[uServoIndex]; // Save pointer to this servos struct
		switch(ss->uServoFunction) 
		{
			case SF_OSC:
			case SF_JOG_ONLY_POS:
				EnablePosServo(ss, uEnableServo);
				break;
			
			case SF_TRAVEL:
			case SF_WIREFEED:
			case SF_JOG_ONLY_VEL:
				EnableVelServo(ss, uEnableServo);
				break;
				
            case SF_AVC:
				EnableAvcServo(ss, uEnableServo);
				break;
				
            case SF_CURRENT:
        	default:
				break;

		}  // switch(ss->uServoFunction) 
	}
}
	
void CheckServoInit(void)
{
	if (_wServoInitDelay != 0)
    {
	    --_wServoInitDelay; // Delay enabling the OSC
	    
	    switch(_wServoInitDelay) 
        {
		    case 20:
			    PerformServoInit(SERVOENABLE_READ);   // Perform ADC->DAC setup only
			    break;

		    case 0:
			    PerformServoInit(SERVOENABLE_ENABLE); // Turn on servo
			    break;
	    }
    }
}

// Set Manip fault based on jog direction
static void SetManipFault(void) 
{
	if (uManipSpeed > 0)
		bManipCurrentLimitFault[uActiveManip] = FWD_DIRECTION;
	else
		bManipCurrentLimitFault[uActiveManip] = REV_DIRECTION;
}

// Returns __TRUE if Manip in Driving into a FAULT
BOOL ManipFaultInJogDirection(void) 
{
	if (bManipCurrentLimitFault[uActiveManip] == NO_FAULT)
		return __FALSE; // No Fault

	if ((uManipSpeed > 0) && (bManipCurrentLimitFault[uActiveManip] == FWD_DIRECTION))
		return __TRUE;

	if ((uManipSpeed < 0) && (bManipCurrentLimitFault[uActiveManip] == REV_DIRECTION))
		return __TRUE;

	return __FALSE;
}

void CheckManipFaults(void) 
{
	if (!_isManipEnabled)
		return;

	// Prevent driving further into the fault
	if (ManipFaultInJogDirection()) 
    {
//		dprintf("JogManip:: Fault in direction\n");
		ShutdownManip(uActiveManip);
		return;
	}

	if (bManipCurrentLimitFault[uActiveManip] != NO_FAULT) 
    {
//		dprintf("JogManip:: POS_DAC_SWING\n");
		SetManipCurrent(POS_DAC_SWING);       // Jog wide open until it becames unstuck
	}

	if (ReadManipFault()) 
    {
		++nManipFaultCount;
		if (nManipFaultCount > 20) 
        {
			if (bManipCurrentLimitFault[uActiveManip] == NO_FAULT) 
            {
				// New fault occured
//				dprintf("JogManip:: New Manip Fault\n");
				SetManipFault();
				ShutdownManip(uActiveManip);
			}
			else 
            {
				// Driving out from a fault
//				dprintf("JogManip:: Driving out Fault1\n");
			}
		}
		else 
        {
			// Driving out from a fault
//			dprintf("JogManip:: Driving out Fault2\n");
		}
	}
	else 
    {
		if (nManipFaultCount != 0) 
			--nManipFaultCount;
		if (nManipFaultCount == 0) 
        {
			bManipCurrentLimitFault[uActiveManip] = NO_FAULT;
			SetManipCurrent(mJogCurrent(uActiveManip)); // Jog normally   
		}
	}

	// Prevent jogging if there is an existing fault.
	if ((uManipSpeed > 0) && (bManipCurrentLimitFault[uActiveManip] == FWD_DIRECTION)) 
    {
//		dprintf("JogManip:: Manip shutdown FWD\n");
		ShutdownManip(uActiveManip);
		return; // Stop jogging in the jammed direction
	}

	if ((uManipSpeed < 0) && (bManipCurrentLimitFault[uActiveManip] == REV_DIRECTION)) 
    {
//		dprintf("JogManip:: Manip shutdown REV\n");
		ShutdownManip(uActiveManip);
		return; // Stop jogging in the jammed direction
	}
}



//////////////////////////////////////////////////////////////////////
//
// void fnHandler(void * pvParameters)
//
//////////////////////////////////////////////////////////////////////
void fnHandler(void)
{
	BYTE uServoIndex;

	//	portTickType xLastWakeTime;
	//	const portTickType xFrequency = 10;

	// printf("Start fnHandler thread\r\n");
	
	// Initialize the xLastWakeTime variable with the current time.
	// xLastWakeTime = xTaskGetTickCount();

	// vTaskDelayUntil(&xLastWakeTime, xFrequency);

	g_bForceBargraphUpdate = __FALSE;
	GetDigitalInput();
	//ReadFaults();
	ReadBarGraphData();
	// ReadAcMainsData();
	// HandlePendant();
	// fnProcessOscSteerCommands();
	DriveCommandBit(GAS_SOLE_ENA2, __TRUE);
	if (bWeldSequence)
	{
		WeldLoop();
	}

	GndFaultCheck();
	CheckServoInit();
	DelayedFanOn();
	CheckLampTimer();

	// Multiple Servo functions
	for (uServoIndex = 0; uServoIndex < mNumServos; ++uServoIndex)
	{
		ReturnToHome(uServoIndex);
		SetFunction(uServoIndex);
		PreWrapFunction(uServoIndex);
	}

	if (bOscManual && !bWeldSequence)
	{
		fnExecuteOscManual();
	}

	fnDumpAllDacs(); // output all DAC changes this interrupt.

	/*
	if (WeldSystem.uDataAqiOutEnable != 0)
	{
		if (_bUpdateDas && !bWeldSequence)
		{
			QueueElapsedTime();
		}
	}
	*/
}

void fnPendantManipJogs(BYTE uKey)
{
	JOG_STRUCT *pJogCommand;
	int nManipButton;
	int nManipButtonList[NUM_MANIP_JOG_BUTTONS] = { PB_MANIP_UP, PB_MANIP_DOWN, PB_MANIP_LEFT, PB_MANIP_RIGHT };
	IW_WELDHEADTYPEDEF *pStructWeldHeadDef = &stInitWeldSched.stWeldHeadTypeDef;
	IW_MANIPJOGARRAY *pManipButtonArray = &pStructWeldHeadDef->stManipJogArray[nMasterTravelDirection - 1];

	for (nManipButton = 0; nManipButton < NUM_MANIP_JOG_BUTTONS; ++nManipButton) 
    {
		if (nManipButtonList[nManipButton] == uKey) 
        {
			pJogCommand = &pManipButtonArray->stManipJogCommand[nManipButton];
			JogManip(pJogCommand);
			return;
		}
	}
}

/* 
void fnProcessExtOscSteer(void)
{
	IW_SERVODEF	 const *pServoDef;
	SERVO_VALUES *ss;
	BYTE uServo;
	int  nMoveIncrement;
//	int  nIsPosition;
//	static int nWasPosition;

	for (uServo = 0; uServo < mNumServos; ++uServo) {
		ss = &ServoStruct[uServo]; // Save pointer to this servos struct
		if ((ss->uServoFunction != SF_OSC) && 
			(ss->uServoFunction != SF_JOG_ONLY_POS)) 
			continue;

		pServoDef = &stInitWeldSched.stWeldHeadTypeDef.stServoDef[uServo];

		if (pServoDef != NULL) {

			if (pServoDef->uExtOscSteerEnable) {
				// Calc the distance turned
				nMoveIncrement	= ExtOscSteerAdc(ss);
				nMoveIncrement /= 16;

				if (abs(nMoveIncrement) != 0) {
//					printf("fnProcessExtOscSteer: nMoveIncrement: %d\n", nMoveIncrement);
					fnBumpOscCentering((OSC_VALUES *)ss, nMoveIncrement); // Use to force a "find" of the osc servo
				}
			}
		}
	}
}
*/

/*
int GetEncoderMove(void)
{
	int nMoveIncrement;
	int nCorrectedMovement;

	// Calc the distance turned
	uSteeringIsPosition	= PendGetPendantEncoder();
	nMoveIncrement	= uSteeringIsPosition - uOscEncoderWasPosition;
	uOscEncoderWasPosition	= uSteeringIsPosition;

	if (abs(nMoveIncrement) > (int)0x7F) { 
		// Roll Over 0->65535 or 65535->0
		nCorrectedMovement = (int)0x100 - abs(nMoveIncrement);
		if (nMoveIncrement > 0)
			nCorrectedMovement *= (-1);
		nMoveIncrement = nCorrectedMovement;
	}

	if (nMoveIncrement != 0)
	{
//		dprintf("GetEncoderMove: Move: %02d Current: %05d\n", nMoveIncrement, nSteeringIsPosition);
	}
		
	return nMoveIncrement;
}

void ResetSteeringPosition(void)
{
	wSteeringWasPosition = wSteeringIsPosition;
}

int fnGetSteerMovement(void)
{
	int nMoveIncrement;
	int nCorrectedMovement;

	// Calc the distance turned
//	wSteeringIsPosition = GetEncoderData();
	nMoveIncrement = (int)(wSteeringIsPosition - wSteeringWasPosition);
	wSteeringWasPosition = wSteeringIsPosition;

	if (abs(nMoveIncrement) > (int)0x7FFF) 
	{
		// Roll Over 0->65535 or 65535->0
		nCorrectedMovement = (int)0x10000 - abs(nMoveIncrement);
		if (nMoveIncrement > 0)
			nCorrectedMovement *= (-1);
		nMoveIncrement = nCorrectedMovement;
	}

	return nMoveIncrement;
}


void fnProcessOscSteerEncoder(void)
{
	SERVO_VALUES *ss;
	BYTE uServo;
	int  nMoveIncrement;

	nMoveIncrement = fnGetSteerMovement();
	for (uServo = 0; uServo < mNumServos; ++uServo) 
	{
		ss = &ServoStruct[uServo]; // Save pointer to this servos struct
		
		if ((ss->uServoFunction == SF_OSC) || 
			(ss->uServoFunction == SF_JOG_ONLY_POS))
		{
			if ((ss->bPendantControl) && (nMoveIncrement != 0))
			{
				if (!bWeldSequence) 
				{
					// Only increase steering is not in-sequence
					if (abs(nMoveIncrement) > 50)
					{
						nMoveIncrement *= nOscSpeedSteer;
					}
				}

				fnBumpOscCentering((OSC_VALUES *)ss, nMoveIncrement); // Used to force a "find" of the osc servo
				QueueEventMessage(ss->uServoIndex, EV_OSCSTEERCHANGE);
//              printf("EV_OSCSTEERCHANGE\n");
			}
		}
	}
}

void fnProcessOscSteerCommands(void)
{
	fnProcessOscSteerEncoder();
//	fnProcessExtOscSteer();
}
*/

static BOOL HandleServoFault(BYTE uFaultMsg)
{
	BOOL bProcessed   = __FALSE;
	BYTE uFaultAction = SA_WARNING_ONLY;

	if ((uFaultMsg & SERVO_TYPE_FAULT) != 0)
	{
//		printf("HandleServoFault: 0x%02X\n", uFaultMsg);
		bProcessed = __TRUE;

		if (ActiveServoFault(uFaultMsg, &uFaultAction)) 
		{
			if ((uFaultMsg & FAULT_SET) != 0) 
			{
				if (bWeldSequence)
				{
					switch(uFaultAction) 
					{
						case SA_WARNING_ONLY:
//							printf("HandleServoFault: Servo Warning type fault: 0x%02X\n", uFaultMsg);
							break;
							
						case SA_DOWNSLOPE:
							fnInitDownSlope();
							uStoppedOnFault = __TRUE;
//							printf("HandleServoFault: Servo Downslope type fault: 0x%02X\n", uFaultMsg);
							break;
								
						case SA_ALLSTOP:
							fnInitPostPurge();
							uStoppedOnFault = __TRUE;
//							printf("HandleServoFault: Servo All Stop type fault: 0x%02X\n", uFaultMsg);
							break;
					}
				}
			}
		}
	}

	return bProcessed; 
}

//////////////////////////////////////////////////////////////////////
//
// void QueueFaultMessage(BYTE uFaultMsg)
//
//////////////////////////////////////////////////////////////////////
void QueueFaultMessage(BYTE uFaultMsg)
{
	EvalFaultMessage(uFaultMsg);

	// Get Out if uLockFaults is set and it's a clear fault
	if (!uLockFaults || ((uFaultMsg & 0x01) != 0))
	{
//		printf("QueueFaultMessage:: Fault: 0x%02X\n", uFaultMsg);
		QueueMessage(&MessageQueue.stFaultData, uFaultMsg);
	}
}

void EvalFaultMessage(BYTE uFaultMsg)
{
	if (HandleServoFault(uFaultMsg))
	{
		return; // ServoFault has been processed
	}
	
	switch(uFaultMsg) 
	{
	// These faults cause a downslope
	case INPUT_AC_SET:
	case PS_TEMP_SET:
		if (bWeldSequence) 
		{
			fnInitDownSlope();
			uStoppedOnFault = __TRUE;
//			printf("EvalFaultMessage: Downslope type fault: 0x%02X\n", uFaultMsg);
		}
		break;
		
	case GROUND_FLT_SET:
		uStoppedOnFault = __TRUE;
		QueueEventMessage(SYSTEM_EVENT, EV_STOPPED_ON_FAULT);
		break;

	// These faults cause an All Stop
	case STUBOUT_FLT_SET:
	case HI_ARC_VOLT_SET:
	case ARC_GAS_FAULT_SET:
		if (bWeldSequence) 
		{
			fnInitPostPurge();
			uStoppedOnFault = __TRUE;
//			printf("EvalFaultMessage: ALL STOP type fault: 0x%02X\n", uFaultMsg);
		}
		break;

	// These faults cause no sequence changes
	case GROUND_FLT_CLR:
		if (bWeldSequence) 
		{
//			printf("EvalFaultMessage: NO ACTION type fault: 0x%02X\n", uFaultMsg);
		}
		break;
		
	// These faults cause a user defined action
	case USER_FLT1_SET:
	case USER_FLT1_CLR:
	case USER_FLT2_SET:
	case USER_FLT2_CLR:
		if (bWeldSequence) 
		{
//			printf("EvalFaultMessage: USER DEFINED type fault: %d\n", uFaultMsg);
		}
		break;
		
	// External Sequence Start Line
	case EXT_SEQ_START:
		if (!bWeldSequence) 
		{
			QueuePendantKey(PB_SEQ_START);
//          printf("EvalFaultMessage: External Seq Start\n");
		}
		break;

	// External Sequence Stop Line
	case EXT_SEQ_STOP:
		QueuePendantKey(PB_SEQ_STOP);
//      printf("EvalFaultMessage: External Seq Stop\n");
		break;																			 
	
	default:
//      printf("EvalFaultMessage: Unknown Fault: 0x%02X\n", uFaultMsg);
		break;
	}
}

void SendServoPosition(BOOL bForceSend)
{
	SERVO_VALUES* ss;
	BYTE uSlot;
	SERVO_POSITION_STRUCT *sps;
	BYTE uServoIndex;
	int  nMoveIncrement;
	
	for (uServoIndex = 1; uServoIndex < mNumServos; ++uServoIndex) 
	{
		ss = &ServoStruct[uServoIndex]; // Save pointer to this servos struct
		uSlot = ss->uServoSlot;
		sps = &ServoPosition[uSlot];

		if (FEEDBACK_ENCODER == sps->nEncoderType) 
        {
			nMoveIncrement = abs(sps->nLastPositionSent - sps->nCurrentPosition);
			if (bForceSend || (nMoveIncrement > sps->nTolerance)) 
			{
//				dprintf("SendServoPosition: nMoveIncrement: %d\n", nMoveIncrement);
				if (uSlot == mPositionServoSlot) 
				{
//					QueueHeadPosition((DWORD)sps->nCurrentPosition);
//					dprintf("SendServoPosition: QueueHeadPosition: %d\n", sps->nCurrentPosition);
				}
//				QueueServoPosition(uServoIndex, (DWORD)sps->nCurrentPosition);
//				dprintf("SendServoPosition:: ServoIndex: %d QueueServoPosition: %d\n", uServoIndex,  sps->nCurrentPosition);
				sps->nLastPositionSent = sps->nCurrentPosition;
			}
		}
	}
}


void GetZeroOscCentering(OSC_VALUES *ss, int64_t nDesiredPosition)
{
	WORD  wHalfOscAmp;
	WORD  wOwgMagnitude;
	DWORD dwOscTempProduct;
	int   nOscPartial;

	wHalfOscAmp      = ss->wOscAmp / 2;
	wOwgMagnitude    = (WORD)(ss->dwOwgDac >> 16);
	dwOscTempProduct = (DWORD)ss->wOscAmp * wOwgMagnitude;
    nOscPartial      = (int)((dwOscTempProduct / 1000) & 0x0000FFFF);

//	ETHERNET_Print("GetZeroOscCentering: nOscCentering: %d\n", ss->nOscCentering);
	ss->nOscCentering = (int64_t)(nDesiredPosition + wHalfOscAmp) - nOscPartial; // Calculate centering needed
//	ETHERNET_Print("GetZeroOscCentering: nOscCentering: %d\n", ss->nOscCentering);
//	fnCommandOscServo(ss);
}

static void SetPositionEncoderValue(const SET_POSITION* pPosInfo)
{
	SERVO_POSITION_STRUCT *sps;
	SERVO_VALUES* ss;
	BYTE uSlot;
	int nServoIndex;

	uSlot = pPosInfo->uServoSlot;
	sps = &ServoPosition[uSlot];
	nServoIndex = LookupServoIndex(uSlot);

	if (nServoIndex != (-1))
	{
		ss  = &ServoStruct[nServoIndex]; // Save pointer to this servos struct
		sps->nCurrentPosition = (int64_t)pPosInfo->nPosition;

		// Make Desired & Current the same
		sps->nCurrentEncoder = sps->nPreviousEncoder = ReadEncoderFeedback(uSlot); // Reset Current & Previous encoder position
		sps->nPreviousPosition = sps->nCurrentPosition;
		sps->nDesiredPosition = sps->nCurrentPosition;

		// Reset OSC_CENTERING if needed
		if ((ss->uServoFunction == SF_OSC) || 
			(ss->uServoFunction == SF_JOG_ONLY_POS)) 
		{
			GetZeroOscCentering((OSC_VALUES *)ss, sps->nCurrentPosition);
		}
		else
		{
			sps->nDesiredPosition *= (int64_t)DESIRED_POS_DIVISOR;
		}

		if (uSlot == mPositionServoSlot) 
		{
			WeldSystem.nLevelPosition = sps->nCurrentPosition;
		}

//		ETHERNET_Print("SetPositionEncoderValue:: Value: %d Slot: %d\n", sps->nCurrentPosition, uSlot);
		SendServoPosition(__TRUE);
	}
}

