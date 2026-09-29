#define UT_CUSTOM_MOCK_ODSessionScreenCell
#define UT_CUSTOM_MOCK_ODSessionScreenMarkDirty

static BYTE ut_cells[24];
static unsigned ut_cell_calls;
static unsigned ut_dirty_calls;

BYTE ODFAR *utm_ODSessionScreenCell(INT nColumn, INT nRow)
{
   ++ut_cell_calls;
   return ut_cells + ((nRow * 4 + nColumn) * 2);
}

void utm_ODSessionScreenMarkDirty(INT nLeft, INT nTop, INT nRight,
   INT nBottom)
{
   ++ut_dirty_calls;
   UT_ASSERT_EQ_INT(1, nLeft);
   UT_ASSERT_EQ_INT(1, nTop);
   UT_ASSERT_EQ_INT(2, nRight);
   UT_ASSERT_EQ_INT(2, nBottom);
}

static void ignores_an_unavailable_screen(void)
{
   bSessionScreenAvailable = FALSE;
   ut_cell_calls = ut_dirty_calls = 0;
   utt_ODSessionScreenClear();
   UT_ASSERT_EQ_UINT(0, ut_cell_calls);
   UT_ASSERT_EQ_UINT(0, ut_dirty_calls);
}

static void clears_the_window_and_homes_the_cursor(void)
{
   unsigned index;
   memset(ut_cells, 0x55, sizeof(ut_cells));
   bSessionScreenAvailable = TRUE;
   SessionScreen.nLeft = 1;
   SessionScreen.nTop = 1;
   SessionScreen.nRight = 2;
   SessionScreen.nBottom = 2;
   SessionScreen.btAttribute = 0x1e;
   SessionScreen.nCursorColumn = 1;
   SessionScreen.nCursorRow = 1;
   ut_cell_calls = ut_dirty_calls = 0;
   utt_ODSessionScreenClear();
   UT_ASSERT_EQ_UINT(2, ut_cell_calls);
   UT_ASSERT_EQ_UINT(1, ut_dirty_calls);
   for(index = 0; index < sizeof(ut_cells); index += 2)
   {
      if(index == 10 || index == 12 || index == 18 || index == 20)
      {
         UT_ASSERT_EQ_UINT(' ', ut_cells[index]);
         UT_ASSERT_EQ_UINT(0x1e, ut_cells[index + 1]);
      }
      else
         UT_ASSERT_EQ_UINT(0x55, ut_cells[index]);
   }
   UT_ASSERT_EQ_INT(0, SessionScreen.nCursorColumn);
   UT_ASSERT_EQ_INT(0, SessionScreen.nCursorRow);
}

static const UTTestCase ut_cases[] = {
   {"unavailable", ignores_an_unavailable_screen},
   {"clear window", clears_the_window_and_homes_the_cursor}
};
