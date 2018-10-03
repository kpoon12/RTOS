//
// sloping.c
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Local include files
#include "typedefs.h"
#include "pkeymap.h"
#include "InitWeldStruct.h"

// Local include files
#include "weldloop.h"
#include "sloping.h"

//////////////////////////////////////////////////////////////////////
//
// G L O B A L   V A R I A B L E S
//
//////////////////////////////////////////////////////////////////////






//////////////////////////////////////////////////////////////////////
//
// E X T E R N A L   V A R I A B L E S
//
//////////////////////////////////////////////////////////////////////
extern BYTE				uSystemWeldPhase;	// in Support.c
extern IW_INITWELD		stInitWeldSched;	// in Support.c
extern SYSTEM_STRUCT	WeldSystem;

long CalculateSlopedDeltaValue(LPWORD pwThisValue, LPWORD pwNextValue)
{
	long lDeltaValue = 0;

	switch(uSystemWeldPhase) 
	{
		case WP_UPSLOPE:
			lDeltaValue = *pwThisValue;
			*pwThisValue = 0; // Cause a slope from 0
			break;
		
		case WP_LEVELWELD:
			lDeltaValue = *pwNextValue - *pwThisValue;
			break;
		
		case WP_DOWNSLOPE:
			lDeltaValue = ((long)*pwThisValue) * (-1L);  // Inverse of current value to cause a slope to zero
			break;
	}
	
	return lDeltaValue;
}

WORD GetPrimaryValue(int uLevel, BYTE uServoNumber)
{
	LPIW_SERVOMULTILEVEL pServoMultiLevel;

	if (uLevel > mLastLevel)
		return 0;  // Bad Level

	pServoMultiLevel = &stInitWeldSched.stMultiLevel[uLevel].stServoMultiLevel[uServoNumber];
	return pServoMultiLevel->wPrimaryValue;
}

WORD GetPrimaryValueExt(int uLevel, BYTE uServoNumber)
{
	LPIW_SERVOMULTILEVEL pServoMultiLevel;

	if (uLevel > mLastLevel)
		return 0;  // Bad Level

	pServoMultiLevel = &stInitWeldSched.stMultiLevel[uLevel].stServoMultiLevel[uServoNumber];
	return pServoMultiLevel->dwPriExternalDac;
}

WORD GetBackgndValue(int uLevel, BYTE uServoNumber)
{
	LPIW_SERVOMULTILEVEL pServoMultiLevel;

	if (uLevel > mLastLevel)
		return 0;  // Bad Level

	pServoMultiLevel = &stInitWeldSched.stMultiLevel[uLevel].stServoMultiLevel[uServoNumber];
	return (WORD)pServoMultiLevel->nBackgroundValue;
}

WORD GetBackgndValueExt(int uLevel, BYTE uServoNumber)
{
	LPIW_SERVOMULTILEVEL pServoMultiLevel;

	if (uLevel > mLastLevel)
		return 0;  // Bad Level

	pServoMultiLevel = &stInitWeldSched.stMultiLevel[uLevel].stServoMultiLevel[uServoNumber];
	return (WORD)pServoMultiLevel->dwBacExternalDac;
}

//////////////////////////////////////////////////////////////////////
//
// WORD  fnSlopeValue(WORD wThisValue, WORD wNextValue)
//
//////////////////////////////////////////////////////////////////////
WORD fnSlopeValue(WORD wThisValue, WORD wNextValue)
{
	int nDeltaValue;
	short wSlopedValue;
	WORD wUpperBits;

	// This function will calculate the delta between This and Next
	// It will also modify the This value when in Upslope
	nDeltaValue = CalculateSlopedDeltaValue(&wThisValue, &wNextValue);

	wUpperBits = (WORD)(WeldSystem.Slope.dwSlopeDac >> 16);
	wSlopedValue = (short)((nDeltaValue * wUpperBits) / (int)0x07FFF);

	return (WORD)(wThisValue + wSlopedValue);
}

WORD fnSlopeValuePrimary(BYTE uServoIndex)
{
	WORD wThis, wNext;

	wThis = GetPrimaryValue(WeldSystem.uWeldLevel + 0, uServoIndex);
	wNext = GetPrimaryValue(WeldSystem.uWeldLevel + 1, uServoIndex);

	return fnSlopeValue(wThis, wNext);
}

WORD fnSlopeValueBackgnd(BYTE uServoIndex)
{
	WORD wThis, wNext;

	wThis = GetBackgndValue(WeldSystem.uWeldLevel + 0, uServoIndex);
	wNext = GetBackgndValue(WeldSystem.uWeldLevel + 1, uServoIndex);

	return fnSlopeValue(wThis, wNext);
}

WORD fnSlopeValuePrimaryExt(BYTE uServoIndex)
{
	WORD wThis, wNext;

	wThis = GetPrimaryValueExt(WeldSystem.uWeldLevel + 0, uServoIndex);
	wNext = GetPrimaryValueExt(WeldSystem.uWeldLevel + 1, uServoIndex);

	return fnSlopeValue(wThis, wNext);
}

WORD fnSlopeValueBackgndExt(BYTE uServoIndex)
{
	WORD wThis, wNext;

	wThis = GetBackgndValueExt(WeldSystem.uWeldLevel + 0, uServoIndex);
	wNext = GetBackgndValueExt(WeldSystem.uWeldLevel + 1, uServoIndex);

	return fnSlopeValue(wThis, wNext);
}

