#define UT_CUSTOM_MOCK_ODSessionScreenRectValid
#define UT_CUSTOM_MOCK_ODSessionScreenCell
#define UT_CUSTOM_MOCK_ODSessionScreenMarkDirty
#define UT_CUSTOM_MOCK_memmove

static BOOL ut_rect_answers[2];
static unsigned ut_rect_calls;
static BYTE ut_cells[32];
static INT ut_cell_columns[8];
static INT ut_cell_rows[8];
static unsigned ut_cell_calls;
static unsigned ut_dirty_calls;
static INT ut_dirty_left;
static INT ut_dirty_top;
static INT ut_dirty_right;
static INT ut_dirty_bottom;

BOOL utm_ODSessionScreenRectValid(INT nLeft, INT nTop, INT nRight,
   INT nBottom)
{
   BOOL result;
   (void)nLeft;
   (void)nTop;
   (void)nRight;
   (void)nBottom;
   result = ut_rect_answers[ut_rect_calls < 2 ? ut_rect_calls : 1];
   ++ut_rect_calls;
   return result;
}

BYTE ODFAR *utm_ODSessionScreenCell(INT nColumn, INT nRow)
{
   if(ut_cell_calls < DIM(ut_cell_columns))
   {
      ut_cell_columns[ut_cell_calls] = nColumn;
      ut_cell_rows[ut_cell_calls] = nRow;
   }
   ++ut_cell_calls;
   return ut_cells + ((nRow * 4 + nColumn) * 2);
}

void *utm_memmove(void *destination, const void *source, size_t count)
{
   BYTE *dest;
   const BYTE *src;
   size_t index;
   dest = (BYTE *)destination;
   src = (const BYTE *)source;
   if(dest > src && dest < src + count)
   {
      index = count;
      while(index-- > 0)
         dest[index] = src[index];
   }
   else
   {
      for(index = 0; index < count; ++index)
         dest[index] = src[index];
   }
   return destination;
}

void utm_ODSessionScreenMarkDirty(INT nLeft, INT nTop, INT nRight,
   INT nBottom)
{
   ++ut_dirty_calls;
   ut_dirty_left = nLeft;
   ut_dirty_top = nTop;
   ut_dirty_right = nRight;
   ut_dirty_bottom = nBottom;
}

static void reset_copy(void)
{
   unsigned index;
   for(index = 0; index < sizeof(ut_cells); ++index)
      ut_cells[index] = (BYTE)index;
   SessionScreen.nLeft = 0;
   SessionScreen.nTop = 0;
   ut_rect_answers[0] = TRUE;
   ut_rect_answers[1] = TRUE;
   ut_rect_calls = ut_cell_calls = ut_dirty_calls = 0;
}

static void rejects_invalid_source_or_destination_rectangles(void)
{
   reset_copy();
   ut_rect_answers[0] = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenCopyText(1, 1, 2, 2, 1, 2));
   UT_ASSERT_EQ_UINT(1, ut_rect_calls);
   UT_ASSERT_EQ_UINT(0, ut_cell_calls);

   reset_copy();
   ut_rect_answers[1] = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenCopyText(1, 1, 2, 2, 1, 2));
   UT_ASSERT_EQ_UINT(2, ut_rect_calls);
   UT_ASSERT_EQ_UINT(0, ut_cell_calls);
}

static void copies_downward_from_bottom_to_top(void)
{
   reset_copy();
   UT_ASSERT_EQ_INT(TRUE, utt_ODSessionScreenCopyText(1, 1, 2, 2, 1, 2));
   UT_ASSERT_EQ_UINT(4, ut_cell_calls);
   UT_ASSERT_EQ_INT(1, ut_cell_rows[0]);
   UT_ASSERT_EQ_INT(2, ut_cell_rows[1]);
   UT_ASSERT_EQ_INT(0, ut_cell_rows[2]);
   UT_ASSERT_EQ_INT(1, ut_cell_rows[3]);
   UT_ASSERT_EQ_UINT(1, ut_dirty_calls);
   UT_ASSERT_EQ_INT(0, ut_dirty_left);
   UT_ASSERT_EQ_INT(1, ut_dirty_top);
   UT_ASSERT_EQ_INT(1, ut_dirty_right);
   UT_ASSERT_EQ_INT(2, ut_dirty_bottom);
   UT_ASSERT_EQ_UINT(0, ut_cells[8]);
   UT_ASSERT_EQ_UINT(8, ut_cells[16]);
}

static void copies_upward_from_top_to_bottom(void)
{
   reset_copy();
   UT_ASSERT_EQ_INT(TRUE, utt_ODSessionScreenCopyText(1, 2, 2, 3, 1, 1));
   UT_ASSERT_EQ_UINT(4, ut_cell_calls);
   UT_ASSERT_EQ_INT(1, ut_cell_rows[0]);
   UT_ASSERT_EQ_INT(0, ut_cell_rows[1]);
   UT_ASSERT_EQ_INT(2, ut_cell_rows[2]);
   UT_ASSERT_EQ_INT(1, ut_cell_rows[3]);
   UT_ASSERT_EQ_UINT(8, ut_cells[0]);
   UT_ASSERT_EQ_UINT(16, ut_cells[8]);
}

static const UTTestCase ut_cases[] = {
   {"invalid rectangles", rejects_invalid_source_or_destination_rectangles},
   {"downward overlap", copies_downward_from_bottom_to_top},
   {"upward overlap", copies_upward_from_top_to_bottom}
};
