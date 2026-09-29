#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK_ODDOSReadSessionClock
void utm_ODDOSReadSessionClock(DWORD *days, DWORD *seconds,
   WORD *milliseconds)
{
   *days = 123;
   *seconds = 456;
   *milliseconds = 789;
}
#endif

#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_GetSystemTimeAsFileTime
void WINAPI utm_GetSystemTimeAsFileTime(LPFILETIME time_value)
{
   ULARGE_INTEGER value;
   value.QuadPart = 123456789UL;
   time_value->dwLowDateTime = value.LowPart;
   time_value->dwHighDateTime = value.HighPart;
}
#endif

#ifdef ODPLAT_NIX
#define UT_CUSTOM_MOCK_clock_gettime
int utm_clock_gettime(clockid_t clock_id, struct timespec *time_value)
{
   UT_ASSERT_EQ_INT(CLOCK_REALTIME, clock_id);
   time_value->tv_sec = 123;
   time_value->tv_nsec = 456000000L;
   return(0);
}
#endif

static void captures_the_platform_baseline_and_resets_output(void)
{
   dwODSessionLastSeconds = 91;
   wODSessionLastMilliseconds = 92;
   bODSessionTimeInitialized = FALSE;
   utt_ODSessionTimeInitialize();

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   UT_ASSERT_EQ_UINT(123, dwODSessionStartDays);
   UT_ASSERT_EQ_UINT(456, dwODSessionStartSeconds);
   UT_ASSERT_EQ_UINT(789, wODSessionStartMilliseconds);
#endif
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(123456789UL, ODSessionStartFileTime.QuadPart);
#endif
#ifdef ODPLAT_NIX
   UT_ASSERT_EQ_UINT(123, ODSessionStartTime.tv_sec);
   UT_ASSERT_EQ_UINT(456000000L, ODSessionStartTime.tv_nsec);
#endif
   UT_ASSERT_EQ_UINT(0, dwODSessionLastSeconds);
   UT_ASSERT_EQ_UINT(0, wODSessionLastMilliseconds);
   UT_ASSERT_EQ_INT(TRUE, bODSessionTimeInitialized);
}

static const UTTestCase ut_cases[] = {
   {"baseline", captures_the_platform_baseline_and_resets_output}
};
