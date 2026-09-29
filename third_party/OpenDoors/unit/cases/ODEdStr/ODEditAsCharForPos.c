#define UT_CUSTOM_MOCK_toupper
#define UT_CUSTOM_MOCK_tolower

int utm_toupper(int character)
{
   UT_ASSERT_EQ_INT((int)(unsigned char)character, character);
   if(character >= 'a' && character <= 'z')
      return(character - 'a' + 'A');
   return(character);
}

int utm_tolower(int character)
{
   UT_ASSERT_EQ_INT((int)(unsigned char)character, character);
   if(character >= 'A' && character <= 'Z')
      return(character - 'A' + 'a');
   return(character);
}

static char convert_at(char format, char entered, INT position)
{
   char format_string[4] = "MMM";
   char input_string[4] = "aa";
   format_string[position] = format;
   pszCurrentFormat = format_string;
   pszCurrentInput = input_string;
   anCurrentFormatOffset[position] = position;
   abCurrentFormatLiteral[position] = FALSE;
   return utt_ODEditAsCharForPos(entered, position);
}

static void returns_literal_and_unconverted_values(void)
{
   char format[] = "!";
   pszCurrentFormat = format;
   anCurrentFormatOffset[0] = 0;
   abCurrentFormatLiteral[0] = TRUE;
   UT_ASSERT_EQ_INT('!', utt_ODEditAsCharForPos('x', 0));
   UT_ASSERT_EQ_INT('x', convert_at('#', 'x', 0));
}

static void applies_simple_case_formats(void)
{
   const char upper_formats[] = "YyFfUu";
   const char lower_formats[] = "Ll";
   unsigned index;
   for(index = 0; upper_formats[index] != '\0'; ++index)
      UT_ASSERT_EQ_INT('A', convert_at(upper_formats[index], 'a', 0));
   for(index = 0; lower_formats[index] != '\0'; ++index)
      UT_ASSERT_EQ_INT('a', convert_at(lower_formats[index], 'A', 0));
}

static void applies_word_capitalization(void)
{
   const char formats[] = "MmCc";
   const char delimiters[] = " .,-";
   unsigned index;
   unsigned delimiter;
   for(index = 0; formats[index] != '\0'; ++index)
      UT_ASSERT_EQ_INT('A', convert_at(formats[index], 'a', 0));

   pszCurrentInput = "aa";
   for(index = 0; formats[index] != '\0'; ++index)
   {
      char format_string[] = "MM";
      format_string[1] = formats[index];
      pszCurrentFormat = format_string;
      anCurrentFormatOffset[1] = 1;
      abCurrentFormatLiteral[0] = TRUE;
      abCurrentFormatLiteral[1] = FALSE;
      UT_ASSERT_EQ_INT('A', utt_ODEditAsCharForPos('a', 1));
      abCurrentFormatLiteral[0] = FALSE;
   }

   for(delimiter = 0; delimiters[delimiter] != '\0'; ++delimiter)
   {
      char input_string[] = "aa";
      char format_string[] = "MM";
      input_string[0] = delimiters[delimiter];
      pszCurrentInput = input_string;
      pszCurrentFormat = format_string;
      anCurrentFormatOffset[1] = 1;
      abCurrentFormatLiteral[0] = FALSE;
      abCurrentFormatLiteral[1] = FALSE;
      UT_ASSERT_EQ_INT('A', utt_ODEditAsCharForPos('a', 1));
   }
   UT_ASSERT_EQ_INT('a', convert_at('M', 'A', 1));
}

static void preserves_high_bit_bytes_during_case_conversion(void)
{
   UT_ASSERT_EQ_UINT(0x80,
      (unsigned char)convert_at('U', (char)0x80, 0));
   UT_ASSERT_EQ_UINT(0x80,
      (unsigned char)convert_at('L', (char)0x80, 0));
}

static const UTTestCase ut_cases[] = {
   {"literal and identity conversion", returns_literal_and_unconverted_values},
   {"simple case conversion", applies_simple_case_formats},
   {"high-bit byte", preserves_high_bit_bytes_during_case_conversion},
   {"word capitalization", applies_word_capitalization}
};
