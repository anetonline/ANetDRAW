#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_WaitForSingleObject
static DWORD ut_wait_result;
static DWORD ut_expected_timeout;
static unsigned ut_wait_calls;

DWORD WINAPI utm_WaitForSingleObject(HANDLE semaphore, DWORD timeout)
{
   ++ut_wait_calls;
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, semaphore);
   UT_ASSERT_EQ_UINT(ut_expected_timeout, timeout);
   return(ut_wait_result);
}

static void maps_wait_results(void)
{
   ut_wait_calls = 0;
   ut_expected_timeout = 123;
   ut_wait_result = WAIT_TIMEOUT;
   UT_ASSERT_EQ_INT(kODRCTimeout,
      utt_ODSemaphoreDown((HANDLE)(DWORD_PTR)41, 123));
   ut_wait_result = WAIT_OBJECT_0;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODSemaphoreDown((HANDLE)(DWORD_PTR)41, 123));
   UT_ASSERT_EQ_UINT(2, ut_wait_calls);
}
#else
#define UT_CUSTOM_MOCK_clock_gettime
#define UT_CUSTOM_MOCK_pthread_cond_timedwait
#define UT_CUSTOM_MOCK_pthread_cond_wait
#define UT_CUSTOM_MOCK_pthread_mutex_lock
#define UT_CUSTOM_MOCK_pthread_mutex_unlock

static struct tODSemaphoreInfo ut_semaphore;
static struct timespec ut_clock;
static int ut_wait_result;
static BOOL ut_wait_adds_count;
static unsigned ut_clock_calls;
static unsigned ut_timed_calls;
static unsigned ut_wait_calls;
static unsigned ut_lock_calls;
static unsigned ut_unlock_calls;

int utm_clock_gettime(clockid_t clock_id, struct timespec *value)
{
   ++ut_clock_calls;
   UT_ASSERT(clock_id == CLOCK_REALTIME);
   *value = ut_clock;
   return(0);
}

int utm_pthread_mutex_lock(pthread_mutex_t *mutex)
{
   ++ut_lock_calls;
   UT_ASSERT_EQ_PTR(&ut_semaphore.mutex, mutex);
   return(0);
}

int utm_pthread_mutex_unlock(pthread_mutex_t *mutex)
{
   ++ut_unlock_calls;
   UT_ASSERT_EQ_PTR(&ut_semaphore.mutex, mutex);
   return(0);
}

int utm_pthread_cond_wait(pthread_cond_t *condition, pthread_mutex_t *mutex)
{
   ++ut_wait_calls;
   UT_ASSERT_EQ_PTR(&ut_semaphore.changed, condition);
   UT_ASSERT_EQ_PTR(&ut_semaphore.mutex, mutex);
   if(ut_wait_adds_count) ut_semaphore.count = 1;
   return(ut_wait_result);
}

int utm_pthread_cond_timedwait(pthread_cond_t *condition,
   pthread_mutex_t *mutex, const struct timespec *deadline)
{
   ++ut_timed_calls;
   UT_ASSERT_EQ_PTR(&ut_semaphore.changed, condition);
   UT_ASSERT_EQ_PTR(&ut_semaphore.mutex, mutex);
   UT_ASSERT_NOT_NULL(deadline);
   if(ut_wait_adds_count) ut_semaphore.count = 1;
   return(ut_wait_result);
}

static void reset_down(INT count)
{
   memset(&ut_semaphore, 0, sizeof(ut_semaphore));
   ut_semaphore.count = count;
   ut_clock.tv_sec = 10;
   ut_clock.tv_nsec = 100000000;
   ut_wait_result = 0;
   ut_wait_adds_count = FALSE;
   ut_clock_calls = 0;
   ut_timed_calls = 0;
   ut_wait_calls = 0;
   ut_lock_calls = 0;
   ut_unlock_calls = 0;
}

static void decrements_without_waiting_when_available(void)
{
   reset_down(2);
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODSemaphoreDown(&ut_semaphore, OD_NO_TIMEOUT));
   UT_ASSERT_EQ_INT(1, ut_semaphore.count);
   UT_ASSERT_EQ_UINT(0, ut_clock_calls);
   UT_ASSERT_EQ_UINT(0, ut_wait_calls);
   UT_ASSERT_EQ_UINT(1, ut_lock_calls);
   UT_ASSERT_EQ_UINT(1, ut_unlock_calls);
}

static void reports_an_immediate_timeout(void)
{
   reset_down(0);
   UT_ASSERT_EQ_INT(kODRCTimeout,
      utt_ODSemaphoreDown(&ut_semaphore, 0));
   UT_ASSERT_EQ_UINT(1, ut_clock_calls);
   UT_ASSERT_EQ_UINT(0, ut_timed_calls);
   UT_ASSERT_EQ_UINT(1, ut_unlock_calls);
}

static void waits_without_a_deadline(void)
{
   reset_down(0);
   ut_wait_adds_count = TRUE;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODSemaphoreDown(&ut_semaphore, OD_NO_TIMEOUT));
   UT_ASSERT_EQ_INT(0, ut_semaphore.count);
   UT_ASSERT_EQ_UINT(1, ut_wait_calls);
   UT_ASSERT_EQ_UINT(0, ut_clock_calls);
}

static void calculates_each_finite_deadline_shape(void)
{
   reset_down(0);
   ut_clock.tv_nsec = 600000000;
   ut_wait_adds_count = TRUE;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODSemaphoreDown(&ut_semaphore, 1500));
   UT_ASSERT_EQ_UINT(1, ut_timed_calls);

   reset_down(0);
   ut_wait_adds_count = TRUE;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODSemaphoreDown(&ut_semaphore, 500));
   UT_ASSERT_EQ_UINT(1, ut_timed_calls);
}

static void maps_native_wait_errors(void)
{
   reset_down(0);
   ut_wait_result = ETIMEDOUT;
   UT_ASSERT_EQ_INT(kODRCTimeout,
      utt_ODSemaphoreDown(&ut_semaphore, 10));
   UT_ASSERT_EQ_UINT(1, ut_unlock_calls);

   reset_down(0);
   ut_wait_result = EINVAL;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODSemaphoreDown(&ut_semaphore, OD_NO_TIMEOUT));
   UT_ASSERT_EQ_UINT(1, ut_unlock_calls);
}
#endif

static const UTTestCase ut_cases[] = {
#ifdef ODPLAT_WIN32
   {"wait results", maps_wait_results}
#else
   {"available", decrements_without_waiting_when_available},
   {"immediate timeout", reports_an_immediate_timeout},
   {"unbounded wait", waits_without_a_deadline},
   {"finite deadlines", calculates_each_finite_deadline_shape},
   {"wait errors", maps_native_wait_errors}
#endif
};
