static void ignores_missing_destination_or_screen(void)
{
   tODVScreenInfo info;
   memset(&info, 0x5a, sizeof(info));
   bSessionScreenAvailable = TRUE;
   utt_ODSessionScreenGetInfo(NULL);
   bSessionScreenAvailable = FALSE;
   utt_ODSessionScreenGetInfo(&info);
   UT_ASSERT_EQ_UINT(0x5a, ((BYTE *)&info)[0]);
}

static void returns_one_based_window_and_cursor(void)
{
   tODVScreenInfo info;
   bSessionScreenAvailable = TRUE;
   SessionScreen.nLeft = 2;
   SessionScreen.nTop = 3;
   SessionScreen.nRight = 10;
   SessionScreen.nBottom = 12;
   SessionScreen.btAttribute = 31;
   SessionScreen.nCursorColumn = 4;
   SessionScreen.nCursorRow = 5;
   SessionScreen.bScrolling = FALSE;
   memset(&info, 0, sizeof(info));
   utt_ODSessionScreenGetInfo(&info);
   UT_ASSERT_EQ_INT(3, info.winleft);
   UT_ASSERT_EQ_INT(4, info.wintop);
   UT_ASSERT_EQ_INT(11, info.winright);
   UT_ASSERT_EQ_INT(13, info.winbottom);
   UT_ASSERT_EQ_UINT(31, info.attribute);
   UT_ASSERT_EQ_INT(5, info.curx);
   UT_ASSERT_EQ_INT(6, info.cury);
   UT_ASSERT_EQ_INT(FALSE, info.scrolling);
}

static const UTTestCase ut_cases[] = {
   {"unavailable info", ignores_missing_destination_or_screen},
   {"screen info", returns_one_based_window_and_cursor}
};
