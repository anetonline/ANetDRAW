#ifdef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock
#endif

#include "common.h"

#ifdef OD_THREAD_SUPPORT
static unsigned ut_locks;
static unsigned ut_unlocks;
void utm_ODMutexLock(tODMutex *mutex)
{ UT_ASSERT_EQ_PTR(&ut_queue.QueueMutex, mutex); ++ut_locks; }
void utm_ODMutexUnlock(tODMutex *mutex)
{ UT_ASSERT_EQ_PTR(&ut_queue.QueueMutex, mutex); ++ut_unlocks; }
#endif

static void reserves_only_available_capacity(void)
{
   tODInQueueHandle handle = ut_queue_handle(4, 0, 0);
#ifdef OD_THREAD_SUPPORT
   ut_locks = ut_unlocks = 0;
#endif
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODInQueueReserveEvent(handle));
   UT_ASSERT_EQ_INT(1, ut_queue.nReservedEntries);
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODInQueueReserveEvent(handle));
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODInQueueReserveEvent(handle));
   UT_ASSERT_EQ_INT(kODRCNoMemory, utt_ODInQueueReserveEvent(handle));
   UT_ASSERT_EQ_INT(3, ut_queue.nReservedEntries);
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT_EQ_UINT(4, ut_locks);
   UT_ASSERT_EQ_UINT(4, ut_unlocks);
#endif
}

static void rejects_a_null_queue(void)
{
   UT_ASSERT_EQ_INT(kODRCInvalidCall,
      utt_ODInQueueReserveEvent((tODInQueueHandle)0));
}

static const UTTestCase ut_cases[] = {
   {"capacity", reserves_only_available_capacity},
   {"null queue", rejects_a_null_queue}
};
