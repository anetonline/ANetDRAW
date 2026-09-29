#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODKrnlQueueUIChange
static unsigned ut_calls;
static BOOL utm_ODKrnlQueueUIChange(tODUIChangeType type, INT value, BYTE reason)
{ ++ut_calls; UT_ASSERT_EQ_INT(kODUIChangeLockout, type); UT_ASSERT_EQ_INT(0, value); UT_ASSERT_EQ_UINT(ERRORLEVEL_HANGUP, reason); return(TRUE); }
#else
#define UT_CUSTOM_MOCK_ODKrnlForceOpenDoorsShutdown
static unsigned ut_calls;
void utm_ODKrnlForceOpenDoorsShutdown(BYTE reason) { ++ut_calls; UT_ASSERT_EQ_UINT(ERRORLEVEL_HANGUP, reason); }
#endif
static void requests_lockout(void)
{
   ut_calls = 0;
#ifndef ODPLAT_WIN32
   memset(&od_control, 0, sizeof(od_control)); od_control.user_security = 100;
#endif
   utt_ODKrnlRequestLockout(); UT_ASSERT_EQ_UINT(1, ut_calls);
#ifndef ODPLAT_WIN32
   UT_ASSERT_EQ_INT(0, od_control.user_security);
#endif
}
static const UTTestCase ut_cases[] = {{"request", requests_lockout}};
