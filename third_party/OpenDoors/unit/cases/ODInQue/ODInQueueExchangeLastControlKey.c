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

static void rejects_a_null_queue(void)
{
   chLastControlKey = 's';
   UT_ASSERT_EQ_INT(0,
      utt_ODInQueueExchangeLastControlKey((tODInQueueHandle)0, 'p'));
   UT_ASSERT_EQ_INT('s', chLastControlKey);
}

static void atomically_exchanges_the_value(void)
{
   tODInQueueHandle handle = ut_queue_handle(3, 0, 0);
#ifdef OD_THREAD_SUPPORT
   ut_locks = ut_unlocks = 0;
#endif
   chLastControlKey = 's';
   UT_ASSERT_EQ_INT('s', utt_ODInQueueExchangeLastControlKey(handle, 0));
   UT_ASSERT_EQ_INT(0, chLastControlKey);
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT_EQ_UINT(1, ut_locks);
   UT_ASSERT_EQ_UINT(1, ut_unlocks);
#endif
}

static const UTTestCase ut_cases[] = {
   {"null queue", rejects_a_null_queue},
   {"exchange", atomically_exchanges_the_value}
};
