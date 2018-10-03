// UTIL.C : implementation file
//
/* $Log: /Software Engineering/M327/RTData/util.c $
 * 
 * 15    2/14/12 9:27a Wedge
 * 
 * 14    1/26/12 9:39a Edward
 * 
 * 13    12/19/11 1:28p Wedge
 * 
 * 12    12/08/11 9:14a Wedge
 * 
 * 11    11/16/11 5:03p Edward
 * 
 * 10    11/10/11 4:38p Edward
 * 
 * 9     4/26/11 4:04p Edward
 * 
 * 7     3/09/11 6:52p Wedge
 * 
 * 6     4/08/09 10:59a Wedge
 * 
 * 5     3/08/09 9:05a Wedge
 * 
 * 4     2/18/09 7:56p Wedge
 * 
 * 3     2/18/09 4:55p Wedge
 * 
 * 2     2/18/09 2:59p Wedge
 * 
 * 1     2/17/09 3:41p Wedge
 * 
 * 1     2/11/09 8:03a Wedge
 * 
 * 1     1/29/08 10:40a Mikhailm
*/

/*****************************************************************************
*
* FILE NAME:    Util.c
*
* DESCRIPTION:	Utility functions for the project
*
\*****************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

int _nFatalError = 0;

#ifdef DEBUG
/*
void dprintf(const char *format, ...)
{
	char szBuffer[80];
	int NbrOfDataToTransfer;
	int TxCounter = 0;
	va_list ap;
	
	va_start(ap, format); 
	vsprintf(szBuffer, format, ap);
	va_end(ap);
	
	NbrOfDataToTransfer = strlen(szBuffer);
	while(NbrOfDataToTransfer--)
	{
		while(UART_GetFlagStatus(UART0, UART_FLAG_TxFIFOFull) == SET)
			; // Wait for FIFO room
		UART_SendData(UART0, szBuffer[TxCounter++]);
	}
}
 
BOOL dPrintfnow(void)
{
	static int nCount = 0;
	if (++nCount == 50) {
		nCount = 0;
		return __TRUE;
	}
	return __FALSE;
}
*/
#endif



void AmiDelay(DWORD dwTime)
{
	volatile DWORD dwCount;

	dwCount = dwTime;
	while(--dwCount)
	{
		__asm("NOP"); // wait
	}
}


void ASSERT(int nError)
{
	if (nError == 0)
	{
//		printf("Debug Assertion Failure\n");
		_nFatalError = 1;
		STM_EVAL_LEDOff(LED1);
		STM_EVAL_LEDOff(LED2);
		STM_EVAL_LEDOff(LED3);
		STM_EVAL_LEDOff(LED4);
		//while(_nFatalError);
	}
}

