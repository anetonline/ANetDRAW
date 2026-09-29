#define UT_CUSTOM_MOCK_ODInQueueGetNextEvent
#define UT_CUSTOM_MOCK_ODSyncAPICheckpoint
#define UT_CUSTOM_MOCK_ODTimerElapsed
#define UT_CUSTOM_MOCK_ODTimerLeft
#define UT_CUSTOM_MOCK_ODTimerStart
#ifdef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_ODSyncAPIReacquire
#define UT_CUSTOM_MOCK_ODSyncAPIRelease
#endif

#define UT_SEQUENCE_CAPACITY 4

static tODResult ut_input_results[UT_SEQUENCE_CAPACITY];
static tODMilliSec ut_timer_left_values[UT_SEQUENCE_CAPACITY];
static BOOL ut_checkpoint_results[UT_SEQUENCE_CAPACITY];
static BOOL ut_elapsed_results[UT_SEQUENCE_CAPACITY];
static unsigned ut_input_count;
static unsigned ut_input_index;
static unsigned ut_left_count;
static unsigned ut_left_index;
static unsigned ut_checkpoint_count;
static unsigned ut_checkpoint_index;
static unsigned ut_elapsed_count;
static unsigned ut_elapsed_index;
static unsigned ut_timer_starts;
static tODMilliSec ut_started_duration;
static tODMilliSec ut_seen_slices[UT_SEQUENCE_CAPACITY];
#ifdef OD_THREAD_SUPPORT
static unsigned ut_releases;
static unsigned ut_reacquires;
#endif

static void reset_fixture(void)
{
   memset(ut_input_results, 0, sizeof(ut_input_results));
   memset(ut_timer_left_values, 0, sizeof(ut_timer_left_values));
   memset(ut_checkpoint_results, 0, sizeof(ut_checkpoint_results));
   memset(ut_elapsed_results, 0, sizeof(ut_elapsed_results));
   memset(ut_seen_slices, 0, sizeof(ut_seen_slices));
   ut_input_count = ut_input_index = 0;
   ut_left_count = ut_left_index = 0;
   ut_checkpoint_count = ut_checkpoint_index = 0;
   ut_elapsed_count = ut_elapsed_index = 0;
   ut_timer_starts = 0;
   ut_started_duration = 0;
   hODInputQueue = (tODInQueueHandle)0;
#ifdef OD_THREAD_SUPPORT
   ut_releases = 0;
   ut_reacquires = 0;
#endif
}

void utm_ODTimerStart(tODTimer *timer, tODMilliSec duration)
{
   UT_ASSERT_NOT_NULL(timer);
   ++ut_timer_starts;
   ut_started_duration = duration;
}

tODMilliSec utm_ODTimerLeft(tODTimer *timer)
{
   UT_ASSERT_NOT_NULL(timer);
   UT_ASSERT(ut_left_index < ut_left_count);
   return ut_timer_left_values[ut_left_index++];
}

tODResult utm_ODInQueueGetNextEvent(tODInQueueHandle handle,
   tODInputEvent *event, tODMilliSec timeout)
{
   UT_ASSERT(handle == hODInputQueue);
   UT_ASSERT_NOT_NULL(event);
   UT_ASSERT(ut_input_index < ut_input_count);
   ut_seen_slices[ut_input_index] = timeout;
   return ut_input_results[ut_input_index++];
}

BOOL utm_ODSyncAPICheckpoint(void)
{
   UT_ASSERT(ut_checkpoint_index < ut_checkpoint_count);
   return ut_checkpoint_results[ut_checkpoint_index++];
}

BOOL utm_ODTimerElapsed(tODTimer *timer)
{
   UT_ASSERT_NOT_NULL(timer);
   UT_ASSERT(ut_elapsed_index < ut_elapsed_count);
   return ut_elapsed_results[ut_elapsed_index++];
}

#ifdef OD_THREAD_SUPPORT
unsigned utm_ODSyncAPIRelease(void)
{
   ++ut_releases;
   return 17;
}

void utm_ODSyncAPIReacquire(unsigned level)
{
   UT_ASSERT_EQ_UINT(17, level);
   ++ut_reacquires;
}
#endif

static void zero_timeout_returns_first_queue_result(void)
{
   tODInputEvent event;
   reset_fixture();
   ut_input_results[0] = kODRCTimeout;
   ut_input_count = 1;
   UT_ASSERT_EQ_INT(kODRCTimeout, utt_ODGetInputWait(&event, 0));
   UT_ASSERT_EQ_UINT(1, ut_timer_starts);
   UT_ASSERT_EQ_UINT(0, ut_started_duration);
   UT_ASSERT_EQ_UINT(0, ut_seen_slices[0]);
   UT_ASSERT_EQ_UINT(0, ut_checkpoint_index);
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT_EQ_UINT(0, ut_releases);
   UT_ASSERT_EQ_UINT(0, ut_reacquires);
#endif
}

static void successful_finite_wait_caps_the_slice(void)
{
   tODInputEvent event;
   reset_fixture();
   ut_timer_left_values[0] = 80;
   ut_left_count = 1;
   ut_input_results[0] = kODRCSuccess;
   ut_input_count = 1;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODGetInputWait(&event, 120));
   UT_ASSERT_EQ_UINT(1, ut_timer_starts);
   UT_ASSERT_EQ_UINT(120, ut_started_duration);
   UT_ASSERT_EQ_UINT(50, ut_seen_slices[0]);
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT_EQ_UINT(1, ut_releases);
   UT_ASSERT_EQ_UINT(1, ut_reacquires);
#endif
}

static void checkpoint_can_abort_a_short_finite_wait(void)
{
   tODInputEvent event;
   reset_fixture();
   ut_timer_left_values[0] = 25;
   ut_left_count = 1;
   ut_input_results[0] = kODRCTimeout;
   ut_input_count = 1;
   ut_checkpoint_results[0] = FALSE;
   ut_checkpoint_count = 1;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure, utt_ODGetInputWait(&event, 40));
   UT_ASSERT_EQ_UINT(25, ut_seen_slices[0]);
   UT_ASSERT_EQ_UINT(0, ut_elapsed_index);
}

static void finite_wait_can_expire_or_continue(void)
{
   tODInputEvent event;
   reset_fixture();
   ut_timer_left_values[0] = 40;
   ut_left_count = 1;
   ut_input_results[0] = kODRCTimeout;
   ut_input_count = 1;
   ut_checkpoint_results[0] = TRUE;
   ut_checkpoint_count = 1;
   ut_elapsed_results[0] = TRUE;
   ut_elapsed_count = 1;
   UT_ASSERT_EQ_INT(kODRCTimeout, utt_ODGetInputWait(&event, 40));

   reset_fixture();
   ut_timer_left_values[0] = 40;
   ut_timer_left_values[1] = 10;
   ut_left_count = 2;
   ut_input_results[0] = kODRCTimeout;
   ut_input_results[1] = kODRCSuccess;
   ut_input_count = 2;
   ut_checkpoint_results[0] = TRUE;
   ut_checkpoint_count = 1;
   ut_elapsed_results[0] = FALSE;
   ut_elapsed_count = 1;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODGetInputWait(&event, 40));
   UT_ASSERT_EQ_UINT(10, ut_seen_slices[1]);
}

static void unlimited_wait_does_not_consult_the_timer(void)
{
   tODInputEvent event;
   reset_fixture();
   ut_input_results[0] = kODRCTimeout;
   ut_input_results[1] = kODRCSuccess;
   ut_input_count = 2;
   ut_checkpoint_results[0] = TRUE;
   ut_checkpoint_count = 1;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODGetInputWait(&event, OD_NO_TIMEOUT));
   UT_ASSERT_EQ_UINT(0, ut_timer_starts);
   UT_ASSERT_EQ_UINT(0, ut_left_index);
   UT_ASSERT_EQ_UINT(0, ut_elapsed_index);
   UT_ASSERT_EQ_UINT(50, ut_seen_slices[0]);
   UT_ASSERT_EQ_UINT(50, ut_seen_slices[1]);
}

static const UTTestCase ut_cases[] = {
   {"zero timeout", zero_timeout_returns_first_queue_result},
   {"capped finite slice", successful_finite_wait_caps_the_slice},
   {"checkpoint abort", checkpoint_can_abort_a_short_finite_wait},
   {"finite expiration", finite_wait_can_expire_or_continue},
   {"unlimited wait", unlimited_wait_does_not_consult_the_timer}
};
