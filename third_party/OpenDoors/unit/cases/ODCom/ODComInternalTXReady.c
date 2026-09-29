static void reports_space_only_below_the_queue_capacity(void)
{
   nTXQueueSize = 4;
   nTXChars = 3;
   UT_ASSERT_EQ_INT(TRUE, utt_ODComInternalTXReady());
   nTXChars = 4;
   UT_ASSERT_EQ_INT(FALSE, utt_ODComInternalTXReady());
   nTXChars = 5;
   UT_ASSERT_EQ_INT(FALSE, utt_ODComInternalTXReady());
}

static const UTTestCase ut_cases[] = {
   {"capacity", reports_space_only_below_the_queue_capacity}
};
