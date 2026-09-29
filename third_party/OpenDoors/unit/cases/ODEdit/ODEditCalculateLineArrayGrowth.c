#define UT_CUSTOM_MOCK_ODSizeMultiply

static BOOL ut_multiply_result;
static size_t ut_multiply_bytes;
static size_t ut_left;
static size_t ut_right;
static unsigned ut_multiply_calls;

int utm_ODSizeMultiply(size_t left, size_t right, size_t *result)
{
   ++ut_multiply_calls;
   ut_left = left;
   ut_right = right;
   UT_ASSERT_NOT_NULL(result);
   if(!ut_multiply_result) return(FALSE);
   *result = ut_multiply_bytes;
   return(TRUE);
}

static void reset_growth(void)
{
   ut_multiply_result = TRUE;
   ut_multiply_bytes = 123;
   ut_left = ut_right = 0;
   ut_multiply_calls = 0;
}

static void calculates_normal_and_boundary_growth(void)
{
   UINT new_size = 0;
   size_t new_bytes = 0;

   reset_growth();
   UT_ASSERT(utt_ODEditCalculateLineArrayGrowth(0, &new_size, &new_bytes));
   UT_ASSERT_EQ_UINT(LINE_ARRAY_GROW_SIZE, new_size);
   UT_ASSERT_EQ_UINT(LINE_ARRAY_GROW_SIZE, ut_left);
   UT_ASSERT(ut_right == sizeof(char *));
   UT_ASSERT_EQ_UINT(123, new_bytes);

   reset_growth();
   UT_ASSERT(utt_ODEditCalculateLineArrayGrowth(
      (UINT)-1 - LINE_ARRAY_GROW_SIZE, &new_size, &new_bytes));
   UT_ASSERT_EQ_UINT((UINT)-1, new_size);
   UT_ASSERT_EQ_UINT((UINT)-1, ut_left);
}

static void rejects_element_and_byte_size_overflow(void)
{
   UINT new_size = 17;
   size_t new_bytes = 19;

   reset_growth();
   UT_ASSERT(!utt_ODEditCalculateLineArrayGrowth(
      (UINT)-1 - LINE_ARRAY_GROW_SIZE + 1, &new_size, &new_bytes));
   UT_ASSERT_EQ_UINT(0, ut_multiply_calls);
   UT_ASSERT_EQ_UINT(17, new_size);
   UT_ASSERT_EQ_UINT(19, new_bytes);

   reset_growth();
   ut_multiply_result = FALSE;
   UT_ASSERT(!utt_ODEditCalculateLineArrayGrowth(
      LINE_ARRAY_GROW_SIZE, &new_size, &new_bytes));
   UT_ASSERT_EQ_UINT(1, ut_multiply_calls);
   UT_ASSERT_EQ_UINT(17, new_size);
   UT_ASSERT_EQ_UINT(19, new_bytes);
}

static const UTTestCase ut_cases[] = {
   {"valid growth", calculates_normal_and_boundary_growth},
   {"overflow", rejects_element_and_byte_size_overflow}
};
