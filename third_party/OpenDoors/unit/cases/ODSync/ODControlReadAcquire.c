#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ResetEvent
#define UT_CUSTOM_MOCK_WaitForSingleObject
#else
#define UT_CUSTOM_MOCK_pthread_cond_wait
#endif

static unsigned ut_locks;
static unsigned ut_unlocks;
static unsigned ut_waits;

void utm_ODMutexLock(tODMutex *mutex)
{
   UT_ASSERT_EQ_PTR(&ControlLock.state, mutex);
   ++ut_locks;
}

void utm_ODMutexUnlock(tODMutex *mutex)
{
   UT_ASSERT_EQ_PTR(&ControlLock.state, mutex);
   ++ut_unlocks;
}

static void finish_wait(void)
{
   ++ut_waits;
   ControlLock.writer = FALSE;
   ControlLock.waiting_writers = 0;
}

#ifdef ODPLAT_WIN32
BOOL WINAPI utm_ResetEvent(HANDLE changed)
{
   UT_ASSERT_EQ_PTR(ControlLock.changed, changed);
   return TRUE;
}

DWORD WINAPI utm_WaitForSingleObject(HANDLE changed, DWORD timeout)
{
   UT_ASSERT_EQ_PTR(ControlLock.changed, changed);
   UT_ASSERT_EQ_UINT(INFINITE, timeout);
   finish_wait();
   return WAIT_OBJECT_0;
}
#else
int utm_pthread_cond_wait(pthread_cond_t *changed, pthread_mutex_t *mutex)
{
   UT_ASSERT_EQ_PTR(&ControlLock.changed, changed);
   UT_ASSERT_EQ_PTR(&ControlLock.state.mutex, mutex);
   finish_wait();
   return 0;
}
#endif

static void reset_lock(void)
{
   ControlLock.readers = 0;
   ControlLock.waiting_writers = 0;
   ControlLock.writer = FALSE;
#ifdef ODPLAT_WIN32
   ControlLock.changed = (HANDLE)1;
#endif
   ut_locks = 0;
   ut_unlocks = 0;
   ut_waits = 0;
}

static void acquires_immediately_without_a_writer(void)
{
   reset_lock();
   utt_ODControlReadAcquire();
   UT_ASSERT_EQ_UINT(1, ControlLock.readers);
   UT_ASSERT_EQ_UINT(0, ut_waits);
}

static void waits_for_an_active_writer(void)
{
   reset_lock();
   ControlLock.writer = TRUE;
   utt_ODControlReadAcquire();
   UT_ASSERT_EQ_UINT(1, ut_waits);
   UT_ASSERT_EQ_UINT(1, ControlLock.readers);
}

static void gives_a_queued_writer_preference(void)
{
   reset_lock();
   ControlLock.waiting_writers = 1;
   utt_ODControlReadAcquire();
   UT_ASSERT_EQ_UINT(1, ut_waits);
   UT_ASSERT_EQ_UINT(1, ControlLock.readers);
}

static const UTTestCase ut_cases[] = {
   {"immediate read", acquires_immediately_without_a_writer},
   {"active writer", waits_for_an_active_writer},
   {"queued writer", gives_a_queued_writer_preference}
};
