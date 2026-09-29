#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_time
#define UT_CUSTOM_MOCK_ODPlatRingBell
#ifdef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock
#define UT_CUSTOM_MOCK_ODSemaphoreUp
#endif

#include "common.h"

char chLastControlKey;
static unsigned ut_copy_calls;
static unsigned ut_bell_calls;
#ifdef OD_THREAD_SUPPORT
static unsigned ut_locks;
static unsigned ut_unlocks;
static INT ut_semaphore_increment;
#endif

time_t utm_time(time_t *result)
{ UT_ASSERT(result == NULL); return((time_t)77); }
void utm_ODPlatRingBell(void) { ++ut_bell_calls; }
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
   ut_semaphore_increment += increment;
}
#endif

static void reset_counts(void)
{
   ut_copy_calls = ut_bell_calls = 0;
#ifdef OD_THREAD_SUPPORT
   ut_locks = ut_unlocks = 0;
   ut_semaphore_increment = 0;
#endif
}

static void rejects_invalid_arguments(void)
{
   tODInputEvent events[2];
   tODInQueueHandle handle = ut_queue_handle(4, 0, 0);
   reset_counts();
   UT_ASSERT_EQ_INT(kODRCInvalidCall,
      utt_ODInQueueAddEvents((tODInQueueHandle)0, events, 2));
   UT_ASSERT_EQ_INT(kODRCInvalidCall,
      utt_ODInQueueAddEvents(handle, NULL, 2));
   UT_ASSERT_EQ_INT(kODRCInvalidCall,
      utt_ODInQueueAddEvents(handle, events, 0));
}

static void rejects_the_complete_group_when_capacity_is_insufficient(void)
{
   tODInputEvent events[2];
   tODInQueueHandle handle = ut_queue_handle(4, 0, 0);
   memset(events, 0, sizeof(events));
   ut_queue.nReservedEntries = 2;
   reset_counts();
   UT_ASSERT_EQ_INT(kODRCNoMemory,
      utt_ODInQueueAddEvents(handle, events, 2));
   UT_ASSERT_EQ_INT(0, ut_queue.nInIndex);
   UT_ASSERT_EQ_UINT(0, ut_copy_calls);
   UT_ASSERT_EQ_UINT(1, ut_bell_calls);
   UT_ASSERT_EQ_INT(77, ut_queue.nLastActivityTime);
}

static void appends_and_wraps_the_complete_group(void)
{
   tODInputEvent events[2];
   tODInQueueHandle handle = ut_queue_handle(4, 3, 3);
   memset(events, 0, sizeof(events));
   events[0].EventType = events[1].EventType = EVENT_CHARACTER;
   events[0].chKeyPress = 'x';
   events[1].chKeyPress = 'P';
   reset_counts();
   chLastControlKey = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODInQueueAddEvents(handle, events, 2));
   UT_ASSERT_EQ_INT(1, ut_queue.nInIndex);
   UT_ASSERT_EQ_INT('x', ut_events[3].chKeyPress);
   UT_ASSERT_EQ_INT('P', ut_events[0].chKeyPress);
   UT_ASSERT_EQ_INT('p', chLastControlKey);
   UT_ASSERT_EQ_UINT(2, ut_copy_calls);
   UT_ASSERT_EQ_UINT(0, ut_bell_calls);
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT_EQ_INT(2, ut_semaphore_increment);
   UT_ASSERT_EQ_UINT(1, ut_locks);
   UT_ASSERT_EQ_UINT(1, ut_unlocks);
#endif
}

static const UTTestCase ut_cases[] = {
   {"invalid arguments", rejects_invalid_arguments},
   {"insufficient capacity", rejects_the_complete_group_when_capacity_is_insufficient},
   {"append and wrap", appends_and_wraps_the_complete_group}
};
