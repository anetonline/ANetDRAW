static void includes_a_terminator_for_each_area_row(void)
{
   tEditInstance instance;

   instance.unAreaWidth = 79;
   instance.unAreaHeight = 24;
   UT_ASSERT_EQ_UINT(80U * 24U, utt_ODEditRememberBufferSize(&instance));
   instance.unAreaWidth = 0;
   instance.unAreaHeight = 3;
   UT_ASSERT_EQ_UINT(3, utt_ODEditRememberBufferSize(&instance));
}

static const UTTestCase ut_cases[] = {
   {"area storage", includes_a_terminator_for_each_area_row}
};
