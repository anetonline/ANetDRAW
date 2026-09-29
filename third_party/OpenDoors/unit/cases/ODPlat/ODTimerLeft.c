#ifdef ODPLAT_DOS
#define UT_CUSTOM_MOCK_clock
#ifndef __WATCOMC__
#define UT_CUSTOM_MOCK_ODDWordMultiply
#endif
static clock_t ut_clock_value;
#ifndef __WATCOMC__
static unsigned ut_multiply_calls;
#endif

clock_t utm_clock(void)
{
   return(ut_clock_value);
}

#ifndef __WATCOMC__
DWORD utm_ODDWordMultiply(DWORD multiplicand, DWORD multiplier)
{
   ++ut_multiply_calls;
   UT_ASSERT_EQ_UINT(55, multiplier);
   return(multiplicand * multiplier);
}
#endif
#elif defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK_OD32BIOSClock
static tODMilliSec ut_clock_value;

DWORD utm_OD32BIOSClock(void)
{
   return(ut_clock_value);
}
#elif defined(ODPLAT_WIN32)
#define UT_CUSTOM_MOCK_GetTickCount
static tODMilliSec ut_clock_value;

DWORD WINAPI utm_GetTickCount(void)
{
   return(ut_clock_value);
}
#elif defined(ODPLAT_NIX)
#define UT_CUSTOM_MOCK_clock_gettime
static struct timespec ut_time_value;

int utm_clock_gettime(clockid_t clock_id, struct timespec *value)
{
   UT_ASSERT_EQ_INT(CLOCK_MONOTONIC, clock_id);
   UT_ASSERT_NOT_NULL(value);
   *value = ut_time_value;
   return(0);
}
#endif

static tODTimer ut_timer;

static void reports_time_remaining_and_expiration(void)
{
   memset(&ut_timer, 0, sizeof(ut_timer));
#ifdef ODPLAT_DOS
   ut_timer.Start = (clock_t)100;
   ut_timer.Duration = (clock_t)20;
#ifndef __WATCOMC__
   ut_multiply_calls = 0;
#endif
   ut_clock_value = (clock_t)110;
#ifdef __WATCOMC__
   UT_ASSERT_EQ_UINT(10, utt_ODTimerLeft(&ut_timer));
#else
   UT_ASSERT_EQ_UINT(550, utt_ODTimerLeft(&ut_timer));
   UT_ASSERT_EQ_UINT(1, ut_multiply_calls);
#endif
   ut_clock_value = (clock_t)121;
   UT_ASSERT_EQ_UINT(0, utt_ODTimerLeft(&ut_timer));
   ut_clock_value = (clock_t)99;
   UT_ASSERT_EQ_UINT(0, utt_ODTimerLeft(&ut_timer));
#elif defined(ODPLAT_DOS32)
   ut_timer.Start = 100;
   ut_timer.Duration = 10;
   ut_clock_value = 105;
   UT_ASSERT_EQ_UINT(275, utt_ODTimerLeft(&ut_timer));
   ut_clock_value = 110;
   UT_ASSERT_EQ_UINT(0, utt_ODTimerLeft(&ut_timer));
   ut_timer.Start = OD_DOS32_TICKS_PER_DAY - 5;
   ut_timer.Duration = 7;
   ut_clock_value = 1;
   UT_ASSERT_EQ_UINT(55, utt_ODTimerLeft(&ut_timer));
   ut_clock_value = 2;
   UT_ASSERT_EQ_UINT(0, utt_ODTimerLeft(&ut_timer));
#elif defined(ODPLAT_WIN32)
   ut_timer.Start = 100;
   ut_timer.Duration = 20;
   ut_clock_value = 110;
   UT_ASSERT_EQ_UINT(10, utt_ODTimerLeft(&ut_timer));
   ut_clock_value = 121;
   UT_ASSERT_EQ_UINT(0, utt_ODTimerLeft(&ut_timer));
   ut_clock_value = 99;
   UT_ASSERT_EQ_UINT(0, utt_ODTimerLeft(&ut_timer));
#elif defined(ODPLAT_NIX)
   ut_timer.Start = 100000;
   ut_timer.Duration = 20;
   ut_time_value.tv_sec = 100;
   ut_time_value.tv_nsec = 10000000;
   UT_ASSERT_EQ_UINT(10, utt_ODTimerLeft(&ut_timer));
   ut_time_value.tv_nsec = 20000000;
   UT_ASSERT_EQ_UINT(0, utt_ODTimerLeft(&ut_timer));
   ut_time_value.tv_sec = 99;
   ut_time_value.tv_nsec = 999000000;
   UT_ASSERT_EQ_UINT(0, utt_ODTimerLeft(&ut_timer));
#endif
}

#if defined(ODPLAT_DOS) || defined(ODPLAT_WIN32) || defined(ODPLAT_NIX)
static void reports_time_remaining_across_counter_rollover(void)
{
   memset(&ut_timer, 0, sizeof(ut_timer));
   ut_timer.Start = (tODMilliSec)0xfffffff0UL;
   ut_timer.Duration = 32;

#ifdef ODPLAT_DOS
#ifndef __WATCOMC__
   ut_multiply_calls = 0;
#endif
   ut_clock_value = (clock_t)0xfffffff8UL;
#ifdef __WATCOMC__
   UT_ASSERT_EQ_UINT(24, utt_ODTimerLeft(&ut_timer));
#else
   UT_ASSERT_EQ_UINT(1320, utt_ODTimerLeft(&ut_timer));
#endif
   ut_clock_value = (clock_t)5;
#ifdef __WATCOMC__
   UT_ASSERT_EQ_UINT(11, utt_ODTimerLeft(&ut_timer));
#else
   UT_ASSERT_EQ_UINT(605, utt_ODTimerLeft(&ut_timer));
#endif
   ut_clock_value = (clock_t)16;
   UT_ASSERT_EQ_UINT(0, utt_ODTimerLeft(&ut_timer));
#ifndef __WATCOMC__
   UT_ASSERT_EQ_UINT(2, ut_multiply_calls);
#endif
#elif defined(ODPLAT_WIN32)
   ut_clock_value = (tODMilliSec)0xfffffff8UL;
   UT_ASSERT_EQ_UINT(24, utt_ODTimerLeft(&ut_timer));
   ut_clock_value = 5;
   UT_ASSERT_EQ_UINT(11, utt_ODTimerLeft(&ut_timer));
   ut_clock_value = 16;
   UT_ASSERT_EQ_UINT(0, utt_ODTimerLeft(&ut_timer));
#else
   ut_time_value.tv_sec = 4294967;
   ut_time_value.tv_nsec = 288000000;
   UT_ASSERT_EQ_UINT(24, utt_ODTimerLeft(&ut_timer));
   ut_time_value.tv_nsec = 301000000;
   UT_ASSERT_EQ_UINT(11, utt_ODTimerLeft(&ut_timer));
   ut_time_value.tv_nsec = 312000000;
   UT_ASSERT_EQ_UINT(0, utt_ODTimerLeft(&ut_timer));
#endif
}
#endif

static const UTTestCase ut_cases[] = {
   {"time left", reports_time_remaining_and_expiration},
#if defined(ODPLAT_DOS) || defined(ODPLAT_WIN32) || defined(ODPLAT_NIX)
   {"counter rollover", reports_time_remaining_across_counter_rollover}
#endif
};
