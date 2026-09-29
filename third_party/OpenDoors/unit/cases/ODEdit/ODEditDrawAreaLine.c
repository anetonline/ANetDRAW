#define UT_CUSTOM_MOCK_od_set_cursor
#define UT_CUSTOM_MOCK_ODEditBufferGetLineLength
#define UT_CUSTOM_MOCK_ODEditBufferGetCharacter
#define UT_CUSTOM_MOCK_od_disp
#define UT_CUSTOM_MOCK_od_clr_line
#define UT_CUSTOM_MOCK_od_repeat

static char ut_text[] = "hello";
static UINT ut_line_length;
static unsigned ut_length_calls;
static unsigned ut_character_calls;
static unsigned ut_disp_calls;
static unsigned ut_clear_calls;
static unsigned ut_repeat_calls;
static BYTE ut_repeat_count;
static UINT ut_cursor_row;
static UINT ut_cursor_column;

void ODCALL utm_od_set_cursor(INT row, INT column)
{
   ut_cursor_row = (UINT)row;
   ut_cursor_column = (UINT)column;
}

UINT utm_ODEditBufferGetLineLength(tEditInstance *instance, UINT line)
{
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT(line < instance->unLinesInBuffer);
   ++ut_length_calls;
   return(ut_line_length);
}

char *utm_ODEditBufferGetCharacter(tEditInstance *instance, UINT line,
   UINT column)
{
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT(line < instance->unLinesInBuffer);
   UT_ASSERT_EQ_UINT(0, column);
   ++ut_character_calls;
   return(ut_text);
}

void ODCALL utm_od_disp(const char *text, INT count, BOOL raw)
{
   ++ut_disp_calls;
   UT_ASSERT_EQ_PTR(ut_text, text);
   UT_ASSERT_EQ_INT((INT)ut_line_length, count);
   UT_ASSERT_EQ_INT(TRUE, raw);
}

void ODCALL utm_od_clr_line(void)
{
   ++ut_clear_calls;
}

void ODCALL utm_od_repeat(char character, BYTE count)
{
   ++ut_repeat_calls;
   UT_ASSERT_EQ_INT(' ', character);
   ut_repeat_count = count;
}

static void reset_draw(tEditInstance *instance, tODEditOptions *options)
{
   instance->pUserOptions = options;
   instance->unLineScrolledToTop = 2;
   instance->unLinesInBuffer = 4;
   instance->unAreaWidth = 10;
   options->nAreaTop = 5;
   options->nAreaLeft = 7;
   ut_line_length = 5;
   ut_length_calls = 0;
   ut_character_calls = 0;
   ut_disp_calls = 0;
   ut_clear_calls = 0;
   ut_repeat_calls = 0;
   ut_repeat_count = 0;
}

static void displays_an_existing_line_and_clears_to_screen_edge(void)
{
   tEditInstance instance;
   tODEditOptions options;
   reset_draw(&instance, &options);
   options.nAreaRight = OD_SCREEN_WIDTH;
   utt_ODEditDrawAreaLine(&instance, 1);
   UT_ASSERT_EQ_UINT(6, ut_cursor_row);
   UT_ASSERT_EQ_UINT(7, ut_cursor_column);
   UT_ASSERT_EQ_UINT(1, ut_length_calls);
   UT_ASSERT_EQ_UINT(1, ut_character_calls);
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
   UT_ASSERT_EQ_UINT(1, ut_clear_calls);
   UT_ASSERT_EQ_UINT(0, ut_repeat_calls);
}

static void fills_the_remainder_of_an_existing_line(void)
{
   tEditInstance instance;
   tODEditOptions options;
   reset_draw(&instance, &options);
   options.nAreaRight = OD_SCREEN_WIDTH - 1;
   utt_ODEditDrawAreaLine(&instance, 0);
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
   UT_ASSERT_EQ_UINT(0, ut_clear_calls);
   UT_ASSERT_EQ_UINT(1, ut_repeat_calls);
   UT_ASSERT_EQ_UINT(5, ut_repeat_count);
}

static void fills_a_line_beyond_the_end_of_the_buffer(void)
{
   tEditInstance instance;
   tODEditOptions options;
   reset_draw(&instance, &options);
   options.nAreaRight = OD_SCREEN_WIDTH - 1;
   utt_ODEditDrawAreaLine(&instance, 2);
   UT_ASSERT_EQ_UINT(0, ut_length_calls);
   UT_ASSERT_EQ_UINT(0, ut_character_calls);
   UT_ASSERT_EQ_UINT(0, ut_disp_calls);
   UT_ASSERT_EQ_UINT(1, ut_repeat_calls);
   UT_ASSERT_EQ_UINT(10, ut_repeat_count);
}

static const UTTestCase ut_cases[] = {
   {"screen edge", displays_an_existing_line_and_clears_to_screen_edge},
   {"existing line", fills_the_remainder_of_an_existing_line},
   {"missing line", fills_a_line_beyond_the_end_of_the_buffer}
};
