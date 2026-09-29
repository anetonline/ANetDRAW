#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODKrnlQueueUIChange
static unsigned ut_calls;
static BOOL utm_ODKrnlQueueUIChange(tODUIChangeType type, INT value,
   BYTE reason)
{
   ++ut_calls;
   UT_ASSERT_EQ_INT(kODUIChangeExit, type);
   UT_ASSERT_EQ_INT(23, value);
   UT_ASSERT_EQ_UINT(TRUE, reason);
   return(TRUE);
}
#else
#define UT_CUSTOM_MOCK_od_exit
static unsigned ut_calls;
void ODCALL utm_od_exit(INT error_level, BOOL term_call)
{
   ++ut_calls;
   UT_ASSERT_EQ_INT(23, error_level);
   UT_ASSERT_EQ_INT(TRUE, term_call);
}
#endif

static void requests_an_exact_exit(void)
{
   ut_calls = 0;
   utt_ODKrnlRequestExit(23, TRUE);
   UT_ASSERT_EQ_UINT(1, ut_calls);
}

static const UTTestCase ut_cases[] = {
   {"request", requests_an_exact_exit}
};
