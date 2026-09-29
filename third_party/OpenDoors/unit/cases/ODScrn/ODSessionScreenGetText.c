#define UT_CUSTOM_MOCK_ODSessionScreenRectValid
#define UT_CUSTOM_MOCK_ODSessionScreenCell
#define UT_CUSTOM_MOCK_memcpy

static BOOL ut_rect_valid;
static BYTE ut_source[24];
static unsigned ut_cell_calls;
static unsigned ut_copy_calls;

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
   return ut_source + ((nRow - 1) * 8);
}

void *utm_memcpy(void *destination, const void *source, size_t count)
{
   BYTE *dest;
   const BYTE *src;
   size_t index;
   ++ut_copy_calls;
   UT_ASSERT_EQ_UINT(4, count);
   dest = (BYTE *)destination;
   src = (const BYTE *)source;
   for(index = 0; index < count; ++index)
      dest[index] = src[index];
   return destination;
}

static void rejects_each_invalid_request_condition(void)
{
   BYTE output[8];
   bSessionScreenAvailable = FALSE;
   ut_rect_valid = TRUE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenGetText(1, 1, 2, 2, output));
   bSessionScreenAvailable = TRUE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenGetText(1, 1, 2, 2, NULL));
   ut_rect_valid = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenGetText(1, 1, 2, 2, output));
}

static void copies_each_requested_row(void)
{
   BYTE output[8];
   unsigned index;
   for(index = 0; index < sizeof(ut_source); ++index)
      ut_source[index] = (BYTE)index;
   memset(output, 0, sizeof(output));
   bSessionScreenAvailable = TRUE;
   SessionScreen.nLeft = 2;
   SessionScreen.nTop = 1;
   ut_rect_valid = TRUE;
   ut_cell_calls = ut_copy_calls = 0;
   UT_ASSERT_EQ_INT(TRUE, utt_ODSessionScreenGetText(2, 1, 3, 2, output));
   UT_ASSERT_EQ_UINT(2, ut_cell_calls);
   UT_ASSERT_EQ_UINT(2, ut_copy_calls);
   UT_ASSERT_EQ_UINT(0, output[0]);
   UT_ASSERT_EQ_UINT(3, output[3]);
   UT_ASSERT_EQ_UINT(8, output[4]);
   UT_ASSERT_EQ_UINT(11, output[7]);
}

static const UTTestCase ut_cases[] = {
   {"invalid request", rejects_each_invalid_request_condition},
   {"copy rows", copies_each_requested_row}
};
