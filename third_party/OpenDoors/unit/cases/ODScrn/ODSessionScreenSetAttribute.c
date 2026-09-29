static void changes_attribute_only_when_available(void)
{
   SessionScreen.btAttribute = 7;
   bSessionScreenAvailable = FALSE;
   utt_ODSessionScreenSetAttribute(31);
   UT_ASSERT_EQ_UINT(7, SessionScreen.btAttribute);
   bSessionScreenAvailable = TRUE;
   utt_ODSessionScreenSetAttribute(31);
   UT_ASSERT_EQ_UINT(31, SessionScreen.btAttribute);
}

static const UTTestCase ut_cases[] = {
   {"attribute availability", changes_attribute_only_when_available}
};
