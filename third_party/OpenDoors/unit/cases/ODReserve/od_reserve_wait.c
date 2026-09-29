#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed
#define UT_CUSTOM_MOCK_ODSyncAPIIsNested
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_ODSyncAPICheckpoint
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_ODTimerStart
#define UT_CUSTOM_MOCK_ODTimerElapsed
#define UT_CUSTOM_MOCK_ODReserveWaitCore
#define UT_CUSTOM_MOCK_ODReservePause
#define UT_CUSTOM_MOCK_ODReserveDetach

static BOOL ut_allowed;
static BOOL ut_init_succeeds;
static BOOL ut_checkpoint;
static BOOL ut_elapsed;
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
void utm_ODTimerStart(tODTimer *timer, tODMilliSec duration)
{ (void)timer; (void)duration; }
BOOL utm_ODTimerElapsed(tODTimer *timer)
{ (void)timer; return(ut_elapsed); }
tODReserveResult utm_ODReserveWaitCore(tODMilliSec timeout)
{
   (void)timeout;
   UT_ASSERT(ut_result_index < ut_result_count);
   return(ut_results[ut_result_index++]);
}
void utm_ODReservePause(tODMilliSec duration)
{ (void)duration; ++ut_pause_calls; }
void utm_ODReserveDetach(void) { ++ut_detach_calls; }

static void reset_wait(void)
{
   memset(&od_control, 0, sizeof(od_control));
   bODInitialized = TRUE;
   eODLifecycleState = kODLifecycleActive;
   bODReserveActive = TRUE;
   bODReserveRequested = TRUE;
   ut_allowed = TRUE;
   ut_init_succeeds = TRUE;
   ut_checkpoint = TRUE;
   ut_elapsed = FALSE;
   ut_result_count = 1;
   ut_result_index = 0;
   ut_results[0] = OD_RESERVE_ACQUIRED;
   ut_pause_calls = 0;
   ut_detach_calls = 0;
}

static void rejects_invalid_states(void)
{
   reset_wait();
   ut_allowed = FALSE;
   UT_ASSERT_EQ_INT(OD_RESERVE_ERROR, utt_od_reserve_wait(0));
   reset_wait();
   bODInitialized = FALSE;
   eODLifecycleState = kODLifecycleTerminal;
   ut_init_succeeds = FALSE;
   UT_ASSERT_EQ_INT(OD_RESERVE_ERROR, utt_od_reserve_wait(0));
   reset_wait();
   bODReserveActive = FALSE;
   UT_ASSERT_EQ_INT(OD_RESERVE_ERROR, utt_od_reserve_wait(0));
   reset_wait();
   bODReserveRequested = FALSE;
   UT_ASSERT_EQ_INT(OD_RESERVE_ERROR, utt_od_reserve_wait(0));
}

static void returns_immediate_results_and_timeouts(void)
{
   reset_wait();
   bODInitialized = FALSE;
   UT_ASSERT_EQ_INT(OD_RESERVE_ACQUIRED, utt_od_reserve_wait(100));
   reset_wait();
   ut_results[0] = OD_RESERVE_PENDING;
   UT_ASSERT_EQ_INT(OD_RESERVE_PENDING, utt_od_reserve_wait(0));
   reset_wait();
   ut_results[0] = OD_RESERVE_PENDING;
   ut_elapsed = TRUE;
   UT_ASSERT_EQ_INT(OD_RESERVE_PENDING, utt_od_reserve_wait(100));
}

static void services_while_pending(void)
{
   reset_wait();
   ut_results[0] = OD_RESERVE_PENDING;
   ut_results[1] = OD_RESERVE_ACQUIRED;
   ut_result_count = 2;
   UT_ASSERT_EQ_INT(OD_RESERVE_ACQUIRED, utt_od_reserve_wait(100));
   UT_ASSERT_EQ_INT(1, ut_pause_calls);

   reset_wait();
   ut_results[0] = OD_RESERVE_PENDING;
   ut_results[1] = OD_RESERVE_ACQUIRED;
   ut_result_count = 2;
   UT_ASSERT_EQ_INT(OD_RESERVE_ACQUIRED,
      utt_od_reserve_wait(OD_NO_TIMEOUT));
}

static void detaches_only_active_errors(void)
{
   reset_wait();
   ut_results[0] = OD_RESERVE_PENDING;
   ut_checkpoint = FALSE;
   UT_ASSERT_EQ_INT(OD_RESERVE_ERROR, utt_od_reserve_wait(100));
   UT_ASSERT_EQ_INT(1, ut_detach_calls);
   reset_wait();
   ut_results[0] = OD_RESERVE_ERROR;
   UT_ASSERT_EQ_INT(OD_RESERVE_ERROR, utt_od_reserve_wait(100));
   UT_ASSERT_EQ_INT(1, ut_detach_calls);
   reset_wait();
   ut_results[0] = OD_RESERVE_ERROR;
   eODLifecycleState = kODLifecycleInitializing;
   UT_ASSERT_EQ_INT(OD_RESERVE_ERROR, utt_od_reserve_wait(100));
   UT_ASSERT_EQ_INT(0, ut_detach_calls);
}

static const UTTestCase ut_cases[] = {
   {"reject", rejects_invalid_states},
   {"immediate", returns_immediate_results_and_timeouts},
   {"service", services_while_pending},
   {"errors", detaches_only_active_errors}
};
