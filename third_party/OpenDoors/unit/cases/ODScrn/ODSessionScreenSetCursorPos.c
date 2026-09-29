static void initialize_window(void)
{
   SessionScreen.nLeft = 2;
   SessionScreen.nRight = 6;
   SessionScreen.nTop = 3;
   SessionScreen.nBottom = 6;
}

static void ignores_unavailable_screen(void)
{
   initialize_window();
   bSessionScreenAvailable = FALSE;
   SessionScreen.nCursorColumn = 2;
   SessionScreen.nCursorRow = 2;
   utt_ODSessionScreenSetCursorPos(5, 4);
   UT_ASSERT_EQ_INT(2, SessionScreen.nCursorColumn);
   UT_ASSERT_EQ_INT(2, SessionScreen.nCursorRow);
}

static void clamps_and_stores_one_based_position(void)
{
   initialize_window();
   bSessionScreenAvailable = TRUE;
   utt_ODSessionScreenSetCursorPos(0, 0);
   UT_ASSERT_EQ_INT(0, SessionScreen.nCursorColumn);
   UT_ASSERT_EQ_INT(0, SessionScreen.nCursorRow);
   utt_ODSessionScreenSetCursorPos(99, 99);
   UT_ASSERT_EQ_INT(4, SessionScreen.nCursorColumn);
   UT_ASSERT_EQ_INT(3, SessionScreen.nCursorRow);
   utt_ODSessionScreenSetCursorPos(3, 2);
   UT_ASSERT_EQ_INT(2, SessionScreen.nCursorColumn);
   UT_ASSERT_EQ_INT(1, SessionScreen.nCursorRow);
}

static const UTTestCase ut_cases[] = {
   {"unavailable cursor", ignores_unavailable_screen},
   {"cursor clamps", clamps_and_stores_one_based_position}
};
