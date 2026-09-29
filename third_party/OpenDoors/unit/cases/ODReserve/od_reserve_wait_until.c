#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed
#define UT_CUSTOM_MOCK_ODSyncAPIIsNested
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_ODSyncAPICheckpoint
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_ODSessionTimeGet
#define UT_CUSTOM_MOCK_ODReserveWaitCore
#define UT_CUSTOM_MOCK_ODReservePause
#define UT_CUSTOM_MOCK_ODReserveDetach

static BOOL ut_allowed;
static BOOL ut_init_succeeds;
static BOOL ut_checkpoint;
static DWORD ut_now_seconds[3];
static WORD ut_now_milliseconds[3];
static int ut_time_count;
static int ut_time_index;
static tODReserveResult ut_results[3];
static int ut_result_count;
static int ut_result_index;
static int ut_pause_calls;
static int ut_detach_calls;

BOOL utm_ODSyncPublicCallAllowed(void) { return(ut_allowed); }
BOOL utm_ODSyncAPIIsNested(void) { return(FALSE); }
void utm_ODSyncAPIEntry(void) { }
void utm_ODSyncAPIExit(void) { }
BOOL utm_ODSyncAPICheckpoint(void) { return(ut_checkpoint); }
void ODCALL utm_od_init(void)
{ if(ut_init_succeeds) bODInitialized = TRUE; }
void utm_ODSessionTimeGet(DWORD *seconds, WORD *milliseconds)
{
   UT_ASSERT(ut_time_index < ut_time_count);
   *seconds = ut_now_seconds[ut_time_index];
   *milliseconds = ut_now_milliseconds[ut_time_index];
   ++ut_time_index;
}
tODReserveResult utm_ODReserveWaitCore(tODMilliSec timeout)
{
   (void)timeout;
   UT_ASSERT(ut_result_index < ut_result_count);
   return(ut_results[ut_result_index++]);
}
void utm_ODReservePause(tODMilliSec duration)
{ (void)duration; ++ut_pause_calls; }
void utm_ODReserveDetach(void) { ++ut_detach_calls; }

static void reset_until(void)
{
   memset(&od_control, 0, sizeof(od_control));
   bODInitialized = TRUE;
   eODLifecycleState = kODLifecycleActive;
   bODReserveActive = TRUE;
   bODReserveRequested = TRUE;
   bODReserveAcquired = FALSE;
   ut_allowed = TRUE;
   ut_init_succeeds = TRUE;
   ut_checkpoint = TRUE;
   ut_time_count = 1;
   ut_time_index = 0;
   ut_now_seconds[0] = 1;
   ut_now_milliseconds[0] = 0;
   ut_result_count = 1;
   ut_result_index = 0;
   ut_results[0] = OD_RESERVE_ACQUIRED;
   ut_pause_calls = 0;
   ut_detach_calls = 0;
}

static void rejects_invalid_states(void)
{
   reset_until();
   ut_allowed = FALSE;
   UT_ASSERT_EQ_INT(OD_RESERVE_ERROR,
      utt_od_reserve_wait_until(2, 0));
   reset_until();
   UT_ASSERT_EQ_INT(OD_RESERVE_ERROR,
      utt_od_reserve_wait_until(2, 1000));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   reset_until();
   bODInitialized = FALSE;
   eODLifecycleState = kODLifecycleTerminal;
   ut_init_succeeds = FALSE;
   UT_ASSERT_EQ_INT(OD_RESERVE_ERROR,
      utt_od_reserve_wait_until(2, 0));
   reset_until();
   bODReserveActive = FALSE;
   UT_ASSERT_EQ_INT(OD_RESERVE_ERROR,
      utt_od_reserve_wait_until(2, 0));
   reset_until();
   bODReserveRequested = FALSE;
   UT_ASSERT_EQ_INT(OD_RESERVE_ERROR,
      utt_od_reserve_wait_until(2, 0));
}

static void handles_acquired_and_reached_deadlines(void)
{
   reset_until();
   bODReserveAcquired = TRUE;
   UT_ASSERT_EQ_INT(OD_RESERVE_ACQUIRED,
      utt_od_reserve_wait_until(2, 0));
   reset_until();
   ut_now_seconds[0] = 3;
   UT_ASSERT_EQ_INT(OD_RESERVE_PENDING,
      utt_od_reserve_wait_until(2, 500));
   reset_until();
   ut_now_seconds[0] = 2;
   ut_now_milliseconds[0] = 500;
   UT_ASSERT_EQ_INT(OD_RESERVE_PENDING,
      utt_od_reserve_wait_until(2, 500));
}

static void services_until_acquired(void)
{
   reset_until();
   bODInitialized = FALSE;
   ut_results[0] = OD_RESERVE_PENDING;
   ut_results[1] = OD_RESERVE_ACQUIRED;
   ut_result_count = 2;
   ut_time_count = 2;
   ut_now_seconds[0] = 1;
   ut_now_seconds[1] = 2;
   ut_now_milliseconds[1] = 400;
   UT_ASSERT_EQ_INT(OD_RESERVE_ACQUIRED,
      utt_od_reserve_wait_until(2, 500));
   UT_ASSERT_EQ_INT(1, ut_pause_calls);

   reset_until();
   ut_now_seconds[0] = 1;
   ut_results[0] = OD_RESERVE_PENDING;
   ut_checkpoint = FALSE;
   UT_ASSERT_EQ_INT(OD_RESERVE_ERROR,
      utt_od_reserve_wait_until(2, 500));
   UT_ASSERT_EQ_INT(1, ut_detach_calls);
}

static void detaches_only_active_errors(void)
{
   reset_until();
   ut_results[0] = OD_RESERVE_ERROR;
   UT_ASSERT_EQ_INT(OD_RESERVE_ERROR,
      utt_od_reserve_wait_until(2, 500));
   UT_ASSERT_EQ_INT(1, ut_detach_calls);
   reset_until();
   ut_results[0] = OD_RESERVE_ERROR;
   eODLifecycleState = kODLifecycleInitializing;
   UT_ASSERT_EQ_INT(OD_RESERVE_ERROR,
      utt_od_reserve_wait_until(2, 500));
   UT_ASSERT_EQ_INT(0, ut_detach_calls);
}

static const UTTestCase ut_cases[] = {
   {"reject", rejects_invalid_states},
   {"deadline", handles_acquired_and_reached_deadlines},
   {"service", services_until_acquired},
   {"errors", detaches_only_active_errors}
};
