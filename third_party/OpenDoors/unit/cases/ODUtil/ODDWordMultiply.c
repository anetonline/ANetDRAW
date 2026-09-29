#define UT_CUSTOM_MOCK_ODDWordShiftLeft
#define UT_CUSTOM_MOCK_ODDWordShiftRight

DWORD utm_ODDWordShiftLeft(DWORD value, BYTE distance)
{
   return value << distance;
}

DWORD utm_ODDWordShiftRight(DWORD value, BYTE distance)
{
   return value >> distance;
}

static void multiplies_zero_even_and_odd_values(void)
{
   UT_ASSERT_EQ_UINT(0, utt_ODDWordMultiply(123, 0));
   UT_ASSERT_EQ_UINT(42, utt_ODDWordMultiply(6, 7));
   UT_ASSERT_EQ_UINT(48, utt_ODDWordMultiply(6, 8));
}

static const UTTestCase ut_cases[] = {
   {"DWORD multiplication", multiplies_zero_even_and_odd_values}
};
