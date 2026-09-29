static void adds_values_within_size_t_range(void)
{
   size_t result = 99;
   UT_ASSERT_EQ_INT(1, utt_ODSizeAdd(10, 20, &result));
   UT_ASSERT(result == (size_t)30);
   UT_ASSERT_EQ_INT(1, utt_ODSizeAdd((size_t)-1, 0, &result));
   UT_ASSERT(result == (size_t)-1);
}

static void rejects_null_result_and_overflow(void)
{
   size_t result = 77;
   UT_ASSERT_EQ_INT(0, utt_ODSizeAdd(0, 0, NULL));
   UT_ASSERT_EQ_INT(0, utt_ODSizeAdd((size_t)-1, 1, &result));
   UT_ASSERT(result == (size_t)77);
}

static const UTTestCase ut_cases[] = {
   {"valid sums", adds_values_within_size_t_range},
   {"invalid sums", rejects_null_result_and_overflow}
};
