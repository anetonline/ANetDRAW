#define UT_CUSTOM_MOCK_ODEditBufferGetCharacter
#define UT_CUSTOM_MOCK_ODEditBufferGetLineLength
#define UT_CUSTOM_MOCK_ODEditGetActualCurPos
#define UT_CUSTOM_MOCK_od_set_cursor
#define UT_CUSTOM_MOCK_od_disp
#define UT_CUSTOM_MOCK_od_clr_line
#define UT_CUSTOM_MOCK_od_repeat

static char ut_line0[] = "hello";
static char ut_line1[] = "world";
static char *ut_lines[2];
static UINT ut_lengths[2];
static UINT ut_actual_row;
static UINT ut_actual_column;
static UINT ut_cursor_rows[4];
static UINT ut_cursor_columns[4];
static unsigned ut_cursor_calls;
static const char *ut_disp_text[4];
static INT ut_disp_lengths[4];
static unsigned ut_disp_calls;
static unsigned ut_clear_calls;
static BYTE ut_repeat_counts[4];
static unsigned ut_repeat_calls;

char *utm_ODEditBufferGetCharacter(tEditInstance *instance, UINT line,
   UINT column)
{
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT(line < 2);
   UT_ASSERT_EQ_UINT(0, column);
   return(ut_lines[line]);
}

UINT utm_ODEditBufferGetLineLength(tEditInstance *instance, UINT line)
{
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT(line < 2);
   return(ut_lengths[line]);
}

void utm_ODEditGetActualCurPos(tEditInstance *instance, UINT *row,
   UINT *column)
{
   UT_ASSERT_NOT_NULL(instance);
   *row = ut_actual_row;
   *column = ut_actual_column;
}

void ODCALL utm_od_set_cursor(INT row, INT column)
{
   UT_ASSERT(ut_cursor_calls < 4);
   ut_cursor_rows[ut_cursor_calls] = (UINT)row;
   ut_cursor_columns[ut_cursor_calls] = (UINT)column;
   ++ut_cursor_calls;
}

void ODCALL utm_od_disp(const char *text, INT length, BOOL local_echo)
{
   UT_ASSERT(ut_disp_calls < 4);
   UT_ASSERT_EQ_INT(TRUE, local_echo);
   ut_disp_text[ut_disp_calls] = text;
   ut_disp_lengths[ut_disp_calls] = length;
   ++ut_disp_calls;
}

void ODCALL utm_od_clr_line(void)
{
   ++ut_clear_calls;
}

void ODCALL utm_od_repeat(char character, BYTE count)
{
   UT_ASSERT_EQ_INT(' ', character);
   UT_ASSERT(ut_repeat_calls < 4);
   ut_repeat_counts[ut_repeat_calls++] = count;
}

static void reset_subarea(tEditInstance *instance, tODEditOptions *options)
{
   ut_lines[0] = ut_line0;
   ut_lines[1] = ut_line1;
   ut_lengths[0] = 5;
   ut_lengths[1] = 5;
   instance->pUserOptions = options;
   instance->unLineScrolledToTop = 0;
   instance->unLinesInBuffer = 2;
   instance->unAreaWidth = 10;
   options->nAreaTop = 3;
   options->nAreaLeft = 4;
   options->nAreaRight = OD_SCREEN_WIDTH - 1;
   ut_actual_row = 3;
   ut_actual_column = 4;
   ut_cursor_calls = 0;
   ut_disp_calls = 0;
   ut_clear_calls = 0;
   ut_repeat_calls = 0;
}

static void accepts_an_empty_redraw_range(void)
{
   tEditInstance instance;
   tODEditOptions options;
   reset_subarea(&instance, &options);
   utt_ODEditRedrawSubArea(&instance, 2, 0, 1, 0);
   UT_ASSERT_EQ_UINT(0, ut_cursor_calls);
   UT_ASSERT_EQ_UINT(0, ut_disp_calls);
   UT_ASSERT_EQ_UINT(0, ut_clear_calls);
   UT_ASSERT_EQ_UINT(0, ut_repeat_calls);
}

static void draws_a_clipped_range_without_moving_an_aligned_cursor(void)
{
   tEditInstance instance;
   tODEditOptions options;
   reset_subarea(&instance, &options);
   ut_actual_row = 3;
   ut_actual_column = 5;
   utt_ODEditRedrawSubArea(&instance, 0, 1, 0, 4);
   UT_ASSERT_EQ_UINT(0, ut_cursor_calls);
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
   UT_ASSERT_EQ_PTR(ut_line0 + 1, ut_disp_text[0]);
   UT_ASSERT_EQ_INT(3, ut_disp_lengths[0]);
   UT_ASSERT_EQ_UINT(0, ut_repeat_calls);
}

static void moves_for_each_independently_changed_cursor_coordinate(void)
{
   tEditInstance instance;
   tODEditOptions options;

   reset_subarea(&instance, &options);
   ut_actual_row = 9;
   ut_actual_column = 5;
   utt_ODEditRedrawSubArea(&instance, 0, 1, 0, 8);
   UT_ASSERT_EQ_UINT(1, ut_cursor_calls);
   UT_ASSERT_EQ_UINT(3, ut_cursor_rows[0]);
   UT_ASSERT_EQ_UINT(5, ut_cursor_columns[0]);
   UT_ASSERT_EQ_UINT(1, ut_repeat_calls);
   UT_ASSERT_EQ_UINT(3, ut_repeat_counts[0]);

   reset_subarea(&instance, &options);
   ut_actual_row = 3;
   ut_actual_column = 9;
   utt_ODEditRedrawSubArea(&instance, 0, 1, 0, 8);
   UT_ASSERT_EQ_UINT(1, ut_cursor_calls);
   UT_ASSERT_EQ_UINT(3, ut_cursor_rows[0]);
   UT_ASSERT_EQ_UINT(5, ut_cursor_columns[0]);
}

static void redraws_multiple_lines_through_the_screen_edge(void)
{
   tEditInstance instance;
   tODEditOptions options;
   reset_subarea(&instance, &options);
   options.nAreaRight = OD_SCREEN_WIDTH;
   utt_ODEditRedrawSubArea(&instance, 0, 0, 1, 10);
   UT_ASSERT_EQ_UINT(1, ut_cursor_calls);
   UT_ASSERT_EQ_UINT(4, ut_cursor_rows[0]);
   UT_ASSERT_EQ_UINT(4, ut_cursor_columns[0]);
   UT_ASSERT_EQ_UINT(2, ut_disp_calls);
   UT_ASSERT_EQ_PTR(ut_line0, ut_disp_text[0]);
   UT_ASSERT_EQ_PTR(ut_line1, ut_disp_text[1]);
   UT_ASSERT_EQ_UINT(2, ut_clear_calls);
   UT_ASSERT_EQ_UINT(0, ut_repeat_calls);
}

static void fills_to_a_non_screen_area_edge(void)
{
   tEditInstance instance;
   tODEditOptions options;
   reset_subarea(&instance, &options);
   utt_ODEditRedrawSubArea(&instance, 0, 0, 0, 10);
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
   UT_ASSERT_EQ_UINT(0, ut_clear_calls);
   UT_ASSERT_EQ_UINT(1, ut_repeat_calls);
   UT_ASSERT_EQ_UINT(5, ut_repeat_counts[0]);
}

static void fills_a_changed_range_beyond_the_buffer(void)
{
   tEditInstance instance;
   tODEditOptions options;
   reset_subarea(&instance, &options);
   instance.unLineScrolledToTop = 2;
   utt_ODEditRedrawSubArea(&instance, 0, 0, 0, 5);
   UT_ASSERT_EQ_UINT(0, ut_disp_calls);
   UT_ASSERT_EQ_UINT(1, ut_repeat_calls);
   UT_ASSERT_EQ_UINT(5, ut_repeat_counts[0]);
}

static const UTTestCase ut_cases[] = {
   {"empty range", accepts_an_empty_redraw_range},
   {"clipped text", draws_a_clipped_range_without_moving_an_aligned_cursor},
   {"cursor coordinates", moves_for_each_independently_changed_cursor_coordinate},
   {"screen edge", redraws_multiple_lines_through_the_screen_edge},
   {"area edge", fills_to_a_non_screen_area_edge},
   {"missing buffer line", fills_a_changed_range_beyond_the_buffer}
};
