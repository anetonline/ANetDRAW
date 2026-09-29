#define UT_CUSTOM_MOCK_ODTimerLeft
#define UT_CUSTOM_MOCK_ODTimerStart
#define UT_CUSTOM_MOCK_od_get_input

#define MOCK_GET_INPUT 140
#define MOCK_TIMER_LEFT 141
#define MOCK_TIMER_START 142

static BOOL ut_input_results[8];
static unsigned short ut_input_count;
static unsigned short ut_input_index;

static void reset_fixture(void)
{
   memset(ut_input_results, 0, sizeof(ut_input_results));
   ut_input_count = 0;
   ut_input_index = 0;
}

tODMilliSec utm_ODTimerLeft(tODTimer *timer)
{
   (void)timer;
   ut_mock_called(MOCK_TIMER_LEFT);
   return 500;
}

void utm_ODTimerStart(tODTimer *timer, tODMilliSec duration)
{
   UT_ASSERT_NOT_NULL(timer);
   UT_ASSERT_EQ_UINT(660, duration);
   ut_mock_called(MOCK_TIMER_START);
}

BOOL ODCALL utm_od_get_input(tODInputEvent *event, tODMilliSec wait,
   WORD flags)
{
   UT_ASSERT_NOT_NULL(event);
   UT_ASSERT_EQ_UINT(500, wait);
   UT_ASSERT_EQ_UINT(GETIN_RAW, flags);
   UT_ASSERT(ut_input_index < ut_input_count);
   ut_mock_called(MOCK_GET_INPUT);
   return ut_input_results[ut_input_index++];
}

static void unavailable_input_returns(void)
{
   reset_fixture();
   ut_input_results[0] = FALSE;
   ut_input_count = 1;

   utt_ODWaitDiscard(2, 660);

   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_GET_INPUT));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_TIMER_LEFT));
}

static void requested_number_of_bytes_returns_early(void)
{
   reset_fixture();
   ut_input_results[0] = TRUE;
   ut_input_results[1] = TRUE;
   ut_input_count = 2;

   utt_ODWaitDiscard(2, 660);

   UT_ASSERT_EQ_UINT(2, ut_mock_count(MOCK_GET_INPUT));
   UT_ASSERT_EQ_UINT(2, ut_mock_count(MOCK_TIMER_LEFT));
}

static void partial_input_stops_when_input_ends(void)
{
   reset_fixture();
   ut_input_results[0] = TRUE;
   ut_input_results[1] = FALSE;
   ut_input_count = 2;

   utt_ODWaitDiscard(3, 660);

   UT_ASSERT_EQ_UINT(2, ut_mock_count(MOCK_GET_INPUT));
   UT_ASSERT_EQ_UINT(2, ut_mock_count(MOCK_TIMER_LEFT));
}

static const UTTestCase ut_cases[] = {
   {"unavailable input", unavailable_input_returns},
   {"requested bytes", requested_number_of_bytes_returns_early},
   {"partial input", partial_input_stops_when_input_ends}
};
