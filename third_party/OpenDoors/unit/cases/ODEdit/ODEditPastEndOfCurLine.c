#define UT_CUSTOM_MOCK_ODEditBufferGetLineLength
static UINT ut_line_length;

UINT utm_ODEditBufferGetLineLength(tEditInstance *instance, UINT line)
{
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT_EQ_UINT(instance->unCurrentLine, line);
   return(ut_line_length);
}

static void distinguishes_end_from_past_end(void)
{
   tEditInstance instance;

   instance.unCurrentLine = 3;
   ut_line_length = 7;
   instance.unCurrentColumn = 6;
   UT_ASSERT_EQ_INT(FALSE, utt_ODEditPastEndOfCurLine(&instance));
   instance.unCurrentColumn = 7;
   UT_ASSERT_EQ_INT(FALSE, utt_ODEditPastEndOfCurLine(&instance));
   instance.unCurrentColumn = 8;
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditPastEndOfCurLine(&instance));
}

static const UTTestCase ut_cases[] = {
   {"past line end", distinguishes_end_from_past_end}
};
