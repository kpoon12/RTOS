//
// sloping.h
//

#ifndef _SLOPING_H
#define _SLOPING_H

WORD fnSlopeValuePrimary(BYTE uServoIndex);
WORD fnSlopeValueBackgnd(BYTE uServoIndex);
WORD fnSlopeValuePrimaryExt(BYTE uServoIndex);
WORD fnSlopeValueBackgndExt(BYTE uServoIndex);

WORD GetPrimaryValue(int uLevel, BYTE uServoNumber);

#endif // _SLOPING_H
