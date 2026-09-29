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

static void releases_one_reservation(void)
{
   tODInQueueHandle handle = ut_queue_handle(4, 0, 0);
   ut_queue.nReservedEntries = 2;
#ifdef OD_THREAD_SUPPORT
   ut_locks = ut_unlocks = 0;
#endif
   utt_ODInQueueCancelReservedEvent(handle);
   UT_ASSERT_EQ_INT(1, ut_queue.nReservedEntries);
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT_EQ_UINT(1, ut_locks);
   UT_ASSERT_EQ_UINT(1, ut_unlocks);
#endif
}

static void ignores_invalid_or_empty_reservations(void)
{
   tODInQueueHandle handle = ut_queue_handle(4, 0, 0);
   utt_ODInQueueCancelReservedEvent((tODInQueueHandle)0);
   utt_ODInQueueCancelReservedEvent(handle);
   UT_ASSERT_EQ_INT(0, ut_queue.nReservedEntries);
}

static const UTTestCase ut_cases[] = {
   {"release", releases_one_reservation},
   {"invalid reservation", ignores_invalid_or_empty_reservations}
};
