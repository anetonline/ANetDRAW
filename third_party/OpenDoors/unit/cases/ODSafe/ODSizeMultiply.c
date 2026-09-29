static void multiplies_values_within_size_t_range(void)
{
   size_t result = 99;
   UT_ASSERT_EQ_INT(1, utt_ODSizeMultiply(10, 20, &result));
   UT_ASSERT(result == (size_t)200);
   UT_ASSERT_EQ_INT(1, utt_ODSizeMultiply((size_t)-1, 0, &result));
   UT_ASSERT(result == (size_t)0);
   UT_ASSERT_EQ_INT(1, utt_ODSizeMultiply((size_t)-1, 1, &result));
   UT_ASSERT(result == (size_t)-1);
}

static void rejects_null_result_and_overflow(void)
{
   size_t result = 77;
   UT_ASSERT_EQ_INT(0, utt_ODSizeMultiply(0, 0, NULL));
   UT_ASSERT_EQ_INT(0, utt_ODSizeMultiply((size_t)-1, 2, &result));
   UT_ASSERT(result == (size_t)77);
}

static const UTTestCase ut_cases[] = {
   {"valid products", multiplies_values_within_size_t_range},
   {"invalid products", rejects_null_result_and_overflow}
};
