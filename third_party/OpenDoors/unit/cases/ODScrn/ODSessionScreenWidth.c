static void returns_width(void)
{
   SessionScreen.nWidth = 132;
   UT_ASSERT_EQ_INT(132, utt_ODSessionScreenWidth());
}

static const UTTestCase ut_cases[] = {
   {"width", returns_width}
};
