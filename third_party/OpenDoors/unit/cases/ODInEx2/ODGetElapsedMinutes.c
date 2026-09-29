#define UT_CUSTOM_MOCK_ODDWordDivide

static unsigned ut_divide_calls;
static DWORD *ut_quotient;
static DWORD ut_dividend;
static DWORD ut_divisor;
static BOOL ut_divide_result;

BOOL utm_ODDWordDivide(DWORD *quotient, DWORD *remainder,
   DWORD dividend, DWORD divisor)
{
   ++ut_divide_calls;
   ut_quotient = quotient;
   UT_ASSERT_NULL(remainder);
   ut_dividend = dividend;
   ut_divisor = divisor;
   return ut_divide_result;
}

static void rejects_invalid_timestamps(void)
{
   DWORD minutes;
   ut_divide_calls = 0;
   UT_ASSERT_EQ_INT(FALSE, utt_ODGetElapsedMinutes(NULL, 0, 1));
   UT_ASSERT_EQ_INT(FALSE,
      utt_ODGetElapsedMinutes(&minutes, (time_t)-1, 1));
   UT_ASSERT_EQ_INT(FALSE,
      utt_ODGetElapsedMinutes(&minutes, 1, (time_t)-1));
   UT_ASSERT_EQ_INT(FALSE, utt_ODGetElapsedMinutes(&minutes, 2, 1));
   UT_ASSERT_EQ_UINT(0, ut_divide_calls);
}

static void delegates_elapsed_seconds_to_dword_division(void)
{
   DWORD minutes;
   ut_divide_calls = 0;
   ut_divide_result = TRUE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODGetElapsedMinutes(&minutes, 120, 367));
   UT_ASSERT_EQ_UINT(1, ut_divide_calls);
   UT_ASSERT_EQ_PTR(&minutes, ut_quotient);
   UT_ASSERT_EQ_UINT(247, ut_dividend);
   UT_ASSERT_EQ_UINT(60, ut_divisor);

   ut_divide_result = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODGetElapsedMinutes(&minutes, 0, 0));
}

static const UTTestCase ut_cases[] = {
   {"invalid timestamps", rejects_invalid_timestamps},
   {"elapsed division", delegates_elapsed_seconds_to_dword_division}
};
