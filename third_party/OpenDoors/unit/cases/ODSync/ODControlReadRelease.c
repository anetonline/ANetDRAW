#define UT_CUSTOM_MOCK_ODControlWakeWaiters
#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock

static unsigned ut_wakes;
static unsigned ut_locks;
static unsigned ut_unlocks;

void utm_ODControlWakeWaiters(void) { ++ut_wakes; }
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

static void reset_release(unsigned readers)
{
   ControlLock.readers = readers;
   ut_wakes = 0;
   ut_locks = 0;
   ut_unlocks = 0;
}

static void wakes_waiters_only_after_the_final_reader(void)
{
   reset_release(2);
   utt_ODControlReadRelease();
   UT_ASSERT_EQ_UINT(1, ControlLock.readers);
   UT_ASSERT_EQ_UINT(0, ut_wakes);
   reset_release(1);
   utt_ODControlReadRelease();
   UT_ASSERT_EQ_UINT(0, ControlLock.readers);
   UT_ASSERT_EQ_UINT(1, ut_wakes);
   UT_ASSERT_EQ_UINT(1, ut_locks);
   UT_ASSERT_EQ_UINT(1, ut_unlocks);
}

static const UTTestCase ut_cases[] = {
   {"final reader", wakes_waiters_only_after_the_final_reader}
};
