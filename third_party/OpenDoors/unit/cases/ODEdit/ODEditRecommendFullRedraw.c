#define UT_CUSTOM_MOCK_ODEditEstDrawBytes
static UINT ut_full_redraw_bytes;
static unsigned ut_estimate_calls;

UINT utm_ODEditEstDrawBytes(tEditInstance *instance, UINT start_line,
   UINT start_column, UINT finish_line, UINT finish_column)
{
   ++ut_estimate_calls;
   UT_ASSERT_EQ_UINT(0, start_line);
   UT_ASSERT_EQ_UINT(0, start_column);
   UT_ASSERT_EQ_UINT(instance->unAreaHeight - 1, finish_line);
   UT_ASSERT_EQ_UINT(instance->unAreaWidth, finish_column);
   return(ut_full_redraw_bytes);
}

static void uses_the_default_for_local_rendering(void)
{
   tEditInstance instance;

   ut_estimate_calls = 0;
   od_control.baud = 0;
   UT_ASSERT_EQ_INT(TRUE,
      utt_ODEditRecommendFullRedraw(&instance, 50, TRUE));
   UT_ASSERT_EQ_INT(FALSE,
      utt_ODEditRecommendFullRedraw(&instance, 50, FALSE));
   UT_ASSERT_EQ_UINT(0, ut_estimate_calls);
}

static void compares_only_the_new_incremental_and_full_output(void)
{
   tEditInstance instance;

   instance.unAreaHeight = 10;
   instance.unAreaWidth = 40;
   od_control.baud = 9600;
   ut_estimate_calls = 0;
   ut_full_redraw_bytes = 100;
   UT_ASSERT_EQ_INT(TRUE,
      utt_ODEditRecommendFullRedraw(&instance, 101, FALSE));
   UT_ASSERT_EQ_INT(FALSE,
      utt_ODEditRecommendFullRedraw(&instance, 100, TRUE));
   UT_ASSERT_EQ_UINT(2, ut_estimate_calls);
}

static const UTTestCase ut_cases[] = {
   {"local default", uses_the_default_for_local_rendering},
   {"remote estimate", compares_only_the_new_incremental_and_full_output}
};
