#define UT_CUSTOM_MOCK_time
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

time_t utm_time(time_t *result)
{
   UT_ASSERT(result == NULL);
   return (time_t)54321;
}

static void stores_current_activity_time(void)
{
   tODInQueueHandle handle = ut_queue_handle(4, 0, 0);
#ifdef OD_THREAD_SUPPORT
   ut_locks = ut_unlocks = 0;
#endif
   utt_ODInQueueResetLastActivity(handle);
   UT_ASSERT_EQ_INT(54321, ut_queue.nLastActivityTime);
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT_EQ_INT(1, ut_locks);
   UT_ASSERT_EQ_INT(1, ut_unlocks);
#endif
}

static const UTTestCase ut_cases[] = {
   {"reset last activity", stores_current_activity_time}
};
