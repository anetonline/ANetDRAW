#define UT_CUSTOM_MOCK_memcpy
#ifdef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock
#define UT_CUSTOM_MOCK_ODSemaphoreDown
#define UT_CUSTOM_MOCK_ODTimerElapsed
#define UT_CUSTOM_MOCK_ODTimerLeft
#define UT_CUSTOM_MOCK_ODTimerStart
#define UT_CUSTOM_MOCK_od_kernel
#define UT_CUSTOM_MOCK_od_sleep
#else
#define UT_CUSTOM_MOCK_ODTimerElapsed
#define UT_CUSTOM_MOCK_ODTimerLeft
#define UT_CUSTOM_MOCK_ODTimerStart
#define UT_CUSTOM_MOCK_od_kernel
#define UT_CUSTOM_MOCK_od_sleep
#endif

#include "common.h"

static unsigned ut_copy_calls;

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

#ifdef OD_THREAD_SUPPORT
static tODResult ut_semaphore_result;
static tODMilliSec ut_seen_timeout;
static tODMilliSec ut_timer_left;
static tODWindowsSubsystem ut_subsystem;
static BOOL ut_timer_elapsed;
static unsigned ut_semaphore_calls;
static unsigned ut_semaphore_success_call;
static unsigned ut_locks;
static unsigned ut_unlocks;
static unsigned ut_kernel_calls;
static unsigned ut_sleep_calls;
static unsigned ut_timer_start_calls;
static unsigned ut_timer_elapsed_calls;
static unsigned ut_timer_left_calls;

tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{
   return(ut_subsystem);
}

tODResult utm_ODSemaphoreDown(tODSemaphoreHandle semaphore,
   tODMilliSec timeout)
{
   UT_ASSERT(semaphore == ut_queue.hItemCountSemaphore);
   ++ut_semaphore_calls;
   ut_seen_timeout = timeout;
   if(ut_semaphore_success_call != 0
      && ut_semaphore_calls == ut_semaphore_success_call)
      return(kODRCSuccess);
   return ut_semaphore_result;
}

void utm_ODTimerStart(tODTimer *timer, tODMilliSec timeout)
{
   UT_ASSERT(timer != NULL); UT_ASSERT_EQ_UINT(100, timeout);
   ++ut_timer_start_calls;
}
BOOL utm_ODTimerElapsed(tODTimer *timer)
{
   UT_ASSERT(timer != NULL); ++ut_timer_elapsed_calls;
   return(ut_timer_elapsed);
}
tODMilliSec utm_ODTimerLeft(tODTimer *timer)
{
   UT_ASSERT(timer != NULL); ++ut_timer_left_calls;
   return(ut_timer_left);
}
void ODCALL utm_od_kernel(void) { ++ut_kernel_calls; }
void ODCALL utm_od_sleep(tODMilliSec milliseconds)
{
   UT_ASSERT_EQ_UINT(0, milliseconds); ++ut_sleep_calls;
}

static void reset_threaded_wait(void)
{
   eODLifecycleState = kODLifecycleActive;
   ut_subsystem = kODWindowsSubsystemGUI;
   ut_semaphore_result = kODRCTimeout;
   ut_semaphore_calls = ut_semaphore_success_call = 0;
   ut_kernel_calls = ut_sleep_calls = 0;
   ut_timer_start_calls = ut_timer_elapsed_calls = ut_timer_left_calls = 0;
   ut_timer_elapsed = FALSE; ut_timer_left = 25;
   ut_copy_calls = ut_locks = ut_unlocks = 0;
}

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

static void reports_semaphore_timeout(void)
{
   tODInputEvent output;
   tODInQueueHandle handle = ut_queue_handle(4, 0, 0);
   reset_threaded_wait();
   UT_ASSERT_EQ_INT(kODRCTimeout,
      utt_ODInQueueGetNextEvent(handle, &output, 123));
   UT_ASSERT_EQ_INT(123, ut_seen_timeout);
   UT_ASSERT_EQ_INT(0, ut_copy_calls);
   UT_ASSERT_EQ_INT(0, ut_locks);
   UT_ASSERT_EQ_INT(0, ut_unlocks);
}

static void removes_event_after_semaphore_wait(void)
{
   tODInputEvent output;
   tODInQueueHandle handle = ut_queue_handle(4, 0, 3);
   ut_events[3].EventType = EVENT_CHARACTER;
   ut_events[3].chKeyPress = 'q';
   reset_threaded_wait();
   ut_semaphore_result = kODRCSuccess;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODInQueueGetNextEvent(handle, &output, OD_NO_TIMEOUT));
   UT_ASSERT_EQ_INT('q', output.chKeyPress);
   UT_ASSERT_EQ_INT(0, ut_queue.nOutIndex);
   UT_ASSERT_EQ_INT(1, ut_copy_calls);
   UT_ASSERT_EQ_INT(1, ut_locks);
   UT_ASSERT_EQ_INT(1, ut_unlocks);
}

static void console_wait_polls_the_cooperative_kernel(void)
{
   tODInputEvent output;
   tODInQueueHandle handle = ut_queue_handle(4, 1, 0);
   ut_events[0].EventType = EVENT_CHARACTER;
   ut_events[0].chKeyPress = 'c';
   reset_threaded_wait();
   ut_subsystem = kODWindowsSubsystemConsole;
   ut_semaphore_success_call = 2;

   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODInQueueGetNextEvent(handle, &output, OD_NO_TIMEOUT));
   UT_ASSERT_EQ_INT('c', output.chKeyPress);
   UT_ASSERT_EQ_UINT(2, ut_semaphore_calls);
   UT_ASSERT_EQ_UINT(0, ut_seen_timeout);
   UT_ASSERT_EQ_UINT(1, ut_kernel_calls);
   UT_ASSERT_EQ_UINT(0, ut_sleep_calls);
   UT_ASSERT_EQ_UINT(1, ut_locks);
   UT_ASSERT_EQ_UINT(1, ut_unlocks);
}

static void console_wait_continues_during_initialization(void)
{
   tODInputEvent output;
   tODInQueueHandle handle = ut_queue_handle(4, 1, 0);
   ut_events[0].EventType = EVENT_CHARACTER;
   ut_events[0].chKeyPress = 'i';
   reset_threaded_wait();
   ut_subsystem = kODWindowsSubsystemConsole;
   eODLifecycleState = kODLifecycleInitializing;
   ut_semaphore_success_call = 3;

   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODInQueueGetNextEvent(handle, &output, OD_NO_TIMEOUT));
   UT_ASSERT_EQ_INT('i', output.chKeyPress);
   UT_ASSERT_EQ_UINT(3, ut_semaphore_calls);
   UT_ASSERT_EQ_UINT(2, ut_kernel_calls);
   UT_ASSERT_EQ_UINT(1, ut_sleep_calls);
}

static void console_wait_handles_timeout_and_lifecycle_paths(void)
{
   tODInputEvent output;
   tODInQueueHandle handle = ut_queue_handle(4, 0, 0);

   reset_threaded_wait();
   ut_subsystem = kODWindowsSubsystemConsole;
   eODLifecycleState = kODLifecycleExitPending;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODInQueueGetNextEvent(handle, &output, OD_NO_TIMEOUT));

   handle = ut_queue_handle(4, 0, 0);
   reset_threaded_wait();
   ut_subsystem = kODWindowsSubsystemConsole;
   UT_ASSERT_EQ_INT(kODRCTimeout,
      utt_ODInQueueGetNextEvent(handle, &output, 0));

   handle = ut_queue_handle(4, 0, 0);
   reset_threaded_wait();
   ut_subsystem = kODWindowsSubsystemConsole;
   ut_timer_elapsed = TRUE;
   UT_ASSERT_EQ_INT(kODRCTimeout,
      utt_ODInQueueGetNextEvent(handle, &output, 100));
   UT_ASSERT_EQ_UINT(1, ut_timer_start_calls);
   UT_ASSERT_EQ_UINT(1, ut_timer_elapsed_calls);
}

static void console_finite_wait_uses_the_remaining_period(void)
{
   tODInputEvent output;
   tODInQueueHandle handle = ut_queue_handle(4, 1, 0);
   ut_events[0].chKeyPress = 'z';
   reset_threaded_wait();
   ut_subsystem = kODWindowsSubsystemConsole;
   ut_timer_left = 0;
   ut_semaphore_success_call = 3;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODInQueueGetNextEvent(handle, &output, 100));
   UT_ASSERT_EQ_UINT(1, ut_timer_left_calls);
   UT_ASSERT_EQ_UINT(1, ODMaxMSToWait);

   handle = ut_queue_handle(4, 1, 0);
   ut_events[0].chKeyPress = 'p';
   reset_threaded_wait();
   ut_subsystem = kODWindowsSubsystemConsole;
   ut_timer_left = 25;
   ut_semaphore_success_call = 3;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODInQueueGetNextEvent(handle, &output, 100));
   UT_ASSERT_EQ_UINT(1, ut_timer_left_calls);
}

static const UTTestCase ut_cases[] = {
   {"semaphore timeout", reports_semaphore_timeout},
   {"threaded dequeue", removes_event_after_semaphore_wait},
   {"console cooperative wait", console_wait_polls_the_cooperative_kernel},
   {"console initialization wait", console_wait_continues_during_initialization},
   {"console timeout paths", console_wait_handles_timeout_and_lifecycle_paths},
   {"console finite wait", console_finite_wait_uses_the_remaining_period}
};

#else

static BOOL ut_timer_elapsed;
static tODMilliSec ut_timer_left;
static unsigned ut_timer_starts;
static unsigned ut_elapsed_calls;
static unsigned ut_left_calls;
static unsigned ut_sleep_calls;
static unsigned ut_kernel_calls;
static unsigned ut_enqueue_on_kernel;

void utm_ODTimerStart(tODTimer *timer, tODMilliSec timeout)
{
   UT_ASSERT(timer != NULL);
   UT_ASSERT_EQ_INT(100, timeout);
   ++ut_timer_starts;
}

BOOL utm_ODTimerElapsed(tODTimer *timer)
{
   UT_ASSERT(timer != NULL);
   ++ut_elapsed_calls;
   return ut_timer_elapsed;
}

tODMilliSec utm_ODTimerLeft(tODTimer *timer)
{
   UT_ASSERT(timer != NULL);
   ++ut_left_calls;
   return ut_timer_left;
}

void ODCALL utm_od_sleep(tODMilliSec milliseconds)
{
   UT_ASSERT_EQ_INT(0, milliseconds);
   ++ut_sleep_calls;
}

void ODCALL utm_od_kernel(void)
{
   ++ut_kernel_calls;
   if(ut_kernel_calls == ut_enqueue_on_kernel)
      ut_queue.nInIndex = 1;
}

static void reset_wait_mocks(void)
{
   eODLifecycleState = kODLifecycleActive;
   ut_timer_elapsed = FALSE;
   ut_timer_left = 50;
   ut_timer_starts = 0;
   ut_elapsed_calls = 0;
   ut_left_calls = 0;
   ut_sleep_calls = 0;
   ut_kernel_calls = 0;
   ut_enqueue_on_kernel = 0;
   ut_copy_calls = 0;
}

static void an_inactive_session_aborts_the_wait(void)
{
   tODInputEvent output;
   tODInQueueHandle handle = ut_queue_handle(4, 0, 0);
   reset_wait_mocks();
   eODLifecycleState = kODLifecycleExitPending;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODInQueueGetNextEvent(handle, &output, OD_NO_TIMEOUT));
   UT_ASSERT_EQ_UINT(0, ut_sleep_calls);
}

static void initializing_session_continues_waiting(void)
{
   tODInputEvent output;
   tODInQueueHandle handle = ut_queue_handle(4, 0, 0);
   reset_wait_mocks();
   eODLifecycleState = kODLifecycleInitializing;
   ut_events[0].chKeyPress = 'i';
   ut_enqueue_on_kernel = 2;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODInQueueGetNextEvent(handle, &output, OD_NO_TIMEOUT));
   UT_ASSERT_EQ_INT('i', output.chKeyPress);
   UT_ASSERT_EQ_UINT(1, ut_sleep_calls);
   UT_ASSERT_EQ_UINT(2, ut_kernel_calls);
}

static void removes_immediately_waiting_event_and_wraps(void)
{
   tODInputEvent output;
   tODInQueueHandle handle = ut_queue_handle(4, 0, 3);
   ut_events[3].chKeyPress = 'q';
   reset_wait_mocks();
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODInQueueGetNextEvent(handle, &output, 0));
   UT_ASSERT_EQ_INT('q', output.chKeyPress);
   UT_ASSERT_EQ_INT(0, ut_queue.nOutIndex);
   UT_ASSERT_EQ_INT(0, ut_kernel_calls);
}

static void zero_timeout_checks_kernel_once(void)
{
   tODInputEvent output;
   tODInQueueHandle handle = ut_queue_handle(4, 0, 0);
   reset_wait_mocks();
   UT_ASSERT_EQ_INT(kODRCTimeout,
      utt_ODInQueueGetNextEvent(handle, &output, 0));
   UT_ASSERT_EQ_INT(1, ut_kernel_calls);
   UT_ASSERT_EQ_INT(0, ut_sleep_calls);
}

static void finite_timeout_can_expire(void)
{
   tODInputEvent output;
   tODInQueueHandle handle = ut_queue_handle(4, 0, 0);
   reset_wait_mocks();
   ut_timer_elapsed = TRUE;
   UT_ASSERT_EQ_INT(kODRCTimeout,
      utt_ODInQueueGetNextEvent(handle, &output, 100));
   UT_ASSERT_EQ_INT(1, ut_timer_starts);
   UT_ASSERT_EQ_INT(1, ut_elapsed_calls);
   UT_ASSERT_EQ_INT(0, ut_sleep_calls);
}

static void finite_wait_uses_positive_and_minimum_kernel_periods(void)
{
   tODInputEvent output;
   tODInQueueHandle handle = ut_queue_handle(4, 0, 0);
   reset_wait_mocks();
   ut_events[0].chKeyPress = 'a';
   ut_enqueue_on_kernel = 2;
   ut_timer_left = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODInQueueGetNextEvent(handle, &output, 100));
   UT_ASSERT_EQ_INT('a', output.chKeyPress);
   UT_ASSERT_EQ_INT(1, ut_left_calls);
   UT_ASSERT_EQ_INT(1, ut_sleep_calls);

   handle = ut_queue_handle(4, 0, 0);
   reset_wait_mocks();
   ut_events[0].chKeyPress = 'b';
   ut_enqueue_on_kernel = 2;
   ut_timer_left = 25;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODInQueueGetNextEvent(handle, &output, 100));
   UT_ASSERT_EQ_INT('b', output.chKeyPress);
   UT_ASSERT_EQ_INT(1, ut_left_calls);
}

static void unlimited_wait_uses_kernel_period(void)
{
   tODInputEvent output;
   tODInQueueHandle handle = ut_queue_handle(4, 0, 0);
   reset_wait_mocks();
   ut_events[0].chKeyPress = 'u';
   ut_enqueue_on_kernel = 2;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODInQueueGetNextEvent(handle, &output, OD_NO_TIMEOUT));
   UT_ASSERT_EQ_INT('u', output.chKeyPress);
   UT_ASSERT_EQ_INT(0, ut_timer_starts);
   UT_ASSERT_EQ_INT(0, ut_elapsed_calls);
   UT_ASSERT_EQ_INT(0, ut_left_calls);
   UT_ASSERT_EQ_INT(1, ut_sleep_calls);
}

static const UTTestCase ut_cases[] = {
   {"immediate dequeue", removes_immediately_waiting_event_and_wraps},
   {"zero timeout", zero_timeout_checks_kernel_once},
   {"finite timeout", finite_timeout_can_expire},
   {"finite wait", finite_wait_uses_positive_and_minimum_kernel_periods},
   {"unlimited wait", unlimited_wait_uses_kernel_period},
   {"initializing session", initializing_session_continues_waiting},
   {"inactive session", an_inactive_session_aborts_the_wait}
};
#endif
