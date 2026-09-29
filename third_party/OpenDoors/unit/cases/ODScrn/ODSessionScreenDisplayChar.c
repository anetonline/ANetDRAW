#define UT_CUSTOM_MOCK_ODSessionScreenCell
#define UT_CUSTOM_MOCK_ODSessionScreenScrollUpOneLine
#define UT_CUSTOM_MOCK_ODSessionScreenMarkDirty
#define UT_CUSTOM_MOCK_ODScrnRingBell

static BYTE ut_cell[2];
static INT ut_cell_column;
static INT ut_cell_row;
static unsigned ut_cell_calls;
static unsigned ut_scroll_calls;
static unsigned ut_dirty_calls;
static unsigned ut_bell_calls;

BYTE ODFAR *utm_ODSessionScreenCell(INT nColumn, INT nRow)
{
   ++ut_cell_calls;
   ut_cell_column = nColumn;
   ut_cell_row = nRow;
   return ut_cell;
}

void utm_ODSessionScreenScrollUpOneLine(void)
{
   ++ut_scroll_calls;
}

void utm_ODSessionScreenMarkDirty(INT nLeft, INT nTop, INT nRight,
   INT nBottom)
{
   ++ut_dirty_calls;
   UT_ASSERT_EQ_INT(ut_cell_column, nLeft);
   UT_ASSERT_EQ_INT(ut_cell_row, nTop);
   UT_ASSERT_EQ_INT(ut_cell_column, nRight);
   UT_ASSERT_EQ_INT(ut_cell_row, nBottom);
}

void utm_ODScrnRingBell(void)
{
   ++ut_bell_calls;
}

static void reset_display(INT nWidth, INT nHeight)
{
   bSessionScreenAvailable = TRUE;
   SessionScreen.nLeft = 2;
   SessionScreen.nTop = 3;
   SessionScreen.nRight = nWidth + 1;
   SessionScreen.nBottom = nHeight + 2;
   SessionScreen.nCursorColumn = 0;
   SessionScreen.nCursorRow = 0;
   SessionScreen.btAttribute = 0x2e;
   ut_cell[0] = ut_cell[1] = 0;
   ut_cell_calls = ut_scroll_calls = ut_dirty_calls = ut_bell_calls = 0;
}

static void ignores_an_unavailable_screen(void)
{
   reset_display(10, 3);
   bSessionScreenAvailable = FALSE;
   utt_ODSessionScreenDisplayChar('X');
   UT_ASSERT_EQ_UINT(0, ut_cell_calls);
}

static void handles_carriage_return_newline_and_backspace(void)
{
   reset_display(10, 3);
   SessionScreen.nCursorColumn = 4;
   utt_ODSessionScreenDisplayChar('\r');
   UT_ASSERT_EQ_INT(0, SessionScreen.nCursorColumn);

   SessionScreen.nCursorRow = 0;
   utt_ODSessionScreenDisplayChar('\n');
   UT_ASSERT_EQ_INT(1, SessionScreen.nCursorRow);
   UT_ASSERT_EQ_UINT(0, ut_scroll_calls);
   SessionScreen.nCursorRow = 2;
   utt_ODSessionScreenDisplayChar('\n');
   UT_ASSERT_EQ_INT(2, SessionScreen.nCursorRow);
   UT_ASSERT_EQ_UINT(1, ut_scroll_calls);

   SessionScreen.nCursorColumn = 0;
   utt_ODSessionScreenDisplayChar('\b');
   UT_ASSERT_EQ_INT(0, SessionScreen.nCursorColumn);
   SessionScreen.nCursorColumn = 3;
   utt_ODSessionScreenDisplayChar('\b');
   UT_ASSERT_EQ_INT(2, SessionScreen.nCursorColumn);
}

static void advances_tabs_with_and_without_wrapping(void)
{
   reset_display(10, 3);
   utt_ODSessionScreenDisplayChar('\t');
   UT_ASSERT_EQ_INT(8, SessionScreen.nCursorColumn);
   UT_ASSERT_EQ_INT(0, SessionScreen.nCursorRow);

   SessionScreen.nCursorColumn = 8;
   utt_ODSessionScreenDisplayChar('\t');
   UT_ASSERT_EQ_INT(0, SessionScreen.nCursorColumn);
   UT_ASSERT_EQ_INT(1, SessionScreen.nCursorRow);
   UT_ASSERT_EQ_UINT(0, ut_scroll_calls);

   SessionScreen.nCursorColumn = 8;
   SessionScreen.nCursorRow = 2;
   utt_ODSessionScreenDisplayChar('\t');
   UT_ASSERT_EQ_INT(0, SessionScreen.nCursorColumn);
   UT_ASSERT_EQ_INT(2, SessionScreen.nCursorRow);
   UT_ASSERT_EQ_UINT(1, ut_scroll_calls);
}

static void rings_the_local_bell(void)
{
   reset_display(10, 3);
   utt_ODSessionScreenDisplayChar('\a');
   UT_ASSERT_EQ_UINT(1, ut_bell_calls);
}

static void writes_printable_characters_and_tracks_wrapping(void)
{
   reset_display(3, 2);
   SessionScreen.nCursorColumn = 0;
   SessionScreen.nCursorRow = 0;
   utt_ODSessionScreenDisplayChar('A');
   UT_ASSERT_EQ_UINT(1, ut_cell_calls);
   UT_ASSERT_EQ_UINT(1, ut_dirty_calls);
   UT_ASSERT_EQ_INT(2, ut_cell_column);
   UT_ASSERT_EQ_INT(3, ut_cell_row);
   UT_ASSERT_EQ_UINT('A', ut_cell[0]);
   UT_ASSERT_EQ_UINT(0x2e, ut_cell[1]);
   UT_ASSERT_EQ_INT(1, SessionScreen.nCursorColumn);

   SessionScreen.nCursorColumn = 2;
   SessionScreen.nCursorRow = 0;
   utt_ODSessionScreenDisplayChar('B');
   UT_ASSERT_EQ_INT(0, SessionScreen.nCursorColumn);
   UT_ASSERT_EQ_INT(1, SessionScreen.nCursorRow);
   UT_ASSERT_EQ_UINT(0, ut_scroll_calls);

   SessionScreen.nCursorColumn = 2;
   SessionScreen.nCursorRow = 1;
   utt_ODSessionScreenDisplayChar('C');
   UT_ASSERT_EQ_INT(0, SessionScreen.nCursorColumn);
   UT_ASSERT_EQ_INT(1, SessionScreen.nCursorRow);
   UT_ASSERT_EQ_UINT(1, ut_scroll_calls);
}

static const UTTestCase ut_cases[] = {
   {"unavailable", ignores_an_unavailable_screen},
   {"line controls", handles_carriage_return_newline_and_backspace},
   {"tabs", advances_tabs_with_and_without_wrapping},
   {"bell", rings_the_local_bell},
   {"printable characters", writes_printable_characters_and_tracks_wrapping}
};
