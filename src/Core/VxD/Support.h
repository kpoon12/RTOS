//
// support.h
//

#ifndef _SUPPORT_H
#define _SUPPORT_H

#define __M317_PS__

#include "typedefs.h"
#include "M327.h"
#include "pkeymap.h"
#include "ComLit.h"
#include <stdint.h>
//#include "..\327Main\ComLit.h"

enum {  ULP_VOLTNOTUSED	 = 0,
        ULP_VOLTENABLE	 = 1,
        ULP_VOLTDISABLED = 2 };

typedef struct tagSERVO_VALUES
{
	// Universal values
	BYTE	uServoFunction;	// ALL
	BYTE    uServoIndex;	// ALL
	BYTE	uServoSlot;		// ALL
	BYTE    uServoControl;  // ALL
	BOOL	bEnabledFlag;	// ALL
	BOOL	bSpiralEnable;	// ALL
	BYTE	uPulseMode;		// ALL
	BOOL	bDacUpdate;		// ALL
	WORD	wBuffDacValue;	// ALL
	BOOL	bPendantControl;// ALL
	BYTE	uExternalDac;   // ALL
	short	nUnlockTime;    // ALL

	// Servo specific
	BYTE	uDirection;			// WFD, TVL, OSC
	BOOL	bIsJogging;			// AVC, WFD, TVL
	BOOL    bIsJogging2;		// ONLY FOR WOODWARD TESTING
	BOOL	bRetracting;		// WFD Only
	DWORD	dwOwgDac;			// OSC Only
	short   nOscSlewCount;		// OSC Only
	long    lOscSpiralCount;	// OSC Only
	BYTE    uOscSpecialMode;	// OSC Only
	int		nOscCentering;		// OSC Only
	int		nOscUpperLim;		// OSC Only
	int		nOscLowerLim;		// OSC Only
	int		nJogSpeed;			// Avc, Osc
	int		nJogSpeed2;		// ONLY FOR WOODWARD TESTING

	// Unionized
	WORD	wSeqDacValue;	// Hold the DAC value to return to after jogs
	WORD	wEnableDelay;	// for AvcEnableDelay & PriWfdPulseDelay
	short	nPlaceHolder;
	WORD	wWireInTime;

} SERVO_VALUES;

#define LPSERVO_VALUES	SERVO_VALUES *

typedef struct tagOSC_VALUES
{
	// Universal values
	BYTE	uServoFunction;	// ALL
	BYTE    uServoIndex;	// ALL
	BYTE	uServoSlot;		// ALL
	BYTE    uServoControl;  // ALL
	BOOL	bEnabledFlag;	// ALL
	BOOL	bSpiralEnable;	// ALL
	BYTE	uPulseMode;		// ALL
	BOOL	bDacUpdate;		// ALL
	WORD	wBuffDacValue;	// ALL
	BOOL	bPendantControl;// ALL
	BYTE	uExternalDac;   // ALL
	short	nUnlockTime;    // ALL

	// Servo specific
	BYTE	uDirection;			// WFD, TVL, OSC
	BOOL	bIsJogging;			// AVC, WFD, TVL
	BOOL	bRetracting;		// WFD Only
	DWORD	dwOwgDac;			// OSC Only
	short   nOscSlewCount;		// OSC Only
	long    lOscSpiralCount;	// OSC Only
	BYTE    uOscSpecialMode;	// OSC Only
	int		nOscCentering;		// OSC Only
	int		nOscUpperLim;		// OSC Only
	int		nOscLowerLim;		// OSC Only
	int		nJogSpeed;			// Avc, Osc

	// Unionized
	WORD	wOscAmp;
	WORD	wInDwellTimer;
	WORD	wOtDwellTimer;
	WORD	wExcTimer;

} OSC_VALUES;
#define LPOSC_VALUES	OSC_VALUES *
#define LPCOSC_VALUES	OSC_VALUES const *

typedef struct tagGAIN_DAC_STRUCT
{
	WORD	wForwardGain;
	WORD	wReverseGain;
} GAIN_DAC_STRUCT;
#define LPGAIN_DAC_STRUCT	GAIN_DAC_STRUCT *
#define LPCGAIN_DAC_STRUCT	GAIN_DAC_STRUCT const *

typedef struct tagSERVO_CONST_STRUCT
{
	WORD	wVelocity;
	WORD	wPosition;
} SERVO_CONST_STRUCT;

typedef struct tagSERVO_POSITION_STRUCT
{
	int nServoIndex;
	int nEncoderType;
	int nRevEncoderCnts;											   
	int nCurrentEncoder;
	int nPreviousEncoder;
	int64_t	nDesiredPosition;
	int64_t	nCurrentPosition;
	int64_t	nPreviousPosition;
	int nActualVelocity;
	int	nLastPositionSent;
	int nTolerance;
	int nActualVelocityCnt;
	int nActualVelocitySum;
    int nActualVelocitySumLast;
} SERVO_POSITION_STRUCT;

#define MAX_GRAPH_PORTS		10


/*
typedef struct tagGRAPH_STRUCT
{
	WORD		wPort;
	WORD		wData;
	WORD		wDataTag;
} GRAPH_INFO, * LPGRAPH_INFO;
*/

#define MAX_AVG_ARRAY 0x0F
typedef struct tagGRAPH_STRUCT
{
	WORD		wPort;
	DWORD		dwDataMessage;
	DWORD		dwDataTag;
	// Averaging info
	WORD		wFeedbackArray[MAX_AVG_ARRAY];
	int			nArrayIndex;

	WORD		wThisCommand;
	WORD		wSentCommand;

	WORD		wThisFeedback;
	WORD		wSentFeedback;
} GRAPH_INFO, * LPGRAPH_INFO;

void DriveCommandBit(DWORD dwAddressData, BOOL bState);
BOOL GetCurrentState(DWORD dwAddressData);
BOOL SetServoCmdBit(BYTE uSlot, WORD wMask, BOOL bState);
void SelectManip(BYTE uManipNum);
void SetManipVolts(WORD wJogVoltage, char uManipSpeed);
void SetManipCurrent(WORD wJogCurrent);
void EnableManip(BYTE uManipNum);
void ShutdownManip(BYTE uManipNum);
BOOL ReadManipFault(void);

BOOL GasFlowing(void);
BOOL HomeSwitchShut(int nSlot);
void SetCommandDacValue(short nValue, const SERVO_VALUES* ss);
void SetDesiredAvcResponse(WORD wValue);
void SetAvcResponseDacValue(WORD wValue);
void SetCurrentEnableBit(BOOL bState);
void SetServoType(BYTE, BYTE);
void SetFeedbackType(void);
void SetEnableBit(BOOL bState, const SERVO_VALUES* ss);
void SetBoosterEnableBit(BOOL bState);
void SetBleederDisableBit(BOOL bState);
void SetExternalEnableBit(BOOL bState, const SERVO_VALUES* );
void SetForwardDirectionBit(BOOL bState, const SERVO_VALUES* ss);
BOOL GetDirectionBit(BYTE uServoSlot);
BOOL GetForwardDirectionBit(const SERVO_VALUES* ss);
void ZeroCommandDac(BYTE uServoSlot);
void SetRfStartBit(BOOL bState);
void SetGasValveOpen(BYTE uState);
BOOL PsInRegulation(void);
WORD GetPowerSupplyVoltage(void);
WORD GetVoltageFeedback(void);
BOOL ArcAboveMax(void);
BOOL ArcBelowMin(void);
BOOL AmpsAboveMin(void);
DWORD ReadFootController(void);
void SetLampOn(BOOL bState);
void CheckLampTimer(void);
void ZeroAllDacs(void);
void BufferLcdDisplay(const char *pDisplayText);
const char* GetLedDisplay(void);
void BufferBitmapDisplay(const char *pDisplayText);
const char* GetBitmapDisplay(void);
void ClearLedDisplay(void);
void EraseServoPostionStruct(void);
BOOL OnConnect(void);
BOOL OnDisconnect(void);
void SetArcOnBit(BOOL bState);

void ResetAllControlPorts(BOOL bAllPorts);
void OscSlewToFunction(int nReason);
void ResetAllFaults(void);
void ReadFaults(void);
void HandlePendant(void);
BYTE GetPendantLED(void);
WORD GetFeedback(BYTE uSlot);
int  LookupServoIndex(BYTE uSlot);
int  ReadEncoderFeedback(BYTE uServoSlot);
void CommandBoosterVoltage(int nValue);
BOOL ExternalLine(BOOL *pState, const SERVO_VALUES* ss);
void InSequenceIO(BOOL bInSequence);
BOOL FootControllerInstalled(void);
void SetFanDelay(void);
void DelayedFanOn(void);
void SendServoPosition(BOOL bForceSend);
BOOL QueueMessage(LPQUEUEDATA pqd, BYTE uMsg);
void StartReturnToHome(void);
void BumpAvcResponseDacValue(void);
int  CalculateDesiredVoltageChange(const SERVO_VALUES* ss, int nAvcVoltageError);
int  CalculateAvcVoltageError(const SERVO_VALUES* ss);
void DefaultAvcServoInit(void);
void SetLevelTime(DWORD dwNewTime);
void ReadBarGraphData( void );
void LimitOscSteering(LPOSC_VALUES ss);
void fnBumpOscCentering(LPOSC_VALUES os, int nOscSteerChange);
void OscSpiralFunction(LPOSC_VALUES os);
void fnTravelDirectionCommand(const SERVO_VALUES* ss);
void CommandExternalDacs(WORD wValue, const SERVO_VALUES* ss);
void DriveExternalDacs(WORD wValue, const SERVO_VALUES* ss);
void JogExternalDacs(short nValue, const SERVO_VALUES* ss);
WORD InFromPort(WORD wPort);
void UserIO_Test(void);  // TEST ONLY
void RF_Test(void);		 // TEST RF ONLY
void DigitalOutput_Test(int wSetbit);  // TEST ONLY
void DigitalInput_Test(void);  // TEST ONLY
void AnalogOutput_Test(WORD wChannel, WORD wTempValue); //TEST ONLY
WORD AnalogInput_Test(int ix); //TEST ONLY
void ReadAcMainsData(void);
void UpdateGraphInfo(GRAPH_INFO *pGraphInfo, WORD wCommandData, WORD wFeedbackData);
int  ExtOscSteerAdc(const SERVO_VALUES *ss);
void PopBreaker(BOOL bEmergency);
void SendPendantSelection(void);
//////////////////////////////////////////
// Marco Replacements
short GetStopDelay(BYTE uServoIndex);

BOOL StartUp(void);
BOOL PowerDown(void);

//#define GROUND_FAULT_TIME		420  // Number of interrupts required to check for ground fault
#define MINIMUM_PREPURGE_TIME	400  // Number of interrupts required to charge the booster cap

// Servo SPI Feedbacks
#define FB_SLOT_OFFSET	0x80	// Usage: ServoSlot + FB_SLOT_OFFSET = Feedback Slot
#define FB_SLOT0		0x80
#define FB_SLOT1		0x81	// TRAVEL_FBK		10A
#define FB_SLOT2		0x82	// WIRE_FBK			13A
#define FB_SLOT3		0x83	// AVC_FBK			108
#define FB_SLOT4		0x84	// OSC_FBK			138
#define FB_SLOT5		0x85	// TILT_FBK			106
#define FB_SLOT6        0x86	// L/LAG_FBK		136
#define FB_SLOT7		0x87	// POSX_FBK			104
#define FB_SLOT8        0x88	// POSY_FBK			134

// Unused ADC Channels
#define FB_ADCH1    0x05	// AD_CH1			10C
#define FB_ADCH2    0x06	// AD_CH2			10E
#define FB_ADCH3    0x07	// AD_CH3			110
#define FB_ADCH4    0x08	// AD_CH4			112
#define FB_ADCH5    0x09	// AD_CH5			114
#define FB_ADCH6    0x0A	// AD_CH6			116
//#define FB_UNUSED_0B    0x0B	// GROUND			118
//#define FB_UNUSED_0C    0x0C	// GROUND			11A
//#define FB_UNUSED_0D    0x0D	// GROUND			11C
//#define FB_UNUSED_0E    0x0E	// GROUND			11E
//#define FB_UNUSED_0F    0x0F	// GROUND			120
//#define FB_UNUSED_14    0x14	// GROUND			12A
//#define FB_UNUSED_15    0x15	// GROUND			12C
//#define FB_UNUSED_16    0x16	// GROUND			12E
//#define FB_UNUSED_1D    0x1D	// GROUND			13C
//#define FB_UNUSED_1E    0x1E	// GROUND			13E
//#define FB_UNUSED_1F    0x1F // DO NOT USE THIS ADDRESS!!!!!!!!!!


// Arc Volt fault literals
//    0 adc = -40.0 arc volts
// 4095 adc = +40.0 arc volts
//TEST
//#define MIN_ARC_VOLTAGE		((int) 153 + (int)ADC_ZERO_OFFSET) //  3.0 Volts
#define MIN_ARC_VOLTAGE		((int) 76 + (int)ADC_ZERO_OFFSET) //  3.0 Volts
#define MAX_ARC_VOLTAGE		((int)1280 + (int)ADC_ZERO_OFFSET) // 25.0 Volts
#define GF_VOLTAGE10  		((int) 512 + (int)ADC_ZERO_OFFSET) // 10.0 Volts
#define GF_VOLTAGE18  		((int) 921 + (int)ADC_ZERO_OFFSET) // 18.0 Volts
#define SP_VOLTAGEP25  		((int)  51 + (int)ADC_ZERO_OFFSET) // 0.25 Volts
#define SP_VOLTAGE0  		((int)   0 + (int)ADC_ZERO_OFFSET) // 0.00 Volts
#define SP_VOLTAGENEGP25	((int) -51 + (int)ADC_ZERO_OFFSET) // -0.25 Volts



// Servo Switch defines
//#define HOME_SWITCH	00
//#define SYNC_LINE		01
//#define LIMIT_SWITCH	02

// Pendant Key Literals
#define PENDANT_PORT_ADDR		0x014E
#define AD_MEM_BASE_PORT_ADDR	0x0102

#define PENDANT_RPT				(1<<10)
#define PENDANT_DA				(1<<11)
#define LED_ADDRESS  			0x1F00

#define DESELECT_ALL			0x00
#define ANALOG_INPUT_ADC		0x01 // SPI Select ADC Slave Devices
#define CURRENT_SERVO_ADC		0x02 // SPI Select ADC Slave Devices
#define EXT_ANALOG_INPUT_ADC	0x03 // SPI Select ADC Slave Devices
#define AC_MAINS_BOARD			0x04
#define ANALOG_OUTPUT_DAC		0x05 // SPI Select DAC Slave Devices
#define CURRENT_SERVO_DAC		0x06 // SPI Select DAC Slave Devices

// Channel Address Bit for next conversion
#define CHANNEL_0		       0x00
#define CHANNEL_1		       0x01
#define CHANNEL_2		       0x02
#define CHANNEL_3              0x03

/*
// FROM SPI.H
#define ANALOG_INPUT_ADC		0x01 // SPI Select ADC Slave Devices
#define CURRENT_SERVO_ADC		0x02 // SPI Select ADC Slave Devices
#define EXT_ANALOG_INPUT_ADC	0x03 // SPI Select ADC Slave Devices
*/

#endif // _SUPPORT_H
