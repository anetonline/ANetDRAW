#if defined(OD_THREAD_SUPPORT) && defined(ODPLAT_WIN32)
#define UT_CUSTOM_MOCK_DeleteCriticalSection
static CRITICAL_SECTION *ut_section;

void WINAPI utm_DeleteCriticalSection(CRITICAL_SECTION *section)
{
   ut_section = section;
}
#elif defined(OD_THREAD_SUPPORT)
#define UT_CUSTOM_MOCK_pthread_mutex_destroy
static pthread_mutex_t *ut_mutex;

int utm_pthread_mutex_destroy(pthread_mutex_t *mutex)
{
   ut_mutex = mutex;
   return 0;
}
#endif

static void destroys_the_platform_mutex(void)
{
   tODMutex mutex;
#if defined(OD_THREAD_SUPPORT) && defined(ODPLAT_WIN32)
   ut_section = NULL;
   utt_ODMutexDestroy(&mutex);
   UT_ASSERT_EQ_PTR(&mutex.cs, ut_section);
#elif defined(OD_THREAD_SUPPORT)
   ut_mutex = NULL;
   utt_ODMutexDestroy(&mutex);
   UT_ASSERT_EQ_PTR(&mutex.mutex, ut_mutex);
#else
   utt_ODMutexDestroy(&mutex);
#endif
}

static const UTTestCase ut_cases[] = {
   {"destroy", destroys_the_platform_mutex}
};
