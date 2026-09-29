static void restores_the_complete_nesting_depth(void)
{
   nAPILevel = 0;
   utt_ODSyncAPIReacquire(3);
   UT_ASSERT_EQ_UINT(3, nAPILevel);
   utt_ODSyncAPIReacquire(2);
   UT_ASSERT_EQ_UINT(2, nAPILevel);
}

static const UTTestCase ut_cases[] = {
   {"reacquire depth", restores_the_complete_nesting_depth}
};
