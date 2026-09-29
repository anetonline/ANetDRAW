#define UT_CUSTOM_MOCK_free

static unsigned ut_free_calls;

#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_CloseHandle
static unsigned ut_close_calls;

BOOL WINAPI utm_CloseHandle(HANDLE handle)
{
   ++ut_close_calls;
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, handle);
   return(TRUE);
}
#else
#define UT_CUSTOM_MOCK_pthread_cond_destroy
#define UT_CUSTOM_MOCK_pthread_mutex_destroy
static struct tODSemaphoreInfo ut_semaphore;
static unsigned ut_cond_calls;
static unsigned ut_mutex_calls;

int utm_pthread_cond_destroy(pthread_cond_t *condition)
{
   ++ut_cond_calls;
   UT_ASSERT_EQ_PTR(&ut_semaphore.changed, condition);
   return(0);
}

int utm_pthread_mutex_destroy(pthread_mutex_t *mutex)
{
   ++ut_mutex_calls;
   UT_ASSERT_EQ_PTR(&ut_semaphore.mutex, mutex);
   return(0);
}
#endif

void utm_free(void *memory)
{
   ++ut_free_calls;
#ifdef ODPLAT_WIN32
   (void)memory;
#else
   UT_ASSERT_EQ_PTR(&ut_semaphore, memory);
#endif
}

static void releases_the_native_semaphore(void)
{
   ut_free_calls = 0;
#ifdef ODPLAT_WIN32
   ut_close_calls = 0;
   utt_ODSemaphoreFree((HANDLE)(DWORD_PTR)41);
   UT_ASSERT_EQ_UINT(1, ut_close_calls);
   UT_ASSERT_EQ_UINT(0, ut_free_calls);
#else
   ut_cond_calls = 0;
   ut_mutex_calls = 0;
   utt_ODSemaphoreFree(&ut_semaphore);
   UT_ASSERT_EQ_UINT(1, ut_cond_calls);
   UT_ASSERT_EQ_UINT(1, ut_mutex_calls);
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
#endif
}

static const UTTestCase ut_cases[] = {
   {"release", releases_the_native_semaphore}
};
