#ifdef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_ODSyncAPIRelease
#define UT_CUSTOM_MOCK_ODThreadSleep
#define UT_CUSTOM_MOCK_ODSyncAPIReacquire
#define UT_CUSTOM_MOCK_ODSyncAPICheckpoint
static unsigned ut_release_calls;
static unsigned ut_sleep_calls;
static unsigned ut_reacquire_calls;
static unsigned ut_checkpoint_calls;
unsigned utm_ODSyncAPIRelease(void)
{
   ++ut_release_calls; return(3);
}
void utm_ODThreadSleep(tODMilliSec milliseconds)
{
   UT_ASSERT(milliseconds == 125); ++ut_sleep_calls;
}
void utm_ODSyncAPIReacquire(unsigned level)
{
   UT_ASSERT_EQ_UINT(3, level); ++ut_reacquire_calls;
}
BOOL utm_ODSyncAPICheckpoint(void)
{
   ++ut_checkpoint_calls;
   return(TRUE);
}
#else
#define UT_CUSTOM_MOCK_od_sleep
static unsigned ut_sleep_calls;
void ODCALL utm_od_sleep(tODMilliSec milliseconds)
{
   UT_ASSERT(milliseconds == 125); ++ut_sleep_calls;
}
#endif

static void sleeps_without_dispatching_api_work(void)
{
   ut_sleep_calls = 0;
#ifdef OD_THREAD_SUPPORT
   ut_release_calls = ut_reacquire_calls = ut_checkpoint_calls = 0;
#endif
   utt_ODAPISleep(125);
   UT_ASSERT_EQ_UINT(1, ut_sleep_calls);
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT_EQ_UINT(1, ut_release_calls);
   UT_ASSERT_EQ_UINT(1, ut_reacquire_calls);
   UT_ASSERT_EQ_UINT(1, ut_checkpoint_calls);
#endif
}

static const UTTestCase ut_cases[] = {
   {"API sleep", sleeps_without_dispatching_api_work}
};
