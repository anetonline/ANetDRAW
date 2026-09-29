#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock
#define UT_CUSTOM_MOCK_ODFrameControlStateChanged
#define UT_CUSTOM_MOCK_memcpy

static unsigned ut_notifications;

void utm_ODMutexLock(tODMutex *pMutex) { UT_ASSERT_EQ_PTR(&KernelStateLock, pMutex); }
void utm_ODMutexUnlock(tODMutex *pMutex) { UT_ASSERT_EQ_PTR(&KernelStateLock, pMutex); }
void utm_ODFrameControlStateChanged(void) { ++ut_notifications; }
void *utm_memcpy(void *destination, const void *source, size_t size)
{
   unsigned char *to = destination;
   const unsigned char *from = source;
   size_t index;
   for(index = 0; index < size; ++index)
      to[index] = from[index];
   return(destination);
}

static void caches_application_state_only_when_the_queue_is_empty(void)
{
   tODUIChange queued;

   memset(&od_control, 0, sizeof(od_control));
   memset(&UIState, 0, sizeof(UIState));
   strcpy(od_control.od_prog_name, "Test Door");
   strcpy(od_control.user_name, "Alice");
   od_control.user_timelimit = 42;
   od_control.od_disable = 0x1234;
   od_control.od_user_keyboard_on = TRUE;
   bKernelStateLockInitialized = TRUE;
   pPendingUIHead = pPendingUITail = NULL;
   ut_notifications = 0;

   bKernelStateLockInitialized = FALSE;
   UT_ASSERT(!utt_ODKrnlRefreshUIState());
   bKernelStateLockInitialized = TRUE;
   UT_ASSERT(utt_ODKrnlRefreshUIState());
   UT_ASSERT(strcmp(UIState.szProgramName, "Test Door") == 0);
   UT_ASSERT(strcmp(UIState.szUserName, "Alice") == 0);
   UT_ASSERT_EQ_INT(42, UIState.nTimeLimit);
   UT_ASSERT_EQ_UINT(0x1234, UIState.wDisable);
   UT_ASSERT(UIState.bUserKeyboardOn);
   UT_ASSERT_EQ_UINT(1, ut_notifications);

   memset(&queued, 0, sizeof(queued));
   pPendingUIHead = pPendingUITail = &queued;
   od_control.user_timelimit = 99;
   UT_ASSERT(!utt_ODKrnlRefreshUIState());
   UT_ASSERT_EQ_INT(42, UIState.nTimeLimit);
   UT_ASSERT_EQ_UINT(1, ut_notifications);
   pPendingUIHead = pPendingUITail = NULL;
}

static const UTTestCase ut_cases[] = {
   {"empty queue merge", caches_application_state_only_when_the_queue_is_empty}
};
#else
static void unavailable_outside_windows(void) { UT_ASSERT(TRUE); }
static const UTTestCase ut_cases[] = {
   {"not compiled", unavailable_outside_windows}
};
#endif
