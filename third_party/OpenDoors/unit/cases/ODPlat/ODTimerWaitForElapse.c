#ifdef ODPLAT_DOS
#define UT_CUSTOM_MOCK_ODTimerElapsed
#define UT_CUSTOM_MOCK_od_sleep
static BOOL ut_elapsed_values[2];
static unsigned ut_elapsed_index;
static unsigned ut_sleep_calls;

BOOL utm_ODTimerElapsed(tODTimer *timer)
{
   UT_ASSERT_NOT_NULL(timer);
   UT_ASSERT(ut_elapsed_index < 2);
   return(ut_elapsed_values[ut_elapsed_index++]);
}

void ODCALL utm_od_sleep(tODMilliSec milliseconds)
{
   ++ut_sleep_calls;
   UT_ASSERT_EQ_UINT(0, milliseconds);
}
#elif defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK_OD32BIOSClock
static tODMilliSec ut_ticks[2];
static unsigned ut_tick_index;
static unsigned ut_tick_count;

DWORD utm_OD32BIOSClock(void)
{
   UT_ASSERT(ut_tick_index < ut_tick_count);
   return(ut_ticks[ut_tick_index++]);
}
#else
#define UT_CUSTOM_MOCK_ODTimerLeft
#define UT_CUSTOM_MOCK_od_sleep
static unsigned ut_left_calls;
static unsigned ut_sleep_calls;

tODMilliSec utm_ODTimerLeft(tODTimer *timer)
{
   ++ut_left_calls;
   UT_ASSERT_NOT_NULL(timer);
   return(37);
}

void ODCALL utm_od_sleep(tODMilliSec milliseconds)
{
   ++ut_sleep_calls;
   UT_ASSERT_EQ_UINT(37, milliseconds);
}
#endif

static tODTimer ut_timer;

static void waits_until_the_timer_elapses(void)
{
   memset(&ut_timer, 0, sizeof(ut_timer));
#ifdef ODPLAT_DOS
   ut_elapsed_index = 0;
   ut_sleep_calls = 0;
   ut_elapsed_values[0] = FALSE;
   ut_elapsed_values[1] = TRUE;
   utt_ODTimerWaitForElapse(&ut_timer);
   UT_ASSERT_EQ_UINT(2, ut_elapsed_index);
   UT_ASSERT_EQ_UINT(1, ut_sleep_calls);
#elif defined(ODPLAT_DOS32)
   ut_timer.Start = 100;
   ut_timer.Duration = 10;
   ut_tick_index = 0;
   ut_tick_count = 2;
   ut_ticks[0] = 105;
   ut_ticks[1] = 110;
   utt_ODTimerWaitForElapse(&ut_timer);
   UT_ASSERT_EQ_UINT(2, ut_tick_index);

   ut_timer.Start = OD_DOS32_TICKS_PER_DAY - 5;
   ut_timer.Duration = 7;
   ut_tick_index = 0;
   ut_tick_count = 2;
   ut_ticks[0] = 1;
   ut_ticks[1] = 2;
   utt_ODTimerWaitForElapse(&ut_timer);
   UT_ASSERT_EQ_UINT(2, ut_tick_index);
#else
   ut_left_calls = 0;
   ut_sleep_calls = 0;
   utt_ODTimerWaitForElapse(&ut_timer);
   UT_ASSERT_EQ_UINT(1, ut_left_calls);
   UT_ASSERT_EQ_UINT(1, ut_sleep_calls);
#endif
}

static const UTTestCase ut_cases[] = {
   {"wait", waits_until_the_timer_elapses}
};
