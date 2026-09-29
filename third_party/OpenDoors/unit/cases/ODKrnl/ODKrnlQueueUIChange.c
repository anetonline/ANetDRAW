#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock
#define UT_CUSTOM_MOCK_ODFrameControlStateChanged
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_MessageBeep
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem

static unsigned ut_locks;
static unsigned ut_unlocks;
static unsigned ut_notifications;
static unsigned ut_allocations;
static BOOL ut_fail_allocation;
static tODUIChange ut_changes[8];
static tODWindowsSubsystem ut_subsystem;

tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{
   return(ut_subsystem);
}

BOOL WINAPI utm_MessageBeep(UINT type)
{
   UT_ASSERT_EQ_UINT(MB_ICONEXCLAMATION, type);
   return(TRUE);
}

void *utm_malloc(size_t size)
{
   UT_ASSERT_EQ_UINT(sizeof(tODUIChange), size);
   if(ut_fail_allocation)
      return(NULL);
   UT_ASSERT(ut_allocations < DIM(ut_changes));
   return(&ut_changes[ut_allocations++]);
}

void utm_free(void *pMemory) { (void)pMemory; }

void utm_ODMutexLock(tODMutex *pMutex)
{
   UT_ASSERT_EQ_PTR(&KernelStateLock, pMutex);
   ++ut_locks;
}

void utm_ODMutexUnlock(tODMutex *pMutex)
{
   UT_ASSERT_EQ_PTR(&KernelStateLock, pMutex);
   ++ut_unlocks;
}

void utm_ODFrameControlStateChanged(void)
{
   ++ut_notifications;
}

static void queues_ordered_changes_and_updates_the_cache(void)
{
   tODUIChange *pFirst;
   tODUIChange *pSecond;

   memset(&UIState, 0, sizeof(UIState));
   ut_subsystem = kODWindowsSubsystemGUI;
   eODLifecycleState = kODLifecycleActive;
   pPendingUIHead = NULL;
   pPendingUITail = NULL;
   ut_locks = ut_unlocks = ut_notifications = ut_allocations = 0;
   ut_fail_allocation = TRUE;
   UT_ASSERT(!utt_ODKrnlQueueUIChange(kODUIChangeChat, 0, 0));
   ut_fail_allocation = FALSE;

   eODLifecycleState = kODLifecycleExitPending;
   UT_ASSERT(!utt_ODKrnlQueueUIChange(kODUIChangeChat, 0, 0));
   UT_ASSERT_EQ_UINT(0, ut_allocations);
   eODLifecycleState = kODLifecycleActive;

   UT_ASSERT(utt_ODKrnlQueueUIChange(kODUIChangeChat, 0, 0));
   UT_ASSERT(utt_ODKrnlQueueUIChange(kODUIChangeKeyboard, 0, 0));
   UT_ASSERT(utt_ODKrnlQueueUIChange(kODUIChangeSysopNext, 0, 0));
   UT_ASSERT(utt_ODKrnlQueueUIChange(kODUIChangeInactivity, 0, 0));
   UT_ASSERT(utt_ODKrnlQueueUIChange(kODUIChangeTime, 7, 0));
   UT_ASSERT(utt_ODKrnlQueueUIChange(kODUIChangeLockout, 0, 3));
   UT_ASSERT(utt_ODKrnlQueueUIChange(kODUIChangeShutdown, 0, 4));
   UT_ASSERT(utt_ODKrnlQueueUIChange((tODUIChangeType)99, 0, 0));
   pFirst = pPendingUIHead;
   pSecond = pFirst->pNext;
   UT_ASSERT_EQ_PTR(pFirst, pPendingUIHead);
   UT_ASSERT_EQ_PTR(&ut_changes[7], pPendingUITail);
   UT_ASSERT_EQ_INT(kODUIChangeChat, pFirst->Type);
   UT_ASSERT(pFirst->bValue);
   UT_ASSERT_EQ_INT(kODUIChangeKeyboard, pSecond->Type);
   UT_ASSERT(pSecond->bValue);
   UT_ASSERT_EQ_INT(kODUIChangeTime, ut_changes[4].Type);
   UT_ASSERT_EQ_INT(7, ut_changes[4].nValue);
   UT_ASSERT_EQ_UINT(3, ut_changes[5].btReason);
   UT_ASSERT_EQ_UINT(4, ut_changes[6].btReason);
   UT_ASSERT_NULL(pPendingUITail->pNext);
   UT_ASSERT(UIState.bChatActive);
   UT_ASSERT(UIState.bUserKeyboardOn);
   UT_ASSERT(UIState.bSysopNext);
   UT_ASSERT(UIState.bInactivityDisabled);
   UT_ASSERT_EQ_INT(7, UIState.nTimeLimit);
   UT_ASSERT_EQ_UINT(8, ut_locks);
   UT_ASSERT_EQ_UINT(8, ut_unlocks);
   UT_ASSERT_EQ_UINT(8, ut_notifications);

   pPendingUIHead = pPendingUITail = NULL;
}

static void rejects_console_mode_changes_without_synchronization(void)
{
   memset(&UIState, 0, sizeof(UIState));
   eODLifecycleState = kODLifecycleActive;
   pPendingUIHead = pPendingUITail = NULL;
   ut_subsystem = kODWindowsSubsystemConsole;
   ut_locks = ut_unlocks = ut_notifications = ut_allocations = 0;
   ut_fail_allocation = FALSE;

   UT_ASSERT(!utt_ODKrnlQueueUIChange(kODUIChangeChat, 0, 0));
   UT_ASSERT_EQ_UINT(0, ut_allocations);
   UT_ASSERT_EQ_UINT(0, ut_locks);
   UT_ASSERT_EQ_UINT(0, ut_unlocks);
   UT_ASSERT_EQ_UINT(0, ut_notifications);
}

static const UTTestCase ut_cases[] = {
   {"ordered FIFO", queues_ordered_changes_and_updates_the_cache},
   {"console rejection", rejects_console_mode_changes_without_synchronization}
};
#else
static void unavailable_without_thread_support(void) { UT_ASSERT(TRUE); }
static const UTTestCase ut_cases[] = {
   {"not compiled", unavailable_without_thread_support}
};
#endif
