#define UT_CUSTOM_MOCK_ODEditBufferGetCharacter
#define UT_CUSTOM_MOCK_ODEditIsEOLForMode

char *utm_ODEditBufferGetCharacter(tEditInstance *instance, UINT line,
   UINT column)
{
   UT_ASSERT(line < instance->unLinesInBuffer);
   return(instance->papchStartOfLine[line] + column);
}

BOOL utm_ODEditIsEOLForMode(tEditInstance *instance, char character)
{
   UT_ASSERT_NOT_NULL(instance);
   return(character == '\r' || character == '\0');
}

static void returns_last_line_length(void)
{
   static char text[] = "abc\rXYZ";
   static char *lines[] = {text, text + 4};
   tEditInstance instance;

   instance.papchStartOfLine = lines;
   instance.unLinesInBuffer = 2;
   instance.unLineArraySize = 2;
   UT_ASSERT_EQ_UINT(3, utt_ODEditBufferGetLineLength(&instance, 1));
}

static void bounds_nonfinal_length_by_next_line(void)
{
   static char text[] = "abcdef\r";
   static char *lines[] = {text, text + 3};
   tEditInstance instance;

   instance.papchStartOfLine = lines;
   instance.unLinesInBuffer = 2;
   instance.unLineArraySize = 2;
   UT_ASSERT_EQ_UINT(3, utt_ODEditBufferGetLineLength(&instance, 0));
   lines[1] = text + 7;
   UT_ASSERT_EQ_UINT(6, utt_ODEditBufferGetLineLength(&instance, 0));
}

static const UTTestCase ut_cases[] = {
   {"last line", returns_last_line_length},
   {"next-line bound", bounds_nonfinal_length_by_next_line}
};
