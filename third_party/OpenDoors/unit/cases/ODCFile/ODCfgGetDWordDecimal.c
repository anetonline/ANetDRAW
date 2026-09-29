#define UT_CUSTOM_MOCK_atol

#define MOCK_ATOL 301

long utm_atol(const char *text)
{
   long value = 0;
   ut_mock_called(MOCK_ATOL);
   while(*text >= '0' && *text <= '9')
   {
      value = value * 10L + (long)(*text - '0');
      ++text;
   }
   return value;
}

static void decimal_dword_skips_to_first_digit(void)
{
   char text[] = "/:x123456tail";
   UT_ASSERT_EQ_UINT(123456UL, utt_ODCfgGetDWordDecimal(text));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_ATOL));
}

static void decimal_dword_accepts_digit_at_start(void)
{
   char text[] = "42";
   UT_ASSERT_EQ_UINT(42, utt_ODCfgGetDWordDecimal(text));
}

static void decimal_dword_accepts_empty_input(void)
{
   char text[] = "";
   UT_ASSERT_EQ_UINT(0, utt_ODCfgGetDWordDecimal(text));
}

static const UTTestCase ut_cases[] = {
   {"skip non-digits", decimal_dword_skips_to_first_digit},
   {"digit at start", decimal_dword_accepts_digit_at_start},
   {"empty input", decimal_dword_accepts_empty_input}
};
