static void applies_ftsc_line_termination(void)
{
   tEditInstance instance;
   tODEditOptions options;

   instance.pUserOptions = &options;
   options.TextFormat = FORMAT_FTSC_MESSAGE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditIsEOLForMode(&instance, '\r'));
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditIsEOLForMode(&instance, '\0'));
   UT_ASSERT_EQ_INT(FALSE, utt_ODEditIsEOLForMode(&instance, '\n'));
   UT_ASSERT_EQ_INT(FALSE, utt_ODEditIsEOLForMode(&instance, 'X'));
}

static void applies_general_line_termination(void)
{
   tEditInstance instance;
   tODEditOptions options;

   instance.pUserOptions = &options;
   options.TextFormat = FORMAT_LINE_BREAKS;
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditIsEOLForMode(&instance, '\n'));
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditIsEOLForMode(&instance, '\r'));
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditIsEOLForMode(&instance, '\0'));
   UT_ASSERT_EQ_INT(FALSE, utt_ODEditIsEOLForMode(&instance, 'X'));
}

static const UTTestCase ut_cases[] = {
   {"FTSC EOL", applies_ftsc_line_termination},
   {"general EOL", applies_general_line_termination}
};
