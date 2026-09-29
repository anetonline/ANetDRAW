static void reset_screen(void)
{
   bSessionScreenAvailable = TRUE;
   SessionScreen.nWidth = 10;
   SessionScreen.nHeight = 8;
   SessionScreen.nLeft = 0;
   SessionScreen.nTop = 0;
   SessionScreen.nRight = 9;
   SessionScreen.nBottom = 7;
   SessionScreen.nCursorColumn = 1;
   SessionScreen.nCursorRow = 1;
}

static void rejects_each_invalid_boundary(void)
{
   reset_screen();
   bSessionScreenAvailable = FALSE;
   utt_ODSessionScreenSetBoundary(1, 1, 4, 4);
   UT_ASSERT_EQ_INT(9, SessionScreen.nRight);
   bSessionScreenAvailable = TRUE;
   utt_ODSessionScreenSetBoundary(0, 1, 4, 4);
   utt_ODSessionScreenSetBoundary(1, 0, 4, 4);
   utt_ODSessionScreenSetBoundary(5, 1, 4, 4);
   utt_ODSessionScreenSetBoundary(1, 5, 4, 4);
   utt_ODSessionScreenSetBoundary(1, 1, 11, 4);
   utt_ODSessionScreenSetBoundary(1, 1, 4, 9);
   UT_ASSERT_EQ_INT(0, SessionScreen.nLeft);
   UT_ASSERT_EQ_INT(9, SessionScreen.nRight);
}

static void sets_boundary_and_clamps_cursor(void)
{
   reset_screen();
   utt_ODSessionScreenSetBoundary(2, 3, 6, 7);
   UT_ASSERT_EQ_INT(1, SessionScreen.nLeft);
   UT_ASSERT_EQ_INT(2, SessionScreen.nTop);
   UT_ASSERT_EQ_INT(5, SessionScreen.nRight);
   UT_ASSERT_EQ_INT(6, SessionScreen.nBottom);
   UT_ASSERT_EQ_INT(1, SessionScreen.nCursorColumn);
   UT_ASSERT_EQ_INT(1, SessionScreen.nCursorRow);

   SessionScreen.nCursorColumn = 9;
   SessionScreen.nCursorRow = 7;
   utt_ODSessionScreenSetBoundary(2, 3, 6, 7);
   UT_ASSERT_EQ_INT(4, SessionScreen.nCursorColumn);
   UT_ASSERT_EQ_INT(4, SessionScreen.nCursorRow);
}

static const UTTestCase ut_cases[] = {
   {"invalid boundaries", rejects_each_invalid_boundary},
   {"boundary and cursor", sets_boundary_and_clamps_cursor}
};
