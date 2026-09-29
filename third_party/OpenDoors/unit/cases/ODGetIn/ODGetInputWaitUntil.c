#define UT_CUSTOM_MOCK_ODGetInputDeadlineSlice
#define UT_CUSTOM_MOCK_ODInQueueGetNextEvent
#define UT_CUSTOM_MOCK_ODSyncAPICheckpoint
#define UT_CUSTOM_MOCK_ODTimerLeft
#define UT_CUSTOM_MOCK_ODTimerStart
#ifdef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_ODSyncAPIReacquire
#define UT_CUSTOM_MOCK_ODSyncAPIRelease
#endif

#define UT_WAIT_CAPACITY 8
static tODMilliSec ut_slices[UT_WAIT_CAPACITY];
static tODMilliSec ut_left[UT_WAIT_CAPACITY];
static tODResult ut_results[UT_WAIT_CAPACITY];
static BOOL ut_checkpoints[UT_WAIT_CAPACITY];
static unsigned ut_slice_count, ut_slice_index;
static unsigned ut_left_count, ut_left_index;
static unsigned ut_result_count, ut_result_index;
static unsigned ut_checkpoint_count, ut_checkpoint_index;
static unsigned ut_timer_starts;
static tODMilliSec ut_started_duration;
static tODMilliSec ut_queue_timeouts[UT_WAIT_CAPACITY];
#ifdef OD_THREAD_SUPPORT
static unsigned ut_releases, ut_reacquires;
#endif

static void reset_fixture(void)
{
   ut_slice_count = ut_slice_index = 0;
   ut_left_count = ut_left_index = 0;
   ut_result_count = ut_result_index = 0;
   ut_checkpoint_count = ut_checkpoint_index = 0;
   ut_timer_starts = 0;
   ut_started_duration = 0;
   hODInputQueue = (tODInQueueHandle)17;
#ifdef OD_THREAD_SUPPORT
   ut_releases = ut_reacquires = 0;
#endif
}

tODMilliSec utm_ODGetInputDeadlineSlice(const tODInputDeadline *deadline)
{
   UT_ASSERT_NOT_NULL(deadline);
   UT_ASSERT(ut_slice_index < ut_slice_count);
   return(ut_slices[ut_slice_index++]);
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
   return(ut_left[ut_left_index++]);
}

tODResult utm_ODInQueueGetNextEvent(tODInQueueHandle queue,
   tODInputEvent *event, tODMilliSec timeout)
{
   UT_ASSERT(queue == hODInputQueue);
   UT_ASSERT_NOT_NULL(event);
   UT_ASSERT(ut_result_index < ut_result_count);
   ut_queue_timeouts[ut_result_index] = timeout;
   return(ut_results[ut_result_index++]);
}

BOOL utm_ODSyncAPICheckpoint(void)
{
   UT_ASSERT(ut_checkpoint_index < ut_checkpoint_count);
   return(ut_checkpoints[ut_checkpoint_index++]);
}

#ifdef OD_THREAD_SUPPORT
unsigned utm_ODSyncAPIRelease(void) { ++ut_releases; return(19); }
void utm_ODSyncAPIReacquire(unsigned level)
{
   UT_ASSERT_EQ_UINT(19, level);
   ++ut_reacquires;
}
#endif

static void reached_deadline_returns_without_touching_the_queue(void)
{
   tODInputEvent event;
   tODInputDeadline deadline = {0, 0};
   reset_fixture();
   ut_slices[0] = 0; ut_slice_count = 1;
   UT_ASSERT_EQ_INT(kODRCTimeout,
      utt_ODGetInputWaitUntil(&event, &deadline, OD_NO_TIMEOUT));
   UT_ASSERT_EQ_UINT(0, ut_result_index);
   UT_ASSERT_EQ_UINT(0, ut_timer_starts);
}

static void returns_input_and_caps_a_relative_sequence_wait(void)
{
   tODInputEvent event;
   tODInputDeadline deadline = {0, 0};
   reset_fixture();
   ut_slices[0] = 40; ut_slice_count = 1;
   ut_results[0] = kODRCSuccess; ut_result_count = 1;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODGetInputWaitUntil(&event, &deadline, OD_NO_TIMEOUT));
   UT_ASSERT_EQ_UINT(40, ut_queue_timeouts[0]);

   reset_fixture();
   ut_slices[0] = 40; ut_slice_count = 1;
   ut_left[0] = 20; ut_left_count = 1;
   ut_results[0] = kODRCSuccess; ut_result_count = 1;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODGetInputWaitUntil(&event, &deadline, 250));
   UT_ASSERT_EQ_UINT(1, ut_timer_starts);
   UT_ASSERT_EQ_UINT(250, ut_started_duration);
   UT_ASSERT_EQ_UINT(20, ut_queue_timeouts[0]);

   reset_fixture();
   ut_slices[0] = 40; ut_slice_count = 1;
   ut_left[0] = 60; ut_left_count = 1;
   ut_results[0] = kODRCSuccess; ut_result_count = 1;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODGetInputWaitUntil(&event, &deadline, 250));
   UT_ASSERT_EQ_UINT(40, ut_queue_timeouts[0]);
}

static void relative_and_lifecycle_limits_abort_waiting(void)
{
   tODInputEvent event;
   tODInputDeadline deadline = {0, 0};
   reset_fixture();
   ut_slices[0] = 40; ut_slice_count = 1;
   ut_left[0] = 0; ut_left_count = 1;
   UT_ASSERT_EQ_INT(kODRCTimeout,
      utt_ODGetInputWaitUntil(&event, &deadline, 250));
   UT_ASSERT_EQ_UINT(0, ut_result_index);

   reset_fixture();
   ut_slices[0] = 40; ut_slice_count = 1;
   ut_results[0] = kODRCTimeout; ut_result_count = 1;
   ut_checkpoints[0] = FALSE; ut_checkpoint_count = 1;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODGetInputWaitUntil(&event, &deadline, OD_NO_TIMEOUT));

   reset_fixture();
   ut_slices[0] = 40; ut_slices[1] = 0; ut_slice_count = 2;
   ut_results[0] = kODRCTimeout; ut_result_count = 1;
   ut_checkpoints[0] = TRUE; ut_checkpoint_count = 1;
   UT_ASSERT_EQ_INT(kODRCTimeout,
      utt_ODGetInputWaitUntil(&event, &deadline, OD_NO_TIMEOUT));
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT_EQ_UINT(1, ut_releases);
   UT_ASSERT_EQ_UINT(1, ut_reacquires);
#endif
}

static const UTTestCase ut_cases[] = {
   {"deadline", reached_deadline_returns_without_touching_the_queue},
   {"input", returns_input_and_caps_a_relative_sequence_wait},
   {"abort", relative_and_lifecycle_limits_abort_waiting}
};
