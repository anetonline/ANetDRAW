#define UT_CUSTOM_MOCK_ODSessionTimeInitialize

#define UT_TIME_READS 12
static unsigned ut_initialize_calls;
static unsigned ut_read_index;
static unsigned ut_read_count;

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK_ODDOSReadSessionClock
static DWORD ut_days[UT_TIME_READS];
static DWORD ut_seconds[UT_TIME_READS];
static WORD ut_milliseconds[UT_TIME_READS];

static void add_read(DWORD days, DWORD seconds, WORD milliseconds)
{
   UT_ASSERT(ut_read_count < UT_TIME_READS);
   ut_days[ut_read_count] = days;
   ut_seconds[ut_read_count] = seconds;
   ut_milliseconds[ut_read_count++] = milliseconds;
}

void utm_ODDOSReadSessionClock(DWORD *days, DWORD *seconds,
   WORD *milliseconds)
{
   UT_ASSERT(ut_read_index < ut_read_count);
   *days = ut_days[ut_read_index];
   *seconds = ut_seconds[ut_read_index];
   *milliseconds = ut_milliseconds[ut_read_index++];
}
#endif

#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_GetSystemTimeAsFileTime
static ULONGLONG ut_file_times[UT_TIME_READS];

static void add_read(DWORD seconds, WORD milliseconds)
{
   UT_ASSERT(ut_read_count < UT_TIME_READS);
   ut_file_times[ut_read_count++] = (ULONGLONG)seconds * 10000000UL
      + (ULONGLONG)milliseconds * 10000UL;
}

void WINAPI utm_GetSystemTimeAsFileTime(LPFILETIME time_value)
{
   ULARGE_INTEGER value;
   UT_ASSERT(ut_read_index < ut_read_count);
   value.QuadPart = ut_file_times[ut_read_index++];
   time_value->dwLowDateTime = value.LowPart;
   time_value->dwHighDateTime = value.HighPart;
}
#endif

#ifdef ODPLAT_NIX
#define UT_CUSTOM_MOCK_clock_gettime
static struct timespec ut_times[UT_TIME_READS];

static void add_read(DWORD seconds, WORD milliseconds)
{
   UT_ASSERT(ut_read_count < UT_TIME_READS);
   ut_times[ut_read_count].tv_sec = (time_t)seconds;
   ut_times[ut_read_count++].tv_nsec = (long)milliseconds * 1000000L;
}

int utm_clock_gettime(clockid_t clock_id, struct timespec *time_value)
{
   UT_ASSERT_EQ_INT(CLOCK_REALTIME, clock_id);
   UT_ASSERT(ut_read_index < ut_read_count);
   *time_value = ut_times[ut_read_index++];
   return(0);
}
#endif

void utm_ODSessionTimeInitialize(void)
{
   ++ut_initialize_calls;
   bODSessionTimeInitialized = TRUE;
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   dwODSessionStartDays = 100;
   dwODSessionStartSeconds = 86399;
   wODSessionStartMilliseconds = 900;
#endif
#ifdef ODPLAT_WIN32
   ODSessionStartFileTime.QuadPart = (ULONGLONG)100 * 10000000UL
      + (ULONGLONG)900 * 10000UL;
#endif
#ifdef ODPLAT_NIX
   ODSessionStartTime.tv_sec = 100;
   ODSessionStartTime.tv_nsec = 900000000L;
#endif
}

static void reset_fixture(void)
{
   ut_initialize_calls = 0;
   ut_read_index = 0;
   ut_read_count = 0;
   bODSessionTimeInitialized = TRUE;
   dwODSessionLastSeconds = 0;
   wODSessionLastMilliseconds = 0;
   utm_ODSessionTimeInitialize();
   ut_initialize_calls = 0;
}

static void add_after_midnight_read(WORD milliseconds)
{
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   add_read(101, 1, milliseconds);
#else
   add_read(102, milliseconds);
#endif
}

static void calculates_elapsed_time_with_subsecond_borrow(void)
{
   DWORD seconds;
   WORD milliseconds;

   reset_fixture();
   add_after_midnight_read(100);
   utt_ODSessionTimeGet(&seconds, &milliseconds);
   UT_ASSERT_EQ_UINT(1, seconds);
   UT_ASSERT_EQ_UINT(200, milliseconds);
}

static void handles_same_second_and_prebaseline_readings(void)
{
   DWORD seconds;
   WORD milliseconds;

   reset_fixture();
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   dwODSessionStartDays = 100;
   dwODSessionStartSeconds = 100;
   wODSessionStartMilliseconds = 100;
   add_read(100, 100, 300);
   add_read(99, 86399, 990);
   add_read(100, 100, 50);
#else
#ifdef ODPLAT_NIX
   ODSessionStartTime.tv_sec = 100;
   ODSessionStartTime.tv_nsec = 100000000L;
#endif
#ifdef ODPLAT_WIN32
   ODSessionStartFileTime.QuadPart = (ULONGLONG)100 * 10000000UL
      + (ULONGLONG)100 * 10000UL;
#endif
   add_read(100, 300);
   add_read(99, 990);
   add_read(100, 50);
#endif
   utt_ODSessionTimeGet(&seconds, &milliseconds);
   UT_ASSERT_EQ_UINT(0, seconds);
   UT_ASSERT_EQ_UINT(200, milliseconds);

   dwODSessionLastSeconds = 0;
   wODSessionLastMilliseconds = 0;
   utt_ODSessionTimeGet(&seconds, &milliseconds);
   UT_ASSERT_EQ_UINT(0, seconds);
   UT_ASSERT_EQ_UINT(0, milliseconds);

   utt_ODSessionTimeGet(&seconds, &milliseconds);
   UT_ASSERT_EQ_UINT(0, seconds);
   UT_ASSERT_EQ_UINT(0, milliseconds);
}

static void clamps_both_kinds_of_backward_correction(void)
{
   DWORD seconds;
   WORD milliseconds;

   reset_fixture();
   dwODSessionLastSeconds = 2;
   wODSessionLastMilliseconds = 0;
   add_after_midnight_read(100);
   utt_ODSessionTimeGet(&seconds, &milliseconds);
   UT_ASSERT_EQ_UINT(2, seconds);
   UT_ASSERT_EQ_UINT(0, milliseconds);

   dwODSessionLastSeconds = 1;
   wODSessionLastMilliseconds = 500;
   add_after_midnight_read(100);
   utt_ODSessionTimeGet(&seconds, &milliseconds);
   UT_ASSERT_EQ_UINT(1, seconds);
   UT_ASSERT_EQ_UINT(500, milliseconds);
}

static void covers_dos_baseline_ordering(void)
{
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   DWORD seconds;
   WORD milliseconds;

   reset_fixture();
   dwODSessionStartDays = 100;
   dwODSessionStartSeconds = 100;
   wODSessionStartMilliseconds = 100;
   add_read(101, 100, 100);
   add_read(100, 101, 100);
   add_read(100, 100, 100);
   add_read(100, 100, 99);
   add_read(100, 99, 999);
   add_read(99, 86399, 999);

   utt_ODSessionTimeGet(&seconds, &milliseconds);
   utt_ODSessionTimeGet(&seconds, &milliseconds);
   utt_ODSessionTimeGet(&seconds, &milliseconds);
   utt_ODSessionTimeGet(&seconds, &milliseconds);
   utt_ODSessionTimeGet(&seconds, &milliseconds);
   utt_ODSessionTimeGet(&seconds, &milliseconds);
#endif
}

static void initializes_lazily_and_supports_optional_outputs(void)
{
   DWORD seconds = 0;
   WORD milliseconds = 0;

   reset_fixture();
   bODSessionTimeInitialized = FALSE;
   add_after_midnight_read(100);
   add_after_midnight_read(100);
   utt_ODSessionTimeGet(&seconds, NULL);
   utt_ODSessionTimeGet(NULL, &milliseconds);
   UT_ASSERT_EQ_UINT(1, ut_initialize_calls);
   UT_ASSERT_EQ_UINT(1, seconds);
   UT_ASSERT_EQ_UINT(200, milliseconds);
}

static const UTTestCase ut_cases[] = {
   {"borrow", calculates_elapsed_time_with_subsecond_borrow},
   {"clock bounds", handles_same_second_and_prebaseline_readings},
   {"backward clamp", clamps_both_kinds_of_backward_correction},
   {"DOS ordering", covers_dos_baseline_ordering},
   {"optional outputs", initializes_lazily_and_supports_optional_outputs}
};
