#define UT_CUSTOM_MOCK_ODSessionScreenRectValid
#define UT_CUSTOM_MOCK_ODSessionScreenCell
#define UT_CUSTOM_MOCK_ODSessionScreenMarkDirty
#define UT_CUSTOM_MOCK_memcpy

static BOOL ut_rect_valid;
static BYTE ut_destination[24];
static unsigned ut_cell_calls;
static unsigned ut_dirty_calls;

BOOL utm_ODSessionScreenRectValid(INT nLeft, INT nTop, INT nRight,
   INT nBottom)
{
   (void)nLeft;
   (void)nTop;
   (void)nRight;
   (void)nBottom;
   return ut_rect_valid;
}

BYTE ODFAR *utm_ODSessionScreenCell(INT nColumn, INT nRow)
{
   ++ut_cell_calls;
   UT_ASSERT_EQ_INT(3, nColumn);
   return ut_destination + ((nRow - 1) * 8);
}

void *utm_memcpy(void *destination, const void *source, size_t count)
{
   BYTE *dest;
   const BYTE *src;
   size_t index;
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
   UT_ASSERT_EQ_INT(3, nLeft);
   UT_ASSERT_EQ_INT(1, nTop);
   UT_ASSERT_EQ_INT(4, nRight);
   UT_ASSERT_EQ_INT(2, nBottom);
}

static void rejects_each_invalid_request_condition(void)
{
   BYTE input[8] = {0};
   bSessionScreenAvailable = FALSE;
   ut_rect_valid = TRUE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenPutText(1, 1, 2, 2, input));
   bSessionScreenAvailable = TRUE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenPutText(1, 1, 2, 2, NULL));
   ut_rect_valid = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenPutText(1, 1, 2, 2, input));
}

static void copies_each_row_and_marks_the_destination(void)
{
   BYTE input[8];
   unsigned index;
   for(index = 0; index < sizeof(input); ++index)
      input[index] = (BYTE)(index + 1);
   memset(ut_destination, 0, sizeof(ut_destination));
   bSessionScreenAvailable = TRUE;
   SessionScreen.nLeft = 2;
   SessionScreen.nTop = 1;
   ut_rect_valid = TRUE;
   ut_cell_calls = ut_dirty_calls = 0;
   UT_ASSERT_EQ_INT(TRUE, utt_ODSessionScreenPutText(2, 1, 3, 2, input));
   UT_ASSERT_EQ_UINT(2, ut_cell_calls);
   UT_ASSERT_EQ_UINT(1, ut_dirty_calls);
   UT_ASSERT_EQ_UINT(1, ut_destination[0]);
   UT_ASSERT_EQ_UINT(4, ut_destination[3]);
   UT_ASSERT_EQ_UINT(5, ut_destination[8]);
   UT_ASSERT_EQ_UINT(8, ut_destination[11]);
}

static const UTTestCase ut_cases[] = {
   {"invalid request", rejects_each_invalid_request_condition},
   {"put rows", copies_each_row_and_marks_the_destination}
};
