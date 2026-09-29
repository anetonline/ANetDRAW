#define UT_CUSTOM_MOCK_ODKrnlQueueUIChange
static unsigned ut_calls;
static BOOL utm_ODKrnlQueueUIChange(tODUIChangeType type, INT value,
   BYTE reason)
{
   ++ut_calls;
   UT_ASSERT_EQ_INT(kODUIChangeShutdown, type);
   UT_ASSERT_EQ_INT(0, value);
   UT_ASSERT_EQ_UINT(7, reason);
   return(TRUE);
}
static void queues_shutdown_reason(void)
{
   ut_calls = 0;
   utt_ODKrnlRequestShutdown(7);
   UT_ASSERT_EQ_UINT(1, ut_calls);
}
static const UTTestCase ut_cases[] = {
   {"request", queues_shutdown_reason}
};
