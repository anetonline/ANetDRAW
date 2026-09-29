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
void utm_ODPlatRingBell(void) { ++ut_bell_calls; }
#ifdef OD_THREAD_SUPPORT
static unsigned ut_locks;
static unsigned ut_unlocks;
static unsigned ut_semaphore_ups;
void utm_ODMutexLock(tODMutex *mutex)
{
   UT_ASSERT_EQ_PTR(&ut_queue.QueueMutex, mutex);
   ++ut_locks;
}
void utm_ODMutexUnlock(tODMutex *mutex)
{
   UT_ASSERT_EQ_PTR(&ut_queue.QueueMutex, mutex);
   ++ut_unlocks;
}
void utm_ODSemaphoreUp(tODSemaphoreHandle semaphore, INT increment)
{
   UT_ASSERT(semaphore == ut_queue.hItemCountSemaphore);
   UT_ASSERT_EQ_INT(1, increment);
   ++ut_semaphore_ups;
}
#endif

time_t utm_time(time_t *result)
{
   UT_ASSERT(result == NULL);
   return (time_t)99;
}

void *utm_memcpy(void *output, const void *input, size_t size)
{
   unsigned char *destination = output;
   const unsigned char *source = input;
   size_t index;
   UT_ASSERT_EQ_UINT(sizeof(tODInputEvent), size);
   for(index = 0; index < size; ++index) destination[index] = source[index];
   ++ut_copy_calls;
   return output;
}

static void reset_counts(void)
{
   ut_copy_calls = 0;
   ut_bell_calls = 0;
#ifdef OD_THREAD_SUPPORT
   ut_locks = ut_unlocks = ut_semaphore_ups = 0;
#endif
}

static void rejects_each_null_argument(void)
{
   tODInputEvent event;
   tODInQueueHandle handle = ut_queue_handle(3, 0, 0);
   reset_counts();
   UT_ASSERT_EQ_INT(kODRCInvalidCall,
      utt_ODInQueueAddEvent((tODInQueueHandle)0, &event));
   UT_ASSERT_EQ_INT(kODRCInvalidCall,
      utt_ODInQueueAddEvent(handle, NULL));
   UT_ASSERT_EQ_INT(0, ut_copy_calls);
}

static void reports_full_queue_after_recording_activity(void)
{
   tODInputEvent event;
   tODInQueueHandle handle = ut_queue_handle(3, 1, 2);
   memset(&event, 0, sizeof(event));
   event.EventType = EVENT_CHARACTER;
   event.bFromRemote = FALSE;
   reset_counts();
   UT_ASSERT_EQ_INT(kODRCNoMemory, utt_ODInQueueAddEvent(handle, &event));
   UT_ASSERT_EQ_INT(99, ut_queue.nLastActivityTime);
   UT_ASSERT_EQ_INT(0, ut_copy_calls);
   UT_ASSERT_EQ_UINT(1, ut_bell_calls);
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT_EQ_INT(1, ut_locks);
   UT_ASSERT_EQ_INT(1, ut_unlocks);
   UT_ASSERT_EQ_INT(0, ut_semaphore_ups);
#endif
}

static void appends_and_wraps_events(void)
{
   tODInputEvent event;
   tODInQueueHandle handle = ut_queue_handle(3, 0, 0);
   event.EventType = EVENT_CHARACTER;
   event.bFromRemote = TRUE;
   event.chKeyPress = 'x';
   reset_counts();
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODInQueueAddEvent(handle, &event));
   UT_ASSERT_EQ_INT(1, ut_queue.nInIndex);
   UT_ASSERT_EQ_INT('x', ut_events[0].chKeyPress);
   ut_queue.nInIndex = 2;
   ut_queue.nOutIndex = 1;
   event.chKeyPress = 'y';
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODInQueueAddEvent(handle, &event));
   UT_ASSERT_EQ_INT(0, ut_queue.nInIndex);
   UT_ASSERT_EQ_INT('y', ut_events[2].chKeyPress);
   UT_ASSERT_EQ_INT(2, ut_copy_calls);
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT_EQ_INT(2, ut_locks);
   UT_ASSERT_EQ_INT(2, ut_unlocks);
   UT_ASSERT_EQ_INT(2, ut_semaphore_ups);
#endif
}

static void records_control_keys_only_when_the_event_is_queued(void)
{
   tODInputEvent event;
   tODInQueueHandle handle = ut_queue_handle(3, 0, 0);

   event.EventType = EVENT_EXTENDED_KEY;
   event.bFromRemote = FALSE;
   event.chKeyPress = 'z';
   chLastControlKey = 0;
   reset_counts();
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODInQueueAddEvent(handle, &event));
   UT_ASSERT_EQ_INT(0, chLastControlKey);

#define TEST_CONTROL_KEY(input, expected) \
   ut_queue.nInIndex = 0; ut_queue.nOutIndex = 0; \
   event.EventType = EVENT_CHARACTER; event.chKeyPress = (input); \
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODInQueueAddEvent(handle, &event)); \
   UT_ASSERT_EQ_INT((expected), chLastControlKey)

   TEST_CONTROL_KEY('s', 's');
   TEST_CONTROL_KEY('S', 's');
   TEST_CONTROL_KEY(3, 's');
   TEST_CONTROL_KEY(11, 's');
   TEST_CONTROL_KEY(0x18, 's');
   TEST_CONTROL_KEY('p', 'p');
   TEST_CONTROL_KEY('P', 'p');
   TEST_CONTROL_KEY('z', 'p');
#undef TEST_CONTROL_KEY

   ut_queue.nInIndex = 0;
   ut_queue.nOutIndex = 0;
   event.chKeyPress = 's';
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODInQueueAddEvent(handle, &event));
   UT_ASSERT_EQ_INT('s', chLastControlKey);

   ut_queue.nInIndex = 1;
   ut_queue.nOutIndex = 2;
   event.chKeyPress = 'P';
   UT_ASSERT_EQ_INT(kODRCNoMemory, utt_ODInQueueAddEvent(handle, &event));
   UT_ASSERT_EQ_INT('s', chLastControlKey);
}

static const UTTestCase ut_cases[] = {
   {"invalid event arguments", rejects_each_null_argument},
   {"full queue", reports_full_queue_after_recording_activity},
   {"append and wrap", appends_and_wraps_events}
   ,{"control keys", records_control_keys_only_when_the_event_is_queued}
};
