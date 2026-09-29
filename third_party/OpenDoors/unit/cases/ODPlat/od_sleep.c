#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_ODSyncAPIIsNested
#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed
static unsigned ut_init_calls;
static unsigned ut_entry_calls;
static unsigned ut_exit_calls;
static BOOL ut_init_succeeds;
static BOOL ut_api_nested;
static BOOL ut_public_call_allowed;

BOOL utm_ODSyncAPIIsNested(void) { return(ut_api_nested); }
BOOL utm_ODSyncPublicCallAllowed(void)
{
   if(!ut_public_call_allowed) od_control.od_error = ERR_GENERALFAILURE;
   return(ut_public_call_allowed);
}

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void)
{
   ++ut_entry_calls;
}

void utm_ODSyncAPIExit(void)
{
   ++ut_exit_calls;
}

#ifdef ODPLAT_DOS
#define UT_CUSTOM_MOCK_ODPlatYield
#define UT_CUSTOM_MOCK_ODTimerStart
#define UT_CUSTOM_MOCK_ODTimerElapsed
static unsigned ut_yield_calls;
static unsigned ut_timer_start_calls;
static tODMilliSec ut_expected_duration;
static BOOL ut_elapsed_results[4];
static unsigned ut_elapsed_count;
static unsigned ut_elapsed_index;

void utm_ODPlatYield(void)
{
   ++ut_yield_calls;
}

void utm_ODTimerStart(tODTimer *timer, tODMilliSec duration)
{
   ++ut_timer_start_calls;
   UT_ASSERT_NOT_NULL(timer);
   UT_ASSERT_EQ_UINT(ut_expected_duration, duration);
}

BOOL utm_ODTimerElapsed(tODTimer *timer)
{
   UT_ASSERT_NOT_NULL(timer);
   UT_ASSERT(ut_elapsed_index < ut_elapsed_count);
   return(ut_elapsed_results[ut_elapsed_index++]);
}
#endif

#ifdef ODPLAT_DOS32
#define UT_CUSTOM_MOCK_memset
#define UT_CUSTOM_MOCK_OD32DPMIRealModeInterrupt
#define UT_CUSTOM_MOCK_ODTimerStart
#define UT_CUSTOM_MOCK_ODTimerWaitForElapse
static unsigned ut_interrupt_calls;
static unsigned ut_timer_start_calls;
static unsigned ut_timer_wait_calls;
static tODMilliSec ut_expected_duration;

void *utm_memset(void *memory, int value, size_t size)
{
   unsigned char *byte = (unsigned char *)memory;
   size_t remaining = size;

   UT_ASSERT_EQ_INT(0, value);
   UT_ASSERT_EQ_UINT(sizeof(tOD32RealModeRegisters), size);
   while(remaining != 0)
   {
      *byte++ = 0;
      --remaining;
   }
   return(memory);
}

BOOL utm_OD32DPMIRealModeInterrupt(BYTE interrupt_number,
   tOD32RealModeRegisters *registers)
{
   ++ut_interrupt_calls;
   UT_ASSERT_EQ_UINT(0x2f, interrupt_number);
   UT_ASSERT_NOT_NULL(registers);
   UT_ASSERT_EQ_UINT(0x1680, registers->eax);
   UT_ASSERT_EQ_UINT(0, registers->ebx);
   UT_ASSERT_EQ_UINT(0, registers->ecx);
   UT_ASSERT_EQ_UINT(0, registers->edx);
   return(TRUE);
}

void utm_ODTimerStart(tODTimer *timer, tODMilliSec duration)
{
   ++ut_timer_start_calls;
   UT_ASSERT_NOT_NULL(timer);
   UT_ASSERT_EQ_UINT(ut_expected_duration, duration);
}

void utm_ODTimerWaitForElapse(tODTimer *timer)
{
   ++ut_timer_wait_calls;
   UT_ASSERT_NOT_NULL(timer);
}
#endif

#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_Sleep
static DWORD ut_expected_sleeps[4];
static unsigned ut_sleep_count;
static unsigned ut_sleep_index;

void WINAPI utm_Sleep(DWORD milliseconds)
{
   UT_ASSERT(ut_sleep_index < ut_sleep_count);
   UT_ASSERT_EQ_UINT(ut_expected_sleeps[ut_sleep_index], milliseconds);
   ++ut_sleep_index;
}
#endif

#ifdef ODPLAT_NIX
#define UT_CUSTOM_MOCK_nanosleep
static long ut_expected_seconds[6];
static long ut_expected_nanoseconds[6];
static int ut_nanosleep_results[6];
static unsigned ut_nanosleep_count;
static unsigned ut_nanosleep_index;

int utm_nanosleep(CONST struct timespec *requested,
   struct timespec *remaining)
{
   unsigned index = ut_nanosleep_index++;
   UT_ASSERT(index < ut_nanosleep_count);
   UT_ASSERT_EQ_INT(ut_expected_seconds[index], requested->tv_sec);
   UT_ASSERT_EQ_INT(ut_expected_nanoseconds[index], requested->tv_nsec);
   if(ut_nanosleep_results[index] == EINTR)
   {
      remaining->tv_sec = ut_expected_seconds[index + 1];
      remaining->tv_nsec = ut_expected_nanoseconds[index + 1];
   }
   return(ut_nanosleep_results[index]);
}
#ifndef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_clock_gettime
static unsigned ut_clock_calls;

int utm_clock_gettime(clockid_t clock_id, struct timespec *time_value)
{
   ++ut_clock_calls;
   UT_ASSERT_EQ_INT(CLOCK_REALTIME, clock_id);
   time_value->tv_sec = 99;
   time_value->tv_nsec = 88;
   return(0);
}
#endif
#endif

static void reset_common(void)
{
   bODInitialized = TRUE;
   eODLifecycleState = kODLifecycleActive;
   ut_init_succeeds = TRUE;
   ut_api_nested = FALSE;
   ut_public_call_allowed = TRUE;
   ut_init_calls = 0;
   ut_entry_calls = 0;
   ut_exit_calls = 0;
#ifdef ODPLAT_DOS
   ut_yield_calls = 0;
   ut_timer_start_calls = 0;
   ut_elapsed_count = 0;
   ut_elapsed_index = 0;
#endif
#ifdef ODPLAT_DOS32
   ut_interrupt_calls = 0;
   ut_timer_start_calls = 0;
   ut_timer_wait_calls = 0;
#endif
#ifdef ODPLAT_WIN32
   ut_sleep_count = 0;
   ut_sleep_index = 0;
#endif
#ifdef ODPLAT_NIX
   ut_nanosleep_count = 0;
   ut_nanosleep_index = 0;
#ifndef OD_THREAD_SUPPORT
   ut_clock_calls = 0;
#endif
#endif
}

static void terminal_session_is_rejected(void)
{
   reset_common(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   ut_public_call_allowed = FALSE;
   utt_od_sleep(0);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_init_calls);
   UT_ASSERT_EQ_UINT(0, ut_entry_calls);

   reset_common(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   eODLifecycleState = kODLifecycleFinalizing;
   ut_api_nested = TRUE;
   ut_public_call_allowed = FALSE;
   utt_od_sleep(0);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(0, ut_entry_calls);

   reset_common();
   eODLifecycleState = kODLifecycleFinalizing;
   ut_api_nested = FALSE;
   ut_public_call_allowed = FALSE;
   utt_od_sleep(0);
   UT_ASSERT_EQ_UINT(0, ut_init_calls);
   UT_ASSERT_EQ_UINT(0, ut_entry_calls);
}

#ifdef ODPLAT_DOS
static void sleeps_and_yields_under_dos(void)
{
   reset_common();
   bODInitialized = FALSE;
   utt_od_sleep(0);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_yield_calls);

   ut_expected_duration = 123;
   ut_elapsed_count = 3;
   ut_elapsed_results[0] = FALSE;
   ut_elapsed_results[1] = FALSE;
   ut_elapsed_results[2] = TRUE;
   utt_od_sleep(123);
   UT_ASSERT_EQ_UINT(1, ut_timer_start_calls);
   UT_ASSERT_EQ_UINT(3, ut_elapsed_index);
   UT_ASSERT_EQ_UINT(3, ut_yield_calls);
   UT_ASSERT_EQ_UINT(2, ut_entry_calls);
   UT_ASSERT_EQ_UINT(2, ut_exit_calls);
}
#elif defined(ODPLAT_DOS32)
static void sleeps_and_yields_under_dos32(void)
{
   reset_common();
   bODInitialized = FALSE;
   utt_od_sleep(0);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_interrupt_calls);

   ut_expected_duration = 123;
   utt_od_sleep(123);
   UT_ASSERT_EQ_UINT(1, ut_timer_start_calls);
   UT_ASSERT_EQ_UINT(1, ut_timer_wait_calls);
   UT_ASSERT_EQ_UINT(2, ut_entry_calls);
   UT_ASSERT_EQ_UINT(2, ut_exit_calls);
}
#elif defined(ODPLAT_WIN32)
static void yields_and_sleeps_under_windows(void)
{
   reset_common();
   bODInitialized = FALSE;
   ut_sleep_count = 2;
   ut_expected_sleeps[0] = 0;
   utt_od_sleep(0);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);

   ut_expected_sleeps[1] = 200;
   utt_od_sleep(200);
   UT_ASSERT_EQ_UINT(2, ut_sleep_index);
   UT_ASSERT_EQ_UINT(2, ut_entry_calls);
   UT_ASSERT_EQ_UINT(2, ut_exit_calls);
}
#else
static void sleeps_under_single_threaded_unix(void)
{
   reset_common();
   bODInitialized = FALSE;
   ut_nanosleep_count = 2;
   ut_expected_seconds[0] = 0;
   ut_expected_nanoseconds[0] = 100000;
   ut_nanosleep_results[0] = EINTR;
   ut_expected_seconds[1] = 0;
   ut_expected_nanoseconds[1] = 100000;
   ut_nanosleep_results[1] = 0;
   utt_od_sleep(0);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);

   ut_nanosleep_count = 4;
   ut_expected_seconds[2] = 1;
   ut_expected_nanoseconds[2] = 234000000;
   ut_nanosleep_results[2] = EINTR;
   ut_expected_seconds[3] = 0;
   ut_expected_nanoseconds[3] = 500000000;
   ut_nanosleep_results[3] = 0;
   utt_od_sleep(1234);
   UT_ASSERT_EQ_UINT(2, ut_clock_calls);
   UT_ASSERT_EQ_UINT(4, ut_nanosleep_index);
   UT_ASSERT_EQ_UINT(2, ut_entry_calls);
   UT_ASSERT_EQ_UINT(2, ut_exit_calls);
}
#endif

static const UTTestCase ut_cases[] = {
#ifdef ODPLAT_DOS
   {"DOS sleep", sleeps_and_yields_under_dos}
#elif defined(ODPLAT_DOS32)
   {"DOS32 sleep", sleeps_and_yields_under_dos32}
#elif defined(ODPLAT_WIN32)
   {"Windows sleep", yields_and_sleeps_under_windows}
#else
   {"UNIX sleep", sleeps_under_single_threaded_unix}
#endif
   ,{"terminal session", terminal_session_is_rejected}
};
