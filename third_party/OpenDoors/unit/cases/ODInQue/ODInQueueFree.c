#define UT_CUSTOM_MOCK_free
#ifdef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_ODMutexDestroy
#define UT_CUSTOM_MOCK_ODSemaphoreFree
#endif

#include "common.h"

static void *ut_freed[2];
static unsigned ut_free_count;
#ifdef OD_THREAD_SUPPORT
static unsigned ut_semaphore_frees;
static unsigned ut_mutex_destroys;

void utm_ODSemaphoreFree(tODSemaphoreHandle semaphore)
{
   UT_ASSERT(semaphore == ut_queue.hItemCountSemaphore);
   ++ut_semaphore_frees;
}

void utm_ODMutexDestroy(tODMutex *mutex)
{
   UT_ASSERT_EQ_PTR(&ut_queue.QueueMutex, mutex);
   ++ut_mutex_destroys;
}
#endif

void utm_free(void *memory)
{
   UT_ASSERT(ut_free_count < 2);
   ut_freed[ut_free_count++] = memory;
}

static void releases_queue_resources(void)
{
   tODInQueueHandle handle = ut_queue_handle(4, 0, 0);
   ut_free_count = 0;
#ifdef OD_THREAD_SUPPORT
   ut_semaphore_frees = 0;
   ut_mutex_destroys = 0;
#endif
   utt_ODInQueueFree(handle);
   UT_ASSERT_EQ_INT(2, ut_free_count);
   UT_ASSERT_EQ_PTR(ut_events, ut_freed[0]);
   UT_ASSERT_EQ_PTR(&ut_queue, ut_freed[1]);
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT_EQ_INT(1, ut_semaphore_frees);
   UT_ASSERT_EQ_INT(1, ut_mutex_destroys);
#endif
}

static const UTTestCase ut_cases[] = {
   {"release queue", releases_queue_resources}
};
