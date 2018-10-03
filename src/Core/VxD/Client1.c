// CLIENT1.C : implementation file
//
/* $Log: /Software Engineering/M327/RTData/Client1.c $
 * 
 * 25    2/24/12 11:25a Wedge
 * 
 * 24    2/23/12 5:21p Wedge
 * 
 * 23    2/23/12 12:18p Wedge
 * 
 * 22    2/21/12 5:37p Wedge
 * 
 * 21    2/21/12 11:04a Wedge
 * 
 * 20    2/17/12 10:26a Wedge
 * 
 * 19    2/16/12 3:04p Wedge
 * 
 * 18    2/16/12 1:23p Wedge
 * 
 * 17    2/15/12 7:46p Wedge
 * 
 * 16    2/15/12 6:55p Wedge
 * 
 * 15    2/15/12 5:41p Wedge
 * 
 * 14    2/15/12 5:12p Wedge
 * 
 * 13    4/26/11 4:03p Edward
 * 
 * 11    3/07/11 3:08p Wedge
 * 
 * 10    4/08/09 10:59a Wedge
 * 
 * 9     3/04/09 4:44p Wedge
 * 
 * 8     3/03/09 7:25p Wedge
 * 
 * 7     3/03/09 5:23p Wedge
 * 
 * 6     2/27/09 1:13p Wedge
 * 
 * 5     2/19/09 3:40p Wedge
 * 
 * 4     2/18/09 7:56p Wedge
 * 
 * 3     2/18/09 2:59p Wedge
 * 
 * 1     2/17/09 3:41p Wedge
 * 
 * 1     2/11/09 8:03a Wedge
 * 
*/

/*****************************************************************************
*
* FILE NAME:    Client1.c
*
* DESCRIPTION:  This is client module 1.
*
\*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lwip/api.h"

// Local include files
#include "typedefs.h"
#include "M327.h"
#include "ComLit.h"

#include "Weldloop.h"
#include "Support.h"
#include "Queue.h"

typedef struct
{
	WORD wCommand;
	WORD wLength;
	BYTE uData[4];
} EMPTY_COMMAND;

extern DWORD			dwVxdFlags;
extern MESSAGE_QUEUE    MessageQueue;
extern BYTE				uPendantSelectionStr[20];
extern BYTE				_isGuiConnected;

#define BUFSIZE sizeof(MESSAGE_QUEUE)
BYTE uCommandBuffer[BUFSIZE];

void AddToBuffer(BYTE* pBuffer, QUEUEDATA* p, BYTE* pIndex)
{
	BYTE ix;

	ix = *pIndex; // Get Inital Index
	pBuffer[ix] = p->uNumBytes;
	++ix; 

	if (p->uNumBytes)
	{
		// Move Message bytes
		memcpy(&pBuffer[ix], &p->QueueID, p->uNumBytes);
		ix += p->uNumBytes;
	}

	*pIndex = ix; // Return current index
}

WORD CalculateBufferSize(void)
{
	WORD wNeeded = sizeof(EMPTY_COMMAND); // 2 bytes for command, 2 bytes for length, 4 BYTE for each of the messages

	wNeeded += MessageQueue.stFaultData.uNumBytes;
	wNeeded += MessageQueue.stPendantData.uNumBytes;
	wNeeded += MessageQueue.stGraphData.uNumBytes;
	wNeeded += MessageQueue.stLongInfoData.uNumBytes;

//	if (dwVxdFlags |= W32IF_MSG_FLAG_LIB_STR)
//	{
//		wNeeded += sizeof(uPendantSelectionStr);
//	}

	return wNeeded;
}

void CalculateSum(BYTE* pBuffer, BYTE* pBuffIndex)
{
    BYTE uSum = 0;
	BYTE* pStart;
	BYTE* pEnd;

	pStart = pBuffer;
	pStart += (2 * sizeof(WORD)); // Skip over Command & Length
	pEnd   = pStart;

	pEnd += MessageQueue.stFaultData.uNumBytes    + 1;
	pEnd += MessageQueue.stPendantData.uNumBytes  + 1;
	pEnd += MessageQueue.stGraphData.uNumBytes    + 1;
	pEnd += MessageQueue.stLongInfoData.uNumBytes + 1;

    // pBuffIndex Now points at the sumcheck location
    while(pStart < pEnd)
    {
        uSum += *pStart;
        ++pStart;
    }

    *pEnd = uSum;

    *pBuffIndex = *pBuffIndex + 1;
}

void AssembleMessageBuffer(BYTE* pBuffer, BYTE* pBuffIndex)
{
	*pBuffIndex = (2 * sizeof(WORD)); // Skip over Command & Length

	AddToBuffer(pBuffer, &MessageQueue.stFaultData,    pBuffIndex);
	AddToBuffer(pBuffer, &MessageQueue.stPendantData,  pBuffIndex);
	AddToBuffer(pBuffer, &MessageQueue.stGraphData,    pBuffIndex);
	AddToBuffer(pBuffer, &MessageQueue.stLongInfoData, pBuffIndex);

//	if (dwVxdFlags |= W32IF_MSG_FLAG_LIB_STR)
//	{
//		QUEUEDATA LibString;
//		LibString.uNumBytes = 20;
//		memcpy(& LibString.QueueID,  uPendantSelectionStr, 20)
//		AddToBuffer(pBuffer, &LibString, &uBuffIndex);
//	}

}

// Allocate, Assemble, and Send a combined QUEUEDATA Package
void SendQueueData( struct netconn *conn )
{
	BYTE uBuffIndex;
	WORD wBytesRequired;

	if (_isGuiConnected)
	{
		wBytesRequired = CalculateBufferSize();  // Note minimum will be sizeof(EMPTY_COMMAND)
		if (dwVxdFlags & W32IF_MSG_FLAG_LIB_STR)
		{
			QUEUEDATA LibString;
			uBuffIndex = (2 * sizeof(WORD));
			LibString.uNumBytes = 20;
			memcpy(& LibString.QueueID,  uPendantSelectionStr, 20);
			AddToBuffer(uCommandBuffer, &LibString, &uBuffIndex);
			uCommandBuffer[0] =  SCM_LIBRARY_STRING_UPDATE;
			uCommandBuffer[1] =  0;
			uCommandBuffer[2] =  (uBuffIndex - (2 * sizeof(WORD)));

			if(ERR_OK == netconn_write(conn, uCommandBuffer, (uint16_t)uBuffIndex, NETCONN_COPY ))
			{
				ClearQueues();
			}
			dwVxdFlags = 0;
		}
		else if ((wBytesRequired > sizeof(EMPTY_COMMAND)) && (wBytesRequired < (BUFSIZE - 0x0020)))
		{
			AssembleMessageBuffer(uCommandBuffer, &uBuffIndex);
            CalculateSum(uCommandBuffer, &uBuffIndex);
			uCommandBuffer[0] =  SCM_FEEDBACK_BURST;
			uCommandBuffer[1] =  0;
			uCommandBuffer[2] =  (uBuffIndex - (2 * sizeof(WORD)));

			if(ERR_OK == netconn_write(conn, uCommandBuffer, (uint16_t)uBuffIndex, NETCONN_COPY ))
			{
				ClearQueues();
			}
		}
		else
		{
			// TODO: Restore this before release or move to its own thread
			QueueEventMessage(SYSTEM_EVENT, EV_HEARTBEAT);
		}
	}
}


