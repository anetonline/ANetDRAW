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

static void reports_empty_and_nonempty_states(void)
{
   tODInQueueHandle handle = ut_queue_handle(4, 0, 0);
#ifdef OD_THREAD_SUPPORT
   ut_locks = ut_unlocks = 0;
#endif
   UT_ASSERT(!utt_ODInQueueWaiting(handle));
   ut_queue.nInIndex = 1;
   UT_ASSERT(utt_ODInQueueWaiting(handle));
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT_EQ_INT(2, ut_locks);
   UT_ASSERT_EQ_INT(2, ut_unlocks);
#endif
}

static const UTTestCase ut_cases[] = {
   {"queue waiting state", reports_empty_and_nonempty_states}
};
