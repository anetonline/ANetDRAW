#define UT_CUSTOM_MOCK_time
#ifdef ODPLAT_NIX
#define UT_CUSTOM_MOCK_sigemptyset
#define UT_CUSTOM_MOCK_sigaddset
#define UT_CUSTOM_MOCK_sigprocmask
#endif
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODMutexInitialize
#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#endif

static time_t ut_times[2];
static unsigned ut_time_calls;
#ifdef ODPLAT_NIX
static unsigned ut_empty_calls;
static unsigned ut_add_calls;
static unsigned ut_mask_calls;
#endif
#ifdef ODPLAT_WIN32
static tODResult ut_mutex_result;
static unsigned ut_initialize_calls;
static unsigned ut_lock_calls;
static unsigned ut_unlock_calls;
static tODWindowsSubsystem ut_subsystem;
#endif

time_t utm_time(time_t *storage)
{
   UT_ASSERT_NULL(storage);
   UT_ASSERT(ut_time_calls < 2);
   return(ut_times[ut_time_calls++]);
}

#ifdef ODPLAT_NIX
int utm_sigemptyset(sigset_t *set)
{
   UT_ASSERT_NOT_NULL(set);
   ++ut_empty_calls;
   return(0);
}

int utm_sigaddset(sigset_t *set, int signal_number)
{
   UT_ASSERT_NOT_NULL(set);
   UT_ASSERT_EQ_INT(SIGHUP, signal_number);
   ++ut_add_calls;
   return(0);
}

int utm_sigprocmask(int operation, const sigset_t *set, sigset_t *old_set)
{
   UT_ASSERT_EQ_INT(SIG_BLOCK, operation);
   UT_ASSERT_NOT_NULL(set);
   UT_ASSERT_NULL(old_set);
   ++ut_mask_calls;
   return(0);
}
#endif

#ifdef ODPLAT_WIN32
tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{
   return(ut_subsystem);
}

tODResult utm_ODMutexInitialize(tODMutex *mutex)
{
   UT_ASSERT_EQ_PTR(&KernelStateLock, mutex);
   ++ut_initialize_calls;
   return(ut_mutex_result);
}

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
#endif

static void reset_initialization(void)
{
   ut_times[0] = 100;
   ut_times[1] = 200;
   ut_time_calls = 0;
#ifdef ODPLAT_NIX
   ut_empty_calls = ut_add_calls = ut_mask_calls = 0;
#endif
#ifdef OD_THREAD_SUPPORT
   ut_subsystem = kODWindowsSubsystemGUI;
   ut_mutex_result = kODRCSuccess;
   ut_initialize_calls = ut_lock_calls = ut_unlock_calls = 0;
   bKernelStateLockInitialized = FALSE;
   pPendingUIHead = pPendingUITail = NULL;
#endif
   od_control.od_status_on = FALSE;
   bLastStatusSetting = FALSE;
   nNextStatusUpdateTime = 0;
   nNextTimeDeductTime = 0;
   nKrnlFuncPending = 3;
   bKernelActive = TRUE;
}

static void initializes_cooperative_kernel_state(void)
{
   reset_initialization();
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODKrnlInitialize());
   UT_ASSERT_EQ_UINT(2, ut_time_calls);
   UT_ASSERT(nNextStatusUpdateTime == 100 + STATUS_UPDATE_PERIOD);
   UT_ASSERT(nNextTimeDeductTime == 260);
   UT_ASSERT(bLastStatusSetting);
   UT_ASSERT(od_control.od_status_on);
   UT_ASSERT_EQ_UINT(0, nKrnlFuncPending);
   UT_ASSERT(!bKernelActive);
#ifdef ODPLAT_NIX
   UT_ASSERT_EQ_UINT(1, ut_empty_calls);
   UT_ASSERT_EQ_UINT(1, ut_add_calls);
   UT_ASSERT_EQ_UINT(1, ut_mask_calls);
#endif
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(1, ut_initialize_calls);
   UT_ASSERT(bKernelStateLockInitialized);
   UT_ASSERT_NULL(pPendingUIHead);
   UT_ASSERT_NULL(pPendingUITail);
#endif
}

#ifdef ODPLAT_WIN32
static void console_mode_uses_only_cooperative_kernel_state(void)
{
   reset_initialization();
   ut_subsystem = kODWindowsSubsystemConsole;

   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODKrnlInitialize());
   UT_ASSERT_EQ_UINT(0, ut_initialize_calls);
   UT_ASSERT(!bKernelStateLockInitialized);
   UT_ASSERT_NULL(pPendingUIHead);
   UT_ASSERT_NULL(pPendingUITail);
}

static void reports_state_lock_initialization_failure_and_reuses_the_lock(void)
{
   reset_initialization();
   ut_mutex_result = kODRCGeneralFailure;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure, utt_ODKrnlInitialize());
   UT_ASSERT(!bKernelStateLockInitialized);
   UT_ASSERT_EQ_UINT(0, ut_lock_calls);

   reset_initialization();
   bKernelStateLockInitialized = TRUE;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODKrnlInitialize());
   UT_ASSERT_EQ_UINT(0, ut_initialize_calls);
   UT_ASSERT_EQ_UINT(0, ut_lock_calls);
   UT_ASSERT_EQ_UINT(0, ut_unlock_calls);

   reset_initialization();
   pPendingUIHead = (tODUIChange *)1;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure, utt_ODKrnlInitialize());
   pPendingUIHead = NULL;

   reset_initialization();
   pPendingUITail = (tODUIChange *)1;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure, utt_ODKrnlInitialize());
   pPendingUITail = NULL;
}
#endif

static const UTTestCase ut_cases[] = {
   {"state", initializes_cooperative_kernel_state},
#ifdef ODPLAT_WIN32
   {"console state", console_mode_uses_only_cooperative_kernel_state},
   {"state lock", reports_state_lock_initialization_failure_and_reuses_the_lock}
#endif
};
