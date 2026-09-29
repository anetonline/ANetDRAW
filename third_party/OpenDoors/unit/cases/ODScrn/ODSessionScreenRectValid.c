static void covers_each_rectangle_constraint(void)
{
   SessionScreen.nLeft = 2;
   SessionScreen.nRight = 5;
   SessionScreen.nTop = 3;
   SessionScreen.nBottom = 5;
   UT_ASSERT_EQ_INT(TRUE, utt_ODSessionScreenRectValid(1, 1, 4, 3));
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenRectValid(0, 1, 4, 3));
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenRectValid(1, 0, 4, 3));
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenRectValid(2, 1, 1, 3));
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenRectValid(1, 2, 4, 1));
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenRectValid(1, 1, 5, 3));
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenRectValid(1, 1, 4, 4));
}

static const UTTestCase ut_cases[] = {
   {"rectangle constraints", covers_each_rectangle_constraint}
};
