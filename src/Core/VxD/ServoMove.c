//
// ServoMove.c
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Local include files
#include "typedefs.h"
#include "M327.h"
#include "ComLit.h"
#include "InitWeldStruct.h"

// Local include files
#include "ServoCom.h"
#include "weldloop.h"
#include "Support.h"
#include "ServoMove.h"

//////////////////////////////////////////////////////////////////////
//
// Global variables defined outside this source module.
//
//////////////////////////////////////////////////////////////////////
extern SERVO_POSITION_STRUCT ServoPosition[MAX_NUMBER_SERVOS];
extern SYSTEM_STRUCT	     WeldSystem;
extern IW_INITWELD			 stInitWeldSched;	// in Support.c
extern SERVO_CONST_STRUCT    ServoConstant[MAX_NUMBER_SERVOS];
extern int    				 nUlpVoltFaultState;   // in Weldloop.c

BOOL dprintfnow(void)
{
	static int nCount = 0;

	if (++nCount == 50) 
	{
		nCount = 0;
		return __TRUE;
	}

	return __FALSE;
}

void InitServoConstants(void)
{
	BYTE ix;

	for (ix = 0; ix < MAX_NUMBER_SERVOS; ++ix) 
	{
		ServoConstant[ix].wPosition = DEFAULT_POS_ERROR;
		ServoConstant[ix].wVelocity = DEFAULT_VEL_ERROR;
	}
}

//////////////////////////////////////////////////////////////////////
//
// void FixDacOverrun(int* pDacValue, int nLimit)
//
//////////////////////////////////////////////////////////////////////
static void FixDacOverrun(int* pDacValue, int nLimit)
{
	if (*pDacValue > nLimit)
		*pDacValue = nLimit;

	nLimit *= (-1);

	if (*pDacValue < nLimit)
		*pDacValue = nLimit;
}

//////////////////////////////////////////////////////////////////////
//
// void fnDriveOscServoOpenLoop(const SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
static void fnDriveOscServoOpenLoop(const SERVO_VALUES* ss)
{
	SERVO_POSITION_STRUCT *sps;

	int nPositionError;
	int nDesiredVelocity;
	int nVelocityError;
	int nOutputVelocity;
	int nSlot;

	nSlot = ss->uServoSlot;
	sps = &ServoPosition[nSlot]; // SPS is Servo Slot based
	nPositionError = (int)(sps->nDesiredPosition - sps->nCurrentPosition);
	nDesiredVelocity = nPositionError;
	nVelocityError   = nDesiredVelocity - sps->nActualVelocity;
	nOutputVelocity  = (nPositionError * ServoConstant[nSlot].wPosition) + (nVelocityError * ServoConstant[nSlot].wVelocity);
	nOutputVelocity /= POSVEL_DIVISOR;
    FixDacOverrun(&nOutputVelocity, 2000);

	// Line up direction bit, and change velocity sign if needed
	if (nOutputVelocity < 0) 
	{
		SetForwardDirectionBit(__FALSE, ss);
		nOutputVelocity *= (-1);
	}
	else 
	{
		SetForwardDirectionBit(__TRUE, ss);
	}

	SetCommandDacValue((short)nOutputVelocity, ss);
}

//////////////////////////////////////////////////////////////////////
//
// void fnDriveServoClosedLoop(SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
static void fnDriveServoClosedLoop(SERVO_VALUES* ss)
{
	if (ss->bDacUpdate) 
    {
		SetCommandDacValue((short)ss->wBuffDacValue, ss);
		ss->bDacUpdate = __FALSE;
	}
}

//////////////////////////////////////////////////////////////////////
//
// void fnDriveVelocityServoOpenLoop(const SERVO_VALUES* ss)
//
//////////////////////////////////////////////////////////////////////
static void fnDriveVelocityServoOpenLoop(const SERVO_VALUES* ss)
{
	SERVO_POSITION_STRUCT *sps;

	static int nPrevDriveCommand = 0;
	int nPositionError;
	int nDesiredVelocity;
	int nVelocityError;
	int nOutputVelocity;
	
	int nSlot;

	nSlot = ss->uServoSlot;
	sps = &ServoPosition[nSlot]; // SPS is Servo Slot based
	nPositionError = (sps->nDesiredPosition / DESIRED_POS_DIVISOR) - sps->nCurrentPosition;
	nDesiredVelocity = nPositionError;
	nVelocityError   = nDesiredVelocity - sps->nActualVelocity;
	nOutputVelocity  = (nPositionError * ServoConstant[nSlot].wPosition) + (nVelocityError * ServoConstant[nSlot].wVelocity);
	nOutputVelocity /= POSVEL_DIVISOR ;
    FixDacOverrun(&nOutputVelocity, 4000);

//	if (nOutputVelocity)
//		dprintf("CommandOsc: nOutputVelocity: %d\n", nOutputVelocity);

	// Line up direction bit, and change velocity sign if needed
	if (nOutputVelocity < 0) 
	{
		SetForwardDirectionBit(__FALSE, ss);
		nOutputVelocity *= (-1);
	}
	else 
	{
		SetForwardDirectionBit(__TRUE, ss);
	}
	
	if (nPrevDriveCommand != nOutputVelocity)
	{
		nOutputVelocity = nPrevDriveCommand + (nOutputVelocity - nPrevDriveCommand) / 4;
		nPrevDriveCommand = nOutputVelocity;
	};

	SetCommandDacValue((short)nOutputVelocity, ss);
}

//////////////////////////////////////////////////////////////////////
//
// void fnCommandOscServoClosedLoop(OSC_VALUES *ss, int nOscCommand)
//
//////////////////////////////////////////////////////////////////////
void fnCommandOscServoClosedLoop(OSC_VALUES* ss, int nOscCommand)
{
	if (nOscCommand < MAX_INNER_POSITION)
		nOscCommand = MAX_INNER_POSITION;

	if (nOscCommand > MAX_OUTER_POSITION)
		nOscCommand = MAX_OUTER_POSITION;

	// Output the drive command
	ss->wBuffDacValue = (WORD)nOscCommand;
	SetCommandDacValue((short)ss->wBuffDacValue, (SERVO_VALUES*)ss);
}

//////////////////////////////////////////////////////////////////////
//
// void fnCommandOscServoOpenLoop(const OSC_VALUES* ss, int64_t nOscCommand)
//
//////////////////////////////////////////////////////////////////////
void fnCommandOscServoOpenLoop(const OSC_VALUES* ss, int64_t nOscCommand)
{
	SERVO_POSITION_STRUCT *sps;

	sps = &ServoPosition[ss->uServoSlot];  // SPS is Servo Slot based
	sps->nDesiredPosition = nOscCommand;
}

static BYTE XlatForwardDirection(const SERVO_VALUES* ss, BOOL bForward)
{
	BYTE uReturnCode = 0;

	if (mDirectionReversed(ss->uServoIndex))
		bForward = (BOOL)!bForward;

	switch(ss->uServoFunction)
	{
		case SF_TRAVEL:
		case SF_JOG_ONLY_VEL:
			if (bForward)
				uReturnCode = LIT_TVL_CW;
			else
				uReturnCode = LIT_TVL_CCW;
			break;

		default:
			if (bForward)
				uReturnCode = WIRE_DIR_RETR;
			else
				uReturnCode = WIRE_DIR_FEED;
			break;
	}

	return uReturnCode;
}

BOOL fnIsForwardDirection(const SERVO_VALUES* ss)
{
	BOOL bForward = __FALSE;

	if ((ss->uServoFunction == SF_TRAVEL) || (ss->uServoFunction == SF_JOG_ONLY_VEL)) 
    {
		switch(ss->uDirection) 
        {
			case LIT_TVL_CCW:
			case LIT_TVL_OFF:
				bForward = __TRUE;
				break;

			default:
			case LIT_TVL_CW:
				break;
		}
	}

	if (ss->uServoFunction == SF_WIREFEED) 
    {
		switch(ss->uDirection) 
        {
			case WIRE_DIR_FEED:
				bForward = __TRUE;
				break;

			default:
			case WIRE_DIR_RETR:
				break;
		}
	}

	if (mDirectionReversed(ss->uServoIndex))
    {
		bForward = (BOOL)!bForward;
    }

	return bForward;
}

void fnBumpAvcDesiredPos(int nMoveAmount)
{
	SERVO_POSITION_STRUCT *sps;

	sps = &ServoPosition[STD_AVC_SLOT]; // Hard coded for today
	sps->nDesiredPosition -= (int64_t)nMoveAmount;
}

void fnCommandVelocityServoOpenLoop(const SERVO_VALUES *ss)
{
	SERVO_POSITION_STRUCT *sps;

	sps = &ServoPosition[ss->uServoSlot];  // SPS is Servo Slot based
	
	if (ss->bEnabledFlag || ss->bIsJogging) 
	{
		int nValue = (int)ss->wBuffDacValue; 
		
		if (!fnIsForwardDirection(ss))
		{
			nValue *= (-1); // Invert the distance
		}

		if (nValue != 0)
		{
			sps->nDesiredPosition += (int64_t)nValue;
			if (ss->uServoControl == SC_ULPHEAD)
			{
				fnBumpAvcDesiredPos(nValue);
			}
		}
	}
}

void fnDriveAvcServoOpenLoopULP(SERVO_VALUES* ss)
{
	SERVO_POSITION_STRUCT* sps;

	int nPositionError;
	int nDesiredVelocity;
	int nVelocityError;
	int nOutputVelocity;
	BYTE uSlot;
//	int nAvcCorrection;
	WORD wResponseDacValue;

	uSlot = ss->uServoSlot;
	sps = &ServoPosition[uSlot]; // SPS is Servo Slot based

	if (ss->bIsJogging2)
	{
		sps->nDesiredPosition -= (int64_t)(ss->nJogSpeed2 * DESIRED_POS_DIVISOR);
	}

	nPositionError = (int)((sps->nDesiredPosition / DESIRED_POS_DIVISOR) - sps->nCurrentPosition);
	nDesiredVelocity = nPositionError;
	nVelocityError   = nDesiredVelocity - sps->nActualVelocity;
	nOutputVelocity  = (nPositionError * ServoConstant[uSlot].wPosition) + (nVelocityError * ServoConstant[uSlot].wVelocity);
	nOutputVelocity /= POSVEL_DIVISOR;
//	nOutputVelocity /= (POSVEL_DIVISOR * 4);
    FixDacOverrun(&nOutputVelocity, 4000);

//	DumpData(sps);

	if (nOutputVelocity < 0)
	{
		SetCommandDacValue((short)AVC_POS_JOG, ss);
	}
	else
	{													 
		SetCommandDacValue((short)AVC_NEG_JOG, ss);
	}

	wResponseDacValue = abs(nOutputVelocity);
	if (wResponseDacValue > 3000)
	{
		wResponseDacValue = 3000;
	}

//	if (dprintfnow(ss->uServoSlot, 80))
//	{
//		printf("DrvOpenLoop::%d\n", ss->uServoSlot);
//		printf("nActualVelocity:%+04d\n\n", wResponseDacValue);
//	}

	SetServoCmdBit(ss->uServoSlot, SLOT_AVC_JOG, __TRUE);
	SetEnableBit(__TRUE, ss);				
	SetAvcResponseDacValue(wResponseDacValue);
	ss->bIsJogging = __FALSE;				
}

BOOL AvcAdjustAllowed(const SERVO_VALUES* ss)
{
	if (ss->bIsJogging)
		return __FALSE; // No voltage error during jogs
	
	if (!WeldSystem.bArcEstablished) 
		return __FALSE; // No voltage error without an arc established

	if (!ss->bEnabledFlag)
		return __FALSE; // Avc is disabled
	
	if (ss->wEnableDelay != 0)
		return __FALSE; // Avc is disabled

	// AVC servo is enabled 
	return __TRUE;
}

void fnCommandAvcServoULP(SERVO_VALUES* ss)
{
	static BOOL bisArcOn = __FALSE;

	if (AvcAdjustAllowed(ss))
	{
		if (!bisArcOn)
		{
			nUlpVoltFaultState = ULP_VOLTENABLE;
			SetServoCmdBit(ss->uServoSlot, SLOT_AVC_JOG, __FALSE);
			SetAvcResponseDacValue(mResponse(ss->uServoIndex));
			ss->bIsJogging = __FALSE;
			
			if (ss->bEnabledFlag && (ss->wEnableDelay == 0))
			{
				SetEnableBit(__TRUE, ss);
			}
			
			bisArcOn = __TRUE;
	    }

		fnDriveServoClosedLoop(ss);
	}
	else
	{
		if (bisArcOn)
		{
			SERVO_POSITION_STRUCT* sps;
			
			nUlpVoltFaultState = ULP_VOLTDISABLED;
			sps = &ServoPosition[ss->uServoSlot]; // SPS is Servo Slot based
			sps->nDesiredPosition  = (sps->nCurrentPosition * DESIRED_POS_DIVISOR);
			sps->nPreviousPosition = sps->nCurrentPosition;
			
			SetEnableBit(__TRUE, ss);
			SetAvcResponseDacValue(2000);
			
			bisArcOn = __FALSE;
		}
		fnDriveAvcServoOpenLoopULP(ss);
	}
}


void fnCommandAvcServo(SERVO_VALUES* ss)
{
	switch(mFeedbackDeviceType(ss->uServoIndex)) 
    {
		case FEEDBACK_ENCODER: // Encoder
			break;

		case FEEDBACK_BACKEMF:  // Back EMF feedback
			// Not coded yet
			break;

		case FEEDBACK_ANALOG:  // Analog Tachometer
			if (ss->uServoControl == SC_CLOSEDLOOP)
			{
				fnDriveServoClosedLoop(ss);  // Standard servo operation
			}
			else if (ss->uServoControl == SC_ULPHEAD)
			{
				fnCommandAvcServoULP(ss);  // Special for ULP Head
			}
			else
			{
				// Nothing to do....Shouldn't hit here
			}
			break;
	}
}


void fnCommandOscServo(OSC_VALUES *ss)
{           
	int  wHalfOscAmp;
	WORD  wOwgMagnitude;
	DWORD dwOscTempProduct;
	int   nOscPartial;
	int   nOscCommand;

	if (ss->bIsJogging)
		fnBumpOscCentering(ss, ss->nJogSpeed);

	wHalfOscAmp      = ss->wOscAmp / 2;
	wOwgMagnitude    = (WORD)(ss->dwOwgDac >> 16);
	dwOscTempProduct = (DWORD)ss->wOscAmp * wOwgMagnitude;
    nOscPartial      = (int)((dwOscTempProduct / 1000) & 0x0000FFFF);
	nOscCommand      = (int)(nOscPartial + ss->nOscCentering - wHalfOscAmp);

	switch(mFeedbackDeviceType(ss->uServoIndex)) 
    {
		case FEEDBACK_ENCODER: // Encoder
			fnCommandOscServoOpenLoop(ss, nOscCommand);
			fnDriveOscServoOpenLoop((SERVO_VALUES*)ss);
			break;

		case FEEDBACK_BACKEMF:  // Back EMF feedback
			// Not coded yet
			break;

		case FEEDBACK_ANALOG:  // Analog Tachometer
			//nOscCommand -= 4000;  // Fixup to coverto to +/- 4000
			fnCommandOscServoClosedLoop(ss, nOscCommand);
			break;
	}
}


void fnCommandVelocityServo(SERVO_VALUES* ss)
{
	switch(ss->uServoControl) 
    {
		case SC_OPENLOOP:
		case SC_ULPHEAD:
			// Drives servo into position based on desired position
			fnCommandVelocityServoOpenLoop(ss);	
			fnDriveVelocityServoOpenLoop(ss);
			break;

		case SC_CLOSEDLOOP:
			fnDriveServoClosedLoop(ss);
			break;
	}
}


void fnCommandServo(SERVO_VALUES* ss)
{
	switch(ss->uServoFunction) 
    {
		case SF_OSC:
		case SF_JOG_ONLY_POS:
			fnCommandOscServo((OSC_VALUES *)ss);
			break;

		case SF_AVC:
			fnCommandAvcServo(ss);
			break;

		default:
			fnCommandVelocityServo(ss);
			break;
	}
}


void JogVelocityServoClosed(SERVO_VALUES* ss, int nServoSpeed)
{
	if (nServoSpeed == 0) 
	{		
		// Stop Jogging
		ss->bIsJogging = __FALSE;
		ss->wBuffDacValue = ss->wSeqDacValue;
		ss->bDacUpdate = __TRUE;
		SetEnableBit(ss->bEnabledFlag, ss);
		if (ss->uServoFunction == SF_TRAVEL)
			fnTravelDirectionCommand(ss);
		else
			SetForwardDirectionBit((BOOL)ss->uDirection, ss);
//		dprintf("JogVelocityServo:: Slot: %d nServoSpeed: %d\n",ss->uServoSlot, nServoSpeed);
		return;
	}
	else 
	{
		// Start Jogging
		ss->bIsJogging = __TRUE;
		if (nServoSpeed > 0) 
		{
			ss->wBuffDacValue = (WORD)((mJogSpeedForward(ss->uServoIndex) * nServoSpeed) / 100);
			SetForwardDirectionBit(__TRUE, ss);
		}
		else 
		{
			//nServoSpeed *= (-1);
			ss->wBuffDacValue = (WORD)((mJogSpeedReverse(ss->uServoIndex) * nServoSpeed) / 100);
			SetForwardDirectionBit(__FALSE, ss);
		}

//		dprintf("JogVelocityServo:: FwdSpeed: %d RevSpeed: %d\n", mJogSpeedForward(ss->uServoIndex), mJogSpeedReverse(ss->uServoSlot));
//		dprintf("JogVelocityServo:: Slot: %d nServoSpeed: %d\n",ss->uServoSlot, nServoSpeed);
//		dprintf("JogVelocityServo:: DacValue: %04X\n", ss->wBuffDacValue);

		SetEnableBit(__TRUE, ss);
		ss->bDacUpdate = __TRUE;
	}
}


void JogVelocityServoOpen(SERVO_VALUES* ss, int nServoSpeed)
{	 		
	if (nServoSpeed == 0) 
	{			
		// Stop Jogging
		ss->bIsJogging = __FALSE;
		ss->wBuffDacValue = ss->wSeqDacValue;
		ss->bDacUpdate = __TRUE;
		SetEnableBit(ss->bEnabledFlag, ss);
		ss->uDirection = mDirection(ss->uServoIndex);  // Restore normal direction
		if (ss->uServoFunction == SF_TRAVEL)
        {
			fnTravelDirectionCommand(ss);
        }
		else
        {
			SetForwardDirectionBit((BOOL)ss->uDirection, ss);
        }
//		dprintf("JogVelocityServo:: Slot: %d nServoSpeed: %d\n",ss->uServoSlot, nServoSpeed);
		return;
	}
	else 
	{
		// Start Jogging
		ss->bIsJogging = __TRUE;
		if (nServoSpeed > 0) 
		{
			int nJogSpeed = mJogSpeedForward(ss->uServoIndex);  // JogSpeed contains DESIRED_POS_DIVISOR
			ss->wBuffDacValue = (WORD)((nJogSpeed * abs(nServoSpeed)) / 100); // (160 counts / interrupt) * 512
			ss->uDirection = XlatForwardDirection(ss, __FALSE);
		}
		else 
		{
			int nJogSpeed = mJogSpeedReverse(ss->uServoIndex);  // JogSpeed contains DESIRED_POS_DIVISOR
			ss->wBuffDacValue = (WORD)((nJogSpeed * abs(nServoSpeed)) / 100); // (160 counts / interrupt) * 512
			ss->uDirection = XlatForwardDirection(ss, __TRUE);
		}
		SetForwardDirectionBit((BOOL)ss->uDirection, ss);

//		dprintf("JogVelocityServo:: FwdSpeed: %d RevSpeed: %d\n", mJogSpeedForward(ss->uServoIndex), mJogSpeedReverse(ss->uServoSlot));
//		dprintf("JogVelocityServo:: Slot: %d nServoSpeed: %d\n",ss->uServoSlot, nServoSpeed);
//		dprintf("JogVelocityServo:: DacValue: %04X\n", ss->wBuffDacValue);

		SetEnableBit(__TRUE, ss);
		ss->bDacUpdate = __TRUE;
	}
}


void JogVelocityServo(SERVO_VALUES* ss, int nServoSpeed)
{
    switch(ss->uServoControl)
    {
        case SC_OPENLOOP:
        case SC_ULPHEAD:
            JogVelocityServoOpen(ss, nServoSpeed);
            break;
        
        case SC_CLOSEDLOOP:
            JogVelocityServoClosed(ss, nServoSpeed);
            break;
    }
}

