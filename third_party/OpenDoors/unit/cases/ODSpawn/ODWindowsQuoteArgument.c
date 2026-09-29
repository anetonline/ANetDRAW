static void assert_quote(const char *argument, const char *expected)
{
   char output[160];
   size_t expected_length = strlen(expected);
   memset(output, 0x5a, sizeof(output));
   UT_ASSERT_EQ_UINT(expected_length,
      utt_ODWindowsQuoteArgument(NULL, argument));
   UT_ASSERT_EQ_UINT(expected_length,
      utt_ODWindowsQuoteArgument(output, argument));
   UT_ASSERT_EQ_INT(0, strcmp(expected, output));
   UT_ASSERT_EQ_INT(0, output[expected_length]);
}

static void quotes_the_microsoft_argument_grammar(void)
{
   assert_quote("", "\"\"");
   assert_quote("plain", "plain");
   assert_quote("two words", "\"two words\"");
   assert_quote("tab\targument", "\"tab\targument\"");
   assert_quote("quote\"inside", "quote\\\"inside");
   assert_quote("\\\"", "\\\\\\\"");
   assert_quote("C:\\plain\\", "C:\\plain\\");
   assert_quote("C:\\Program Files\\Common\\",
      "\"C:\\Program Files\\Common\\\\\"");
}

static const UTTestCase ut_cases[] = {
   {"Microsoft quoting", quotes_the_microsoft_argument_grammar}
};
