#define UT_CUSTOM_MOCK_ODEmulateGetTextInfo
#define UT_CUSTOM_MOCK_ODEmulateSetCursorPos
#define UT_CUSTOM_MOCK_ODScrnDisplayString
#define UT_CUSTOM_MOCK_ODScrnEnableScrolling

static unsigned ut_cursor_calls;
static INT ut_columns[4];
static INT ut_rows[4];
static unsigned ut_display_calls;
static char ut_display_text[4][8];
static unsigned ut_scroll_calls;
static BOOL ut_scrolling[2];

void utm_ODEmulateGetTextInfo(tODVScreenInfo *info)
{
   memset(info, 0, sizeof(*info));
   info->curx = 20;
   info->cury = 10;
}

void utm_ODEmulateSetCursorPos(INT column, INT row)
{
   UT_ASSERT(ut_cursor_calls < DIM(ut_columns));
   ut_columns[ut_cursor_calls] = column;
   ut_rows[ut_cursor_calls] = row;
   ++ut_cursor_calls;
}

void ODCALL utm_ODScrnDisplayString(const char *text)
{
   size_t index = 0;
   UT_ASSERT(ut_display_calls < DIM(ut_display_text));
   while(index + 1 < sizeof(ut_display_text[0]) && text[index] != '\0') {
      ut_display_text[ut_display_calls][index] = text[index];
      ++index;
   }
   ut_display_text[ut_display_calls][index] = '\0';
   ++ut_display_calls;
}

void utm_ODScrnEnableScrolling(BOOL enabled)
{
   UT_ASSERT(ut_scroll_calls < DIM(ut_scrolling));
   ut_scrolling[ut_scroll_calls++] = enabled;
}

static void fills_each_row_and_restores_the_cursor(void)
{
   memset(ut_display_text, 0, sizeof(ut_display_text));
   ut_cursor_calls = ut_display_calls = ut_scroll_calls = 0;

   utt_ODEmulateFillArea(3, 4, 5, 5, '#');

   UT_ASSERT_EQ_UINT(3, ut_cursor_calls);
   UT_ASSERT_EQ_INT(3, ut_columns[0]);
   UT_ASSERT_EQ_INT(4, ut_rows[0]);
   UT_ASSERT_EQ_INT(3, ut_columns[1]);
   UT_ASSERT_EQ_INT(5, ut_rows[1]);
   UT_ASSERT_EQ_INT(20, ut_columns[2]);
   UT_ASSERT_EQ_INT(10, ut_rows[2]);
   UT_ASSERT_EQ_UINT(2, ut_display_calls);
   UT_ASSERT(strcmp("###", ut_display_text[0]) == 0);
   UT_ASSERT(strcmp("###", ut_display_text[1]) == 0);
   UT_ASSERT_EQ_UINT(2, ut_scroll_calls);
   UT_ASSERT_EQ_INT(FALSE, ut_scrolling[0]);
   UT_ASSERT_EQ_INT(TRUE, ut_scrolling[1]);
}

static const UTTestCase ut_cases[] = {
   {"filled rectangle", fills_each_row_and_restores_the_cursor}
};
