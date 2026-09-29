#define UT_CUSTOM_MOCK_ODKrnlDispatchPending
#define UT_CUSTOM_MOCK_od_exit
#define UT_CUSTOM_MOCK_ODSyncSessionShutdown
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODScrnPublish
#endif
static unsigned ut_publishes;
static BOOL ut_allow_callbacks;
static unsigned ut_pending_dispatches;
static char ut_call_order[3];
static unsigned ut_call_order_length;
static unsigned ut_exit_calls;
static INT ut_error_level;
static BOOL ut_term_call;
static unsigned ut_shutdown_calls;

void ODCALL utm_od_exit(INT error_level, BOOL term_call)
{
   ++ut_exit_calls;
   ut_error_level = error_level;
   ut_term_call = term_call;
}

void utm_ODSyncSessionShutdown(void) { ++ut_shutdown_calls; }

void utm_ODKrnlDispatchPending(BOOL allow_callbacks)
{
   ++ut_pending_dispatches;
   ut_allow_callbacks = allow_callbacks;
   ut_call_order[ut_call_order_length++] = 'D';
}
#ifdef ODPLAT_WIN32
void utm_ODScrnPublish(void)
{
   ++ut_publishes;
   ut_call_order[ut_call_order_length++] = 'P';
}
#endif

static void returns_defensively_when_no_api_level_is_active(void)
{
   nAPILevel = 0;
   ut_publishes = 0;
   ut_pending_dispatches = 0;
   ut_call_order_length = 0;
   eODLifecycleState = kODLifecycleActive;
   ut_exit_calls = 0;
   ut_shutdown_calls = 0;
   utt_ODSyncAPIExit();
   UT_ASSERT_EQ_UINT(0, ut_publishes);
   UT_ASSERT_EQ_UINT(0, ut_pending_dispatches);
}

static void defers_release_until_the_outermost_exit(void)
{
   nAPILevel = 2;
   eODLifecycleState = kODLifecycleActive;
   ut_publishes = 0;
   ut_pending_dispatches = 0;
   ut_call_order_length = 0;
   utt_ODSyncAPIExit();
   UT_ASSERT_EQ_UINT(1, nAPILevel);
   UT_ASSERT_EQ_UINT(0, ut_publishes);
   UT_ASSERT_EQ_UINT(0, ut_pending_dispatches);

   utt_ODSyncAPIExit();
   UT_ASSERT_EQ_UINT(0, nAPILevel);
   UT_ASSERT_EQ_UINT(1, ut_pending_dispatches);
   UT_ASSERT_EQ_INT(TRUE, ut_allow_callbacks);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(1, ut_publishes);
   UT_ASSERT_EQ_UINT(2, ut_call_order_length);
   UT_ASSERT_EQ_INT('D', ut_call_order[0]);
   UT_ASSERT_EQ_INT('P', ut_call_order[1]);
#else
   UT_ASSERT_EQ_UINT(0, ut_publishes);
#endif
}

static void finalizes_a_pending_exit_at_the_outermost_boundary(void)
{
   nAPILevel = 1;
   eODLifecycleState = kODLifecycleExitPending;
   nODPendingExitErrorLevel = 23;
   bODPendingExitTermCall = TRUE;
   ut_exit_calls = 0;
   ut_shutdown_calls = 0;
   ut_pending_dispatches = 0;
   utt_ODSyncAPIExit();
   UT_ASSERT_EQ_UINT(1, ut_exit_calls);
   UT_ASSERT_EQ_INT(23, ut_error_level);
   UT_ASSERT_EQ_INT(TRUE, ut_term_call);
   UT_ASSERT_EQ_INT(kODLifecycleFinalizing, eODLifecycleState);
   UT_ASSERT_EQ_UINT(0, ut_pending_dispatches);
   UT_ASSERT_EQ_UINT(0, ut_shutdown_calls);
}

static void shuts_down_synchronization_after_terminal_unwind(void)
{
   nAPILevel = 1;
   eODLifecycleState = kODLifecycleTerminal;
   ut_shutdown_calls = 0;
   utt_ODSyncAPIExit();
   UT_ASSERT_EQ_UINT(1, ut_shutdown_calls);
}

static const UTTestCase ut_cases[] = {
   {"inactive exit", returns_defensively_when_no_api_level_is_active},
   {"nested exit", defers_release_until_the_outermost_exit},
   {"pending exit", finalizes_a_pending_exit_at_the_outermost_boundary},
   {"terminal unwind", shuts_down_synchronization_after_terminal_unwind}
};
