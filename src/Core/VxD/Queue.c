//
// Queue.c
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Local include files
#include "typedefs.h"
#include "M327.h"
#include "ComLit.h"

// Local include files
#include "Weldloop.h"
#include "Support.h"
#include "Queue.h"
#include "util.h"


//////////////////////////////////////////////////////////////////////
//
// E X T E R N A L   V A R I A B L E S
//
//////////////////////////////////////////////////////////////////////

extern SYSTEM_STRUCT	WeldSystem;
extern MESSAGE_QUEUE    MessageQueue;
extern DWORD			dwElapsedTime;


////////////////////////////
// L O C A L S
BOOL bOverflow = __FALSE;


WORD GetNumBytes(QUEUEDATA *pRHS)
{
	WORD wNumBytes;

//	os_mut_wait(QueueInUse, NO_TIMEOUT);  // Get the mutex for the queue

	wNumBytes = pRHS->uNumBytes;

//	os_mut_release(QueueInUse);		  	 // Release the Queue

	return wNumBytes; 
}

//////////////////////////////////////////////////////////////////////
//
// void ClearOneQueue(LPQUEUEDATA pqd)
//
//////////////////////////////////////////////////////////////////////
void ClearOneQueue(LPQUEUEDATA pqd)
{
	memset(pqd, 0, sizeof(QUEUEDATA));
}

void ResetQueueData(QUEUEDATA *pRHS)
{
//	os_mut_wait(QueueInUse, NO_TIMEOUT);  // Get the semaphore for the queue

	ClearOneQueue(pRHS);
	
//	os_mut_release(QueueInUse);		  	 // Release the Queue
}

//////////////////////////////////////////////////////////////////////
//
// void ClearQueues(void)
//
//////////////////////////////////////////////////////////////////////
void ClearQueues(void)
{
	ClearOneQueue(&MessageQueue.stFaultData);
	ClearOneQueue(&MessageQueue.stPendantData);
	ClearOneQueue(&MessageQueue.stGraphData);
	ClearOneQueue(&MessageQueue.stLongInfoData);
//	dwVxdFlags &= ~W32IF_MSG_FLAG_LIB_STR;
}

//////////////////////////////////////////////////////////////////////
//
// BOOL QueueMessage( LPQUEUEDATA pqd, BYTE uMsg )
//
//////////////////////////////////////////////////////////////////////
BOOL QueueMessage(QUEUEDATA* pQueData, BYTE uMsg)
{
	if (pQueData->uNumBytes > MAX_QUEUE_SIZE)
	{
		// No room in the QUEUE
		return __FALSE;
	}
	
	pQueData->QueueID[pQueData->uNumBytes] = uMsg;
	++pQueData->uNumBytes;
	
	return __TRUE;
}

//////////////////////////////////////////////////////////////////////
//
// void QueuePendantKey(BYTE uKey)
//
//////////////////////////////////////////////////////////////////////
void QueuePendantKey(BYTE uKey)
{
	QueueMessage(&MessageQueue.stPendantData, uKey);
}

//////////////////////////////////////////////////////////////////////
//
// void QueueLongInfo(DWORD dwInfo)
//
//////////////////////////////////////////////////////////////////////
void QueueLongInfo(DWORD dwInfo)
{
	BYTE i;
	for (i = 0; i < 4; ++i) 
	{
		QueueMessage(&MessageQueue.stLongInfoData, (BYTE)(dwInfo & 0x000000FF));
		dwInfo >>= 8;
	}
}

//////////////////////////////////////////////////////////////////////
//
// void QueueLevelTime(void)
//
//////////////////////////////////////////////////////////////////////
void QueueLevelTime(void)
{
	DWORD dwSend;
	DWORD dwTime;

	dwTime = WeldSystem.dwLevelTimeCount / 10;

	dwSend = ((DWORD)BG_LEVEL_TIME << 28) | (dwTime & 0x0FFFFFFF);
	QueueLongInfo(dwSend);
}

//////////////////////////////////////////////////////////////////////
//
// void QueueElapsedTime(void)
//
//////////////////////////////////////////////////////////////////////
void QueueElapsedTime(void)
{
	DWORD dwSend;

	QueueLevelTime();
	dwSend = ((DWORD)BG_ELAPSED_TIME << 28) | ((dwElapsedTime / 10) & 0x0FFFFFFF);
	QueueLongInfo(dwSend);
}

//////////////////////////////////////////////////////////////////////
//
// void QueueHeadPosition(DWORD dwPos)
//
//////////////////////////////////////////////////////////////////////
void QueueHeadPosition(DWORD dwPos)
{
	DWORD dwSend;

	dwSend = ((DWORD)BG_HEAD_POS << 28) | (dwPos & 0x0FFFFFFF);
	QueueLongInfo(dwSend);
}

void QueueServoPosition(BYTE uServo, DWORD dwPos)
{
	DWORD dwSend;

	dwSend = ((DWORD)uServo << 28) | (dwPos & 0x0FFFFFFF);
	QueueLongInfo(dwSend);
//	dprintf("QueueServoPosition:: 0x%08X\r\n", dwSend);
}

//////////////////////////////////////////////////////////////////////
//
// void QueueEventMessage(BYTE uServoNum, BYTE uEventMsg)
//
//////////////////////////////////////////////////////////////////////
void QueueEventMessage(BYTE uServo, BYTE uEventMsg)
{
	DWORD dwDataAqi;

	dwDataAqi = ((DWORD)BG_EVENTS << 28) | ((DWORD)uServo << 8) | (DWORD)uEventMsg;
	QueueBarGraphMessage(dwDataAqi);
//	printf("QueueEventMessage:: Event: %04X\r\n", uEventMsg);

	if (uEventMsg != EV_HEARTBEAT)
	{
		if (WeldSystem.uDataAqiOutEnable != 0)
		{
			QueueElapsedTime();
		}
	}

//	printf("QueueEventMessage:: QueueElapsedTime: %d\r\n", dwElapsedTime);
}

//////////////////////////////////////////////////////////////////////
//
// void QueueBarGraphMessage(DWORD dwBarGraphMsg)
//
//////////////////////////////////////////////////////////////////////
void QueueBarGraphMessage(DWORD dwBarGraphMsg)
{
	BYTE i;
	for (i = 0; i < 4; ++i) 
	{
		QueueMessage(&MessageQueue.stGraphData, (BYTE)(dwBarGraphMsg & 0x000000FF));
		dwBarGraphMsg >>= 8;
	}
}

