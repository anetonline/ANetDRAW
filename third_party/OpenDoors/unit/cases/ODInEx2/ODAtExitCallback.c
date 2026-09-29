#define UT_CUSTOM_MOCK_od_exit
static unsigned ut_exit_calls;
static INT ut_exit_code;
void ODCALL utm_od_exit(INT errorlevel, BOOL hangup)
{
   ut_exit_code = errorlevel; UT_ASSERT(!hangup); ++ut_exit_calls;
}

static void ignores_an_inactive_library(void)
{
   bODInitialized = FALSE; bPreOrExit = FALSE; ut_exit_calls = 0;
   utt_ODAtExitCallback();
   UT_ASSERT_EQ_UINT(0, ut_exit_calls); UT_ASSERT(!bPreOrExit);
}

static void uses_configured_or_fallback_errorlevel(void)
{
   memset(od_control.od_errorlevel, 0, sizeof(od_control.od_errorlevel));
   bODInitialized = TRUE; bPreOrExit = FALSE; ut_exit_calls = 0;
   utt_ODAtExitCallback();
   UT_ASSERT(bPreOrExit); UT_ASSERT_EQ_UINT(1, ut_exit_calls);
   UT_ASSERT_EQ_INT(6, ut_exit_code);
   memset(od_control.od_errorlevel, 0, sizeof(od_control.od_errorlevel));
   od_control.od_errorlevel[0] = 1; od_control.od_errorlevel[7] = 42;
   bPreOrExit = FALSE; ut_exit_calls = 0;
   utt_ODAtExitCallback();
   UT_ASSERT(bPreOrExit); UT_ASSERT_EQ_UINT(1, ut_exit_calls);
   UT_ASSERT_EQ_INT(42, ut_exit_code);
}

static const UTTestCase ut_cases[] = {
   {"inactive", ignores_an_inactive_library},
   {"error level", uses_configured_or_fallback_errorlevel}
};
