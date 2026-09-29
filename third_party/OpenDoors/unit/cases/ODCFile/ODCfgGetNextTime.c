#define UT_CUSTOM_MOCK_atoi

int utm_atoi(const char *text)
{
   int value = 0;
   while(*text >= '0' && *text <= '9')
   {
      value = value * 10 + (*text - '0');
      ++text;
   }
   return value;
}

static void assert_time(char *text, BYTE count, WORD first, WORD second,
   WORD third, char remainder)
{
   char *position = text;
   utt_ODCfgGetNextTime(&position);
   UT_ASSERT_EQ_UINT(count, btTimeNumVals);
   UT_ASSERT_EQ_UINT(first, awTimeVal[0]);
   UT_ASSERT_EQ_UINT(second, awTimeVal[1]);
   UT_ASSERT_EQ_UINT(third, awTimeVal[2]);
   UT_ASSERT_EQ_INT(remainder, *position);
}

static void parses_three_values_and_all_separators(void)
{
   char colon[] = "  12:34:56x";
   char period[] = "1.2.3x";
   char comma[] = "1,2,3x";
   char semicolon[] = "1;2;3x";
   assert_time(colon, 3, 12, 34, 56, 'x');
   assert_time(period, 3, 1, 2, 3, 'x');
   assert_time(comma, 3, 1, 2, 3, 'x');
   assert_time(semicolon, 3, 1, 2, 3, 'x');
}

static void stops_at_non_digit_or_after_three_values(void)
{
   char nondigit[] = "\t9x8";
   char below_digit[] = "/9";
   char fourth[] = "1:2:3:4";
   assert_time(nondigit, 1, 9, 0, 0, 'x');
   assert_time(below_digit, 0, 0, 0, 0, '/');
   assert_time(fourth, 3, 1, 2, 3, '4');
}

static void empty_and_digit_only_inputs_are_accepted(void)
{
   char empty[] = "";
   char spaces[] = " \t";
   char digit[] = "7";
   assert_time(empty, 0, 0, 0, 0, '\0');
   assert_time(spaces, 0, 0, 0, 0, '\0');
   assert_time(digit, 1, 7, 0, 0, '\0');
}

static const UTTestCase ut_cases[] = {
   {"three values and separators", parses_three_values_and_all_separators},
   {"stopping position", stops_at_non_digit_or_after_three_values},
   {"short inputs", empty_and_digit_only_inputs_are_accepted}
};
