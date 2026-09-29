static void shifts_across_word_boundary(void)
{
   UT_ASSERT_EQ_UINT(0x00018000UL, utt_ODDWordShiftLeft(0x0000c000UL, 1));
   UT_ASSERT_EQ_UINT(0x56780000UL, utt_ODDWordShiftLeft(0x12345678UL, 16));
   UT_ASSERT_EQ_UINT(0x12345678UL, utt_ODDWordShiftLeft(0x12345678UL, 0));
   UT_ASSERT_EQ_UINT(0, utt_ODDWordShiftLeft(0xffffffffUL, 32));
}

static const UTTestCase ut_cases[] = {
   {"left shifts", shifts_across_word_boundary}
};
