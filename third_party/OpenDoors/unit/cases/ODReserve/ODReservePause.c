#define UT_CUSTOM_MOCK_ODTimerStart
#define UT_CUSTOM_MOCK_ODTimerWaitForElapse
static tODMilliSec ut_delay;
void utm_ODTimerStart(tODTimer *timer, tODMilliSec delay)
{ (void)timer; ut_delay = delay; }
void utm_ODTimerWaitForElapse(tODTimer *timer) { (void)timer; }
static void waits_for_the_requested_delay(void)
{ ut_delay = 0; utt_ODReservePause(37); UT_ASSERT_EQ_UINT(37, ut_delay); }
static const UTTestCase ut_cases[] = {
   {"delay", waits_for_the_requested_delay}
};
