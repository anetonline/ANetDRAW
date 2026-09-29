#define UT_CUSTOM_MOCK_ODEditBufferGetLineLength
static UINT ut_lengths[5];

UINT utm_ODEditBufferGetLineLength(tEditInstance *instance, UINT line)
{
   UT_ASSERT(line < instance->unLinesInBuffer);
   return(ut_lengths[line]);
}

static void estimates_single_and_multiple_lines(void)
{
   tEditInstance instance;

   UT_ASSERT_EQ_UINT(7,
      utt_ODEditEstDrawBytes(&instance, 2, 3, 2, 10));

   instance.unLineScrolledToTop = 1;
   instance.unLinesInBuffer = 3;
   ut_lengths[1] = 10;
   ut_lengths[2] = 20;
   UT_ASSERT_EQ_UINT(56,
      utt_ODEditEstDrawBytes(&instance, 0, 2, 3, 9));
}

static const UTTestCase ut_cases[] = {
   {"draw estimate", estimates_single_and_multiple_lines}
};
