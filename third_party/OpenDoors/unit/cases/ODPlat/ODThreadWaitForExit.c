#define UT_CUSTOM_MOCK_ODSyncAPILevelActive

BOOL utm_ODSyncAPILevelActive(void)
{
   return(FALSE);
}

#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_WaitForSingleObject
static unsigned ut_wait_calls;

DWORD WINAPI utm_WaitForSingleObject(HANDLE object, DWORD timeout)
{
   ++ut_wait_calls;
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, object);
   UT_ASSERT_EQ_UINT(INFINITE, timeout);
   return(WAIT_OBJECT_0);
}
#else
#define UT_CUSTOM_MOCK_pthread_join
static unsigned ut_wait_calls;

int utm_pthread_join(pthread_t thread, void **result)
{
   ++ut_wait_calls;
   UT_ASSERT(thread == (pthread_t)41);
   UT_ASSERT_NULL(result);
   return(0);
}
#endif

static void joins_the_requested_thread(void)
{
   ut_wait_calls = 0;
#ifdef ODPLAT_WIN32
   utt_ODThreadWaitForExit((HANDLE)(DWORD_PTR)41);
#else
   utt_ODThreadWaitForExit((pthread_t)41);
#endif
   UT_ASSERT_EQ_UINT(1, ut_wait_calls);
}

static const UTTestCase ut_cases[] = {
   {"join", joins_the_requested_thread}
};
