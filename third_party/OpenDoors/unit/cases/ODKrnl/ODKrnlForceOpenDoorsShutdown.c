#define UT_CUSTOM_MOCK_od_exit

static unsigned ut_exit_calls;
static INT ut_exit_level;
static BOOL ut_exit_hangup;

void ODCALL utm_od_exit(INT error_level, BOOL hangup)
{
   ++ut_exit_calls;
   ut_exit_level = error_level;
   ut_exit_hangup = hangup;
}
static void reset_shutdown(void)
{
   memset(od_control.od_errorlevel, 0, sizeof(od_control.od_errorlevel));
   ut_exit_calls = 0;
   ut_exit_level = -1;
   ut_exit_hangup = FALSE;
}

static void maps_default_reasons_and_hangup_policy(void)
{
   reset_shutdown();
   utt_ODKrnlForceOpenDoorsShutdown(ERRORLEVEL_HANGUP);
   UT_ASSERT_EQ_UINT(1, ut_exit_calls);
   UT_ASSERT_EQ_INT(ERRORLEVEL_HANGUP - 1, ut_exit_level);
   UT_ASSERT(ut_exit_hangup);
   UT_ASSERT_EQ_UINT(ERRORLEVEL_HANGUP - 1, btExitReason);

   reset_shutdown();
   utt_ODKrnlForceOpenDoorsShutdown(ERRORLEVEL_INACTIVITY);
   UT_ASSERT(ut_exit_hangup);

   reset_shutdown();
   utt_ODKrnlForceOpenDoorsShutdown(ERRORLEVEL_TIMEOUT);
   UT_ASSERT(!ut_exit_hangup);
   UT_ASSERT_EQ_INT(ERRORLEVEL_TIMEOUT - 1, ut_exit_level);
}

static void uses_the_client_errorlevel_table_when_enabled(void)
{
   reset_shutdown();
   od_control.od_errorlevel[0] = 1;
   od_control.od_errorlevel[ERRORLEVEL_TIMEOUT] = 47;
   utt_ODKrnlForceOpenDoorsShutdown(ERRORLEVEL_TIMEOUT);
   UT_ASSERT_EQ_UINT(1, ut_exit_calls);
   UT_ASSERT_EQ_INT(47, ut_exit_level);
   UT_ASSERT(!ut_exit_hangup);
}

static const UTTestCase ut_cases[] = {
   {"default mapping", maps_default_reasons_and_hangup_policy},
   {"client mapping", uses_the_client_errorlevel_table_when_enabled}
};
