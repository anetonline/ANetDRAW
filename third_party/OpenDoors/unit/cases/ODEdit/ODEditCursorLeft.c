#define UT_CUSTOM_MOCK_ODEditBufferGetLineLength
#define UT_CUSTOM_MOCK_ODEditGotoPreviousLine
static UINT ut_line_length;
static unsigned ut_previous_calls;

UINT utm_ODEditBufferGetLineLength(tEditInstance *instance, UINT line)
{
   UT_ASSERT_EQ_UINT(instance->unCurrentLine, line);
   return(ut_line_length);
}

void utm_ODEditGotoPreviousLine(tEditInstance *instance)
{
   ++ut_previous_calls;
   UT_ASSERT(instance->unCurrentLine > 0);
   --instance->unCurrentLine;
}

static void handles_each_left_boundary(void)
{
   tEditInstance instance;

   ut_line_length = 5;
   ut_previous_calls = 0;
   instance.bWordWrapLongLines = TRUE;
   instance.unCurrentLine = 2;
   instance.unCurrentColumn = 8;
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditCursorLeft(&instance));
   UT_ASSERT_EQ_UINT(5, instance.unCurrentColumn);

   instance.bWordWrapLongLines = FALSE;
   instance.unCurrentColumn = 3;
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditCursorLeft(&instance));
   UT_ASSERT_EQ_UINT(2, instance.unCurrentColumn);

   instance.bWordWrapLongLines = TRUE;
   instance.unCurrentLine = 2;
   instance.unCurrentColumn = 0;
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditCursorLeft(&instance));
   UT_ASSERT_EQ_UINT(1, instance.unCurrentLine);
   UT_ASSERT_EQ_UINT(5, instance.unCurrentColumn);
   UT_ASSERT_EQ_UINT(1, ut_previous_calls);

   instance.unCurrentLine = 0;
   instance.unCurrentColumn = 0;
   UT_ASSERT_EQ_INT(FALSE, utt_ODEditCursorLeft(&instance));
   instance.bWordWrapLongLines = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODEditCursorLeft(&instance));
}

static const UTTestCase ut_cases[] = {
   {"left boundaries", handles_each_left_boundary}
};
