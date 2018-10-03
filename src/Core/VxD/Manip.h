//
// support.h
//

#ifndef __MANIP_H__
#define __MANIP_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>

#include "typedefs.h"
#include "GPIO.h"

// Manipulator current limit fault literals
#define NO_FAULT			0
#define FWD_DIRECTION		1
#define REV_DIRECTION		2

void SelectManip(BYTE uManipNum);
void SetManipDirection(char uManipSpeed);
void EnableManip(BYTE uManipNum);
void ShutdownManip(BYTE uManipNum);
BOOL ReadManipFault(void);
BOOL ManipFaultInJogDirection(void);
void CheckManipFaults(void);

#endif // __MANIP_H__
