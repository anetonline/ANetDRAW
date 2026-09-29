#define UT_CUSTOM_MOCK_ODComOutbound
#define UT_CUSTOM_MOCK_ODTimerElapsed
#define UT_CUSTOM_MOCK_ODTimerStart
#define UT_CUSTOM_MOCK_od_sleep
#define UT_CUSTOM_MOCK_od_kernel

static int ut_outbound_values[3];
static unsigned ut_outbound_count;
static unsigned ut_outbound_index;
static unsigned ut_timer_starts;
static tODMilliSec ut_timer_duration;
static unsigned ut_elapsed_calls;
static BOOL ut_elapsed;
static unsigned ut_sleep_calls;
static BOOL ut_shutdown_on_sleep;
static BOOL ut_pending_on_sleep;
static unsigned ut_kernel_calls;

tODResult utm_ODComOutbound(tPortHandle port, int *waiting)
{
   UT_ASSERT_EQ_PTR(hSerialPort, port);
   UT_ASSERT(waiting != NULL);
   UT_ASSERT(ut_outbound_index < ut_outbound_count);
   *waiting = ut_outbound_values[ut_outbound_index++];
   return kODRCSuccess;
}

void utm_ODTimerStart(tODTimer *timer, tODMilliSec duration)
{
   UT_ASSERT(timer != NULL);
   ++ut_timer_starts;
   ut_timer_duration = duration;
}

BOOL utm_ODTimerElapsed(tODTimer *timer)
{
   UT_ASSERT(timer != NULL);
   ++ut_elapsed_calls;
   return ut_elapsed;
}

void ODCALL utm_od_sleep(tODMilliSec milliseconds)
{
   UT_ASSERT_EQ_UINT(0, milliseconds);
   ++ut_sleep_calls;
   if(ut_shutdown_on_sleep) bODInitialized = FALSE;
   if(ut_pending_on_sleep) eODLifecycleState = kODLifecycleExitPending;
}

void ODCALL utm_od_kernel(void) { ++ut_kernel_calls; }

static void reset_drain(void)
{
   od_control.baud = 9600;
   hSerialPort = (tPortHandle)1;
   bODInitialized = TRUE;
   eODLifecycleState = kODLifecycleActive;
   ut_outbound_count = 0;
   ut_outbound_index = 0;
   ut_timer_starts = 0;
   ut_timer_duration = 0;
   ut_elapsed_calls = 0;
   ut_elapsed = FALSE;
   ut_sleep_calls = 0;
   ut_shutdown_on_sleep = FALSE;
   ut_pending_on_sleep = FALSE;
   ut_kernel_calls = 0;
}

static void local_mode_returns_without_starting_a_timer(void)
{
   reset_drain();
   od_control.baud = 0;
   utt_ODWaitDrain(123);
   UT_ASSERT_EQ_UINT(0, ut_timer_starts);
   UT_ASSERT_EQ_UINT(0, ut_outbound_index);
}

static void an_empty_queue_returns_immediately(void)
{
   reset_drain();
   ut_outbound_values[0] = 0;
   ut_outbound_count = 1;
   utt_ODWaitDrain(123);
   UT_ASSERT_EQ_UINT(1, ut_timer_starts);
   UT_ASSERT_EQ_UINT(123, ut_timer_duration);
   UT_ASSERT_EQ_UINT(0, ut_elapsed_calls);
   UT_ASSERT_EQ_UINT(1, ut_outbound_index);
   UT_ASSERT_EQ_UINT(0, ut_sleep_calls);
}

static void an_elapsed_timeout_returns_with_queued_output(void)
{
   reset_drain();
   ut_outbound_values[0] = 4;
   ut_outbound_count = 1;
   ut_elapsed = TRUE;
   utt_ODWaitDrain(123);
   UT_ASSERT_EQ_UINT(1, ut_elapsed_calls);
   UT_ASSERT_EQ_UINT(0, ut_sleep_calls);
}

static void an_unbounded_wait_yields_until_the_queue_drains(void)
{
   reset_drain();
   ut_outbound_values[0] = 4;
   ut_outbound_values[1] = 0;
   ut_outbound_count = 2;
   utt_ODWaitDrain(OD_NO_TIMEOUT);
   UT_ASSERT_EQ_UINT(0, ut_timer_starts);
   UT_ASSERT_EQ_UINT(0, ut_elapsed_calls);
   UT_ASSERT_EQ_UINT(1, ut_sleep_calls);
   UT_ASSERT_EQ_UINT(2, ut_outbound_index);
   UT_ASSERT_EQ_UINT(1, ut_kernel_calls);
}

static void a_running_timer_yields_before_the_queue_drains(void)
{
   reset_drain();
   ut_outbound_values[0] = 4;
   ut_outbound_values[1] = 0;
   ut_outbound_count = 2;
   utt_ODWaitDrain(123);
   UT_ASSERT_EQ_UINT(1, ut_elapsed_calls);
   UT_ASSERT_EQ_UINT(1, ut_sleep_calls);
   UT_ASSERT_EQ_UINT(1, ut_kernel_calls);
}

static void shutdown_after_yield_stops_without_entering_the_kernel(void)
{
   reset_drain();
   ut_outbound_values[0] = 4;
   ut_outbound_count = 1;
   ut_shutdown_on_sleep = TRUE;
   eODLifecycleState = kODLifecycleExitPending;
   utt_ODWaitDrain(OD_NO_TIMEOUT);
   UT_ASSERT_EQ_UINT(1, ut_sleep_calls);
   UT_ASSERT_EQ_UINT(1, ut_outbound_index);
   UT_ASSERT_EQ_UINT(0, ut_kernel_calls);
}

static void pending_exit_skips_the_kernel_after_yield(void)
{
   reset_drain();
   ut_outbound_values[0] = 4;
   ut_outbound_values[1] = 0;
   ut_outbound_count = 2;
   ut_pending_on_sleep = TRUE;
   utt_ODWaitDrain(OD_NO_TIMEOUT);
   UT_ASSERT_EQ_UINT(1, ut_sleep_calls);
   UT_ASSERT_EQ_UINT(0, ut_kernel_calls);
}

static const UTTestCase ut_cases[] = {
   {"local mode", local_mode_returns_without_starting_a_timer},
   {"empty queue", an_empty_queue_returns_immediately},
   {"elapsed timeout", an_elapsed_timeout_returns_with_queued_output},
   {"unbounded wait", an_unbounded_wait_yields_until_the_queue_drains},
   {"running timer", a_running_timer_yields_before_the_queue_drains},
   {"shutdown", shutdown_after_yield_stops_without_entering_the_kernel},
   {"pending exit", pending_exit_skips_the_kernel_after_yield}
};
