#define UT_CUSTOM_MOCK_ODInQueueGetNextEvent
#define UT_CUSTOM_MOCK_ODInQueueWaiting
#define UT_CUSTOM_MOCK_ODSyncAPICheckpoint
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_kernel
#ifdef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_ODSyncAPIRelease
#define UT_CUSTOM_MOCK_ODSyncAPIReacquire
#endif

static BOOL ut_waiting_values[4];
static unsigned ut_waiting_count;
static unsigned ut_waiting_index;
static tODResult ut_results[6];
static tODInputEvent ut_events[6];
static unsigned ut_result_count;
static unsigned ut_result_index;
static BOOL ut_checkpoint_values[4];
static unsigned ut_checkpoint_count;
static unsigned ut_checkpoint_index;
static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static unsigned ut_kernel_calls;
static BOOL ut_init_succeeds;
#ifdef OD_THREAD_SUPPORT
static unsigned ut_release_calls;
static unsigned ut_reacquire_calls;
#endif

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

BOOL utm_ODInQueueWaiting(tODInQueueHandle queue)
{
   UT_ASSERT_EQ_PTR(hODInputQueue, queue);
   UT_ASSERT(ut_waiting_index < ut_waiting_count);
   return ut_waiting_values[ut_waiting_index++];
}

tODResult utm_ODInQueueGetNextEvent(tODInQueueHandle queue,
   tODInputEvent *event, tODMilliSec timeout)
{
   UT_ASSERT_EQ_PTR(hODInputQueue, queue);
   UT_ASSERT(event != NULL);
   UT_ASSERT(ut_result_index < ut_result_count);
   if(timeout == 0)
      UT_ASSERT(ut_waiting_index > 0);
   else
      UT_ASSERT_EQ_UINT(50, timeout);
   *event = ut_events[ut_result_index];
   return ut_results[ut_result_index++];
}

BOOL utm_ODSyncAPICheckpoint(void)
{
   UT_ASSERT(ut_checkpoint_index < ut_checkpoint_count);
   return ut_checkpoint_values[ut_checkpoint_index++];
}

void ODCALL utm_od_kernel(void) { ++ut_kernel_calls; }
#ifdef OD_THREAD_SUPPORT
unsigned utm_ODSyncAPIRelease(void)
{
   ++ut_release_calls;
   return 4;
}

void utm_ODSyncAPIReacquire(unsigned level)
{
   UT_ASSERT_EQ_UINT(4, level);
   ++ut_reacquire_calls;
}
#endif

static void reset_key(void)
{
   unsigned index;
   bODInitialized = TRUE;
   eODLifecycleState = kODLifecycleActive;
   hODInputQueue = (tODInQueueHandle)1;
   od_control.od_last_input = 99;
   ut_waiting_count = 0;
   ut_waiting_index = 0;
   ut_result_count = 0;
   ut_result_index = 0;
   ut_checkpoint_count = 0;
   ut_checkpoint_index = 0;
   for(index = 0; index < 6; ++index)
   {
      ut_results[index] = kODRCSuccess;
      ut_events[index].EventType = EVENT_CHARACTER;
      ut_events[index].chKeyPress = 0;
      ut_events[index].bFromRemote = FALSE;
   }
   ut_init_calls = 0;
   ut_entries = 0;
   ut_exits = 0;
   ut_kernel_calls = 0;
   ut_init_succeeds = TRUE;
#ifdef OD_THREAD_SUPPORT
   ut_release_calls = 0;
   ut_reacquire_calls = 0;
#endif
   ut_kernel_calls = 0;
}

static void terminal_session_is_rejected(void)
{
   reset_key(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT_EQ_INT(0, utt_od_get_key(FALSE));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void pending_session_is_rejected(void)
{
   reset_key();
   eODLifecycleState = kODLifecycleExitPending;
   ut_waiting_values[0] = FALSE;
   ut_waiting_count = 1;
   UT_ASSERT_EQ_INT(0, utt_od_get_key(FALSE));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void a_nonwaiting_read_returns_zero_for_an_empty_queue(void)
{
   reset_key();
   bODInitialized = FALSE;
   ut_waiting_values[0] = FALSE;
   ut_waiting_count = 1;
   UT_ASSERT_EQ_INT(0, utt_od_get_key(FALSE));
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(0, ut_result_index);
   UT_ASSERT_EQ_UINT(1, ut_kernel_calls);
}

static void a_nonwaiting_queue_failure_returns_zero(void)
{
   reset_key();
   ut_waiting_values[0] = TRUE;
   ut_waiting_count = 1;
   ut_results[0] = kODRCGeneralFailure;
   ut_result_count = 1;
   UT_ASSERT_EQ_INT(0, utt_od_get_key(FALSE));
   UT_ASSERT_EQ_UINT(1, ut_result_index);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static void a_nonwaiting_read_ignores_line_feed_and_tracks_remote_input(void)
{
   reset_key();
   ut_waiting_values[0] = TRUE;
   ut_waiting_values[1] = TRUE;
   ut_waiting_count = 2;
   ut_events[0].chKeyPress = '\n';
   ut_events[1].chKeyPress = 'R';
   ut_events[1].bFromRemote = TRUE;
   ut_result_count = 2;
   UT_ASSERT_EQ_INT('R', utt_od_get_key(FALSE));
   UT_ASSERT_EQ_UINT(2, ut_waiting_index);
   UT_ASSERT_EQ_UINT(2, ut_result_index);
   UT_ASSERT_EQ_INT(0, od_control.od_last_input);
}

static void a_waiting_read_retries_timeouts_and_line_feed(void)
{
   reset_key();
   ut_results[0] = kODRCTimeout;
   ut_results[1] = kODRCSuccess;
   ut_results[2] = kODRCSuccess;
   ut_events[1].chKeyPress = '\n';
   ut_events[2].chKeyPress = 'L';
   ut_events[2].bFromRemote = FALSE;
   ut_result_count = 3;
   ut_checkpoint_values[0] = TRUE;
   ut_checkpoint_count = 1;
   UT_ASSERT_EQ_INT('L', utt_od_get_key(TRUE));
   UT_ASSERT_EQ_UINT(3, ut_result_index);
   UT_ASSERT_EQ_UINT(1, ut_checkpoint_index);
   UT_ASSERT_EQ_INT(1, od_control.od_last_input);
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT_EQ_UINT(3, ut_release_calls);
   UT_ASSERT_EQ_UINT(3, ut_reacquire_calls);
#endif
}

static void a_failed_checkpoint_stops_a_waiting_read(void)
{
   reset_key();
   ut_results[0] = kODRCGeneralFailure;
   ut_result_count = 1;
   ut_checkpoint_values[0] = FALSE;
   ut_checkpoint_count = 1;
   UT_ASSERT_EQ_INT(0, utt_od_get_key(TRUE));
   UT_ASSERT_EQ_UINT(1, ut_checkpoint_index);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static const UTTestCase ut_cases[] = {
   {"empty nonwaiting queue", a_nonwaiting_read_returns_zero_for_an_empty_queue},
   {"failed nonwaiting dequeue", a_nonwaiting_queue_failure_returns_zero},
   {"nonwaiting line feed", a_nonwaiting_read_ignores_line_feed_and_tracks_remote_input},
   {"waiting retries", a_waiting_read_retries_timeouts_and_line_feed},
   {"failed checkpoint", a_failed_checkpoint_stops_a_waiting_read},
   {"pending session", pending_session_is_rejected},
   {"terminal session", terminal_session_is_rejected}
};
