#define UT_CUSTOM_MOCK_ODControlWakeWaiters
#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock

static unsigned ut_wakes;

void utm_ODControlWakeWaiters(void) { ++ut_wakes; }
void utm_ODMutexLock(tODMutex *mutex)
{
   UT_ASSERT_EQ_PTR(&ControlLock.state, mutex);
}
void utm_ODMutexUnlock(tODMutex *mutex)
{
   UT_ASSERT_EQ_PTR(&ControlLock.state, mutex);
}

static void clears_writer_ownership_and_wakes_waiters(void)
{
   ControlLock.writer = TRUE;
   ut_wakes = 0;
   utt_ODControlWriteRelease();
   UT_ASSERT_EQ_INT(FALSE, ControlLock.writer);
   UT_ASSERT_EQ_UINT(1, ut_wakes);
}

static const UTTestCase ut_cases[] = {
   {"write release", clears_writer_ownership_and_wakes_waiters}
};
