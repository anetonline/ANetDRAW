#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ResetEvent
#define UT_CUSTOM_MOCK_WaitForSingleObject
#else
#define UT_CUSTOM_MOCK_pthread_cond_wait
#endif

static unsigned ut_waits;

void utm_ODMutexLock(tODMutex *mutex)
{
   UT_ASSERT_EQ_PTR(&ControlLock.state, mutex);
}

void utm_ODMutexUnlock(tODMutex *mutex)
{
   UT_ASSERT_EQ_PTR(&ControlLock.state, mutex);
}

static void finish_wait(void)
{
   ++ut_waits;
   ControlLock.writer = FALSE;
   ControlLock.readers = 0;
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
   ut_waits = 0;
}

static void expect_write_acquired(unsigned waits)
{
   utt_ODControlWriteAcquire();
   UT_ASSERT_EQ_UINT(waits, ut_waits);
   UT_ASSERT_EQ_INT(TRUE, ControlLock.writer);
   UT_ASSERT_EQ_UINT(0, ControlLock.waiting_writers);
}

static void acquires_immediately_without_other_owners(void)
{
   reset_lock();
   expect_write_acquired(0);
}

static void waits_for_an_active_writer(void)
{
   reset_lock();
   ControlLock.writer = TRUE;
   expect_write_acquired(1);
}

static void waits_for_active_readers(void)
{
   reset_lock();
   ControlLock.readers = 2;
   expect_write_acquired(1);
}

static const UTTestCase ut_cases[] = {
   {"immediate write", acquires_immediately_without_other_owners},
   {"active writer", waits_for_an_active_writer},
   {"active readers", waits_for_active_readers}
};
