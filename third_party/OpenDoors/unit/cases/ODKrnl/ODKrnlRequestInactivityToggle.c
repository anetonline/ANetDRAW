#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODKrnlQueueUIChange
static unsigned ut_calls;
static BOOL utm_ODKrnlQueueUIChange(tODUIChangeType type, INT value, BYTE reason)
{ ++ut_calls; UT_ASSERT_EQ_INT(kODUIChangeInactivity, type); UT_ASSERT_EQ_INT(0, value); UT_ASSERT_EQ_UINT(0, reason); return(TRUE); }
#endif
static void requests_inactivity_toggle(void)
{
#ifdef ODPLAT_WIN32
   ut_calls = 0; utt_ODKrnlRequestInactivityToggle(); UT_ASSERT_EQ_UINT(1, ut_calls);
#else
   memset(&od_control, 0, sizeof(od_control)); utt_ODKrnlRequestInactivityToggle(); UT_ASSERT(od_control.od_disable_inactivity);
#endif
}
static const UTTestCase ut_cases[] = {{"request", requests_inactivity_toggle}};
