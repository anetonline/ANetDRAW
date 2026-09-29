static void begins_only_when_available(void)
{
   bSessionScreenEmulating = FALSE;
   bSessionScreenAvailable = FALSE;
   utt_ODSessionScreenBeginEmulation();
   UT_ASSERT_EQ_INT(FALSE, bSessionScreenEmulating);
   bSessionScreenAvailable = TRUE;
   utt_ODSessionScreenBeginEmulation();
   UT_ASSERT_EQ_INT(TRUE, bSessionScreenEmulating);
}

static const UTTestCase ut_cases[] = {
   {"begin availability", begins_only_when_available}
};
