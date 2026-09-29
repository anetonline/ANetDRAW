#define UT_CUSTOM_MOCK_ODSessionScreenCell
#define UT_CUSTOM_MOCK_ODSessionScreenMarkDirty
#define UT_CUSTOM_MOCK_memmove

static BYTE ut_cells[32];
static unsigned ut_cell_calls;
static unsigned ut_move_calls;
static unsigned ut_dirty_calls;

BYTE ODFAR *utm_ODSessionScreenCell(INT nColumn, INT nRow)
{
   ++ut_cell_calls;
   return ut_cells + ((nRow * 4 + nColumn) * 2);
}

void *utm_memmove(void *destination, const void *source, size_t count)
{
   BYTE *dest;
   const BYTE *src;
   size_t index;
   ++ut_move_calls;
   UT_ASSERT_EQ_UINT(4, count);
   dest = (BYTE *)destination;
   src = (const BYTE *)source;
   for(index = 0; index < count; ++index)
      dest[index] = src[index];
   return destination;
}

void utm_ODSessionScreenMarkDirty(INT nLeft, INT nTop, INT nRight,
   INT nBottom)
{
   ++ut_dirty_calls;
   UT_ASSERT_EQ_INT(1, nLeft);
   UT_ASSERT_EQ_INT(1, nTop);
   UT_ASSERT_EQ_INT(2, nRight);
   UT_ASSERT_EQ_INT(3, nBottom);
}

static void reset_scroll(void)
{
   unsigned index;
   for(index = 0; index < sizeof(ut_cells); ++index)
      ut_cells[index] = (BYTE)index;
   SessionScreen.nLeft = 1;
   SessionScreen.nTop = 1;
   SessionScreen.nRight = 2;
   SessionScreen.nBottom = 3;
   SessionScreen.btAttribute = 0x1f;
   ut_cell_calls = ut_move_calls = ut_dirty_calls = 0;
}

static void does_nothing_when_scrolling_is_disabled(void)
{
   reset_scroll();
   SessionScreen.bScrolling = FALSE;
   utt_ODSessionScreenScrollUpOneLine();
   UT_ASSERT_EQ_UINT(0, ut_cell_calls);
   UT_ASSERT_EQ_UINT(0, ut_dirty_calls);
}

static void moves_rows_and_clears_the_bottom_row(void)
{
   reset_scroll();
   SessionScreen.bScrolling = TRUE;
   utt_ODSessionScreenScrollUpOneLine();
   UT_ASSERT_EQ_UINT(5, ut_cell_calls);
   UT_ASSERT_EQ_UINT(2, ut_move_calls);
   UT_ASSERT_EQ_UINT(1, ut_dirty_calls);
   UT_ASSERT_EQ_UINT(18, ut_cells[10]);
   UT_ASSERT_EQ_UINT(26, ut_cells[18]);
   UT_ASSERT_EQ_UINT(' ', ut_cells[26]);
   UT_ASSERT_EQ_UINT(0x1f, ut_cells[27]);
   UT_ASSERT_EQ_UINT(' ', ut_cells[28]);
   UT_ASSERT_EQ_UINT(0x1f, ut_cells[29]);
}

static const UTTestCase ut_cases[] = {
   {"scrolling disabled", does_nothing_when_scrolling_is_disabled},
   {"scroll one row", moves_rows_and_clears_the_bottom_row}
};
