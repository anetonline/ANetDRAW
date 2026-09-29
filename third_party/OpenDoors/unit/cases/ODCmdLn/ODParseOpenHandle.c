static void accepts_zero_plus_and_platform_maximum(void)
{
   DWORD_PTR value = 99;
   char maximum[3 * sizeof(DWORD_PTR) + 1];
   DWORD_PTR working = (DWORD_PTR)-1;
   unsigned index = sizeof(maximum) - 1;
   maximum[index] = '\0';
   do
   {
      maximum[--index] = (char)('0' + working % 10);
      working /= 10;
   } while(working != 0);

   UT_ASSERT_EQ_INT(TRUE, utt_ODParseOpenHandle("0", &value));
   UT_ASSERT_EQ_UINT(0, value);
   UT_ASSERT_EQ_INT(TRUE, utt_ODParseOpenHandle("+42", &value));
   UT_ASSERT_EQ_UINT(42, value);
   UT_ASSERT_EQ_INT(TRUE, utt_ODParseOpenHandle(&maximum[index], &value));
   UT_ASSERT(value == (DWORD_PTR)-1);
}

static void rejects_empty_nondecimal_and_overflow(void)
{
   DWORD_PTR value = 77;
   char overflow[3 * sizeof(DWORD_PTR) + 3];
   DWORD_PTR working = (DWORD_PTR)-1;
   unsigned index = sizeof(overflow) - 2;
   overflow[index] = '\0';
   do
   {
      overflow[--index] = (char)('0' + working % 10);
      working /= 10;
   } while(working != 0);
   memmove(&overflow[index + 1], &overflow[index],
      strlen(&overflow[index]) + 1);
   overflow[index] = '1';

   UT_ASSERT_EQ_INT(FALSE, utt_ODParseOpenHandle("", &value));
   UT_ASSERT_EQ_UINT(77, value);
   UT_ASSERT_EQ_INT(FALSE, utt_ODParseOpenHandle("+", &value));
   UT_ASSERT_EQ_UINT(77, value);
   UT_ASSERT_EQ_INT(FALSE, utt_ODParseOpenHandle("-1", &value));
   UT_ASSERT_EQ_UINT(77, value);
   UT_ASSERT_EQ_INT(FALSE, utt_ODParseOpenHandle("1x", &value));
   UT_ASSERT_EQ_UINT(77, value);
   UT_ASSERT_EQ_INT(FALSE, utt_ODParseOpenHandle(&overflow[index], &value));
   UT_ASSERT_EQ_UINT(77, value);
}

static const UTTestCase ut_cases[] = {
   {"valid native handles", accepts_zero_plus_and_platform_maximum},
   {"invalid native handles", rejects_empty_nondecimal_and_overflow}
};
