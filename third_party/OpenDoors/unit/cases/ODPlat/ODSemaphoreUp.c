#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ReleaseSemaphore
static unsigned ut_release_calls;

BOOL WINAPI utm_ReleaseSemaphore(HANDLE semaphore, LONG increment,
   LPLONG previous)
{
   ++ut_release_calls;
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, semaphore);
   UT_ASSERT_EQ_INT(3, increment);
   UT_ASSERT_NULL(previous);
   return(TRUE);
}

static void forwards_the_increment(void)
{
   ut_release_calls = 0;
   utt_ODSemaphoreUp((HANDLE)(DWORD_PTR)41, 3);
   UT_ASSERT_EQ_UINT(1, ut_release_calls);
}
#else
#define UT_CUSTOM_MOCK_pthread_cond_broadcast
#define UT_CUSTOM_MOCK_pthread_mutex_lock
#define UT_CUSTOM_MOCK_pthread_mutex_unlock
static struct tODSemaphoreInfo ut_semaphore;
static unsigned ut_broadcast_calls;
static unsigned ut_lock_calls;
static unsigned ut_unlock_calls;

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

int utm_pthread_cond_broadcast(pthread_cond_t *condition)
{
   ++ut_broadcast_calls;
   UT_ASSERT_EQ_PTR(&ut_semaphore.changed, condition);
   return(0);
}

static void reset_up(INT count, INT maximum)
{
   memset(&ut_semaphore, 0, sizeof(ut_semaphore));
   ut_semaphore.count = count;
   ut_semaphore.maximum = maximum;
   ut_broadcast_calls = 0;
   ut_lock_calls = 0;
   ut_unlock_calls = 0;
}

static void increments_and_wakes_waiters_within_the_maximum(void)
{
   reset_up(2, 6);
   utt_ODSemaphoreUp(&ut_semaphore, 3);
   UT_ASSERT_EQ_INT(5, ut_semaphore.count);
   UT_ASSERT_EQ_UINT(1, ut_broadcast_calls);
   UT_ASSERT_EQ_UINT(1, ut_lock_calls);
   UT_ASSERT_EQ_UINT(1, ut_unlock_calls);
}

static void ignores_an_increment_beyond_the_maximum(void)
{
   reset_up(5, 6);
   utt_ODSemaphoreUp(&ut_semaphore, 2);
   UT_ASSERT_EQ_INT(5, ut_semaphore.count);
   UT_ASSERT_EQ_UINT(0, ut_broadcast_calls);
   UT_ASSERT_EQ_UINT(1, ut_lock_calls);
   UT_ASSERT_EQ_UINT(1, ut_unlock_calls);
}
#endif

static const UTTestCase ut_cases[] = {
#ifdef ODPLAT_WIN32
   {"increment", forwards_the_increment}
#else
   {"increment", increments_and_wakes_waiters_within_the_maximum},
   {"maximum", ignores_an_increment_beyond_the_maximum}
#endif
};
