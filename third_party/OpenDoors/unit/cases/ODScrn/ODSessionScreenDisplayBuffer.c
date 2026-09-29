#define UT_CUSTOM_MOCK_ODSessionScreenDisplayChar

static unsigned ut_calls;
static unsigned ut_chars[4];

void utm_ODSessionScreenDisplayChar(unsigned char ch)
{
   if(ut_calls < DIM(ut_chars))
      ut_chars[ut_calls] = (unsigned)ch;
   ++ut_calls;
}

static void forwards_exactly_the_requested_bytes(void)
{
   char text[] = {'A', (char)0xff, 'B'};
   ut_calls = 0;
   utt_ODSessionScreenDisplayBuffer(text, 0);
   UT_ASSERT_EQ_UINT(0, ut_calls);
   utt_ODSessionScreenDisplayBuffer(text, 3);
   UT_ASSERT_EQ_UINT(3, ut_calls);
   UT_ASSERT_EQ_UINT('A', ut_chars[0]);
   UT_ASSERT_EQ_UINT(255, ut_chars[1]);
   UT_ASSERT_EQ_UINT('B', ut_chars[2]);
}

static const UTTestCase ut_cases[] = {
   {"display bytes", forwards_exactly_the_requested_bytes}
};
