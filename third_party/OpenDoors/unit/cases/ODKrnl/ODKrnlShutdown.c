#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock
#define UT_CUSTOM_MOCK_ODMutexDestroy
#define UT_CUSTOM_MOCK_free

static unsigned ut_lock_calls;
static unsigned ut_unlock_calls;
static unsigned ut_destroy_calls;
static unsigned ut_free_calls;

void utm_ODMutexLock(tODMutex *mutex)
{
   UT_ASSERT_EQ_PTR(&KernelStateLock, mutex);
   ++ut_lock_calls;
}

void utm_ODMutexUnlock(tODMutex *mutex)
{
   UT_ASSERT_EQ_PTR(&KernelStateLock, mutex);
   ++ut_unlock_calls;
}

void utm_ODMutexDestroy(tODMutex *mutex)
{
   UT_ASSERT_EQ_PTR(&KernelStateLock, mutex);
   ++ut_destroy_calls;
}

void utm_free(void *memory)
{
   UT_ASSERT_NOT_NULL(memory);
   ++ut_free_calls;
}
#endif

static void owns_no_resources_that_require_shutdown(void)
{
#ifdef ODPLAT_WIN32
   bKernelStateLockInitialized = FALSE;
#endif
   utt_ODKrnlShutdown();
   UT_ASSERT(TRUE);
}

#ifdef ODPLAT_WIN32
static void releases_the_queue_and_mutex(void)
{
   tODUIChange first;
   tODUIChange second;

   first.pNext = &second;
   second.pNext = NULL;
   pPendingUIHead = &first;
   pPendingUITail = &second;
   bKernelStateLockInitialized = TRUE;
   ut_lock_calls = ut_unlock_calls = ut_destroy_calls = ut_free_calls = 0;

   utt_ODKrnlShutdown();

   UT_ASSERT_EQ_UINT(1, ut_lock_calls);
   UT_ASSERT_EQ_UINT(1, ut_unlock_calls);
   UT_ASSERT_EQ_UINT(1, ut_destroy_calls);
   UT_ASSERT_EQ_UINT(2, ut_free_calls);
   UT_ASSERT_NULL(pPendingUIHead);
   UT_ASSERT_NULL(pPendingUITail);
   UT_ASSERT(!bKernelStateLockInitialized);
}
#endif

static const UTTestCase ut_cases[] = {
   {"no resources", owns_no_resources_that_require_shutdown}
#ifdef ODPLAT_WIN32
   ,{"queue and mutex", releases_the_queue_and_mutex}
#endif
};
