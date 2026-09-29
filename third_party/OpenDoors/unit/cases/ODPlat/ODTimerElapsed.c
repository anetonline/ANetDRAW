#define UT_CUSTOM_MOCK_ODTimerLeft
static tODMilliSec ut_time_left;
static unsigned ut_time_left_calls;

tODMilliSec utm_ODTimerLeft(tODTimer *timer)
{
   ++ut_time_left_calls;
   UT_ASSERT_NOT_NULL(timer);
   return(ut_time_left);
}

static tODTimer ut_timer;

static void reports_elapsed_and_pending_timers(void)
{
   memset(&ut_timer, 0, sizeof(ut_timer));
   ut_time_left_calls = 0;
   ut_time_left = 1;
   UT_ASSERT_EQ_INT(FALSE, utt_ODTimerElapsed(&ut_timer));
   ut_time_left = 0;
   UT_ASSERT_EQ_INT(TRUE, utt_ODTimerElapsed(&ut_timer));
   UT_ASSERT_EQ_UINT(2, ut_time_left_calls);
}

static const UTTestCase ut_cases[] = {
   {"elapsed state", reports_elapsed_and_pending_timers}
};
