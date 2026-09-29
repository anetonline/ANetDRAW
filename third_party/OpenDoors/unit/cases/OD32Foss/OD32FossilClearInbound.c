#define UT_CUSTOM_MOCK_OD32FossilSimpleCall

static unsigned ut_calls;

void utm_OD32FossilSimpleCall(BYTE port, BYTE function_number)
{
   UT_ASSERT_EQ_UINT(12, port);
   UT_ASSERT_EQ_UINT(10, function_number);
   ++ut_calls;
}

static void requests_input_purge(void)
{
   ut_calls = 0;
   utt_OD32FossilClearInbound(12);
   UT_ASSERT_EQ_UINT(1, ut_calls);
}

static const UTTestCase ut_cases[] = {
   {"input purge", requests_input_purge}
};
