//
// Manip.c
//

// Local include files
#include "Manip.h"

BYTE  bManipCurrentLimitFault[MAX_NUMBER_OF_MANIPS];
BOOL  _isManipEnabled;
BYTE  uActiveManip;
char  _uManipSpeed;

extern void DriveCommandBit(DWORD dwAddressData, BOOL bState);

// Drive the "select" lines to 
// select the manip channel
void SelectManip(BYTE uManipNum)
{
	switch(uManipNum)
	{
		case 0:
			DriveCommandBit(MANIP_AD0, __FALSE);
			DriveCommandBit(MANIP_AD1, __FALSE);
			break;
		case 1:
			DriveCommandBit(MANIP_AD0, __TRUE);
			DriveCommandBit(MANIP_AD1, __FALSE);
			break;
		case 2:
			DriveCommandBit(MANIP_AD0, __FALSE);
			DriveCommandBit(MANIP_AD1, __TRUE);
			break;
		case 3:
			DriveCommandBit(MANIP_AD0, __TRUE);
			DriveCommandBit(MANIP_AD1, __TRUE);
			break;
	}
}

// Set the direction for the Manip Number
// Speed = -100% to +100%
void SetManipDirection(char uManipSpeed)
{
	_uManipSpeed = uManipSpeed;
	if (uManipSpeed > 0)
		DriveCommandBit(MANIP_DIR, __TRUE);
	else
		DriveCommandBit(MANIP_DIR, __FALSE);
}

// Enable the Manipulator
void EnableManip(BYTE uManipNum)
{
	SelectManip(uManipNum);			  // Then select a manipulator (which does the outport)
	_isManipEnabled = __TRUE;
	DriveCommandBit(MANIP_ENABLE, __TRUE);
}

// Set the Voltage & current DACs to zero
// Set the Enable to __FALSE
void ShutdownManip(BYTE uManipNum)
{
	DriveCommandBit(MANIP_ENABLE, __FALSE);
}

BOOL ReadManipFault(void)
{
	BOOL isManipFaulted = __FALSE;

//	isManipFaulted |= GetCurrentState(MAN_IN1);
//	isManipFaulted |= GetCurrentState(MAN_IN2);
//	isManipFaulted |= GetCurrentState(MAN_IN3);
//	isManipFaulted |= GetCurrentState(MAN_IN4);

	return isManipFaulted;
}

