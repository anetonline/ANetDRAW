static void returns_availability(void)
{
   bSessionScreenAvailable = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenAvailable());
   bSessionScreenAvailable = TRUE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODSessionScreenAvailable());
}

static const UTTestCase ut_cases[] = {
   {"availability", returns_availability}
};
