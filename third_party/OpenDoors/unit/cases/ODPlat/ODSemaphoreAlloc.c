#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_CreateSemaphoreA
static HANDLE ut_create_result;
static unsigned ut_create_calls;

HANDLE WINAPI utm_CreateSemaphoreA(LPSECURITY_ATTRIBUTES attributes,
   LONG initial, LONG maximum, LPCSTR name)
{
   ++ut_create_calls;
   UT_ASSERT_NULL(attributes);
   UT_ASSERT_EQ_INT(2, initial);
   UT_ASSERT_EQ_INT(5, maximum);
   UT_ASSERT_NULL(name);
   return(ut_create_result);
}

static void reports_native_failure_and_success(void)
{
   tODSemaphoreHandle semaphore;
   ut_create_calls = 0;
   ut_create_result = NULL;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODSemaphoreAlloc(&semaphore, 2, 5));
   ut_create_result = (HANDLE)(DWORD_PTR)41;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODSemaphoreAlloc(&semaphore, 2, 5));
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, semaphore);
   UT_ASSERT_EQ_UINT(2, ut_create_calls);
}
#else
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_pthread_cond_init
#define UT_CUSTOM_MOCK_pthread_mutex_destroy
#define UT_CUSTOM_MOCK_pthread_mutex_init

static struct tODSemaphoreInfo ut_semaphore;
static BOOL ut_malloc_fails;
static int ut_mutex_result;
static int ut_cond_result;
static unsigned ut_free_calls;
static unsigned ut_mutex_destroy_calls;

void *utm_malloc(size_t size)
{
   UT_ASSERT_EQ_UINT(sizeof(ut_semaphore), size);
   return(ut_malloc_fails ? NULL : &ut_semaphore);
}

void utm_free(void *memory)
{
   ++ut_free_calls;
   UT_ASSERT_EQ_PTR(&ut_semaphore, memory);
}

int utm_pthread_mutex_init(pthread_mutex_t *mutex,
   const pthread_mutexattr_t *attributes)
{
   UT_ASSERT_EQ_PTR(&ut_semaphore.mutex, mutex);
   UT_ASSERT_NULL(attributes);
   return(ut_mutex_result);
}

int utm_pthread_cond_init(pthread_cond_t *condition,
   const pthread_condattr_t *attributes)
{
   UT_ASSERT_EQ_PTR(&ut_semaphore.changed, condition);
   UT_ASSERT_NULL(attributes);
   return(ut_cond_result);
}

int utm_pthread_mutex_destroy(pthread_mutex_t *mutex)
{
   ++ut_mutex_destroy_calls;
   UT_ASSERT_EQ_PTR(&ut_semaphore.mutex, mutex);
   return(0);
}

static void reset_alloc(void)
{
   memset(&ut_semaphore, 0, sizeof(ut_semaphore));
   ut_malloc_fails = FALSE;
   ut_mutex_result = 0;
   ut_cond_result = 0;
   ut_free_calls = 0;
   ut_mutex_destroy_calls = 0;
}

static void reports_each_allocation_stage(void)
{
   tODSemaphoreHandle semaphore;
   reset_alloc();
   ut_malloc_fails = TRUE;
   UT_ASSERT_EQ_INT(kODRCNoMemory,
      utt_ODSemaphoreAlloc(&semaphore, 2, 5));

   reset_alloc();
   ut_mutex_result = 1;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODSemaphoreAlloc(&semaphore, 2, 5));
   UT_ASSERT_NULL(semaphore);
   UT_ASSERT_EQ_UINT(1, ut_free_calls);

   reset_alloc();
   ut_cond_result = 1;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODSemaphoreAlloc(&semaphore, 2, 5));
   UT_ASSERT_NULL(semaphore);
   UT_ASSERT_EQ_UINT(1, ut_mutex_destroy_calls);
   UT_ASSERT_EQ_UINT(1, ut_free_calls);

   reset_alloc();
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODSemaphoreAlloc(&semaphore, 2, 5));
   UT_ASSERT_EQ_PTR(&ut_semaphore, semaphore);
   UT_ASSERT_EQ_INT(2, semaphore->count);
   UT_ASSERT_EQ_INT(5, semaphore->maximum);
   UT_ASSERT_EQ_UINT(0, ut_free_calls);
}
#endif

static const UTTestCase ut_cases[] = {
#ifdef ODPLAT_WIN32
   {"native results", reports_native_failure_and_success}
#else
   {"allocation stages", reports_each_allocation_stage}
#endif
};
