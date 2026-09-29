static void changes_scrolling_only_when_available(void)
{
   SessionScreen.bScrolling = TRUE;
   bSessionScreenAvailable = FALSE;
   utt_ODSessionScreenEnableScrolling(FALSE);
   UT_ASSERT_EQ_INT(TRUE, SessionScreen.bScrolling);
   bSessionScreenAvailable = TRUE;
   utt_ODSessionScreenEnableScrolling(FALSE);
   UT_ASSERT_EQ_INT(FALSE, SessionScreen.bScrolling);
}

static const UTTestCase ut_cases[] = {
   {"scroll availability", changes_scrolling_only_when_available}
};
