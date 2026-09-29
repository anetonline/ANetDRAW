static struct tm ut_time_record;

static void requires_both_valid_time_results(void)
{
   UT_ASSERT(!utt_ODLogTimeRecordSucceeded((time_t)-1, NULL));
   UT_ASSERT(!utt_ODLogTimeRecordSucceeded((time_t)-1, &ut_time_record));
   UT_ASSERT(!utt_ODLogTimeRecordSucceeded((time_t)0, NULL));
   UT_ASSERT(utt_ODLogTimeRecordSucceeded((time_t)0, &ut_time_record));
}

static const UTTestCase ut_cases[] = {
   {"time results", requires_both_valid_time_results}
};
