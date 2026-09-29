#define UT_CUSTOM_MOCK_atoi

#define MOCK_ATOI 300

int utm_atoi(const char *text)
{
   int value = 0;
   ut_mock_called(MOCK_ATOI);
   while(*text >= '0' && *text <= '9')
   {
      value = value * 10 + (*text - '0');
      ++text;
   }
   return value;
}

static void decimal_word_skips_to_first_digit(void)
{
   char text[] = "/:x123tail";
   UT_ASSERT_EQ_UINT(123, utt_ODCfgGetWordDecimal(text));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_ATOI));
}

static void decimal_word_accepts_digit_at_start(void)
{
   char text[] = "42";
   UT_ASSERT_EQ_UINT(42, utt_ODCfgGetWordDecimal(text));
}

static void decimal_word_accepts_empty_input(void)
{
   char text[] = "";
   UT_ASSERT_EQ_UINT(0, utt_ODCfgGetWordDecimal(text));
}

static const UTTestCase ut_cases[] = {
   {"skip non-digits", decimal_word_skips_to_first_digit},
   {"digit at start", decimal_word_accepts_digit_at_start},
   {"empty input", decimal_word_accepts_empty_input}
};
