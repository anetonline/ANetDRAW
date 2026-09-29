#define UT_CUSTOM_MOCK_strlen

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

static void normalizes_each_supported_line_ending(void)
{
   char crlf[] = "line\r\n";
   char lf[] = "line\n";
   char cr[] = "line\r";
   BOOL complete;
   UT_ASSERT_EQ_UINT(4, utt_ODStringNormalizeLine(crlf, &complete));
   UT_ASSERT_EQ_INT(TRUE, complete);
   UT_ASSERT_EQ_INT(0, strcmp("line", crlf));
   UT_ASSERT_EQ_UINT(4, utt_ODStringNormalizeLine(lf, &complete));
   UT_ASSERT_EQ_INT(TRUE, complete);
   UT_ASSERT_EQ_UINT(4, utt_ODStringNormalizeLine(cr, &complete));
   UT_ASSERT_EQ_INT(FALSE, complete);
}

static void leaves_unterminated_physical_lines_and_empty_strings(void)
{
   char text[] = "line";
   char empty[] = "";
   BOOL complete = TRUE;
   UT_ASSERT_EQ_UINT(4, utt_ODStringNormalizeLine(text, &complete));
   UT_ASSERT_EQ_INT(FALSE, complete);
   UT_ASSERT_EQ_UINT(0, utt_ODStringNormalizeLine(empty, &complete));
   UT_ASSERT_EQ_INT(FALSE, complete);
}

static const UTTestCase ut_cases[] = {
   {"line endings", normalizes_each_supported_line_ending},
   {"incomplete lines", leaves_unterminated_physical_lines_and_empty_strings}
};
