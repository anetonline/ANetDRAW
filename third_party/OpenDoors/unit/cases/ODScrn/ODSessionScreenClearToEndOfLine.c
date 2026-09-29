#define UT_CUSTOM_MOCK_ODSessionScreenCell
#define UT_CUSTOM_MOCK_ODSessionScreenMarkDirty

static BYTE ut_line_cells[12];
static unsigned ut_cell_calls;
static unsigned ut_dirty_calls;

BYTE ODFAR *utm_ODSessionScreenCell(INT nColumn, INT nRow)
{
   ++ut_cell_calls;
   UT_ASSERT_EQ_INT(3, nColumn);
   UT_ASSERT_EQ_INT(2, nRow);
   return ut_line_cells + 2;
}

void utm_ODSessionScreenMarkDirty(INT nLeft, INT nTop, INT nRight,
   INT nBottom)
{
   ++ut_dirty_calls;
   UT_ASSERT_EQ_INT(3, nLeft);
   UT_ASSERT_EQ_INT(2, nTop);
   UT_ASSERT_EQ_INT(5, nRight);
   UT_ASSERT_EQ_INT(2, nBottom);
}

static void ignores_an_unavailable_screen(void)
{
   bSessionScreenAvailable = FALSE;
   ut_cell_calls = ut_dirty_calls = 0;
   utt_ODSessionScreenClearToEndOfLine();
   UT_ASSERT_EQ_UINT(0, ut_cell_calls);
   UT_ASSERT_EQ_UINT(0, ut_dirty_calls);
}

static void clears_from_the_cursor_through_the_right_edge(void)
{
   unsigned index;
   memset(ut_line_cells, 0x55, sizeof(ut_line_cells));
   bSessionScreenAvailable = TRUE;
   SessionScreen.nLeft = 1;
   SessionScreen.nTop = 1;
   SessionScreen.nRight = 5;
   SessionScreen.nCursorColumn = 2;
   SessionScreen.nCursorRow = 1;
   SessionScreen.btAttribute = 0x2f;
   ut_cell_calls = ut_dirty_calls = 0;
   utt_ODSessionScreenClearToEndOfLine();
   UT_ASSERT_EQ_UINT(1, ut_cell_calls);
   UT_ASSERT_EQ_UINT(1, ut_dirty_calls);
   UT_ASSERT_EQ_UINT(0x55, ut_line_cells[0]);
   for(index = 2; index < 8; index += 2)
   {
      UT_ASSERT_EQ_UINT(' ', ut_line_cells[index]);
      UT_ASSERT_EQ_UINT(0x2f, ut_line_cells[index + 1]);
   }
   UT_ASSERT_EQ_UINT(0x55, ut_line_cells[8]);
}

static const UTTestCase ut_cases[] = {
   {"unavailable", ignores_an_unavailable_screen},
   {"clear line tail", clears_from_the_cursor_through_the_right_edge}
};
