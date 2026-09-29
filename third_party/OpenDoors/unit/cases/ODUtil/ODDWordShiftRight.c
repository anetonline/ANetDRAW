static void shifts_across_word_boundary(void)
{
   UT_ASSERT_EQ_UINT(0x00006000UL, utt_ODDWordShiftRight(0x0000c000UL, 1));
   UT_ASSERT_EQ_UINT(0x00001234UL, utt_ODDWordShiftRight(0x12345678UL, 16));
   UT_ASSERT_EQ_UINT(0x12345678UL, utt_ODDWordShiftRight(0x12345678UL, 0));
   UT_ASSERT_EQ_UINT(0, utt_ODDWordShiftRight(0xffffffffUL, 32));
}

static const UTTestCase ut_cases[] = {
   {"right shifts", shifts_across_word_boundary}
};
