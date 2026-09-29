#define UT_CUSTOM_MOCK_ODInQueueResetLastActivity
#define UT_CUSTOM_MOCK_ODSizeMultiply
#define UT_CUSTOM_MOCK_calloc
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_malloc
#ifdef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_ODMutexDestroy
#define UT_CUSTOM_MOCK_ODMutexInitialize
#define UT_CUSTOM_MOCK_ODSemaphoreAlloc
#define UT_CUSTOM_MOCK_ODSemaphoreFree
#endif

#include "common.h"

static tInputQueueInfo ut_info_storage;
static tODInputEvent ut_event_storage[8];
static BOOL ut_size_fails;
static BOOL ut_info_allocation_fails;
static BOOL ut_event_allocation_fails;
static unsigned ut_info_frees;
static unsigned ut_event_frees;
static unsigned ut_activity_resets;
#ifdef OD_THREAD_SUPPORT
static tODResult ut_semaphore_result;
static tODResult ut_mutex_result;
static unsigned ut_semaphore_frees;
static unsigned ut_mutex_destroys;
#endif

static void reset_alloc_fixture(void)
{
   memset(&ut_info_storage, 0xa5, sizeof(ut_info_storage));
   memset(ut_event_storage, 0xa5, sizeof(ut_event_storage));
   ut_size_fails = FALSE;
   ut_info_allocation_fails = FALSE;
   ut_event_allocation_fails = FALSE;
   ut_info_frees = 0;
   ut_event_frees = 0;
   ut_activity_resets = 0;
#ifdef OD_THREAD_SUPPORT
   ut_semaphore_result = kODRCSuccess;
   ut_mutex_result = kODRCSuccess;
   ut_semaphore_frees = 0;
   ut_mutex_destroys = 0;
#endif
}

int utm_ODSizeMultiply(size_t left, size_t right, size_t *result)
{
   UT_ASSERT_EQ_UINT(4, left);
   UT_ASSERT_EQ_UINT(sizeof(tODInputEvent), right);
   UT_ASSERT_NOT_NULL(result);
   if(ut_size_fails) return FALSE;
   *result = left * right;
   return TRUE;
}

void *utm_malloc(size_t size)
{
   UT_ASSERT_EQ_UINT(sizeof(tInputQueueInfo), size);
   return ut_info_allocation_fails ? NULL : &ut_info_storage;
}

void *utm_calloc(size_t count, size_t size)
{
   UT_ASSERT_EQ_UINT(1, count);
   UT_ASSERT_EQ_UINT(4 * sizeof(tODInputEvent), size);
   if(ut_event_allocation_fails) return NULL;
   memset(ut_event_storage, 0, sizeof(ut_event_storage));
   return ut_event_storage;
}

void utm_free(void *memory)
{
   if(memory == &ut_info_storage)
      ++ut_info_frees;
   else if(memory == ut_event_storage)
      ++ut_event_frees;
   else
      UT_ASSERT(FALSE);
}

void utm_ODInQueueResetLastActivity(tODInQueueHandle handle)
{
   UT_ASSERT_EQ_PTR(&ut_info_storage,
      ODHANDLE2PTR(handle, tInputQueueInfo));
   ++ut_activity_resets;
}

#ifdef OD_THREAD_SUPPORT
tODResult utm_ODSemaphoreAlloc(tODSemaphoreHandle *semaphore,
   INT initial_count, INT maximum_count)
{
   UT_ASSERT_NOT_NULL(semaphore);
   UT_ASSERT_EQ_INT(0, initial_count);
   UT_ASSERT_EQ_INT(4, maximum_count);
   if(ut_semaphore_result == kODRCSuccess)
      *semaphore = (tODSemaphoreHandle)(void *)&ut_semaphore_token;
   return ut_semaphore_result;
}

void utm_ODSemaphoreFree(tODSemaphoreHandle semaphore)
{
   UT_ASSERT(semaphore ==
      (tODSemaphoreHandle)(void *)&ut_semaphore_token);
   ++ut_semaphore_frees;
}

tODResult utm_ODMutexInitialize(tODMutex *mutex)
{
   UT_ASSERT_EQ_PTR(&ut_info_storage.QueueMutex, mutex);
   return ut_mutex_result;
}

void utm_ODMutexDestroy(tODMutex *mutex)
{
   UT_ASSERT_EQ_PTR(&ut_info_storage.QueueMutex, mutex);
   ++ut_mutex_destroys;
}
#endif

static void rejects_invalid_arguments(void)
{
   tODInQueueHandle handle = ODPTR2HANDLE(&ut_info_storage,
      tInputQueueInfo);
   reset_alloc_fixture();
   UT_ASSERT_EQ_INT(kODRCInvalidCall, utt_ODInQueueAlloc(NULL, 4));

   UT_ASSERT_EQ_INT(kODRCInvalidCall, utt_ODInQueueAlloc(&handle, 1));
   UT_ASSERT_NULL(ODHANDLE2PTR(handle, tInputQueueInfo));

   handle = ODPTR2HANDLE(&ut_info_storage, tInputQueueInfo);
   ut_size_fails = TRUE;
   UT_ASSERT_EQ_INT(kODRCNoMemory, utt_ODInQueueAlloc(&handle, 4));
   UT_ASSERT_NULL(ODHANDLE2PTR(handle, tInputQueueInfo));
}

static void reports_allocation_failures_and_cleans_up(void)
{
   tODInQueueHandle handle;
   reset_alloc_fixture();
   ut_info_allocation_fails = TRUE;
   UT_ASSERT_EQ_INT(kODRCNoMemory, utt_ODInQueueAlloc(&handle, 4));
   UT_ASSERT_NULL(ODHANDLE2PTR(handle, tInputQueueInfo));
   UT_ASSERT_EQ_UINT(0, ut_info_frees);
   UT_ASSERT_EQ_UINT(0, ut_event_frees);

   reset_alloc_fixture();
   ut_event_allocation_fails = TRUE;
   UT_ASSERT_EQ_INT(kODRCNoMemory, utt_ODInQueueAlloc(&handle, 4));
   UT_ASSERT_NULL(ODHANDLE2PTR(handle, tInputQueueInfo));
   UT_ASSERT_EQ_UINT(1, ut_info_frees);
   UT_ASSERT_EQ_UINT(0, ut_event_frees);
}

#ifdef OD_THREAD_SUPPORT
static void reports_synchronization_failures_and_cleans_up(void)
{
   tODInQueueHandle handle;
   reset_alloc_fixture();
   ut_semaphore_result = kODRCGeneralFailure;
   UT_ASSERT_EQ_INT(kODRCNoMemory, utt_ODInQueueAlloc(&handle, 4));
   UT_ASSERT_EQ_UINT(1, ut_info_frees);
   UT_ASSERT_EQ_UINT(1, ut_event_frees);
   UT_ASSERT_EQ_UINT(0, ut_semaphore_frees);
   UT_ASSERT_EQ_UINT(0, ut_mutex_destroys);

   reset_alloc_fixture();
   ut_mutex_result = kODRCGeneralFailure;
   UT_ASSERT_EQ_INT(kODRCNoMemory, utt_ODInQueueAlloc(&handle, 4));
   UT_ASSERT_EQ_UINT(1, ut_info_frees);
   UT_ASSERT_EQ_UINT(1, ut_event_frees);
   UT_ASSERT_EQ_UINT(1, ut_semaphore_frees);
   UT_ASSERT_EQ_UINT(0, ut_mutex_destroys);
}
#endif

static void initializes_a_queue(void)
{
   tODInQueueHandle handle;
   tInputQueueInfo *info;
   reset_alloc_fixture();
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODInQueueAlloc(&handle, 4));
   info = ODHANDLE2PTR(handle, tInputQueueInfo);
   UT_ASSERT_EQ_PTR(&ut_info_storage, info);
   UT_ASSERT_EQ_PTR(ut_event_storage, info->paEvents);
   UT_ASSERT_EQ_INT(4, info->nQueueEntries);
   UT_ASSERT_EQ_INT(0, info->nInIndex);
   UT_ASSERT_EQ_INT(0, info->nOutIndex);
   UT_ASSERT_EQ_INT(0, info->nReservedEntries);
   UT_ASSERT_EQ_UINT(1, ut_activity_resets);
   UT_ASSERT_EQ_UINT(0, ut_info_frees);
   UT_ASSERT_EQ_UINT(0, ut_event_frees);
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT(info->hItemCountSemaphore ==
      (tODSemaphoreHandle)(void *)&ut_semaphore_token);
#endif
}

static const UTTestCase ut_cases[] = {
   {"invalid arguments", rejects_invalid_arguments},
   {"allocation failures", reports_allocation_failures_and_cleans_up},
#ifdef OD_THREAD_SUPPORT
   {"synchronization failures", reports_synchronization_failures_and_cleans_up},
#endif
   {"success", initializes_a_queue}
};
