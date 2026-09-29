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

static void divides_with_quotient_remainder_and_optional_outputs(void)
{
   DWORD quotient = 99;
   DWORD remainder = 99;
   UT_ASSERT_EQ_INT(TRUE, utt_ODDWordDivide(&quotient, &remainder, 17, 5));
   UT_ASSERT_EQ_UINT(3, quotient);
   UT_ASSERT_EQ_UINT(2, remainder);
   UT_ASSERT_EQ_INT(TRUE, utt_ODDWordDivide(NULL, NULL, 2, 5));
   UT_ASSERT_EQ_INT(TRUE, utt_ODDWordDivide(&quotient, &remainder, 8, 2));
   UT_ASSERT_EQ_UINT(4, quotient);
   UT_ASSERT_EQ_UINT(0, remainder);
}

static void rejects_zero_divisor(void)
{
   DWORD quotient = 99;
   DWORD remainder = 99;
   UT_ASSERT_EQ_INT(FALSE, utt_ODDWordDivide(&quotient, &remainder, 17, 0));
   UT_ASSERT_EQ_UINT(99, quotient);
   UT_ASSERT_EQ_UINT(99, remainder);
}

static const UTTestCase ut_cases[] = {
   {"DWORD division", divides_with_quotient_remainder_and_optional_outputs},
   {"zero divisor", rejects_zero_divisor}
};
