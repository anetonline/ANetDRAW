#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODKrnlQueueUIChange
static unsigned ut_calls;
static BOOL utm_ODKrnlQueueUIChange(tODUIChangeType type, INT value, BYTE reason)
{ ++ut_calls; UT_ASSERT_EQ_INT(kODUIChangeTime, type); UT_ASSERT_EQ_INT(-20, value); UT_ASSERT_EQ_UINT(0, reason); return(TRUE); }
#endif
static void requests_time_adjustment(void)
{
#ifdef ODPLAT_WIN32
   ut_calls = 0; utt_ODKrnlRequestTimeAdjustment(-20); UT_ASSERT_EQ_UINT(1, ut_calls);
#else
   memset(&od_control, 0, sizeof(od_control)); od_control.user_timelimit = 10;
   utt_ODKrnlRequestTimeAdjustment(-20); UT_ASSERT_EQ_INT(-10, od_control.user_timelimit);
#endif
}
static const UTTestCase ut_cases[] = {{"request", requests_time_adjustment}};
