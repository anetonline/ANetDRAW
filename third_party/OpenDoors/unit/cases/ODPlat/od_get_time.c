#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_ODSessionTimeGet
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_ODSyncAPIIsNested
#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed

static unsigned ut_init_calls;
static unsigned ut_get_calls;
static unsigned ut_entry_calls;
static unsigned ut_exit_calls;
static BOOL ut_init_succeeds;

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
   if(ut_init_succeeds)
      bODInitialized = TRUE;
}

void utm_ODSessionTimeGet(DWORD *seconds, WORD *milliseconds)
{
   ++ut_get_calls;
   if(seconds != NULL) *seconds = 12;
   if(milliseconds != NULL) *milliseconds = 345;
}

void utm_ODSyncAPIEntry(void) { ++ut_entry_calls; }
void utm_ODSyncAPIExit(void) { ++ut_exit_calls; }

static void reset_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
   ut_init_calls = ut_get_calls = ut_entry_calls = ut_exit_calls = 0;
   ut_init_succeeds = TRUE;
   bODInitialized = TRUE;
   eODLifecycleState = kODLifecycleActive;
}

static BOOL lifecycle_guard_allows_call(void)
{
   OD_RETURN_IF_SESSION_ENDED(FALSE);
   return(TRUE);
}

static void returns_requested_components_and_initializes(void)
{
   DWORD seconds = 0;
   WORD milliseconds = 0;

   reset_fixture();
   bODInitialized = FALSE;
   utt_od_get_time(&seconds, &milliseconds);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(12, seconds);
   UT_ASSERT_EQ_UINT(345, milliseconds);

   seconds = 0;
   utt_od_get_time(&seconds, NULL);
   UT_ASSERT_EQ_UINT(12, seconds);
   milliseconds = 0;
   utt_od_get_time(NULL, &milliseconds);
   UT_ASSERT_EQ_UINT(345, milliseconds);
   UT_ASSERT_EQ_UINT(3, ut_get_calls);
   UT_ASSERT_EQ_UINT(3, ut_entry_calls);
   UT_ASSERT_EQ_UINT(3, ut_exit_calls);
}

static void accepts_active_session_with_dirty_registers(void)
{
   DWORD seconds = 0;
   WORD milliseconds = 0;

   reset_fixture();
#ifdef __TURBOC__
   /* Turbo C 2.01 must not let an unrelated high byte in DX affect the
    * lifecycle guard generated at the start of the public entry point. */
   ASM mov dx, 0xff00
#endif
   UT_ASSERT(lifecycle_guard_allows_call());
   utt_od_get_time(&seconds, &milliseconds);
   UT_ASSERT_EQ_UINT(12, seconds);
   UT_ASSERT_EQ_UINT(345, milliseconds);
   UT_ASSERT_EQ_UINT(1, ut_entry_calls);
}

static void rejects_missing_outputs_and_terminal_sessions(void)
{
   DWORD seconds = 77;

   reset_fixture();
   utt_od_get_time(NULL, NULL);
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_get_calls);
   UT_ASSERT_EQ_UINT(1, ut_exit_calls);

   reset_fixture();
   bODInitialized = FALSE;
   ut_init_succeeds = FALSE;
   eODLifecycleState = kODLifecycleTerminal;
   utt_od_get_time(&seconds, NULL);
   UT_ASSERT_EQ_UINT(77, seconds);
   UT_ASSERT_EQ_UINT(0, ut_entry_calls);
}

static const UTTestCase ut_cases[] = {
   {"components", returns_requested_components_and_initializes},
   {"dirty-registers", accepts_active_session_with_dirty_registers},
   {"errors", rejects_missing_outputs_and_terminal_sessions}
};
