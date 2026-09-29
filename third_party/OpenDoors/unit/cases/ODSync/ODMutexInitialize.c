#if defined(OD_THREAD_SUPPORT) && defined(ODPLAT_WIN32)
#define UT_CUSTOM_MOCK_InitializeCriticalSection
static CRITICAL_SECTION *ut_section;

void WINAPI utm_InitializeCriticalSection(CRITICAL_SECTION *section)
{
   ut_section = section;
}
#elif defined(OD_THREAD_SUPPORT)
#define UT_CUSTOM_MOCK_pthread_mutex_init
static pthread_mutex_t *ut_mutex;
static int ut_result;

int utm_pthread_mutex_init(pthread_mutex_t *mutex,
   const pthread_mutexattr_t *attributes)
{
   UT_ASSERT(attributes == NULL);
   ut_mutex = mutex;
   return ut_result;
}
#endif

static void initializes_the_platform_mutex(void)
{
   tODMutex mutex;
#if defined(OD_THREAD_SUPPORT) && defined(ODPLAT_WIN32)
   ut_section = NULL;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODMutexInitialize(&mutex));
   UT_ASSERT_EQ_PTR(&mutex.cs, ut_section);
#elif defined(OD_THREAD_SUPPORT)
   ut_mutex = NULL;
   ut_result = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODMutexInitialize(&mutex));
   UT_ASSERT_EQ_PTR(&mutex.mutex, ut_mutex);
   ut_result = 1;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure, utt_ODMutexInitialize(&mutex));
#else
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODMutexInitialize(&mutex));
#endif
}

static const UTTestCase ut_cases[] = {
   {"initialize", initializes_the_platform_mutex}
};
