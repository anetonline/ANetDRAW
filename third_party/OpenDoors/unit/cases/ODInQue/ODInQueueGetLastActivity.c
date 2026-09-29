#ifdef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock
#endif

#include "common.h"

#ifdef OD_THREAD_SUPPORT
static unsigned ut_locks;
static unsigned ut_unlocks;
void utm_ODMutexLock(tODMutex *mutex)
{
   UT_ASSERT_EQ_PTR(&ut_queue.QueueMutex, mutex);
   ++ut_locks;
}
void utm_ODMutexUnlock(tODMutex *mutex)
{
   UT_ASSERT_EQ_PTR(&ut_queue.QueueMutex, mutex);
   ++ut_unlocks;
}
#endif

static void returns_stored_activity_time(void)
{
   tODInQueueHandle handle = ut_queue_handle(4, 0, 0);
   ut_queue.nLastActivityTime = (time_t)12345;
#ifdef OD_THREAD_SUPPORT
   ut_locks = ut_unlocks = 0;
#endif
   UT_ASSERT_EQ_INT(12345, utt_ODInQueueGetLastActivity(handle));
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT_EQ_INT(1, ut_locks);
   UT_ASSERT_EQ_INT(1, ut_unlocks);
#endif
}

static const UTTestCase ut_cases[] = {
   {"last activity", returns_stored_activity_time}
};
