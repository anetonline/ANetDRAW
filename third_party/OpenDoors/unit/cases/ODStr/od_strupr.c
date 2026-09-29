#define UT_CUSTOM_MOCK_toupper

int utm_toupper(int value)
{
   UT_ASSERT_EQ_INT((int)(unsigned char)value, value);
   if(value >= 'a' && value <= 'z')
      return value - ('a' - 'A');
   return value;
}

static void converts_each_character_and_returns_input(void)
{
   char text[] = "aB1";
   UT_ASSERT_EQ_PTR(text, utt_od_strupr(text));
   UT_ASSERT_EQ_INT(0, strcmp("AB1", text));
}

static void accepts_empty_string(void)
{
   char text[] = "";
   UT_ASSERT_EQ_PTR(text, utt_od_strupr(text));
}

static void preserves_high_bit_bytes(void)
{
   char text[] = {(char)0x80, '\0'};
   UT_ASSERT_EQ_PTR(text, utt_od_strupr(text));
   UT_ASSERT_EQ_UINT(0x80, (unsigned char)text[0]);
}

static const UTTestCase ut_cases[] = {
   {"uppercase string", converts_each_character_and_returns_input},
   {"high-bit byte", preserves_high_bit_bytes},
   {"empty string", accepts_empty_string}
};
