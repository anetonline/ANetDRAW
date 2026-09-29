#define UT_CUSTOM_MOCK_isspace
#define UT_CUSTOM_MOCK_strstr

int utm_isspace(int value)
{
   return(value == ' ' || value == '\t' || value == '\r' || value == '\n');
}

char *utm_strstr(const char *text, const char *search)
{
   size_t index;
   size_t search_index;

   for(index = 0; text[index] != '\0'; ++index)
   {
      for(search_index = 0;
         search[search_index] != '\0' &&
         text[index + search_index] == search[search_index];
         ++search_index)
      {
      }
      if(search[search_index] == '\0')
         return((char *)(text + index));
   }
   return(NULL);
}

static void recognizes_compact_and_comma_separated_framing(void)
{
   UT_ASSERT(utt_ODFramingIsEightBit("8N1"));
   UT_ASSERT(utt_ODFramingIsEightBit("N,8,1"));
   UT_ASSERT(utt_ODFramingIsEightBit("  \t8E1\r\n"));
}

static void rejects_framing_without_an_explicit_eight_bit_field(void)
{
   UT_ASSERT(!utt_ODFramingIsEightBit("7E1"));
   UT_ASSERT(!utt_ODFramingIsEightBit("N,7,1"));
   UT_ASSERT(!utt_ODFramingIsEightBit("18N1"));
   UT_ASSERT(!utt_ODFramingIsEightBit(""));
}

static const UTTestCase ut_cases[] = {
   {"eight-bit framing", recognizes_compact_and_comma_separated_framing},
   {"other framing", rejects_framing_without_an_explicit_eight_bit_field}
};
