#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock
#define UT_CUSTOM_MOCK_memcpy

static unsigned ut_locks;
static unsigned ut_unlocks;

void utm_ODMutexLock(tODMutex *pMutex) { UT_ASSERT_EQ_PTR(&KernelStateLock, pMutex); ++ut_locks; }
void utm_ODMutexUnlock(tODMutex *pMutex) { UT_ASSERT_EQ_PTR(&KernelStateLock, pMutex); ++ut_unlocks; }
void *utm_memcpy(void *destination, const void *source, size_t size)
{
   unsigned char *to = destination;
   const unsigned char *from = source;
   size_t index;
   for(index = 0; index < size; ++index)
      to[index] = from[index];
   return(destination);
}

static void copies_the_cached_state_under_the_queue_mutex(void)
{
   tODUIState State;

   memset(&UIState, 0, sizeof(UIState));
   memset(&State, 0, sizeof(State));
   strcpy(UIState.szUserName, "Bob");
   UIState.nTimeLimit = 17;
   bKernelStateLockInitialized = TRUE;
   ut_locks = ut_unlocks = 0;

   utt_ODKrnlGetUIState(NULL);
   bKernelStateLockInitialized = FALSE;
   utt_ODKrnlGetUIState(&State);
   bKernelStateLockInitialized = TRUE;

   utt_ODKrnlGetUIState(&State);
   UT_ASSERT(strcmp(State.szUserName, "Bob") == 0);
   UT_ASSERT_EQ_INT(17, State.nTimeLimit);
   UT_ASSERT_EQ_UINT(1, ut_locks);
   UT_ASSERT_EQ_UINT(1, ut_unlocks);
}

static const UTTestCase ut_cases[] = {
   {"cached copy", copies_the_cached_state_under_the_queue_mutex}
};
#else
static void unavailable_outside_windows(void) { UT_ASSERT(TRUE); }
static const UTTestCase ut_cases[] = {
   {"not compiled", unavailable_outside_windows}
};
#endif
