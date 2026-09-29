#define UT_CUSTOM_MOCK_ODGetInputCore
#define UT_CUSTOM_MOCK_ODGetInputDeadlineSlice
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_ODSyncAPIIsNested
#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed

static unsigned ut_core_calls, ut_init_calls, ut_entries, ut_exits;
static BOOL ut_core_result, ut_init_succeeds;
static tODMilliSec ut_deadline_slice;

BOOL utm_ODSyncAPIIsNested(void) { return(FALSE); }
BOOL utm_ODSyncPublicCallAllowed(void)
{
   if(eODLifecycleState == kODLifecycleTerminal)
   {
      od_control.od_error = ERR_GENERALFAILURE;
      return(FALSE);
   }
   return(TRUE);
}
void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}
void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }
BOOL utm_ODGetInputCore(tODInputEvent *event, tODMilliSec timeout,
   WORD flags, const tODInputDeadline *deadline)
{
   ++ut_core_calls;
   UT_ASSERT_NOT_NULL(event);
   UT_ASSERT_EQ_UINT(OD_NO_TIMEOUT, timeout);
   UT_ASSERT_EQ_UINT(7, flags);
   UT_ASSERT_NOT_NULL(deadline);
   UT_ASSERT_EQ_UINT(12, deadline->dwSeconds);
   UT_ASSERT_EQ_UINT(345, deadline->wMilliseconds);
   return(ut_core_result);
}

tODMilliSec utm_ODGetInputDeadlineSlice(
   const tODInputDeadline *deadline)
{
   UT_ASSERT_NOT_NULL(deadline);
   UT_ASSERT_EQ_UINT(12, deadline->dwSeconds);
   UT_ASSERT_EQ_UINT(345, deadline->wMilliseconds);
   return(ut_deadline_slice);
}

static void reset_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
   ut_core_calls = ut_init_calls = ut_entries = ut_exits = 0;
   ut_core_result = TRUE;
   ut_init_succeeds = TRUE;
   ut_deadline_slice = 1;
   bODInitialized = TRUE;
   eODLifecycleState = kODLifecycleActive;
}

static void forwards_a_normalized_absolute_deadline(void)
{
   tODInputEvent event;
   reset_fixture();
   bODInitialized = FALSE;
   UT_ASSERT(utt_od_get_input_until(&event, 12, 345, 7));
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_core_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);

   ut_core_result = FALSE;
   UT_ASSERT(!utt_od_get_input_until(&event, 12, 345, 7));
}

static void rejects_invalid_milliseconds_and_terminal_sessions(void)
{
   tODInputEvent event;

   reset_fixture();
   UT_ASSERT(!utt_od_get_input_until(NULL, 12, 345, 7));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_core_calls);
   UT_ASSERT_EQ_UINT(1, ut_exits);

   reset_fixture();
   UT_ASSERT(!utt_od_get_input_until(&event, 12, 1000, 7));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_core_calls);
   UT_ASSERT_EQ_UINT(1, ut_exits);

   reset_fixture();
   bODInitialized = FALSE;
   ut_init_succeeds = FALSE;
   eODLifecycleState = kODLifecycleTerminal;
   UT_ASSERT(!utt_od_get_input_until(&event, 12, 345, 7));
   UT_ASSERT_EQ_UINT(0, ut_core_calls);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void rejects_an_already_reached_deadline_before_input_processing(void)
{
   tODInputEvent event;
   reset_fixture();
   ut_deadline_slice = 0;
   UT_ASSERT(!utt_od_get_input_until(&event, 12, 345, 7));
   UT_ASSERT_EQ_UINT(0, ut_core_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static const UTTestCase ut_cases[] = {
   {"forward", forwards_a_normalized_absolute_deadline},
   {"errors", rejects_invalid_milliseconds_and_terminal_sessions},
   {"reached", rejects_an_already_reached_deadline_before_input_processing}
};
