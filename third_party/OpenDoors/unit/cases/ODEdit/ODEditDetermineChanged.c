#define UT_CUSTOM_MOCK_ODEditBufferGetCharacter
#define UT_CUSTOM_MOCK_ODEditBufferGetLineLength
#define UT_CUSTOM_MOCK_strlen

static char *ut_current_lines[2];
static UINT ut_current_lengths[2];

char *utm_ODEditBufferGetCharacter(tEditInstance *instance, UINT line,
   UINT column)
{
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT(line < 2);
   UT_ASSERT_EQ_UINT(0, column);
   return(ut_current_lines[line]);
}

UINT utm_ODEditBufferGetLineLength(tEditInstance *instance, UINT line)
{
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT(line < 2);
   return(ut_current_lengths[line]);
}

size_t utm_strlen(const char *text)
{
   size_t length;
   length = 0;
   while(text[length] != '\0')
      ++length;
   return(length);
}

static void reset_detector(tEditInstance *instance, UINT width, UINT height,
   UINT lines)
{
   instance->unAreaWidth = width;
   instance->unAreaHeight = height;
   instance->unLineScrolledToTop = 0;
   instance->unLinesInBuffer = lines;
}

static BOOL determine(tEditInstance *instance, void *remembered, UINT upper,
   UINT lower, UINT *start_line, UINT *start_column, UINT *finish_line,
   UINT *finish_column)
{
   *start_line = 99;
   *start_column = 99;
   *finish_line = 99;
   *finish_column = 99;
   return(utt_ODEditDetermineChanged(instance, remembered, upper, lower,
      start_line, start_column, finish_line, finish_column));
}

static void reports_unchanged_lines_with_default_boundaries(void)
{
   char current0[] = "abc";
   char current1[] = "de";
   char remembered[12] = "abc\0\0\0de";
   tEditInstance instance;
   UINT start_line;
   UINT start_column;
   UINT finish_line;
   UINT finish_column;

   ut_current_lines[0] = current0;
   ut_current_lines[1] = current1;
   ut_current_lengths[0] = 3;
   ut_current_lengths[1] = 2;
   reset_detector(&instance, 5, 2, 2);
   UT_ASSERT_EQ_INT(FALSE, determine(&instance, remembered,
      REDRAW_NO_BOUNDARY, REDRAW_NO_BOUNDARY, &start_line, &start_column,
      &finish_line, &finish_column));
   UT_ASSERT_EQ_UINT(99, start_line);
   UT_ASSERT_EQ_UINT(99, finish_line);
}

static void bounds_an_intermittently_changed_range(void)
{
   char current[] = "abcde";
   char remembered[6] = "axcye";
   tEditInstance instance;
   UINT start_line;
   UINT start_column;
   UINT finish_line;
   UINT finish_column;

   ut_current_lines[0] = current;
   ut_current_lengths[0] = 5;
   reset_detector(&instance, 5, 1, 1);
   UT_ASSERT_EQ_INT(TRUE, determine(&instance, remembered, 0, 1,
      &start_line, &start_column, &finish_line, &finish_column));
   UT_ASSERT_EQ_UINT(0, start_line);
   UT_ASSERT_EQ_UINT(1, start_column);
   UT_ASSERT_EQ_UINT(0, finish_line);
   UT_ASSERT_EQ_UINT(4, finish_column);
}

static void includes_the_full_longer_current_tail(void)
{
   char current[] = "abcd";
   char remembered[6] = "ab";
   tEditInstance instance;
   UINT start_line;
   UINT start_column;
   UINT finish_line;
   UINT finish_column;

   ut_current_lines[0] = current;
   ut_current_lengths[0] = 4;
   reset_detector(&instance, 5, 1, 1);
   UT_ASSERT_EQ_INT(TRUE, determine(&instance, remembered, 0, 1,
      &start_line, &start_column, &finish_line, &finish_column));
   UT_ASSERT_EQ_UINT(2, start_column);
   UT_ASSERT_EQ_UINT(4, finish_column);
}

static void includes_the_full_longer_remembered_tail(void)
{
   char current[] = "ab";
   char remembered[6] = "abcd";
   tEditInstance instance;
   UINT start_line;
   UINT start_column;
   UINT finish_line;
   UINT finish_column;

   ut_current_lines[0] = current;
   ut_current_lengths[0] = 2;
   reset_detector(&instance, 5, 1, 1);
   UT_ASSERT_EQ_INT(TRUE, determine(&instance, remembered, 0, 1,
      &start_line, &start_column, &finish_line, &finish_column));
   UT_ASSERT_EQ_UINT(2, start_column);
   UT_ASSERT_EQ_UINT(4, finish_column);
}

static void treats_a_line_beyond_the_buffer_as_empty(void)
{
   char remembered[6] = "x";
   tEditInstance instance;
   UINT start_line;
   UINT start_column;
   UINT finish_line;
   UINT finish_column;

   reset_detector(&instance, 5, 1, 0);
   UT_ASSERT_EQ_INT(TRUE, determine(&instance, remembered, 0, 1,
      &start_line, &start_column, &finish_line, &finish_column));
   UT_ASSERT_EQ_UINT(0, start_line);
   UT_ASSERT_EQ_UINT(0, start_column);
   UT_ASSERT_EQ_UINT(1, finish_column);
}

static void accepts_an_empty_explicit_boundary_range(void)
{
   char remembered[6] = "x";
   tEditInstance instance;
   UINT start_line;
   UINT start_column;
   UINT finish_line;
   UINT finish_column;

   reset_detector(&instance, 5, 1, 0);
   UT_ASSERT_EQ_INT(FALSE, determine(&instance, remembered, 1, 1,
      &start_line, &start_column, &finish_line, &finish_column));
}

static const UTTestCase ut_cases[] = {
   {"unchanged", reports_unchanged_lines_with_default_boundaries},
   {"intermittent changes", bounds_an_intermittently_changed_range},
   {"current tail", includes_the_full_longer_current_tail},
   {"remembered tail", includes_the_full_longer_remembered_tail},
   {"missing current line", treats_a_line_beyond_the_buffer_as_empty},
   {"empty boundaries", accepts_an_empty_explicit_boundary_range}
};
