static void returns_height(void)
{
   SessionScreen.nHeight = 60;
   UT_ASSERT_EQ_INT(60, utt_ODSessionScreenHeight());
}

static const UTTestCase ut_cases[] = {
   {"height", returns_height}
};
