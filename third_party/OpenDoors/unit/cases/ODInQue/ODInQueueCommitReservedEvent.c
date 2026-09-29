#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_time
#ifdef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock
#define UT_CUSTOM_MOCK_ODSemaphoreUp
#endif

#include "common.h"

char chLastControlKey;
static unsigned ut_copy_calls;
#ifdef OD_THREAD_SUPPORT
static unsigned ut_locks;
static unsigned ut_unlocks;
static unsigned ut_semaphore_ups;
#endif

time_t utm_time(time_t *result)
{ UT_ASSERT(result == NULL); return((time_t)88); }
void *utm_memcpy(void *output, const void *input, size_t size)
{
   unsigned char *destination = output;
   const unsigned char *source = input;
   size_t index;
   UT_ASSERT_EQ_UINT(sizeof(tODInputEvent), size);
   for(index = 0; index < size; ++index) destination[index] = source[index];
   ++ut_copy_calls;
   return(output);
}
#ifdef OD_THREAD_SUPPORT
void utm_ODMutexLock(tODMutex *mutex)
{ UT_ASSERT_EQ_PTR(&ut_queue.QueueMutex, mutex); ++ut_locks; }
void utm_ODMutexUnlock(tODMutex *mutex)
{ UT_ASSERT_EQ_PTR(&ut_queue.QueueMutex, mutex); ++ut_unlocks; }
void utm_ODSemaphoreUp(tODSemaphoreHandle semaphore, INT increment)
{
   UT_ASSERT(semaphore == ut_queue.hItemCountSemaphore);
   UT_ASSERT_EQ_INT(1, increment);
   ++ut_semaphore_ups;
}
#endif

static void rejects_invalid_or_unreserved_commits(void)
{
   tODInputEvent event;
   tODInQueueHandle handle = ut_queue_handle(4, 0, 0);
   memset(&event, 0, sizeof(event));
   UT_ASSERT_EQ_INT(kODRCInvalidCall,
      utt_ODInQueueCommitReservedEvent((tODInQueueHandle)0, &event));
   UT_ASSERT_EQ_INT(kODRCInvalidCall,
      utt_ODInQueueCommitReservedEvent(handle, NULL));
   UT_ASSERT_EQ_INT(kODRCInvalidCall,
      utt_ODInQueueCommitReservedEvent(handle, &event));
}

static void commits_the_reserved_event(void)
{
   tODInputEvent event;
   tODInQueueHandle handle = ut_queue_handle(4, 3, 3);
   memset(&event, 0, sizeof(event));
   event.EventType = EVENT_CHARACTER;
   event.bFromRemote = TRUE;
   event.chKeyPress = 'S';
   ut_queue.nReservedEntries = 1;
   ut_copy_calls = 0;
   chLastControlKey = 0;
#ifdef OD_THREAD_SUPPORT
   ut_locks = ut_unlocks = ut_semaphore_ups = 0;
#endif
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODInQueueCommitReservedEvent(handle, &event));
   UT_ASSERT_EQ_INT(0, ut_queue.nReservedEntries);
   UT_ASSERT_EQ_INT(0, ut_queue.nInIndex);
   UT_ASSERT_EQ_INT('S', ut_events[3].chKeyPress);
   UT_ASSERT_EQ_INT('s', chLastControlKey);
   UT_ASSERT_EQ_INT(88, ut_queue.nLastActivityTime);
   UT_ASSERT_EQ_UINT(1, ut_copy_calls);
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT_EQ_UINT(1, ut_locks);
   UT_ASSERT_EQ_UINT(1, ut_unlocks);
   UT_ASSERT_EQ_UINT(1, ut_semaphore_ups);
#endif
}

static const UTTestCase ut_cases[] = {
   {"invalid commit", rejects_invalid_or_unreserved_commits},
   {"commit", commits_the_reserved_event}
};
