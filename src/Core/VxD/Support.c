//
// support.c
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "Task.h"

// Local include files
#include "string.h"
#include "typedefs.h"
#include "ComLit.h"
#include "pkeymap.h"
#include "InitWeldStruct.h"

// Local include files
#include "Weldloop.h"
#include "ServoCom.h"
#include "util.h"
#include "FaultList.h" // Definitions for Fault Messages

#include "GPIO.h"
#include "CAN.h"
#include "ADC.h"
#include "DAC.h"
#include "SPI.h"
#include "ServoMove.h"

#include "Support.h"

////////////////////////
// P R O T O T Y P E S
extern void UpdateGraphInfo(GRAPH_INFO* pGraphInfo, WORD wCommandData, WORD wFeedbackData);
extern BOOL AvcAdjustAllowed(const SERVO_VALUES* ss);
extern void ResetSteeringPosition(void);
extern void SendFeedbackUpdate(GRAPH_INFO* pGraphInfo, BOOL bForceSend);
extern BOOL fnIsForwardDirection(const SERVO_VALUES* ss);

//#define __TEST_DATA__

//////////////////////////////////////////////////////////////////////
//
// L O C A L  V A R I A B L E S
//
//////////////////////////////////////////////////////////////////////
static const char *pAmi = "ARC MACHINES INC";
static int  nPendDispCharCnt; // Number of Pending Characters
static BYTE uPendDispBuff[17]; // 16 + 1 Bytes
static BYTE uPendBitmapBuff[132];

//RM static const BYTE uSwitchMaskList[4] = {LIMIT_SWITCH1, LIMIT_SWITCH2, CAN_BUS_FAIL, TECHNOSOFT_FAULT};
//////////////////////////////////////////////////////////////////////
//
// E X T E R N A L   V A R I A B L E S
//
//////////////////////////////////////////////////////////////////////
extern SERVO_POSITION_STRUCT ServoPosition[MAX_NUMBER_SERVOS]; 	// in Weldloop.c  // SPS is Servo Slot based
extern BOOL   bWeldSequence; 		// in Weldloop.c
extern BOOL   bStubOutEnable; 		// in Weldloop.c
extern int    nUlpVoltFaultState;   // in Weldloop.c
extern BOOL	  bHighArcEnable; 		// in Weldloop.c
extern BYTE	  uGasFaultEnable;  	// in Weldloop.c
extern BOOL	  bPriCoolantEnable;	// in Weldloop.c
extern BOOL	  bAuxCoolantEnable;	// in Weldloop.c
extern BYTE   bCheckGroundFault;	// in Weldloop.c
extern BYTE	  uNumGraphPorts;		// in Weldloop.c
extern BYTE	  uSystemWeldPhase;
extern DWORD  dwVxdFlags;
extern BYTE	  uLockFaults;
extern int	  PendantKeyServoMap[SF_COUNT];

extern void InitializeWeldParameters(void);	    // in Weldloop.c
extern void fnColdShutdown(void);				// in Weldloop.c
extern void QueueFaultMessage(BYTE uFaultMsg);
extern void JogServo(LPC_JOG_STRUCT pJogStruct);// in Weldloop.c
//extern void fnPendantManipJogs(BYTE uKey);

// These are defined in w32intf.c
//extern MESSAGE_QUEUE    MessageQueue;
//extern BOOL			_isAvcCardPresent;
//extern BYTE			_isGuiConnected;
//BOOL _ioTest = __FALSE;
extern BYTE	_isGuiConnected; // W32intf.c

//////////////////////////////////////////////////////////////////////
//
// G L O B A L   V A R I A B L E S  as defined in this source module
//
//////////////////////////////////////////////////////////////////////
PD_PENDANTINFO	_stPendantInfo = { {"  !!NO WAIT!!   ",
									"THIS IS ALL CAPS",
									"THIS IS TOO CAPS",
									"  HERE'S SPACE  ",
									"HERE'S SOME MORE",
									"IF YOU SCROLL   ",
									"THROUGH THIS    ",
									"WITH THE KEY    ",
									"HELD DOWN       ",
									"YOU MAY FIND    ",
									"THAT IT WILL    ",
									"SCROLL FOR YOU  ",
									"WHAT A BARGIN!  ",
									"IS THAT DLLABLE?",
									"LET'S MALLOCATE ",
									"DON'T PRESS THAT" },

								 {	"0123456789ABCDEF",
									"123456789ABCDEF0",
									"23456789ABCDEF01",
									"3456789ABCDEF012",
									"456789ABCDEF0123",
									"56789ABCDEF01234",
									"6789ABCDEF012345",
									"789ABCDEF0123456",
									"89ABCDEF01234567",
									"9ABCDEF012345678",
									"ABCDEF0123456789",
									"BCDEF0123456789A",
									"CDEF0123456789AB",
									"DEF0123456789ABC",
									"EF0123456789ABCD",
									"F0123456789ABCDE" } };
BYTE _uAuxLineFunction[4];

IW_INITWELD	stInitWeldSched;	

BYTE uPendantScrollMode = PENDANT_SCROLL_STOP;
BYTE uPendantSelectionStr[20];
GRAPH_INFO GraphInfo[MAX_GRAPH_PORTS];

BYTE  bManipCurrentLimitFault[MAX_NUMBER_OF_MANIPS];
BOOL  _isManipEnabled;
BYTE  uActiveManip;

SERVO_VALUES* pMasterAvcServo = NULL;
LPOSC_VALUES   pMasterOscServo = NULL;

SERVO_VALUES	      ServoStruct[MAX_NUMBER_SERVOS];
SERVO_POSITION_STRUCT ServoPosition[MAX_NUMBER_SERVOS];  // SPS is Servo Slot based
CAN_SERVOFDBK		  ServoFeedback[MAX_NUMBER_SERVOS];
CAN_SERVOCMD		  ServoCommand[MAX_NUMBER_SERVOS];

SYSTEM_STRUCT	      WeldSystem;
SERVO_CONST_STRUCT    ServoConstant[MAX_NUMBER_SERVOS];

BOOL bStubOutEnable    = __TRUE;
BOOL bHighArcEnable    = __TRUE;
int  nUlpVoltFaultState = ULP_VOLTNOTUSED;
BYTE uGasFaultEnable   = GAS_HILOFAULTENABLE;
BOOL bPriCoolantEnable = __TRUE;
BOOL bAuxCoolantEnable = __TRUE;

BOOL bWeldSequence = __FALSE;
BYTE uSystemWeldPhase = WP_EXIT + 1;	// Default to illegal
BYTE uLockFaults = __FALSE;
BYTE uNumGraphPorts = 0;
WORD wCurrentFeedback[8];
int	 PendantKeyServoMap[SF_COUNT];

int	nHB_CheckCount 		 = 0;
int	nHB_FailCount  		 = 0;
int nHomeDirection    	 = 1; // 1 = forward, (-1) = reverse
int nHomeDuringPostpurge = 0;
BYTE bCheckGroundFault		= __FALSE;
BOOL bResetAvcOscPositions  = __TRUE;

DWORD dwElapsedTime;
DWORD dwLampTimer;
BOOL  _isAvcCardPresent;
BYTE  uPendantScrollIndex = 0;
int   nFanOnDelay = 2000;
int   nLampTimer = 0;
int   nBoosterVoltage = 180;

int	nMasterTvlSlot;
int nMasterTravelDirection = LIT_TVL_CCW;

ACMAIN_DATA stAcMainsData;
WORD _wServoInitDelay = 0;  // Delay enableing the OSC
BOOL _bUpdateDas = __FALSE;

BOOL g_bForceBargraphUpdate = __FALSE;

//////////////////////////////////////////////////////////////////////
//
// L O C A L   V A R I A B L E S
//
//////////////////////////////////////////////////////////////////////
DAC_CAL_STRUCT DacCalMap[MAX_NUMBER_SERVOS];

static BOOL bGasValveIsOpen    	= __FALSE;
static BOOL bGasFlowFault      	= __FALSE;
static WORD wGasFlowDelay;

static BOOL isTempFault     	= __FALSE; 
static BOOL isTempFaultLast		= __FALSE; 

// Ramping AVC Response values
static WORD _wAvcResponseActual;
static WORD _wAvcResponseDesired;


//////////////////////////////////////////////////////////////////////
//
// L O C A L   P R O T O T Y P E S
//
//////////////////////////////////////////////////////////////////////
static void SelectWirefeeder(void);
static void SendBargraphUpdate(GRAPH_INFO *pGraphInfo, BOOL bForceSend);
static BOOL IsFeedbackWithoutCommand(GRAPH_INFO *pGraphInfo);
static void SendFeedbackWithoutCommand(GRAPH_INFO *pGraphInfo, WORD wNewFeedback);
static void SetAverageFeedback(GRAPH_INFO *pGraphInfo, WORD wNewFeedback);
static void CaptureDacCommands(WORD wCommand, const SERVO_VALUES* ss);
uint32_t IO_DigitalOutput = 0;



//========================================================================
//
// Begin Code
//

static void SetServoFeedbackBits(BYTE uServoSlot, BYTE uState)
{
//	ASSERT(uServoSlot < MAX_NUMBER_SERVOS);
//	if (uServoSlot < MAX_NUMBER_SERVOS)
//	{
//        CAN_SERVOCMD* pServoCommand = &ServoCommand[uServoSlot];
//
//        pServoCommand->wModeCommand &= USE_CLEARMASK;
//		pServoCommand->wModeCommand |= uState;
//	}
}

BOOL SetServoCmdBit(BYTE uServoSlot, WORD wMask, BOOL bState)
{
	BOOL bStatus = __FALSE;

	ASSERT(uServoSlot < MAX_NUMBER_SERVOS);
	if (uServoSlot < MAX_NUMBER_SERVOS)
	{
        CAN_SERVOCMD* pServoCommand = &ServoCommand[uServoSlot];

		WORD wModeCommand = pServoCommand->wModeCommand;

		if (bState)
			pServoCommand->wModeCommand |= wMask;
		else
			pServoCommand->wModeCommand &= ~wMask;

		if(wModeCommand != pServoCommand->wModeCommand)
		{
		   	switch(wMask)
			{
				case SLOT_ENABLE:
					if(bState){
						TechSendCMD(uServoSlot, CMDAxison);
					}
					else
					{
						TechSendCMD(uServoSlot, CMDAxisoff);
					}
					break;

				case SLOT_AVC_JOG:
					if(bState)
					{
				    	TechWriteCommand(uServoSlot, SetAvcJog, 0);
					}
					else
					{
				    	TechWriteCommand(uServoSlot, SetAvcJog, 1);
					}
					break;

		        default:
		            break;
			}
		}
	}

	return bStatus;
}

static BOOL GetServoCmdBit(BYTE uServoSlot, WORD wMask)
{
	ASSERT(uServoSlot < MAX_NUMBER_SERVOS);
	if (uServoSlot < MAX_NUMBER_SERVOS)
    {
		return (BOOL)(ServoCommand[uServoSlot].wModeCommand & wMask);
    }

	return __FALSE; // Should not hit here
}

static BOOL ServoSwitchActive(BYTE uSlot, WORD wMask)
{
	WORD wGetBit;
	WORD wCurrentServoDI;

	wCurrentServoDI = TechReadFeedback(uSlot, GET_LIMITSW);  //ServoFeedback[uSlot].wFaults
	if (uSlot < MAX_NUMBER_SERVOS)
	{
		switch(wMask)
		{
			case LIMIT_SWITCH1:
				wGetBit = SERVO_SWITCH1;
				break;

			case LIMIT_SWITCH2:
				wGetBit = SERVO_SWITCH2;
				break;

/*
			case CAN_BUS_FAIL:
				wGetBit = SERVO_FAULT;
				break;

			case TECHN_FAULT:
				wGetBit = TECHN_FAULT;
				break;

			default:
				wGetBit = 0x0000;
				break;
*/
		}

		if((wCurrentServoDI & SERVO_IS_ENABLE) == SERVO_IS_ENABLE)
		{
			if ((wCurrentServoDI & wGetBit) == 0)
			{
				// All servo switches are active low
				return __TRUE;
			}
		}
	}

	return __FALSE;
}												

void DriveCommandBit(DWORD dwAddressData, BOOL bState)
{
	uint32_t wBitData  = 0;

	switch(dwAddressData)
	{
		case ILLUM:
			wBitData = 0x00000001;
			break;
		case GAS_SOLE_ENA1:
			wBitData = 0x00000002;
			break;
		case WIRE_SELECT:
			wBitData = 0x00000004;
			break;
		case WHEAD2_SEL:
			wBitData = 0x00000008;
			break;
		case CURRENT_ENA:
			wBitData = 0x00000010;
			break;
		case IN_SEQ:
			wBitData = 0x00000020;
			break;
		case AUX_OUT1:
			wBitData = 0x00000040;
			break;
		case AUX_OUT2:
			wBitData = 0x00000080;
			break;
		case RF_ENABLE:
			wBitData = 0x00000100;
			break;
		case BOOST_ENA:
			wBitData = 0x00000200;
			break;
		case BLEED_DISABLE:
			wBitData = 0x00000400;
			break;
		case EMERG_STOP_OUT:
			wBitData = 0x00000800;
			break;
		case FAN12:
			wBitData = 0x00001000;
			break;
		case FAN56:
			wBitData = 0x00002000;
			break;
//		case FAN34:
//			wBitData = 0x00004000;
//			break;
		case GNDFLT_RELAY:
			wBitData = 0x00008000;
			break;
		case M1MODE1:
			wBitData = 0x00010000;
			break;
		case MANIP_AD0:
			wBitData = 0x00020000;
			break;
		case MANIP_AD1:
			wBitData = 0x00040000;
			break;
		case MANIP_DIR:
			wBitData = 0x00080000;
			break;
		case MANIP_ENABLE:
			wBitData = 0x00200000;
			break;
		case GAS_SOLE_ENA2:
			wBitData = 0x00400000;
			break;
		case TRAVEL_ENOUT:
			wBitData = 0x00800000;
			break;
		case WIRE_ENOUT:
			wBitData = 0x01000000;
			break;
		case M1SLEEP:
			wBitData = 0x02000000;
			break;
		case M1MODE2:
			wBitData = 0x04000000;
			break;
	}

	if (bState)
		IO_DigitalOutput |= wBitData;
	else
		IO_DigitalOutput &= ~wBitData;

	IO_DigitalUpdate(IO_DigitalOutput);
}

static void ClearDacCalTable(void)
{
	BYTE ix;
	DAC_CAL_STRUCT nZero = {0, OG_SCALER, 0, OG_SCALER};

	for (ix = 0; ix < MAX_NUMBER_SERVOS; ++ix) 
	{
		DacCalMap[ix] = nZero;
	}
}


//////////////////////////////////////////////////////////////////////
//
// WORD InFromPort(WORD wPort)
//
//////////////////////////////////////////////////////////////////////
WORD InFromPort(WORD wPort)
{
	// Need to get the following feedback
	// Keycode
	// Encoder
	//GetEncoderKeyCode();
	return 0x0000;
}

//////////////////////////////////////////////////////////////////////
//
// BOOL GasFlowing(void)
//
// Comments:
//       Returns __TRUE if gas is flowing within limits
//////////////////////////////////////////////////////////////////////
BOOL GasFlowing(void)
{
	BOOL isGasFlowing = __TRUE;//__FALSE;
// if(GetCurrentState(GAS_FLOW_SW)
//   isGasFlowing = __TRUE;
//   if (!bCheckGroundFault)
//   {
//      // Do not check gas while checking ground fault
//      switch (uGasFaultEnable)
//      {
//         case GAS_FAULTDISABLE:
//            break;
//
//         case GAS_HIFAULTENABLE:
//         case GAS_LOFAULTENABLE:
//         case GAS_HILOFAULTENABLE:
//         default:
//		 	// GAS_FLOW is active low; 0 == FLOWING
////            isGasFlowing = (BOOL)(GPIO_ReadBit(GPIO2, GAS_FLOW) == 0);
//            break;
//      }
//   }
   
   return isGasFlowing;
}

//////////////////////////////////////////////////////////////////////
//
// void ZeroAllDacs(void)
//
//////////////////////////////////////////////////////////////////////
void ZeroAllDacs(void)
{
    CAN_SERVOCMD* pServoCommand;
	BYTE uServoSlot;

//	WriteDacData(PA_EXTERNAL_TVL_CMD,	DAC_VOLT_OFFSET);
//	WriteDacData(PA_EXTERNAL_WIRE_CMD,	DAC_VOLT_OFFSET);
//	WriteDacData(PA_AMP_COMMAND,		DAC_VOLT_OFFSET);

	for (uServoSlot = 0; uServoSlot < MAX_NUMBER_SERVOS; ++uServoSlot)
	{
        pServoCommand = &ServoCommand[uServoSlot];

        pServoCommand->uServoSlot 		 = uServoSlot;
		pServoCommand->nCommandPort 	 = 0;
		pServoCommand->nCurrentLimitPort = 0;
		pServoCommand->nGainPort 		 = 0;
		pServoCommand->nResponsePort 	 = 0;
	}
}

//////////////////////////////////////////////////////////////////////
//
// BOOL HomeSwitchShut(int nSlot)
//
//////////////////////////////////////////////////////////////////////
BOOL HomeSwitchShut(int nSlot)
{
	return ServoSwitchActive((BYTE)nSlot, LIMIT_SWITCH1);
}

//////////////////////////////////////////////////////////////////////
//
// void SetForwardDirectionBit(BOOL bState, SERVO_VALUES* ss)
//
// Comments:
//       Output the "Direction Bit" for the servo in nSlot
//       bState = __TRUE = Forward
//       nSlot  = Slot number of the servo to be controlled
//
//////////////////////////////////////////////////////////////////////
void SetForwardDirectionBit(BOOL bState, const SERVO_VALUES* ss)
{
	if (ss->uServoSlot == 0)
		return;  // NO DIRECTION BOOL for the Current Servo

	if (mDirectionReversed(ss->uServoIndex))
		bState = (BOOL)!bState;

	SetServoCmdBit(ss->uServoSlot, SLOT_FWD_DIRECTION, (BOOL)bState);

	if (ss->bIsJogging)
		return; // Do not change bHeadDirectionFwd set direction for a JOG

	if (ss->uServoSlot == mPositionServoSlot)
		WeldSystem.bHeadDirectionFwd = bState;
}

//////////////////////////////////////////////////////////////////////
//
// BOOL GetDirectionBit(BYTE uServoSlot)
//
//////////////////////////////////////////////////////////////////////
BOOL GetDirectionBit(BYTE uServoSlot)
{
	if (uServoSlot == 0)
		return __FALSE;  // NO DIRECTION BOOL for the Current Servo, should never hit here

	return GetServoCmdBit(uServoSlot, SLOT_FWD_DIRECTION);
}

//////////////////////////////////////////////////////////////////////
//
// BOOL GetForwardDirectionBit(const SERVO_VALUES* ss)
// DO NOT USE THIS FUNCTION any further.
// It's only use in in generating the Extrnal DAC commands, and must maintain prevous operations
// Use GetDirectionBit(BYTE uServoSlot), which is correct regradless of Reverse Motor Direction state

//////////////////////////////////////////////////////////////////////
BOOL GetForwardDirectionBit(const SERVO_VALUES* ss)
{
	BOOL bForwardDirection;

	bForwardDirection = GetDirectionBit(ss->uServoSlot);

	if (mDirectionReversed(ss->uServoIndex))
		bForwardDirection = (BOOL)!bForwardDirection;

	return bForwardDirection;
}

void DriveExternalDacs(WORD wValue, const SERVO_VALUES* ss)
{
	BYTE uBitTest = 1;
	WORD wOutputPort;
	WORD wOutputValue = 0;

	// Start with Bit 0, and shift left to cover all External Dacs 
	while (uBitTest < 3) {
		wOutputPort = 0;
		switch(ss->uExternalDac & uBitTest) {
//			case 1: wOutputPort = PA_EXTERNAL_TVL_CMD;		break;
//			case 2: wOutputPort = PA_EXTERNAL_WIRE_CMD;		break;
			case 0:
			default:
				// Dac not selected.....do nothing
				break;
		}

		if (wOutputPort) {
			if (GetForwardDirectionBit(ss)) {
				wOutputValue = (WORD)(DAC_VOLT_OFFSET - (int)wValue); // Output Negative Values for FORWARD direction
			}
			else {
				wOutputValue = DAC_VOLT_OFFSET + wValue; // Output Positive Values for REVERSE direction
			}
//			printf("DriveExternalDacs:: Port: 0x%04X Value: 0x%04X\n", wOutputPort, wOutputValue);
			WriteDacData(wOutputPort, wOutputValue);
		}

		uBitTest = (BYTE)(uBitTest << 1);
	}
}

void JogExternalDacs(short nValue, const SERVO_VALUES* ss)
{
	BYTE uBitTest = 1;
	WORD wOutputPort;
	WORD wOutputValue = 0;

	// Start with Bit 0, and shift left to cover all External Dacs 
	while (uBitTest < 3) {
		wOutputPort = 0;
		switch(ss->uExternalDac & uBitTest) {
//			case 1: wOutputPort = PA_EXTERNAL_TVL_CMD;		break;
//			case 2: wOutputPort = PA_EXTERNAL_WIRE_CMD;		break;
			case 0:
			default:
				// Dac not selected.....do nothing
				break;
		}

		if (wOutputPort) {
			wOutputValue = DAC_VOLT_OFFSET + nValue;
//			printf("JogExternalDacs::Port: 0x%04X Value: 0x%04X\n", wOutputPort, wOutputValue);
			WriteDacData(wOutputPort, wOutputValue);
		}

		uBitTest = (BYTE)(uBitTest << 1);
	}
}

BOOL IsCommandValid(SERVO_VALUES const *ss)
{
	BOOL bCommandIsValid = __TRUE;

	switch(ss->uServoFunction)
	{
		case SF_CURRENT:
			if (!WeldSystem.uWeldMode || 
				!WeldSystem.bArcEstablished)
			{
				bCommandIsValid = __FALSE;
			}
			break;
			
		case SF_WIREFEED:
		case SF_TRAVEL:    
			if (!ss->bEnabledFlag)
			{
				bCommandIsValid = __FALSE;
			}
			break;
			
		case SF_AVC:
			if (!WeldSystem.uWeldMode		|| 
				!WeldSystem.bArcEstablished ||
				!ss->bEnabledFlag           ||
				(ss->wEnableDelay != 0))
			{
				bCommandIsValid = __FALSE;
			}
			break;
			
		case SF_OSC:
		case SF_JOG_ONLY_POS:
		default:
			// No action required
			break;
	}

	return bCommandIsValid;
}

void CommandExternalDacs(WORD wValue, const SERVO_VALUES* ss)
{
	if (!ss->bIsJogging) {
		DriveExternalDacs(wValue, ss);
	}
}

//////////////////////////////////////////////////////////////////////
//
// void AdjustCmdDacCommand(short* pValue, const SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
void AdjustCmdDacCommand(short* pValue, const SERVO_VALUES* ss)
{
	int ix = ss->uServoIndex;

	if (ix < MAX_NUMBER_SERVOS)
	{
		int nValue = *pValue;
		DAC_CAL_STRUCT *pDacCalMap = &DacCalMap[ix];

		nValue	= (int)nValue + ((int)nValue * ((int)pDacCalMap->nCmdGain - OG_SCALER)) / OG_SCALER;
		nValue += pDacCalMap->nCmdOffset;

		if(ss->uServoFunction == SF_OSC)
		{
			if (nValue > 8000)
				nValue = 8000;
			if (nValue < 0)
				nValue = 0;

		}
		else
		{
			if (nValue > 4000)
				nValue = 4000;
			if (nValue < -4000)
				nValue = -4000;
		}

		*pValue = nValue;
	}
}

//////////////////////////////////////////////////////////////////////
//
// void SetCommandDacValue(short nValue, BYTE uSlot)
//
//////////////////////////////////////////////////////////////////////
void SetCommandDacValue(short nValue, const SERVO_VALUES* ss)
{
	CAN_SERVOCMD* pServoCommand = &ServoCommand[ss->uServoSlot];
	CaptureDacCommands(nValue, ss);  // Used for Data Stream

	AdjustCmdDacCommand(&nValue, ss);
	switch(ss->uServoFunction) 
	{	 
		case SF_CURRENT: 
			nValue >>= 2; // Convert 0 to 4000 outputs to 0 to 2000 Dac Values
						  //0x0000 = -10 Volts, DAC_VOLT_OFFSET = 0x800 = 0V, 0x0FFF = +10 Volts
			nValue += DAC_VOLT_OFFSET;
			nValue &= 0x0FFF; // Prevent Overrun
			//WriteDacData(PA_AMP_COMMAND, (WORD)nValue);
			if(pServoCommand->nCommandPort != nValue)
			{
				pServoCommand->nCommandPort = nValue;
				//ServoCmd(pServoCommand);
				WriteDacData(PA_AMP_COMMAND, nValue);
			}
			break;

		case SF_TRAVEL:
			if(!fnIsForwardDirection(ss))
				nValue *= (-1); // REVERSE Direction
		case SF_OSC:
		case SF_AVC:
		case SF_WIREFEED:
		case SF_JOG_ONLY_VEL:

			if(pServoCommand->nCommandPort != nValue)
			{
				pServoCommand->nCommandPort = nValue;
				//ServoCmd(pServoCommand);
		    	TechWriteCommand(ss->uServoSlot, SetCommand, nValue);
			}
			break;
	}
		
//	ETHERNET_Print("SetCommandDacValue:: Port: %X Dac: %X\n", PortMap[ss->uServoSlot].nCommandPort, wOutputValue);
}

//////////////////////////////////////////////////////////////////////
//
// void SetDesiredAvcResponse(WORD wValue)
//
//////////////////////////////////////////////////////////////////////
void SetDesiredAvcResponse(WORD wValue)
{
//	ETHERNET_Print("SetDesiredAvcResponse:: wAvcResponseDesired: %04X wAvcResponseActual: %04X\n",
	// _wAvcResponseDesired, _wAvcResponseActual);
	_wAvcResponseDesired = wValue;
}

//////////////////////////////////////////////////////////////////////
//
// void OutputAvcResponse(WORD wValue)
//
//////////////////////////////////////////////////////////////////////
static void OutputAvcResponse(WORD wValue)
{
	short nResponsePort;
	// Test for overflow
	if (wValue > POS_DAC_SWING)
	{
//		printf("SetResponseDacValue:: Overflow wValue: %04X\n", wValue);
		wValue = 0;
	}

//	ETHERNET_Print("SetResponseDacValue:: wValue: %04X\n", wValue);

	// Only slot 3 has an AVC response DAC STD_AVC_SLOT
	nResponsePort =	ServoCommand[STD_AVC_SLOT].nResponsePort;
	ServoCommand[STD_AVC_SLOT].nResponsePort = (short)wValue;
	if(nResponsePort !=	ServoCommand[STD_AVC_SLOT].nResponsePort)
    {
		//ServoResp((const CAN_SERVOCMD*)& ServoCommand[STD_AVC_SLOT]);
    	TechWriteCommand(STD_AVC_SLOT, SetSpeed, ServoCommand[STD_AVC_SLOT].nResponsePort);
    }
}


void SetAvcResponseDacValue(WORD wValue)
{
	_wAvcResponseDesired = wValue;
	_wAvcResponseActual  = wValue;

	OutputAvcResponse(wValue);
}

void BumpAvcResponseDacValue(void)
{
	if (mNumServos != 0)
	{
		if (_wAvcResponseDesired < _wAvcResponseActual) 
		{
			SetAvcResponseDacValue(_wAvcResponseDesired);
			return;
		}
	
		//Ramp UP the Avc Response Value
		_wAvcResponseActual += 20; // Bump it 20
//		dprintf("BumpAvcResponseDacValue::  wAvcResponseDesired: %04X wAvcResponseActual: %04X\n", 
//											_wAvcResponseDesired, _wAvcResponseActual);
	
		// Check for overrun
		if (_wAvcResponseActual > _wAvcResponseDesired)
        {
			_wAvcResponseActual = _wAvcResponseDesired; // Reset to Desired on an overrun
        }

		OutputAvcResponse(_wAvcResponseDesired);
	}
}

void SetFanDelay(void)
{
	nFanOnDelay = 2000;
	DriveCommandBit(FAN12, __FALSE);
	DriveCommandBit(FAN56, __FALSE);
}

void DelayedFanOn(void)
{
	if (nFanOnDelay != 0)
    {
	    --nFanOnDelay;

	    if (nFanOnDelay < 1500)
	    {
		    DriveCommandBit(FAN12, __TRUE);
	    }

	    if (nFanOnDelay < 1000)
	    {
		    DriveCommandBit(FAN56, __TRUE);
	    }
    }
}

//////////////////////////////////////////////////////////////////////
//
// void SetCurrentEnableBit(BOOL bState)
//
// Comments:
//       Output the "Current Enable Bit" for the servo in nSlot
//       bState = __TRUE  Enable BOOL = active
//
//////////////////////////////////////////////////////////////////////
void SetCurrentEnableBit(BOOL bState)
{
	DriveCommandBit(CURRENT_ENA, bState);

/*
	// Drive the EXTERNAL current enable BOOL
	DriveCommandBit(EXT_CURR_ENABLEOUT, bState);
*/

//	ETHERNET_Print("SetCurrentEnableBit:: State: %02X\n", bState);
}

// Extra Code to init HeadPostion Indicators with Soft Encoders
void SetFeedbackTypeHeadPosition(void)
{
	SERVO_POSITION_STRUCT *sps;

	if (mHeadPositionIndicator) 
    {
		sps = &ServoPosition[mPositionServoSlot];  // SPS is Servo Slot based
		if ((mHeadPositionFeedbackType == FEEDBACK_ENCODER) || 
            (mHeadPositionFeedbackType == FEEDBACK_POSITIONFROMVELOCITY)) 
        {
			sps->nEncoderType    = mHeadPositionFeedbackType;
			sps->nRevEncoderCnts = mHeadRevEncoderCnts;
		}
	}
}

void SetFeedbackType(void)
{
	BYTE uServo, uSlot;
	SERVO_POSITION_STRUCT *sps;

	for (uServo = 0; uServo < mNumServos; ++uServo) 
    {
		uSlot = mServoSlot(uServo);
		sps = &ServoPosition[uSlot]; // SPS is Servo Slot based

		sps->nEncoderType = (-1); 
		sps->nServoIndex = uServo;

		if (mServoFunction(uServo) != SF_CURRENT) 
		{
			if (mFeedbackDeviceType(uServo) == FEEDBACK_ENCODER) 
			{
				sps->nEncoderType = FEEDBACK_ENCODER; 
			}

			if (mPositionDeviceType(uServo) == FEEDBACK_ENCODER)
			{
				sps->nEncoderType = FEEDBACK_ENCODER; 
			}

			sps->nServoIndex = uServo;
			if ((mFeedbackDeviceType(uServo) == FEEDBACK_ENCODER) || 
				(mPositionDeviceType(uServo) == FEEDBACK_ENCODER))
			{
				sps->nRevEncoderCnts = mServoRevEncoderCnts(uServo);
			}

			switch(mFeedbackDeviceType(uServo)) 
			{
				case FEEDBACK_ENCODER:			// Encoder
//					SetServoFeedbackBits(uServo, USE_DIGITAL);
					sps->nTolerance = 40;		// Used for Analog Pot head positions
					break;

				case FEEDBACK_ANALOG:			// Analog Tachometer
//					SetServoFeedbackBits(uServo, USE_ANALOG);
					sps->nTolerance = 4000;		// Used for Analog Pot head positions
					break;

				case FEEDBACK_BACKEMF: 			// Back EMF feedback
//					SetServoFeedbackBits(uServo, USE_BEMF);
					sps->nTolerance = 4000;		// Used for Analog Pot head positions
					break;
			}
		}
	}

	// Extra Code to init HeadPosition Indicators with Soft Encoders
	SetFeedbackTypeHeadPosition();
}

void SetServoType(BYTE uServoType, BYTE uSlot)
{
	BOOL bIsVelocityServo = __FALSE;

	if (uSlot != 0)
	{
		if (uServoType == ST_VELOCITY)
		{
			bIsVelocityServo = __TRUE;
		}
	
		SetServoCmdBit(uSlot, SLOT_IS_VEL_SERVO, bIsVelocityServo);
	}
}

//////////////////////////////////////////////////////////////////////
//
// BOOL GetEnableBit(BYTE uServoSlot)
//
// Comments:
//       Output the "Enable Bit" for the servo in nSlot
//
//////////////////////////////////////////////////////////////////////
BOOL GetEnableBit(BYTE uServoSlot)
{
	return GetServoCmdBit(uServoSlot, SLOT_ENABLE);
}

//////////////////////////////////////////////////////////////////////
//
// void SetEnableBit(BOOL bState, BYTE uSlot)
//
// Comments:
//       Output the "Enable Bit" for the servo in nSlot
//
//////////////////////////////////////////////////////////////////////
void SetEnableBit(BOOL bState, const SERVO_VALUES* ss)
{
	SetExternalEnableBit(bState, ss);

	if ((ss->uServoControl == SC_OPENLOOP) ||
		(ss->uServoControl == SC_ULPHEAD))
	{
		if (bState == __FALSE)
		{
			return;  // DO NOT DISABLE OPENLOOP SERVOS
		}
	}

	SetServoCmdBit(ss->uServoSlot, SLOT_ENABLE, bState);
}

//////////////////////////////////////////////////////////////////////
//
// void SetBoosterEnableBit(BOOL bState)
//
// Comments:
//       Output the "Booster Enable Bit"
//
//////////////////////////////////////////////////////////////////////
void SetBoosterEnableBit(BOOL bState)
{
	// BOOST_ENA: BOOL 3 of port 148
	DriveCommandBit(BOOST_ENA, bState);
//	ETHERNET_Print("SetBoosterEnableBit:: State: %d\n", bState);
}

//////////////////////////////////////////////////////////////////////
//
// void SetBleederDisableBit(BOOL bState)
//
// Comments:
//       Output the "Bleeder Disable Bit"
//
//////////////////////////////////////////////////////////////////////
void SetBleederDisableBit(BOOL bState)
{
	// BLEED_DISABLE: BOOL 6 of port 148
	DriveCommandBit(BLEED_DISABLE, bState);
}

//////////////////////////////////////////////////////////////////////
//
// void SetExternalEnableBit(BOOL bState, SERVO_VALUES* ss)
//
// Comments:
//       Output the "External Enable Bit" for the servo in nSlot
//
//////////////////////////////////////////////////////////////////////
void SetExternalEnableBit(BOOL bState, const SERVO_VALUES* ss)
{
	// 0x0000 = -10V 0x0FFF = +10V
	WORD wValue = 0x0400; // +5Volts

	wValue *= (WORD)bState;
	wValue += DAC_VOLT_OFFSET;  // Add in the Zero volt offset
	 
	switch(ss->uExternalDac)
	{
		case 1:  // Analog Output #1
//			WriteDacData(PA_TRAV_ENOUT, wValue);
			break;

		case 2:  // Analog Output #2
//			WriteDacData(PA_WIRE_ENOUT, wValue);
			break;

		case 3:// Analog Output both #1 and #2
//			WriteDacData(PA_TRAV_ENOUT, wValue);
//			WriteDacData(PA_WIRE_ENOUT, wValue);
			break;

		case 0: // None Selected
		default:
			// Do nothing
			break;
	}
}

//////////////////////////////////////////////////////////////////////
//
// void ZeroCommandDac(BYTE uSlot)
//
// Comments:
//       Set the command dac to drive the at minimum speed
//       nSlot  = Slot number of the servo to be controlled
//
//////////////////////////////////////////////////////////////////////
void ZeroCommandDac(BYTE uServoSlot)
{
//	CAN_SERVOCMD* pServoCommand;
//
//	ASSERT(uServoSlot < MAX_NUMBER_SERVOS);
//	if (uServoSlot < MAX_NUMBER_SERVOS)
//	{
//		pServoCommand = &ServoCommand[uServoSlot];
//		if(pServoCommand->nCommandPort != 0)
//		{
//			pServoCommand->nCommandPort = 0;
//			ServoCmd(pServoCommand);
//		}
//	}
}													

//////////////////////////////////////////////////////////////////////
//
// void SetRfStartBit(BOOL bState)
//
// Comments:
//       Output the "RF Start BOOL"
//
//////////////////////////////////////////////////////////////////////
void SetRfStartBit(BOOL bState)
{
	DriveCommandBit(RF_ENABLE, bState);
}

BOOL FootControllerInstalled(void)
{
	return GetCurrentState(FOOT_CON_INSTALLED);
}

// TODO: This needs to be corrected
DWORD ReadFootController(void)
{
	short nInData;

	if (!FootControllerInstalled())
		return (DWORD)ADC_ZERO_OFFSET;

	nInData = GetFeedback(FB_FOOTCONTROL) & 0x0FFF;
	if (nInData < 0)
		nInData = ADC_ZERO_OFFSET;

	return (DWORD)nInData;
}

//////////////////////////////////////////////////////////////////////
//
// void SetGasValveOpen(BYTE uState)
//
// Comments:
//       Open gas valve if passed __TRUE
//
//////////////////////////////////////////////////////////////////////
void SetGasValveOpen(BYTE uState)
{
	// Set the global gas valve BOOLean
	bGasValveIsOpen = (BOOL)uState;
	if (uState)
	{
		wGasFlowDelay = 50; // Delay gas faults for 0.50 seconds
	}
	
	// GAS_SOLE_ENA: BOOL 1 of port 148
	DriveCommandBit(GAS_SOLE_ENA1, (BOOL)uState);

/*
	// Set External Gas manifold command dac
	if (uState)
		WriteDacData(PA_EXTERNAL_GAS_CMD, (WORD)(mTorchGasRate + DAC_VOLT_OFFSET));
	else
		WriteDacData(PA_EXTERNAL_GAS_CMD, (WORD)(0 + DAC_VOLT_OFFSET));
*/
}

//////////////////////////////////////////////////////////////////////
//
// BOOL PsInRegulation(void)
//
//////////////////////////////////////////////////////////////////////
BOOL PsInRegulation(void)
{
	return GetCurrentState(CUR_DETECTED);
}

//////////////////////////////////////////////////////////////////////
//
// WORD GetPowerSupplyVoltage(void)
//
//////////////////////////////////////////////////////////////////////
WORD GetPowerSupplyVoltage(void)
{
	return GetFeedback(FB_SLOT3) & 0x0FFF;

}

//////////////////////////////////////////////////////////////////////
//
// WORD GetVoltageFeedback(void)
//
//////////////////////////////////////////////////////////////////////
WORD GetVoltageFeedback(void)
{
    WORD wVoltage;
	if (_isAvcCardPresent)
    {
		wVoltage = GetFeedback(FB_SLOT3) & 0x0FFF;
    }
	else 
    {
		wVoltage = GetFeedback(FB_SLOT3) & 0x0FFF;
    }

    return wVoltage;

}

BOOL isAvcJogging(void)
{
	SERVO_VALUES* ss;

	if (_isAvcCardPresent)
    {
		ss = &ServoStruct[STD_AVC_SLOT];
		if (ss->bIsJogging)
		{
			return __TRUE;
		}
	}

	return __FALSE;
}


//////////////////////////////////////////////////////////////////////
//
// BOOL ArcAboveMax(void)
//
// Comments:
//       Return __TRUE if the Arc vlotage is above the maximum allowed
//
//////////////////////////////////////////////////////////////////////
BOOL ArcAboveMax(void)
{
	WORD wVoltage;

	if (isAvcJogging())
	{
		return __FALSE; // No voltage error during jogs
	}

	if (!bHighArcEnable || (nUlpVoltFaultState == ULP_VOLTDISABLED)) 
	{
//		dprintf("HighArc Disabled\n");
		return __FALSE;
	}

	wVoltage = GetVoltageFeedback();
	if (wVoltage > MAX_ARC_VOLTAGE) 
	{
//		dprintf("ArcAboveMax:: Voltage: %X Limit: %X\n", wVoltage, MAX_ARC_VOLTAGE);
		return __TRUE;
	}

	return __FALSE;
}

//////////////////////////////////////////////////////////////////////
//
// BOOL ArcBelowMin(void)
//
// Comments:
//       Return __TRUE if the Arc vlotage is below the minimum allowed
//
//////////////////////////////////////////////////////////////////////
BOOL ArcBelowMin(void)
{
	WORD wVoltage;

	if (isAvcJogging())
	{
		return __FALSE; // No voltage error during jogs
	}

	if (!bStubOutEnable || (nUlpVoltFaultState == ULP_VOLTDISABLED)) 
	{
//		dprintf("StubOut Disabled\n");
		return __FALSE;
	}

	wVoltage = GetVoltageFeedback();
	if (wVoltage < MIN_ARC_VOLTAGE) {
//		dprintf("ArcBelowMin:: Voltage: %X Limit: %X\n", wVoltage, MIN_ARC_VOLTAGE);
		return __TRUE;
	}

	return __FALSE;
}

//////////////////////////////////////////////////////////////////////
//
// void SetLampOn(BOOL bState)
//
// Comments:
//       bState = __TRUE  Set Lamp BOOL = active
//
//////////////////////////////////////////////////////////////////////
void SetLampOn(BOOL bState)
{
	WeldSystem.uLampOn = bState;
	dwLampTimer = 0;
	if (bState) {
		dwLampTimer = (DWORD)(nLampTimer * 6000); // 6000 = interrupts / minute
	}
	DriveCommandBit(ILLUM, bState);
}

void CheckLampTimer(void)
{
	if (dwLampTimer == 0)
		return;

	--dwLampTimer;
	
	if (dwLampTimer == 0)
	{
		QueuePendantKey(PB_LAMP);
	}
}

void fnPendantServoJog(BYTE uKey, int nJogSpeed)
{
	JOG_STRUCT PendantJogStruct;
	BYTE uFunction = SF_TRAVEL;

	switch(uKey)
	{
	case PB_TVL_CW_JOG:
	case PB_TVL_CCW_JOG:
		uFunction = SF_TRAVEL;
		break;

	case PB_WFD_FD_JOG:
	case PB_WFD_RT_JOG:
		uFunction = SF_WIREFEED;
		break;

	case PB_AVC_UP_JOG:
	case PB_AVC_DN_JOG:
		uFunction = SF_AVC;
		break;
	}

	PendantJogStruct.uServo = (BYTE)PendantKeyServoMap[uFunction];
	PendantJogStruct.uSpeed = (char)nJogSpeed;

	JogServo((JOG_STRUCT const *)&PendantJogStruct);
}

void EraseServoPostionStruct(void)
{
	SERVO_POSITION_STRUCT *sps;
	BYTE uServoSlot;

	for (uServoSlot = 0; uServoSlot < MAX_NUMBER_SERVOS; ++uServoSlot) {
		sps = &ServoPosition[uServoSlot];  // SPS is Servo Slot based
		sps->nEncoderType = (-1);
	}

}

void ClearLedDisplay(void)
{
	memset(uPendDispBuff, ' ', PENDANT_LED_WIDTH);
	nPendDispCharCnt = PENDANT_LED_WIDTH;
}

void BufferLcdDisplay(const char *pDisplayText)
{
	memcpy(uPendDispBuff, pDisplayText, PENDANT_LED_WIDTH);
//	SendPendantCommand(PenCommand_LcdAsciiData);
}

const char* GetLedDisplay(void)
{
	return (const char*)uPendDispBuff;
}

void ClearBitmapDisplay(void)
{
	memset(uPendBitmapBuff, ' ', PENDANT_BITMAP_WIDTH);
}

void BufferBitmapDisplay(const char *pDisplayBitmap)
{
	memcpy(uPendBitmapBuff, pDisplayBitmap, PENDANT_BITMAP_WIDTH);
//	SendPendantCommand(PenCommand_LcdBitmapData);
}

const char* GetBitmapDisplay(void)
{
	return (const char*)uPendBitmapBuff;
}
//////////////////////////////////////////////////////////////////////
//
// BOOL StartUp(void)
//
// Comments:
//       Is called once to init
//			hardware, tables, queues, and anything else we feel like.
//
//////////////////////////////////////////////////////////////////////
BOOL StartUp(void)
{
	// First zero all the hardware by calling fnColdShutdown
	_isGuiConnected = 0;

//	dwCoolantFlowCountPri = 0;
	_uAuxLineFunction[0] = 0;
	_uAuxLineFunction[1] = 0;
	_uAuxLineFunction[2] = 0;
	_uAuxLineFunction[3] = 0;

	// Clear DAC offset & Gain Values
	ClearDacCalTable();
 	InitServoConstants();

	// Must clear DacCalTable before calling fnColdShutdown
	fnColdShutdown();

	// Clear the Weld Schedule memory
	memset(&stInitWeldSched, 0, sizeof(stInitWeldSched));

	memset(ServoPosition,    0, sizeof(ServoPosition));
	EraseServoPostionStruct();

	//memset(ServoFeedback, 0, sizeof(CAN_SERVOFDBK) * MAX_NUMBER_SERVOS);
	//memset(ServoCommand,  0, sizeof(CAN_SERVOCMD)  * MAX_NUMBER_SERVOS);

	ZeroAllDacs();

	stInitWeldSched.uNumberOfServos = 0;

	// Init some structure variables
	InitializeWeldParameters();

	return __TRUE;
}

//////////////////////////////////////////////////////////////////////
//
// BOOL OnConnect(void)
//
// Comments:
//       Is called when the server connects to us to
//			turn on pumps and other stuff that shouldn't be active
//			untill the server connects.
//
//////////////////////////////////////////////////////////////////////
BOOL OnConnect(void)
{
//	printf("OnConnect\r\n");

	// Initialize LED buffer and set length
	BufferLcdDisplay(pAmi);

	//Make sure CAN work when GUI connect to RxData.
	CANinit();

	return __TRUE;
}

//////////////////////////////////////////////////////////////////////
//
// BOOL OnDisconnect(void)
//
// Comments:
//       Is called when the server closes.
//
//////////////////////////////////////////////////////////////////////
BOOL OnDisconnect(void)
{
//	printf("OnDisconnect\r\n");

//	if (_isGuiConnected)
//	{
		PowerDown();
//	}


	return __TRUE;
}

void SetArcOnBit(BOOL bState)
{
//	dwVxdFlags |= W32IF_MSG_FLAG_ARC_ON;  // Indicate new Arc on Data available
}

void PopBreaker(BOOL bEmergency)
{
	if (bEmergency)
	{
		DriveCommandBit(EMERG_STOP_OUT, __TRUE);
		for(;;);  // Die Here
	}
	else
	{
		int nIndex;
		for (nIndex = 0; nIndex < 10; ++nIndex)
		{
			vTaskDelay(1000); // Delay 1000 ms
		}
		DriveCommandBit(EMERG_STOP_OUT, __TRUE);
	}
}


//////////////////////////////////////////////////////////////////////
//
// BOOL PowerDown(void)
//
// Comments:
//		This function sets a flag which will, upon the next 10ms cycle,
//			send a message to the server to disconnect, shutdown windows,
//			and power off the machine.
//      This should only be called in an emergency situation.
//
//		TBD - We need to handle the case of the server not being connected.
//
//////////////////////////////////////////////////////////////////////
BOOL PowerDown(void)
{
	// Stop what hardware we can
	fnColdShutdown();
	PopBreaker(__FALSE);
	return __TRUE;
}

//////////////////////////////////////////////////////////////////////
//
//	void SetAllControlPorts(void)
//
// Comments:
//
//////////////////////////////////////////////////////////////////////
void ResetAllControlPorts(BOOL bAllPorts)
{
	BYTE uServo;

	DriveCommandBit(WHEAD2_SEL,		__FALSE);

	// DriveCommandBit(MAN_ENA,		__FALSE);
	// DriveCommandBit(MAN_AS0,		__FALSE);
	// DriveCommandBit(MAN_AS1,		__FALSE);
	// DriveCommandBit(MAN_AS2,		__FALSE);
	DriveCommandBit(CURRENT_ENA,	__FALSE);
	DriveCommandBit(IN_SEQ,			__FALSE);

#ifdef TRAVEL_ENOUT
	DriveCommandBit(TRAVEL_ENOUT,	__FALSE);
#endif // TRAVEL_ENOUT
	DriveCommandBit(AUX_OUT1,		__FALSE);
	DriveCommandBit(AUX_OUT2,		__FALSE);

#ifdef AUX_OUT3
	DriveCommandBit(AUX_OUT3,	__FALSE);
#endif // AUX_OUT3

#ifdef AUX_OUT4
	DriveCommandBit(AUX_OUT4,	__FALSE);
#endif // AUX_OUT4

	DriveCommandBit(RF_ENABLE,		__FALSE);
	DriveCommandBit(GAS_SOLE_ENA1,	WeldSystem.uPurgeOn);
	DriveCommandBit(BOOST_ENA,		__FALSE);
	DriveCommandBit(EMERG_STOP_OUT,		__FALSE);
	DriveCommandBit(BLEED_DISABLE,	__FALSE);

	if (nFanOnDelay != 0)
	{
		DelayedFanOn();
	}
	else
	{
		DriveCommandBit(FAN12, __TRUE);
		DriveCommandBit(FAN56, __TRUE);
	}

	DriveCommandBit(WIRE_SELECT,	__FALSE);
	DriveCommandBit(ILLUM,			WeldSystem.uLampOn);
	DriveCommandBit(GNDFLT_RELAY,   __FALSE);

	InSequenceIO(bWeldSequence);

	if (bAllPorts) 
	{
		for(uServo = 0; uServo < MAX_NUMBER_SERVOS; ++uServo) 
		{
			SetServoCmdBit(uServo, SLOT_ENABLE, 	   __FALSE);
			SetServoCmdBit(uServo, SLOT_USE_DIGITAL1,  __FALSE);
			SetServoCmdBit(uServo, SLOT_USE_DIGITAL2,  __FALSE);
			SetServoCmdBit(uServo, SLOT_IS_VEL_SERVO,  __TRUE);
			SetServoCmdBit(uServo, SLOT_FWD_DIRECTION, __TRUE);
			SetServoCmdBit(uServo, SLOT_AVC_JOG,	   __FALSE);
		}
	}
}

void CheckGas(void)
{
	if (!bGasValveIsOpen) 
	{
		// Valve is SHUT, therefore no fault
		if (bGasFlowFault)
		{
			QueueFaultMessage(ARC_GAS_FAULT_CLR);
			bGasFlowFault = __FALSE;
		}
	}
	else
	{
		// Gas Valve is OPEN
		if (wGasFlowDelay)
			--wGasFlowDelay; // Opening the valve will set "wGasFlowDelay"

		if (!GasFlowing()) 
		{
			// No Flow
			if (!bGasFlowFault && (wGasFlowDelay == 0))
			{
				QueueFaultMessage(ARC_GAS_FAULT_SET);
				bGasFlowFault = __TRUE;
			}
		}
		else
		{
			if (bGasFlowFault) 
			{
				QueueFaultMessage(ARC_GAS_FAULT_CLR);
				bGasFlowFault = __FALSE;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
//
// void OscSlewToFunction(int nReason)
//
//////////////////////////////////////////////////////////////////////
void OscSlewToFunction(int nReason)
{
	OSC_VALUES *os;
	BYTE       uServo;
	
	for (uServo = 0; uServo < mNumServos; ++uServo)	
	{
		os = (OSC_VALUES *)&ServoStruct[uServo];

		if (os->uServoFunction == SF_OSC) 
		{
			if (mOscSpecialMode(uServo) == nReason) 
			{
				os->nOscSlewCount -= mBackgroundValue(os->uServoIndex);	
			}
		}
	}
}

static void HandleAuxLine1(void)
{
	static BOOL uAux1LastState = __FALSE; 
	BOOL uAux1CurrentState; 

	uAux1CurrentState = GetCurrentState(AUX_IN1);
	if (uAux1LastState == uAux1CurrentState) 
	{
		return; // no state change
	}

	switch(_uAuxLineFunction[0]) 
	{
		case AUX_SEQ_STOP:
			if (uAux1CurrentState) 
			{
//				dprintf("HandleAuxLine1:: Queue PB_ALL_STOP\n");
				QueuePendantKey(PB_ALL_STOP);
			}
			break;

		case AUX_USER_DEF1:
		default:
//			dprintf("HandleAuxLine1:: AUX_USER_DEF1 Change\n");
			if (uAux1CurrentState) 
			{
//				dprintf("HandleAuxLine1:: Queue USER_FLT1_SET\n");
				QueueFaultMessage(USER_FLT1_SET);
			}
			else 
			{
//				dprintf("HandleAuxLine1:: Queue USER_FLT1_CLR\n");
				QueueFaultMessage(USER_FLT1_CLR);
			}
			break;
	}

	uAux1LastState = uAux1CurrentState;
}

static void HandleAuxLine2(void)
{
	static BOOL uAux2LastState = __FALSE; 
	BOOL uAux2CurrentState; 

	uAux2CurrentState = GetCurrentState(AUX_IN2);
	if (uAux2LastState == uAux2CurrentState) 
	{
		return; // no state change
	}

	switch(_uAuxLineFunction[1]) 
	{
		case AUX_JOG_REV:
			if (uAux2CurrentState) 
			{
//				printf("HandleAuxLine2:: AUX_JOG_REV Jog\n");
				fnPendantServoJog(PB_TVL_CW_JOG, (-100));
			}
			else 
			{
//				printf("HandleAuxLine2:: AUX_JOG_REV Stop\n");
				fnPendantServoJog(PB_TVL_CW_JOG, 0);
			}
			break;

		case AUX_USER_DEF2:
		default:
//			printf("HandleAuxLine2:: AUX_USER_DEF2 Change\n");
			//if (uAux2CurrentState)
				QueueFaultMessage(USER_FLT2_SET);
			//else
				QueueFaultMessage(USER_FLT2_CLR);
			break;

	}
	
	uAux2LastState = uAux2CurrentState;
}

static void CheckTemperature(void)
{
 	static BYTE uTempFaultCount = 0;
	isTempFault = !GetCurrentState(TEMP_FAULT); // FAULT is active low

	if (isTempFaultLast != isTempFault)
	{
		++uTempFaultCount;
		if(uTempFaultCount > 25)
		{
			if (isTempFault) 
			{
				QueueFaultMessage(PS_TEMP_SET);
			}
			else
			{
				QueueFaultMessage(PS_TEMP_CLR);
			}
			isTempFaultLast = isTempFault;
		}
	}
	else
	{
		uTempFaultCount = 0;
	}
}

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
static void SendServoFault(WORD wActiveBit, BOOL bState, BYTE uServoIndex)
{
	BYTE uFaultMessage = SERVO_TYPE_FAULT;

	if (bState)
	{
		uFaultMessage |= FAULT_SET;
	}

	switch(wActiveBit)
	{
		case SERVO_SWITCH2:	uFaultMessage |= (0x00 << 1);	break;
		case SERVO_SWITCH1:	uFaultMessage |= (0x01 << 1);	break;
		case SERVO_FAULT:	uFaultMessage |= (0x02 << 1);	break;
		case TECHN_FAULT:	uFaultMessage |= (0x03 << 1);	break;
	}

	/*********************************************************
	 * TODO: Decode Technosoft limit switch bits

	// Or in Servo Number
	uServoIndex  &= 0x0F;
	uServoIndex <<= 3;
	uFaultMessage |= uServoIndex;

	QueueFaultMessage(uFaultMessage);
	************************************************************/
}

static void GenerateServoFaults(WORD wPastState, WORD wCurrentState, BYTE uServoIndex)
{
	WORD wActiveBit;
	WORD wBitMask;
	BYTE ix;

	wActiveBit = (wCurrentState ^ wPastState);

	for (ix = 0; ix < 15; ++ix)
	{
		wBitMask = (1 << ix);
		if ((wBitMask & wActiveBit) != 0)
		{
			SendServoFault(wActiveBit, (BOOL)((wCurrentState & wBitMask) != wBitMask), uServoIndex);
		}
	}
}

// TODO: How do we check for Limit Switch and Technosoft faults
static void CheckServoFaults(void)
{
	BYTE uServoIndex;
	SERVO_VALUES* ss;
	BYTE uSlot;
	WORD wFaults;

	for (uServoIndex = 1; uServoIndex < mNumServos; ++uServoIndex) 
	{
		ss = &ServoStruct[uServoIndex];
		uSlot = ss->uServoSlot;		

		wFaults = GetServoFaults(uSlot);
		if (ServoFeedback[uSlot].wPastServoFault != wFaults)
		{
			GenerateServoFaults(ServoFeedback[uSlot].wPastServoFault, wFaults, uSlot);
			ServoFeedback[uSlot].wPastServoFault = wFaults;
		}
	}
}


// Handles AUX_LINE 1 thru 4, and the HALT_SEQ line
void CheckAuxLines(void)
{
	HandleAuxLine1();
	HandleAuxLine2();
}

//////////////////////////////////////////////////////////////////////
//
// void ReadFaults()
//
// Comments:
//       Reads the hardware for faults and puts it on the FaultQueue
//
//////////////////////////////////////////////////////////////////////
void ReadFaults(void)
{
	CheckGas();
	CheckAuxLines();
	CheckTemperature();
	CheckServoFaults();
}

void SendOpenLoopBargraph(SERVO_POSITION_STRUCT* sps)
{
	GRAPH_INFO* pGraphInfo = &GraphInfo[sps->nServoIndex];
	if (pGraphInfo->wPort == 0xFFFF) 
	{
		WORD wTheFeedback = (WORD)sps->nActualVelocitySum;

		// TODO: Need to add Command reporting when WDR is implemented
        //              Location    COMMAND       FEEDBACK     
		UpdateGraphInfo(pGraphInfo, wTheFeedback, wTheFeedback);
//		printf("SendOpenLoopBargraph:: Data: %08X\n", pGraphInfo->dwDataMessage);
		SendFeedbackUpdate(pGraphInfo, __TRUE);
	}
}

static void UpdateActualVelocity(SERVO_POSITION_STRUCT* sps)
{
	sps->nActualVelocity = (sps->nCurrentPosition - sps->nPreviousPosition); // ActualVelocity is measured in Counts / Interrupt

	sps->nActualVelocitySum += abs(sps->nActualVelocity);					// Velocity is always positive
	++sps->nActualVelocityCnt;
	if (sps->nActualVelocityCnt > 9) 
	{
        sps->nActualVelocitySumLast = sps->nActualVelocitySum;
		SendOpenLoopBargraph(sps);
//		printf("ActualVelocity:: Sum: %d\n", sps->nActualVelocitySumLast);
		sps->nActualVelocityCnt = 0;
		sps->nActualVelocitySum = 0;
        // DO NOT ZERO nActualVelocitySumLast
	}
}

SERVO_VALUES*  LookupServoValuePtr(BYTE uServoSlot)
{
	SERVO_VALUES* ss;
	BYTE  uServoIndex;

	for (uServoIndex = 0; uServoIndex < mNumServos; ++uServoIndex) 
	{
		ss = &ServoStruct[uServoIndex]; // Save pointer to this servos struct
		if (ss->uServoSlot == uServoSlot) 
		{
			return ss;
		}
	}

	return NULL; // SHould hit here
}

void GetSoftEncoderData(BYTE uServoSlot, SERVO_POSITION_STRUCT *sps)
{
	int nVelocity;

	// USES soft encoder; Read Analog Velocity to intergrate and determine position
	sps->nPreviousEncoder = 0;  // Needed to force an update and an increment

	if (GetEnableBit(uServoSlot)) 
    {
		nVelocity = (int)(GetFeedback(uServoSlot + FB_SLOT_OFFSET) - (int)ADC_ZERO_OFFSET);
		if (!GetDirectionBit(uServoSlot))
        {
			nVelocity *= (-1);  // Reverse the sign; Reverse direction counts down
        }
		sps->nCurrentEncoder = nVelocity;
	}
	else 
    {
		// Servo is not enabled, therefore not moving (in theory)
		sps->nCurrentEncoder = 0;
	}
}

static void ReadServoEncoderPosition(void)
{
    BYTE uSlot;
    int  nMoveIncrement;
    int  nCorrectedMovement;
    SERVO_POSITION_STRUCT *sps;
    
    for (uSlot = 1; uSlot < MAX_NUMBER_SERVOS; ++uSlot) 
    {
        sps = &ServoPosition[uSlot];
        if (sps->nEncoderType != (-1))
        {
            // Save previous encoder, get current encoder data
            sps->nPreviousEncoder = sps->nCurrentEncoder;	// Save the previous encoder
            
            switch(sps->nEncoderType) 
            {
                case FEEDBACK_POSITIONFROMVELOCITY:
                    GetSoftEncoderData(uSlot, sps);  // Read Soft Encoder, and deposit into nCurrentEncoder
                    break;
                
                case FEEDBACK_ENCODER:
                    sps->nCurrentEncoder = ReadEncoderFeedback(uSlot); // Get new encoder
                    break;
                
                default:
                    // Shound't hit here
                    break;
            }
            
            nMoveIncrement = sps->nCurrentEncoder - sps->nPreviousEncoder;
            
            if (abs(nMoveIncrement) > (int)0x7FFF) 
            { 
                // Roll Over 0->65535 or 65535->0
                nCorrectedMovement = (int)0x10000 - abs(nMoveIncrement);
                if (nMoveIncrement > 0)
                {
                    nCorrectedMovement *= (-1);
                }
                nMoveIncrement = nCorrectedMovement;
            }

            if (sps->nRevEncoderCnts != 0)
            {
                nMoveIncrement *= (-1);
            }
            
//          if (nMoveIncrement != 0)
//          {
//              nMoveIncrement = nMoveIncrement;
//              dprintf("ReadEncoder: Slot: %02d Move: %02d Current: %05d\n", uSlot, nMoveIncrement, sps->nCurrentEncoder);
//          }
            
            sps->nPreviousPosition = sps->nCurrentPosition;							 // Save the previous position
            sps->nCurrentPosition += nMoveIncrement;								 // Calc the new position
            
            if (sps->nEncoderType == FEEDBACK_ENCODER) 
            {
                UpdateActualVelocity(sps);  // Soft encoders do not need to calc velocity
            }
            
            if (mHeadPositionIndicator)
            {
                if (uSlot == mPositionServoSlot)
                {
                    // Update Level Position
                    WeldSystem.nLevelPosition = sps->nCurrentPosition;
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////
//
// void ReadBarGraphData()
//
// Comments:
//       Reads the hardware for the BarGraphs and puts it on the 
//			BarGraphQueue
//
//////////////////////////////////////////////////////////////////////
#ifndef DESKTOP_RUN
void ReadBarGraphData(void)
{
	GRAPH_INFO *pGraphInfo;
	static BYTE uResetGraphs = 0;
	int ix;
	BOOL bForceSend = __FALSE;

	++uResetGraphs;
	if ((uResetGraphs & 0x007F) == 0)
	{
		// reset graphs every 1.28 seconds (10 milliSeconds * 128)
		bForceSend = __TRUE;
	}

	// TODO: Code this function when Encoder reading from servos is coded
	// ReadServoEncoderPosition(); // Updates LevelPosition also for HeadIndicator

	if (((uResetGraphs % 10) == 0) || bForceSend)
	{
		// TODO: Code this function when Encoder reading from servos is coded
		// SendServoPosition(bForceSend);
	
		for (ix = 0; ix < uNumGraphPorts; ++ix)
		{
			pGraphInfo = &GraphInfo[ix];
			if (pGraphInfo->wPort != 0xFFFF)
			{
				wCurrentFeedback[ix] = GetFeedback(pGraphInfo->wPort) & 0x0FFF;
				SendFeedbackWithoutCommand(pGraphInfo, wCurrentFeedback[ix]);

/*
				// TODO: Code this function when WeldDataRecording is implemented
				if (IsFeedbackWithoutCommand(pGraphInfo))
				{
					SendFeedbackWithoutCommand(pGraphInfo, wNewFeedback);
				}
				else
				{
					SetAverageFeedback(pGraphInfo, wNewFeedback);
					SendBargraphUpdate(pGraphInfo, bForceSend);
				}
*/
			}
		}
	
		if (WeldSystem.uDataAqiOutEnable)
	    {
			QueueLevelTime();
	    }
	}
}
#else
void ReadBarGraphData(void)
{
	static BYTE uResetGraphs = 0;
	static DWORD dwDataMessage = 0;
	static WORD wThisFeedback;

	++uResetGraphs;
	if ((uResetGraphs & 0x007F) == 0)
	{
		++wThisFeedback;
		wThisFeedback &= 0x0FFF;

		// Or in new command & feedback
		dwDataMessage |= ((DWORD)wThisFeedback) << 16;
		dwDataMessage |= ((DWORD)wThisFeedback) << 0;
		dwDataMessage |= (DWORD)1 << 28;
		QueueBarGraphMessage(dwDataMessage);
	}
}
#endif

// Reset all faults to the inactive state
void ResetAllFaults(void)
{
	BYTE uServoSlot;

	bGasFlowFault 	 = __FALSE;
	isTempFaultLast  = isTempFault 	    = __FALSE;
	uLockFaults   	 = __FALSE;
	QueueFaultMessage(GROUND_FLT_CLR);

	// Upon clear faults, reset the flow fault counts as well
	//dwCoolantFlowCountPri = 0;

	// Clear all Servo Faults
	for (uServoSlot = 0; uServoSlot < MAX_NUMBER_SERVOS; ++uServoSlot) 
	{
//		ServoFeedback[uServoSlot].wFaults         = 0;
//		ServoFeedback[uServoSlot].wPastServoFault = 0;
	}
}

WORD GetFeedbackADIO(BYTE uChannel)
{
	WORD wAnalogFeedback = 0;
	wAnalogFeedback = ReadADC(uChannel);
	return wAnalogFeedback;
}

#ifndef DESKTOP_RUN
WORD GetFeedbackServo(BYTE uChannel)
{
	WORD wAnalogFeedback;

	if ((uChannel == STD_AVC_SLOT) && !_isAvcCardPresent)
	{
		wAnalogFeedback = GetFeedbackADIO(FB_PS_VOLTAGE);
	}
	else
    {
		BYTE uServoSlot = uChannel - FB_SLOT_OFFSET;
		wAnalogFeedback = (short)TechReadFeedback(uServoSlot, GET_TACH);
		wAnalogFeedback = wAnalogFeedback >> 4;
	}

	return wAnalogFeedback;
}
#else
WORD GetFeedbackServo(BYTE uChannel)
{
	static short nAnalogFeedback;
	nAnalogFeedback += 0x0010;

	return (WORD)nAnalogFeedback;
}
#endif // DESKTOP_RUN





WORD GetFeedback(BYTE uChannel)
{
	int nAnalogFeedback;
	DAC_CAL_STRUCT *pDacCalMap = &DacCalMap[uChannel & 0x0F];

	// Channels FB_SLOT1 thru FB_SLOT8 are read from SPI Servo Interface
	switch(uChannel)
	{
		case FB_SLOT1:
		case FB_SLOT2:
		case FB_SLOT4:
		case FB_SLOT5:
		case FB_SLOT6:
		case FB_SLOT7:
		case FB_SLOT8:
			nAnalogFeedback = GetFeedbackServo(uChannel);
			break;

		// Sepecial Case AVC Servo
		case FB_SLOT3:
			if (!_isAvcCardPresent)
			{
				nAnalogFeedback = GetFeedbackADIO(FB_PS_VOLTAGE);
			}
			else
			{
				nAnalogFeedback = GetFeedbackServo(uChannel);
			}
			break;

		case FB_SLOT0:
			nAnalogFeedback = GetFeedbackADIO(FB_CURRENT);
			break;

		default:
			nAnalogFeedback = GetFeedbackADIO(uChannel);
			break;
	}
	nAnalogFeedback &=  0x0FFF;
	nAnalogFeedback	= nAnalogFeedback + (nAnalogFeedback * (pDacCalMap->nFdbkGain - OG_SCALER)) / OG_SCALER;
	nAnalogFeedback += pDacCalMap->nFdbkOffset;

	return (WORD)nAnalogFeedback;
}

static void SelectFirstWirefeeder(BOOL bState)
{
	// THERE IS ONLY 1 WIREFEED SELECT BOOL
	DriveCommandBit(WIRE_SELECT, (BOOL)!bState);
}

int LookupServoIndex(BYTE uSlot)
{
	BYTE uServoIndex;
	SERVO_VALUES* ss;

	if (mNumServos != 0)
	{
		for (uServoIndex = 0; uServoIndex < mNumServos; ++uServoIndex) 
		{
			ss = &ServoStruct[uServoIndex]; // Save pointer to this servos struct
			if (ss->uServoSlot == uSlot)
			{
				return uServoIndex;
			}
		}

		ASSERT(0);
	}
	return (-1); // Should not hit here
}

int ReadEncoderFeedback(BYTE uServoSlot)
{
	if (GetServoEncoder(uServoSlot))
	{
		return ServoFeedback[uServoSlot].nPositionFeedback;
	}

	return 0; // Will hit here if no schedule loaded
}


//////////////////////////////////////////////////////////////////////
//
// void CommandBoosterVoltage(int wValue)
//
//////////////////////////////////////////////////////////////////////
void CommandBoosterVoltage(int nValue)
{
	WORD wBoosterVoltage;

	// Command Booster Voltage
	wBoosterVoltage = (WORD)(((DWORD)nValue * (DWORD)POS_DAC_SWING) / MAX_BOOSTER_VOLTAGE);
	wBoosterVoltage += DAC_VOLT_OFFSET;
//	ETHERNET_Print("CommandBoosterVoltage:: Dac: %X\n", wBoosterVoltage);

	WriteDacData(PA_BOOSTER_VOLT, wBoosterVoltage);
}

//////////////////////////////////////////////////////////////////////
//
// BOOL ExternalLine(BOOL *pState, const SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
BOOL FindSyncLine(const SERVO_VALUES* ss, LPBYTE pSwitchNum, BYTE uLineType)
{
	IW_SERVODEF			*pServoDef;
	IW_SERVO_SWITCH		*pServoSwitch;
	BYTE uSwitchNumber;

	pServoDef = SD_ADDR(ss->uServoIndex);
	for (uSwitchNumber = 0; uSwitchNumber < 4; ++uSwitchNumber) 
    {
		pServoSwitch = &pServoDef->stServoSwitch[uSwitchNumber];
		if (pServoSwitch != NULL) 
        {
			if (pServoSwitch->uSwitchActive && (pServoSwitch->uSwitchType == SYNC_LINE))
            {
                *pSwitchNum = uSwitchNumber;  // Switch is found
                return __TRUE;
            }
		}
	}

	return __FALSE;
}

//////////////////////////////////////////////////////////////////////
//
// BOOL ExternalLine(BOOL *pState, const SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
BOOL ExternalServoLine(BOOL *pState, const SERVO_VALUES* ss, BYTE uLineType)
{
	BYTE uSwitchNumber;
	
	if (FindSyncLine(ss, &uSwitchNumber, uLineType)) 
    {
//		*pState = (BOOL)ServoSwitchActive(ss->uServoSlot, uSwitchMaskList[uSwitchNumber]);
		return __TRUE;
	}

	return __FALSE; // No active sync switch
}

//////////////////////////////////////////////////////////////////////
//
// BOOL ExternalLine(BOOL *pState, const SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
BOOL ExternalLine(BOOL *pState, const SERVO_VALUES* ss)
{

	if (ss->uServoSlot != 0) 
    {
		return ExternalServoLine(pState, ss, SYNC_LINE);
	}
	else 
    {
#ifdef __M415_PS__
		const DWORD dwSwitchMaskList[4] = {AUX_IN3, AUX_IN4, AUX_IN3, AUX_IN4};
		BYTE uSwitchNumber;
	
		if (FindSyncLine(ss, &uSwitchNumber, SYNC_LINE))
        {
			DWORD dwSwitchMask;
			dwSwitchMask = dwSwitchMaskList[uSwitchNumber];
			*pState = GetCurrentState(dwSwitchMask);

			return __TRUE;
		}
#endif // __M415_PS__
	}

	return __FALSE;
}

void InSequenceIO(BOOL bInSequence)
{
	//REM DriveCommandBit(CHART_FEED, bInSequence);
	DriveCommandBit(IN_SEQ,     bInSequence);
}

int CalculateAvcVoltageError(const SERVO_VALUES* ss)
{
	int  nAvcFeedback;
	int  nAvcCommand;
	int  nAvcVoltageError = 0;

	if (AvcAdjustAllowed(ss)) 
	{
		nAvcFeedback = (int)(GetFeedback(FB_SLOT3) & 0x0FFF); // Get Current Avc Feedback
		nAvcFeedback -= ADC_ZERO_OFFSET;							// remove offset
		//////////////////////////////////////////////////////////
		// Voltage = Feedback * 40V / 2048adc
		// Voltage = DAC * 40V / 4000dac
		// Therefore (Feedback * 40V / 2048adc) = (DAC * 40V / 4000dac)
		// Solving for Feedback from DAC yeilds
		// Feedback = (DAC * 2048) / 8000 or (DAC * 64) / 125

		// Get Command and convert to feeback
		nAvcCommand  = ss->wSeqDacValue;					  
		nAvcCommand  *= 64;
		nAvcCommand  /= 125;

		// A positive error equals a DRIVE UP command should be generated
		nAvcVoltageError = nAvcCommand - nAvcFeedback;
//		dprintf("CalculateAvcVoltageError:: nAvcFeedback %d Command %d\n", nAvcFeedback, nAvcCommand);
	}

	return nAvcVoltageError;
}

int CalculateDesiredVoltageChange(const SERVO_VALUES* ss, int nAvcVoltageError)
{
	int nDesiredVoltageChange;
	WORD wResponse;

	wResponse = mResponse(ss->uServoIndex);

	nDesiredVoltageChange = (nAvcVoltageError * wResponse) / 1200;

	return nDesiredVoltageChange;
}

void DefaultAvcServoInit(void)
{
	SetServoCmdBit(STD_AVC_SLOT, SLOT_AVC_JOG,       __FALSE);

	SetServoCmdBit(STD_AVC_SLOT, SLOT_ENABLE, 		 __FALSE);
	SetServoCmdBit(STD_AVC_SLOT, SLOT_IS_VEL_SERVO,	 __FALSE);
	SetServoCmdBit(STD_AVC_SLOT, SLOT_FWD_DIRECTION, __TRUE);
	SetServoCmdBit(STD_AVC_SLOT, (SLOT_USE_DIGITAL1 | SLOT_USE_DIGITAL2), __FALSE);  // Set to Analog feedback
}

//////////////////////////////////////////////////////////////////////
//
// void LimitOscSteering(LPOSC_VALUES ss)
//
//////////////////////////////////////////////////////////////////////
void LimitOscSteering(LPOSC_VALUES ss)
{
	switch(mFeedbackDeviceType(ss->uServoIndex)) 
    {
		case FEEDBACK_BACKEMF: 	// Back EMF feedback
		case FEEDBACK_ANALOG:     // Analog Tachometer
			// Upper Limit checking
			if (ss->nOscCentering > ss->nOscUpperLim) 
			{
				ss->nOscCentering = ss->nOscUpperLim;
//				dprintf("ss->nOscCentering = ss->nOscUpperLim\n");
			}
			
			// Lower Limit checking
			if (ss->nOscCentering < ss->nOscLowerLim) 
			{
				ss->nOscCentering = ss->nOscLowerLim;
//				dprintf("ss->nOscCentering = ss->nOscLowerLim\n");
			}
			break;

		case FEEDBACK_ENCODER:	// Encoder
		default:
			break;
	}
}

//////////////////////////////////////////////////////////////////////
//
// void fnBumpOscCentering(OSC_VALUES *os, int nOscSteerChange)
//
//////////////////////////////////////////////////////////////////////
void fnBumpOscCentering(OSC_VALUES* os, int nOscSteerChange)
{
	os->nOscCentering += nOscSteerChange;
//	printf("fnBumpOscCentering:: OscCentering: %d\n", os->nOscCentering);

	switch(mFeedbackDeviceType(os->uServoIndex)) 
	{
		case FEEDBACK_ENCODER: // Encoder
			// dprintf("fnBumpOscCentering:: OscCentering: %d nOscSteerChange: %d\n", os->nOscCentering, nOscSteerChange);
			break;

		case FEEDBACK_BACKEMF:  // Back EMF feedback
			// Not coded yet
			break;

		case FEEDBACK_ANALOG:  // Analog Tachometer
//			printf("fnBumpOscCentering:: Change: %d\n", nOscSteerChange);
//			printf("fnBumpOscCentering:: nOscCentering: %d\n", os->nOscCentering);
//			printf("fnBumpOscCentering:: Upper: %d Lower: %d\n", os->nOscUpperLim, os->nOscLowerLim);
//			printf("fnBumpOscCentering:: OscFeedback: %d \n", GetOscPosition(os));
			LimitOscSteering(os);
			break;
	}

//	ETHERNET_Print("fnBumpOscCentering:: nOscCentering: %d uSlot: %d\n", os->nOscCentering, uSlot);
}

void OscSpiralFunction(LPOSC_VALUES os)
{
	BYTE uServoIndex;
	int nOscSteerChange;

	if (!os->bSpiralEnable)
		return;  // Spiral not enabled
	
	uServoIndex = os->uServoIndex;
	os->lOscSpiralCount += mOscSpiralInc(uServoIndex);
//	ETHERNET_Print("OscSpiralFunction:: lOscSpiralCount: %08X\n", os->lOscSpiralCount);

	nOscSteerChange = (int)(os->lOscSpiralCount / 0x10000);  // Take only the upper word value
//	ETHERNET_Print("OscSpiralFunction:: lOscSpiralCount: %04X\n", nOscSteerChange);

	if (nOscSteerChange != 0)
	{
		//Use it
		fnBumpOscCentering(os, nOscSteerChange); // Reverse the sign to match what's done with OSC SLEW TO

		// Kill off the upper WORD, retaining the lower bits
		os->lOscSpiralCount -= (long)(nOscSteerChange * 0x10000);
	}
}

//////////////////////////////////////////////////////////////////////
//
// void SetCwTravelDirection(BYTE bDir, SERVO_VALUES const *ss)
//
//////////////////////////////////////////////////////////////////////
static void SetCwTravelDirection(BYTE bDir, SERVO_VALUES const *ss)
{
	if (bDir) 
	{
		SetForwardDirectionBit(__FALSE, ss);
	}
	else 
	{
		SetForwardDirectionBit(__TRUE, ss);
	}
}
	
//////////////////////////////////////////////////////////////////////
//
// void fnTravelDirectionCommand(BYTE)
//
//////////////////////////////////////////////////////////////////////
void fnTravelDirectionCommand(const SERVO_VALUES* ss)
{
//	ETHERNET_Print("fnTravelDirectionCommand:: Command:%d\n", ss->uDirection);
	if (ss->uServoSlot == nMasterTvlSlot)
	{
		nMasterTravelDirection = ss->uDirection;
	}

	switch(ss->uDirection) 
    {
	case LIT_TVL_OFF:
//		dprintf("fnTravelDirectionCommand:: LIT_TVL_OFF\n");
		SetEnableBit(__FALSE, ss);
		SetCwTravelDirection(__FALSE, ss);
		QueueEventMessage(ss->uServoIndex, EV_TVL_MODE_OFF);
		break;
        
	case LIT_TVL_CCW:
//		dprintf("fnTravelDirectionCommand:: LIT_TVL_CCW\n");
		if (ss->bEnabledFlag)
        {
			SetEnableBit(__TRUE, ss);
        }
		SetCwTravelDirection(__FALSE, ss);
		QueueEventMessage(ss->uServoIndex, EV_TVL_MODE_CCW);
		break;
        
	case LIT_TVL_CW:
//		dprintf("fnTravelDirectionCommand:: LIT_TVL_CW\n");
		if (ss->bEnabledFlag)
			SetEnableBit(__TRUE, ss);
		SetCwTravelDirection(__TRUE, ss);
		QueueEventMessage(ss->uServoIndex, EV_TVL_MODE_CW);
		break;
	
     default:
		break;
	}
	SelectWirefeeder();
}

//////////////////////////////////////////////////////////////////////
//
// static void SelectWirefeeder(void)
//
//////////////////////////////////////////////////////////////////////
static void SelectWirefeeder(void)
{
	BYTE uServo;
	SERVO_VALUES* ss;

	for (uServo = 0; uServo < mNumServos; ++uServo) 
	{
		ss = &ServoStruct[uServo]; // Save pointer to this servos struct
		if (ss->uServoFunction != SF_WIREFEED)
			continue;

		switch(nMasterTravelDirection) 
		{
			case LIT_TVL_CCW: // CCW & OFF use the same wirefeered
			case LIT_TVL_OFF:
				if (mMotorSelect(ss->uServoIndex) == WFD_MOTOR_LEADING)
					SelectFirstWirefeeder(__TRUE);
				else
					SelectFirstWirefeeder(__FALSE);
				break;
                
			case LIT_TVL_CW:
				if (mMotorSelect(ss->uServoIndex) == WFD_MOTOR_LEADING)
					SelectFirstWirefeeder(__FALSE);
				else
					SelectFirstWirefeeder(__TRUE);
				break;
		}
	}
}

void SetLevelTime(DWORD dwNewTime)
{
	WeldSystem.dwLevelTime = dwNewTime;
}

//////////////////////////////////////////////////////////////////////
// Macro Replacements
short GetStopDelay(BYTE uServoIndex)
{
	LPIW_ONELEVEL      pOneLevel		= &stInitWeldSched.stOneLevel;
	LPIW_SERVOONELEVEL pServoOneLevel	= &pOneLevel->stServoOneLevel[uServoIndex];

	if (pServoOneLevel->nStopDelay == 0)
		pServoOneLevel->nStopDelay = 1;  // Stp delay must decrement to zero from some number, therefore set it to non-zero.
	
	return pServoOneLevel->nStopDelay;
}
//////////////////////////////////////////////////////////////////////
// NEW FEEDBACK AVERAGING CODE

BOOL NewCommandData(GRAPH_INFO *pGraphInfo)
{
	if (pGraphInfo->wSentCommand != pGraphInfo->wThisCommand)
	{
		return __TRUE;
	}

	return __FALSE;
}

/*
WORD MakeFeedbackFromCommand(GRAPH_INFO *pGraphInfo)
{
	// Make feedback data from Command
	WORD wFeedbackData;

	wFeedbackData = (WORD)(((DWORD)pGraphInfo->wThisCommand * 2048) / 8000);
	wFeedbackData += ADC_ZERO_OFFSET;  // make the data look like it came from the ADC

	return wFeedbackData;
}
*/

void FillArray(GRAPH_INFO *pGraphInfo, WORD wFeedbackData)
{
	BYTE ix;

	// fill array with calculated (projected) feedback
	for(ix = 0; ix < MAX_AVG_ARRAY; ++ix)
	{
		pGraphInfo->wFeedbackArray[ix] = wFeedbackData;
	}

	pGraphInfo->nArrayIndex = 0;
}

void PushFeedback(GRAPH_INFO *pGraphInfo, WORD wFeedbackData)
{
	pGraphInfo->wFeedbackArray[pGraphInfo->nArrayIndex] = wFeedbackData;

	++pGraphInfo->nArrayIndex;
	pGraphInfo->nArrayIndex &= MAX_AVG_ARRAY;  // Roll over the index
}

void Add2Array(GRAPH_INFO *pGraphInfo, WORD wFeedbackData)
{
	if (NewCommandData(pGraphInfo))
	{
		FillArray(pGraphInfo, wFeedbackData);
	}
	else
	{
		PushFeedback(pGraphInfo, wFeedbackData);
	}
}

void CalcFeedbackAverage(GRAPH_INFO *pGraphInfo)
{
	DWORD dwAverage = 0;
	BYTE ix;

	for(ix = 0; ix < MAX_AVG_ARRAY; ++ix)
	{
		dwAverage += pGraphInfo->wFeedbackArray[ix];
	}
	pGraphInfo->wThisFeedback = (WORD)(dwAverage / MAX_AVG_ARRAY);
}

void SetAverageFeedback(GRAPH_INFO *pGraphInfo, WORD wNewFeedback)
{
	Add2Array(pGraphInfo, wNewFeedback);
	CalcFeedbackAverage(pGraphInfo);
}

void BuildGraphInfoMsg(GRAPH_INFO *pGraphInfo)
{
//	static const DWORD dwClearCommandMask  = 0x00000FFF;  // Clear Channel Tag * Current Feedback data MASK
//	static const DWORD dwClearFeedbackMask = 0x0FFF0000;  // Clear Channel Tag * Current Command  data MASK

	// Clear Command & Feedback data fields
	pGraphInfo->dwDataMessage = 0;

	// Or in new command & feedback
	pGraphInfo->dwDataMessage |= ((DWORD)pGraphInfo->wThisCommand & 0x0FFF)  << 16;
	pGraphInfo->dwDataMessage |= ((DWORD)pGraphInfo->wThisFeedback & 0x0FFF) << 0;

	// Or in Tag Value (channel)
	pGraphInfo->dwDataMessage |= pGraphInfo->dwDataTag;
}

void UpdateGraphInfo(GRAPH_INFO *pGraphInfo, WORD wCommandData, WORD wFeedbackData)
{
	if (wCommandData != 0xFFFF)
	{
		pGraphInfo->wThisCommand = wCommandData;
	}

	if (wFeedbackData != 0xFFFF)
	{
		pGraphInfo->wThisFeedback = wFeedbackData;
	}
}


static BOOL UpdateRequired(GRAPH_INFO *pGraphInfo)
{
	static const int COMMAND_DIFF  = 5;
	static const int FEEDBACK_DIFF = 5;
	int nDiff;

	nDiff = (int)pGraphInfo->wSentCommand - (int)pGraphInfo->wThisCommand;
	if (abs(nDiff) > COMMAND_DIFF)
	{
		return __TRUE;
	}

	nDiff = (int)pGraphInfo->wSentFeedback - (int)pGraphInfo->wThisFeedback;
	if (abs(nDiff) > FEEDBACK_DIFF)
	{
		return __TRUE;
	}

	return __FALSE; // No update required
}

static void UpdateSent(GRAPH_INFO *pGraphInfo)
{
	pGraphInfo->wSentCommand  = pGraphInfo->wThisCommand;
	pGraphInfo->wSentFeedback = pGraphInfo->wThisFeedback;
}

static void SendBargraphUpdate(GRAPH_INFO *pGraphInfo, BOOL bForceSend)
{
	if (UpdateRequired(pGraphInfo) || bForceSend)
	{
		BuildGraphInfoMsg(pGraphInfo);
		QueueBarGraphMessage(pGraphInfo->dwDataMessage);
		UpdateSent(pGraphInfo);
	}
}

static BOOL IsFeedbackWithoutCommand(GRAPH_INFO *pGraphInfo)
{
	if (pGraphInfo->wPort == FB_PS_VOLTAGE)
    {
        return __TRUE;
    }

    return __FALSE;
}

static void SendFeedbackWithoutCommand(GRAPH_INFO *pGraphInfo, WORD wNewFeedback)
{
	pGraphInfo->wThisCommand  = 0x07FF;
	pGraphInfo->wThisFeedback = wNewFeedback;

	//if (UpdateRequired(pGraphInfo))
	//{
		BuildGraphInfoMsg(pGraphInfo);
		QueueBarGraphMessage(pGraphInfo->dwDataMessage);
		UpdateSent(pGraphInfo);
	//}
}

// Used for capturing & sending command values
// for use with the TCP/IP Data Stream
static void CaptureDacCommands(WORD wCommand, const SERVO_VALUES* ss)
{
	// Capture DAC only is used with analog feedback
	if (mFeedbackDeviceType(ss->uServoIndex) == FEEDBACK_ANALOG)
	{
		GRAPH_INFO *pGraphInfo = &GraphInfo[ss->uServoIndex];
		if (IsCommandValid(ss))
		{
			if ((ss->uServoFunction != SF_OSC) &&
				(ss->uServoFunction != SF_JOG_ONLY_POS))
			{
				// OSC Servo and Jog Only Position do not use the ZERO VOLT OFFSET (DAC Command)
				// Normal Servo Enabled
				// Scale down the 12 bit DAC Command to 10; Matching the ADC Values
				wCommand = (WORD)(((DWORD)wCommand * 2048) / 8000);
				if (ss->uServoFunction == SF_AVC)
				{
					wCommand *= 2; // Don't understand why this is needed here....
				}
				wCommand += ADC_ZERO_OFFSET;  // make the data look like it came from the ADC
			}
			else
			{
				int nDacCommand = wCommand - ZERO_VOLT_OFFSET; // Result will be Pos or Neg
				// Scale down the 12 bit DAC Command to 10; Matching the ADC Values
				nDacCommand *= (-1); // Command is reversed from feedback
				nDacCommand *= 2048; // Max ADC Counts
				nDacCommand /= 8000; // Max DAC Counts
				nDacCommand += ADC_ZERO_OFFSET;
				wCommand = (WORD)nDacCommand;
			}
		}
		else
		{
			// Special Condition Servo is Disabled, in EnableDelay, or no arc established
			// Create a Command Data from the measured feedback
			wCommand = pGraphInfo->wThisFeedback; // Set to feedback already depostied
		}
		UpdateGraphInfo(pGraphInfo, wCommand, 0xFFFF);
	}

}
