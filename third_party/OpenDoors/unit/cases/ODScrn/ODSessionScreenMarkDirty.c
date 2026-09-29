static void starts_a_new_dirty_rectangle(void)
{
   SessionScreen.bDirty = FALSE;
   utt_ODSessionScreenMarkDirty(3, 4, 7, 8);
   UT_ASSERT_EQ_INT(TRUE, SessionScreen.bDirty);
   UT_ASSERT_EQ_INT(3, SessionScreen.nDirtyLeft);
   UT_ASSERT_EQ_INT(4, SessionScreen.nDirtyTop);
   UT_ASSERT_EQ_INT(7, SessionScreen.nDirtyRight);
   UT_ASSERT_EQ_INT(8, SessionScreen.nDirtyBottom);
}

static void expands_only_past_existing_edges(void)
{
   SessionScreen.bDirty = TRUE;
   SessionScreen.nDirtyLeft = 3;
   SessionScreen.nDirtyTop = 4;
   SessionScreen.nDirtyRight = 7;
   SessionScreen.nDirtyBottom = 8;
   utt_ODSessionScreenMarkDirty(2, 3, 9, 10);
   UT_ASSERT_EQ_INT(2, SessionScreen.nDirtyLeft);
   UT_ASSERT_EQ_INT(3, SessionScreen.nDirtyTop);
   UT_ASSERT_EQ_INT(9, SessionScreen.nDirtyRight);
   UT_ASSERT_EQ_INT(10, SessionScreen.nDirtyBottom);

   utt_ODSessionScreenMarkDirty(4, 5, 6, 7);
   UT_ASSERT_EQ_INT(2, SessionScreen.nDirtyLeft);
   UT_ASSERT_EQ_INT(3, SessionScreen.nDirtyTop);
   UT_ASSERT_EQ_INT(9, SessionScreen.nDirtyRight);
   UT_ASSERT_EQ_INT(10, SessionScreen.nDirtyBottom);
}

static const UTTestCase ut_cases[] = {
   {"new rectangle", starts_a_new_dirty_rectangle},
   {"rectangle union", expands_only_past_existing_edges}
};
