/**
@file Queue.h
@ 

*/
#ifndef _QUEUE_H
#define _QUEUE_H

#include "typedefs.h"
#include "ComLit.h"

typedef struct
{
	QUEUEDATA stFaultData;
	QUEUEDATA stPendantData;
	QUEUEDATA stGraphData;
	QUEUEDATA stLongInfoData;
} MESSAGE_QUEUE;

WORD GetNumBytes(QUEUEDATA* pRHS);
void ResetQueueData(QUEUEDATA* pRHS);
void ClearQueues(void);
void QueuePendantKey(BYTE uKey);
void QueueLongInfo(DWORD dwInfo);
void QueueLevelTime(void);
void QueueElapsedTime(void);
void QueueHeadPosition(DWORD dwPos);
void QueueServoPosition(BYTE uServo, DWORD dwPos);
void QueueDataAqiEvent(BYTE uServo, BYTE uEventMsg);
void QueueEventMessage(BYTE uServoNum, BYTE uEventMsg);
void QueueBarGraphMessage(DWORD dwBarGraphMsg);
BOOL QueueMessage(QUEUEDATA* pQueData, BYTE uMsg);

#endif	// _QUEUE_H
