#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_od_strlwr
#define UT_CUSTOM_MOCK_strlwr
#define UT_CUSTOM_MOCK_toupper

static unsigned ut_strlen_calls;

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   ++ut_strlen_calls;
   while(text[length] != '\0') ++length;
   return length;
}

static char *ut_lower_string(char *text)
{
   char *position;
   for(position = text; *position != '\0'; ++position)
      if(*position >= 'A' && *position <= 'Z')
         *position = (char)(*position - 'A' + 'a');
   return text;
}

char *utm_od_strlwr(char *text) { return ut_lower_string(text); }
char *utm_strlwr(char *text) { return ut_lower_string(text); }

int utm_toupper(int value)
{
   UT_ASSERT_EQ_INT((int)(unsigned char)value, value);
   if(value >= 'a' && value <= 'z') return value - 'a' + 'A';
   return value;
}

static void accepts_an_empty_name(void)
{
   char name[1] = {'\0'};
   ut_strlen_calls = 0;
   utt_ODStringToName(name);
   UT_ASSERT(name[0] == '\0');
   UT_ASSERT_EQ_UINT(0, ut_strlen_calls);
}

static void normalizes_case_word_boundaries_and_line_endings(void)
{
   char name[] = "a B\tC,d.e-f\r\n";
   char expected[] = "A B\tC,D.E-F";
   size_t index;
   utt_ODStringToName(name);
   for(index = 0; expected[index] != '\0'; ++index)
      UT_ASSERT(name[index] == expected[index]);
   UT_ASSERT(name[index] == '\0');
}

static void leaves_a_name_without_a_line_ending_terminated(void)
{
   char name[] = "ALICE";
   char expected[] = "Alice";
   size_t index;
   utt_ODStringToName(name);
   for(index = 0; expected[index] != '\0'; ++index)
      UT_ASSERT(name[index] == expected[index]);
   UT_ASSERT(name[index] == '\0');
}

static void preserves_high_bit_bytes_at_case_boundaries(void)
{
   char name[] = {(char)0x80, ' ', (char)0x80, '\0'};
   utt_ODStringToName(name);
   UT_ASSERT_EQ_UINT(0x80, (unsigned char)name[0]);
   UT_ASSERT_EQ_UINT(0x80, (unsigned char)name[2]);
}

static const UTTestCase ut_cases[] = {
   {"empty name", accepts_an_empty_name},
   {"name normalization", normalizes_case_word_boundaries_and_line_endings},
   {"high-bit boundaries", preserves_high_bit_bytes_at_case_boundaries},
   {"no line ending", leaves_a_name_without_a_line_ending_terminated}
};
