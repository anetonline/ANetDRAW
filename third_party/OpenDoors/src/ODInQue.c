/* OpenDoors Online Software Programming Toolkit
 * (C) Copyright 1991 - 1999 by Brian Pirie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 *
 *        File: ODInQue.h
 *
 * Description: OpenDoors input queue management. This input queue is where
 *              all input events (e.g. keystrokes) from both local and remote
 *              systems are combined into a single stream.
 *
 *   Revisions: Date          Ver   Who  Change
 *              ---------------------------------------------------------------
 *              Nov 16, 1995  6.00  BP   Created.
 *              Nov 17, 1995  6.00  BP   Added multithreading support.
 *              Jan 04, 1996  6.00  BP   tODInQueueEvent -> tODInputEvent.
 *              Jan 30, 1996  6.00  BP   Replaced od_yield() with od_sleep().
 *              Jan 30, 1996  6.00  BP   Add semaphore timeout.
 *              Jan 30, 1996  6.00  BP   Add ODInQueueGetNextEvent() timeout.
 *              Feb 19, 1996  6.00  BP   Changed version number to 6.00.
 *              Mar 03, 1996  6.10  BP   Begin version 6.10.
 *              Aug 10, 2003  6.23  SH   *nix support
 */

#define BUILDING_OPENDOORS

#include <stdlib.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODGen.h"
#include "ODInQue.h"
#include "ODPlat.h"
#include "ODKrnl.h"
#include "ODSafe.h"
#include "ODSync.h"


/* Input queue handle structure. */
typedef struct
{
   tODInputEvent *paEvents;
   INT nQueueEntries;
   INT nInIndex;
   INT nOutIndex;
   INT nReservedEntries;
   time_t nLastActivityTime;
#ifdef OD_THREAD_SUPPORT
   tODSemaphoreHandle hItemCountSemaphore;
   tODMutex QueueMutex;
#endif /* OD_THREAD_SUPPORT */
} tInputQueueInfo;

#define OD_IN_QUEUE_USED(pQueue) \
   (((pQueue)->nInIndex - (pQueue)->nOutIndex \
      + (pQueue)->nQueueEntries) % (pQueue)->nQueueEntries)
#define OD_IN_QUEUE_AVAILABLE(pQueue) \
   ((pQueue)->nQueueEntries - 1 - OD_IN_QUEUE_USED(pQueue) \
      - (pQueue)->nReservedEntries)
#define OD_IN_QUEUE_RECORD_CONTROL_KEY(pEvent) \
   do { \
      if((pEvent)->EventType == EVENT_CHARACTER) { \
         switch((pEvent)->chKeyPress) { \
            case 's': case 'S': case 3: case 11: case 0x18: \
               chLastControlKey = 's'; \
               break; \
            case 'p': case 'P': \
               chLastControlKey = 'p'; \
               break; \
         } \
      } \
   } while(0)


/* ----------------------------------------------------------------------------
 * ODInQueueAlloc()
 *
 * Allocates a new input queue.
 *
 * Parameters: phInQueue         - Pointer to location where a handle to the
 *                                 newly allocated input queue should be
 *                                 stored.
 *
 *             nInitialQueueSize - The minimum number of events that the
 *                                 input queue should be able to hold.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODInQueueAlloc(tODInQueueHandle *phInQueue, INT nInitialQueueSize)
{
   tInputQueueInfo *pInputQueueInfo = NULL;
   tODInputEvent *pInputQueue = NULL;
   tODResult Result = kODRCNoMemory;
   size_t nQueueBytes;

   ASSERT(phInQueue != NULL);

   if(phInQueue == NULL) return(kODRCInvalidCall);
   *phInQueue = (tODInQueueHandle)0;

   if(nInitialQueueSize < 2)
      return(kODRCInvalidCall);
   if(!ODSizeMultiply((size_t)nInitialQueueSize, sizeof(tODInputEvent),
      &nQueueBytes))
      return(kODRCNoMemory);

   /* Attempt to allocate a serial port information structure. */
   pInputQueueInfo = malloc(sizeof(tInputQueueInfo));

   /* If memory allocation failed, return with failure. */
   if(pInputQueueInfo == NULL) goto CleanUp;

   /* Initialize semaphore handles to NULL. */
#ifdef OD_THREAD_SUPPORT
   pInputQueueInfo->hItemCountSemaphore = NULL;
#endif /* OD_THREAD_SUPPORT */
   
   /* Attempt to allocate space for the queue itself. */
   pInputQueue = calloc(1, nQueueBytes);
   if(pInputQueue == NULL) goto CleanUp;

   /* Create semaphores if this is a multithreaded platform. */
#ifdef OD_THREAD_SUPPORT
   if(ODSemaphoreAlloc(&pInputQueueInfo->hItemCountSemaphore, 0,
      nInitialQueueSize) != kODRCSuccess)
   {
      goto CleanUp;
   }

   if(ODMutexInitialize(&pInputQueueInfo->QueueMutex) != kODRCSuccess)
   {
      goto CleanUp;
   }
#endif /* OD_THREAD_SUPPORT */

   /* Initialize input queue information structure. */
   pInputQueueInfo->paEvents = pInputQueue;
   pInputQueueInfo->nQueueEntries = nInitialQueueSize;
   pInputQueueInfo->nInIndex = 0;
   pInputQueueInfo->nOutIndex = 0;
   pInputQueueInfo->nReservedEntries = 0;

   /* Convert intut queue information structure pointer to a handle. */
   *phInQueue = ODPTR2HANDLE(pInputQueueInfo, tInputQueueInfo);

   /* Reset the time of the last activity. */
   ODInQueueResetLastActivity(*phInQueue);

   Result = kODRCSuccess;

CleanUp:
   if(Result != kODRCSuccess)
   {
#ifdef OD_THREAD_SUPPORT
      if(pInputQueueInfo != NULL
         && pInputQueueInfo->hItemCountSemaphore != NULL)
      {
         ODSemaphoreFree(pInputQueueInfo->hItemCountSemaphore);
      }
#endif /* OD_THREAD_SUPPORT */

#ifdef OD_THREAD_SUPPORT
      if(pInputQueue != NULL) free(pInputQueue);
#endif
      if(pInputQueueInfo != NULL) free(pInputQueueInfo);
      *phInQueue = ODPTR2HANDLE(NULL, tInputQueueInfo);
   }

   /* Return with the appropriate result code. */
   return(Result);
}


/* ----------------------------------------------------------------------------
 * ODInQueueFree()
 *
 * Destroys an input queue that was previously created by ODInQueueAlloc().
 *
 * Parameters: hInQueue - Handle to the input queue to destroy.
 *
 *     Return: void
 */
void ODInQueueFree(tODInQueueHandle hInQueue)
{
   tInputQueueInfo *pInputQueueInfo = ODHANDLE2PTR(hInQueue, tInputQueueInfo);

   ASSERT(pInputQueueInfo != NULL);

   /* Deallocate semaphores, if appropriate. */
#ifdef OD_THREAD_SUPPORT
   ASSERT(pInputQueueInfo->hItemCountSemaphore != NULL);
   ODSemaphoreFree(pInputQueueInfo->hItemCountSemaphore);
   ODMutexDestroy(&pInputQueueInfo->QueueMutex);
#endif /* OD_THREAD_SUPPORT */

   /* Deallocate the input queue itself. */
   ASSERT(pInputQueueInfo->paEvents != NULL);
   free(pInputQueueInfo->paEvents);

   /* Deallocate port information structure. */
   free(pInputQueueInfo);
}


/* ----------------------------------------------------------------------------
 * ODInQueueWaiting()
 *
 * Determines whether or not an event is currently waiting in the input queue.
 *
 * Parameters: hInQueue - Handle to the input queue to check.
 *
 *     Return: TRUE if there is one or more waiting events, or FALSE if the
 *             queue is empty.
 */
BOOL ODInQueueWaiting(tODInQueueHandle hInQueue)
{
   tInputQueueInfo *pInputQueueInfo = ODHANDLE2PTR(hInQueue, tInputQueueInfo);
   BOOL bEventWaiting;

   ASSERT(pInputQueueInfo != NULL);

   /* There is data waiting in the queue if the in index is not equal to */
   /* the out index.                                                     */
#ifdef OD_THREAD_SUPPORT
   ODMutexLock(&pInputQueueInfo->QueueMutex);
#endif
   bEventWaiting = (pInputQueueInfo->nInIndex != pInputQueueInfo->nOutIndex);
#ifdef OD_THREAD_SUPPORT
   ODMutexUnlock(&pInputQueueInfo->QueueMutex);
#endif

   return(bEventWaiting);
}


/* ----------------------------------------------------------------------------
 * ODInQueueAddEvent()
 *
 * Adds a new event to the input queue.
 *
 * Parameters: hInQueue  - Handle to the input queue to add an event to.
 *
 *             pEvent    - Pointer to the event structure to obtain the
 *                         event information from.
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODInQueueAddEvent(tODInQueueHandle hInQueue,
   tODInputEvent *pEvent)
{
   tInputQueueInfo *pInputQueueInfo = ODHANDLE2PTR(hInQueue, tInputQueueInfo);
   ASSERT(pInputQueueInfo != NULL);
   ASSERT(pEvent != NULL);
   if(pInputQueueInfo == NULL || pEvent == NULL) return(kODRCInvalidCall);

   /* Serialize access to add event function. */
#ifdef OD_THREAD_SUPPORT
   ODMutexLock(&pInputQueueInfo->QueueMutex);
#endif /* OD_THREAD_SUPPORT */

   /* Reset the time of the last activity. */
   pInputQueueInfo->nLastActivityTime = time(NULL);

   /* If the queue is full, then return an out of space error. */
   if(OD_IN_QUEUE_AVAILABLE(pInputQueueInfo) < 1)
   {
      /* Allow further access to input queue. */
#ifdef OD_THREAD_SUPPORT
      ODMutexUnlock(&pInputQueueInfo->QueueMutex);
#endif /* OD_THREAD_SUPPORT */

      ODPlatRingBell();

      return(kODRCNoMemory);
   }

   /* Otherwise, add the new event to the input queue. */
   memcpy(&pInputQueueInfo->paEvents[pInputQueueInfo->nInIndex], pEvent,
      sizeof(tODInputEvent));

   OD_IN_QUEUE_RECORD_CONTROL_KEY(pEvent);

   /* Update queue in index. */
   pInputQueueInfo->nInIndex = (pInputQueueInfo->nInIndex + 1)
      % pInputQueueInfo->nQueueEntries;

   /* Increment queue items count semaphore. */
#ifdef OD_THREAD_SUPPORT
   ODSemaphoreUp(pInputQueueInfo->hItemCountSemaphore, 1);
#endif /* OD_THREAD_SUPPORT */

   /* Allow further access to add event function. */
#ifdef OD_THREAD_SUPPORT
   ODMutexUnlock(&pInputQueueInfo->QueueMutex);
#endif /* OD_THREAD_SUPPORT */

   return(kODRCSuccess);
}


/* Atomically adds one or more events, or rejects the complete group. */
tODResult ODInQueueAddEvents(tODInQueueHandle hInQueue,
   const tODInputEvent *paEvents, INT nEvents)
{
   tInputQueueInfo *pInputQueueInfo = ODHANDLE2PTR(hInQueue, tInputQueueInfo);
   INT nEvent;

   ASSERT(pInputQueueInfo != NULL);
   ASSERT(paEvents != NULL);
   ASSERT(nEvents > 0);
   if(pInputQueueInfo == NULL || paEvents == NULL || nEvents <= 0)
      return(kODRCInvalidCall);

#ifdef OD_THREAD_SUPPORT
   ODMutexLock(&pInputQueueInfo->QueueMutex);
#endif
   pInputQueueInfo->nLastActivityTime = time(NULL);
   if(OD_IN_QUEUE_AVAILABLE(pInputQueueInfo) < nEvents)
   {
#ifdef OD_THREAD_SUPPORT
      ODMutexUnlock(&pInputQueueInfo->QueueMutex);
#endif
      ODPlatRingBell();
      return(kODRCNoMemory);
   }

   for(nEvent = 0; nEvent < nEvents; ++nEvent)
   {
      memcpy(&pInputQueueInfo->paEvents[pInputQueueInfo->nInIndex],
         &paEvents[nEvent], sizeof(tODInputEvent));
      OD_IN_QUEUE_RECORD_CONTROL_KEY(&paEvents[nEvent]);
      pInputQueueInfo->nInIndex = (pInputQueueInfo->nInIndex + 1)
         % pInputQueueInfo->nQueueEntries;
   }
#ifdef OD_THREAD_SUPPORT
   ODSemaphoreUp(pInputQueueInfo->hItemCountSemaphore, nEvents);
   ODMutexUnlock(&pInputQueueInfo->QueueMutex);
#endif
   return(kODRCSuccess);
}


/* Atomically reserves room for one event without making it readable. */
tODResult ODInQueueReserveEvent(tODInQueueHandle hInQueue)
{
   tInputQueueInfo *pInputQueueInfo = ODHANDLE2PTR(hInQueue, tInputQueueInfo);
   tODResult Result = kODRCNoMemory;

   ASSERT(pInputQueueInfo != NULL);
   if(pInputQueueInfo == NULL) return(kODRCInvalidCall);
#ifdef OD_THREAD_SUPPORT
   ODMutexLock(&pInputQueueInfo->QueueMutex);
#endif
   if(OD_IN_QUEUE_AVAILABLE(pInputQueueInfo) > 0)
   {
      ++pInputQueueInfo->nReservedEntries;
      Result = kODRCSuccess;
   }
#ifdef OD_THREAD_SUPPORT
   ODMutexUnlock(&pInputQueueInfo->QueueMutex);
#endif
   return(Result);
}


/* Releases an unused event reservation. */
void ODInQueueCancelReservedEvent(tODInQueueHandle hInQueue)
{
   tInputQueueInfo *pInputQueueInfo = ODHANDLE2PTR(hInQueue, tInputQueueInfo);

   ASSERT(pInputQueueInfo != NULL);
   if(pInputQueueInfo == NULL) return;
#ifdef OD_THREAD_SUPPORT
   ODMutexLock(&pInputQueueInfo->QueueMutex);
#endif
   ASSERT(pInputQueueInfo->nReservedEntries > 0);
   if(pInputQueueInfo->nReservedEntries > 0)
      --pInputQueueInfo->nReservedEntries;
#ifdef OD_THREAD_SUPPORT
   ODMutexUnlock(&pInputQueueInfo->QueueMutex);
#endif
}


/* Commits an event into a slot previously reserved by this subsystem. */
tODResult ODInQueueCommitReservedEvent(tODInQueueHandle hInQueue,
   const tODInputEvent *pEvent)
{
   tInputQueueInfo *pInputQueueInfo = ODHANDLE2PTR(hInQueue, tInputQueueInfo);

   ASSERT(pInputQueueInfo != NULL);
   ASSERT(pEvent != NULL);
   if(pInputQueueInfo == NULL || pEvent == NULL) return(kODRCInvalidCall);
#ifdef OD_THREAD_SUPPORT
   ODMutexLock(&pInputQueueInfo->QueueMutex);
#endif
   ASSERT(pInputQueueInfo->nReservedEntries > 0);
   if(pInputQueueInfo->nReservedEntries <= 0)
   {
#ifdef OD_THREAD_SUPPORT
      ODMutexUnlock(&pInputQueueInfo->QueueMutex);
#endif
      return(kODRCInvalidCall);
   }

   --pInputQueueInfo->nReservedEntries;
   pInputQueueInfo->nLastActivityTime = time(NULL);
   memcpy(&pInputQueueInfo->paEvents[pInputQueueInfo->nInIndex], pEvent,
      sizeof(tODInputEvent));
   OD_IN_QUEUE_RECORD_CONTROL_KEY(pEvent);
   pInputQueueInfo->nInIndex = (pInputQueueInfo->nInIndex + 1)
      % pInputQueueInfo->nQueueEntries;
#ifdef OD_THREAD_SUPPORT
   ODSemaphoreUp(pInputQueueInfo->hItemCountSemaphore, 1);
   ODMutexUnlock(&pInputQueueInfo->QueueMutex);
#endif
   return(kODRCSuccess);
}


/* Atomically obtains and replaces the file-display control-key side channel. */
char ODInQueueExchangeLastControlKey(tODInQueueHandle hInQueue,
   char chNewValue)
{
   tInputQueueInfo *pInputQueueInfo = ODHANDLE2PTR(hInQueue, tInputQueueInfo);
   char chOldValue;

   ASSERT(pInputQueueInfo != NULL);
   if(pInputQueueInfo == NULL)
      return(0);

#ifdef OD_THREAD_SUPPORT
   ODMutexLock(&pInputQueueInfo->QueueMutex);
#endif
   chOldValue = chLastControlKey;
   chLastControlKey = chNewValue;
#ifdef OD_THREAD_SUPPORT
   ODMutexUnlock(&pInputQueueInfo->QueueMutex);
#endif
   return(chOldValue);
}


/* ----------------------------------------------------------------------------
 * ODInQueueGetNextEvent()
 *
 * Obtains the next event from the input queue. If no events are currently
 * waiting in the input queue, this function blocks until an item is added
 * to the queue, or the maximum wait time is reached.
 *
 * Parameters: hInQueue - Handle to the input queue to obtain the next event
 *                        from.
 *
 *             pEvent   - Pointer to structure to store input event information
 *                        in.
 *
 *             Timeout  - Maximum time, in milliseconds, to wait for next input
 *                        event. A value of OD_NO_TIMEOUT causes this function
 *                        to only return when an input event is obtained.
 *
 *     Return: kODRCSuccess on succes, or kODRCTimeout if the maximum wait time
 *             is exceeded.
 */
tODMilliSec ODMaxMSToWait = 1;
tODResult ODInQueueGetNextEvent(tODInQueueHandle hInQueue,
   tODInputEvent *pEvent, tODMilliSec Timeout)
{
   tInputQueueInfo *pInputQueueInfo = ODHANDLE2PTR(hInQueue, tInputQueueInfo);

   ASSERT(pInputQueueInfo != NULL);
   ASSERT(pEvent != NULL);

#ifdef OD_THREAD_SUPPORT

#ifdef ODPLAT_WIN32
   if(ODPlatGetWindowsSubsystem() == kODWindowsSubsystemConsole)
   {
      tODTimer Timer;

      if(Timeout != 0 && Timeout != OD_NO_TIMEOUT)
         ODTimerStart(&Timer, Timeout);

      for(;;)
      {
         /* Console mode must never block on this semaphore: keyboard and
          * communications producers run in the cooperative kernel below. */
         if(ODSemaphoreDown(pInputQueueInfo->hItemCountSemaphore, 0)
            == kODRCSuccess)
         {
            break;
         }
         /* Console mode has no keyboard producer thread.  Poll the same
          * cooperative kernel as DOS, then check the semaphore again so a
         * newly queued event can be consumed without an extra yield. */
         CALL_KERNEL_IF_NEEDED();
         if(ODSemaphoreDown(pInputQueueInfo->hItemCountSemaphore, 0)
            == kODRCSuccess)
         {
            break;
         }
         if(eODLifecycleState >= kODLifecycleExitPending)
            return(kODRCGeneralFailure);
         if(Timeout == 0)
            return(kODRCTimeout);
         if(Timeout != OD_NO_TIMEOUT && ODTimerElapsed(&Timer))
            return(kODRCTimeout);

         od_sleep(0);
         if(Timeout == OD_NO_TIMEOUT)
         {
            ODMaxMSToWait = 250;
         }
         else
         {
            ODMaxMSToWait = ODTimerLeft(&Timer);
            if(ODMaxMSToWait == 0)
               ODMaxMSToWait = 1;
         }
         CALL_KERNEL_IF_NEEDED();
         ODMaxMSToWait = 1;
      }
   }
   else
#endif /* ODPLAT_WIN32 */
   {
      ASSERT(Timeout == 0 || !ODSyncAPILevelActive());

      /* Where a producer thread exists, block on the queue item count. */
      if(ODSemaphoreDown(pInputQueueInfo->hItemCountSemaphore, Timeout)
         == kODRCTimeout)
      {
         return(kODRCTimeout);
      }
   }

#else /* !OD_THREAD_SUPPORT */

   /* In non-multithreaded implementations, we check queue in and out     */
   /* indicies to determine whether there are any events waiting in the   */
   /* queue. If the queue is empty we loop, calling od_kernel() to check  */
   /* for new events and od_yeild() to give more time to other processors */
   /* if there is nothing for us to do, until an event is added to the    */
   /* queue.                                                              */
   if(pInputQueueInfo->nInIndex == pInputQueueInfo->nOutIndex)
   {
      tODTimer Timer;

      /* If a timeout has been specified, then start timer to keep track */
      /* of how long we have been waiting.                               */
      if(Timeout != 0 && Timeout != OD_NO_TIMEOUT)
      {
         ODTimerStart(&Timer, Timeout);
      }

      /* As soon as we see that there is nothing in the queue, we do an */
      /* od_kernel() call to check for new input.                       */
      CALL_KERNEL_IF_NEEDED();

      /* As long as we don't have new input, we loop, yielding to other */
      /* processes, and then giving od_kernel() a chance to run.        */
      while(pInputQueueInfo->nInIndex == pInputQueueInfo->nOutIndex)
      {
         if(eODLifecycleState >= kODLifecycleExitPending)
            return(kODRCGeneralFailure);

         /* If a timeout has been specified, then ensure that the maximum */
         /* wait time has not elapsed.                                    */
         if(Timeout != 0 && Timeout != OD_NO_TIMEOUT
            && ODTimerElapsed(&Timer))
         {
            return(kODRCTimeout);
         }

         if (Timeout == 0)
            return(kODRCTimeout);

         /* Yield the processor to other tasks. */
         od_sleep(0);

         /* Call od_kernel(). */
         if (Timeout == OD_NO_TIMEOUT)
            ODMaxMSToWait = 250; /* Kernel timer period. */
         else {
            ODMaxMSToWait = ODTimerLeft(&Timer);
            if (ODMaxMSToWait == 0)
               ODMaxMSToWait = 1;
         }
         CALL_KERNEL_IF_NEEDED();
         ODMaxMSToWait = 1;
      }
   }

#endif /* !OD_THREAD_SUPPORT */

#ifdef OD_THREAD_SUPPORT
   ODMutexLock(&pInputQueueInfo->QueueMutex);
#endif
   /* Copy next input event from the queue into the caller's structure. */
   memcpy(pEvent, &pInputQueueInfo->paEvents[pInputQueueInfo->nOutIndex],
      sizeof(tODInputEvent));

   /* Move out pointer to the next queue item, wrapping back to the start */
   /* of the queue if needed.                                             */
   pInputQueueInfo->nOutIndex
      = (pInputQueueInfo->nOutIndex + 1) % pInputQueueInfo->nQueueEntries;
#ifdef OD_THREAD_SUPPORT
   ODMutexUnlock(&pInputQueueInfo->QueueMutex);
#endif

   /* Now, return with success. */
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODInQueueEmpty()
 *
 * Removes all events from the input queue.
 *
 * Parameters: hInQueue - Handle to the input queue to be emptied.
 *
 *     Return: void
 */
void ODInQueueEmpty(tODInQueueHandle hInQueue)
{
   tODInputEvent InputEvent;

   ASSERT(hInQueue != NULL);

   /* Remove all items from the queue. */
   while(ODInQueueWaiting(hInQueue))
   {
      ODInQueueGetNextEvent(hInQueue, &InputEvent, 0);
   }
}


/* ----------------------------------------------------------------------------
 * ODInQueueGetLastActivity()
 *
 * Returns the time of the last input activity. This is the latest of the time
 * that the queue was created, the time of the last call to
 * ODInQueueAddEvent() on this input queue, and the time of the last call to
 * ODInQueueResetLastActivity() on this input queue.
 *
 * Parameters: hInQueue - Handle to the input queue.
 *
 *     Return: void
 */
time_t ODInQueueGetLastActivity(tODInQueueHandle hInQueue)
{
   tInputQueueInfo *pInputQueueInfo = ODHANDLE2PTR(hInQueue, tInputQueueInfo);
   time_t LastActivity;

   ASSERT(pInputQueueInfo != NULL);

   /* Returns the last activity time. */
#ifdef OD_THREAD_SUPPORT
   ODMutexLock(&pInputQueueInfo->QueueMutex);
#endif
   LastActivity = pInputQueueInfo->nLastActivityTime;
#ifdef OD_THREAD_SUPPORT
   ODMutexUnlock(&pInputQueueInfo->QueueMutex);
#endif
   return(LastActivity);
}


/* ----------------------------------------------------------------------------
 * ODInQueueResetLastActivity()
 *
 * Resets the time of the last input activity to the current time.
 *
 * Parameters: hInQueue - Handle to the input queue.
 *
 *     Return: void
 */
void ODInQueueResetLastActivity(tODInQueueHandle hInQueue)
{
   tInputQueueInfo *pInputQueueInfo = ODHANDLE2PTR(hInQueue, tInputQueueInfo);

   ASSERT(pInputQueueInfo != NULL);

   /* Resets the last activity time to the current time. */
#ifdef OD_THREAD_SUPPORT
   ODMutexLock(&pInputQueueInfo->QueueMutex);
#endif
   pInputQueueInfo->nLastActivityTime = time(NULL);
#ifdef OD_THREAD_SUPPORT
   ODMutexUnlock(&pInputQueueInfo->QueueMutex);
#endif
}
