#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_strncmp
#define UT_CUSTOM_MOCK_strchr

size_t utm_strlen(const char *text)
{
   const char *end = text;
   while(*end != '\0') ++end;
   return (size_t)(end - text);
}

int utm_strncmp(const char *left, const char *right, size_t count)
{
   while(count != 0)
   {
      unsigned char left_char = (unsigned char)*left;
      unsigned char right_char = (unsigned char)*right;
      if(left_char != right_char) return (int)left_char - (int)right_char;
      if(left_char == 0) return 0;
      ++left;
      ++right;
      --count;
   }
   return 0;
}

char *utm_strchr(const char *text, int character)
{
   char wanted = (char)character;
   for(;;)
   {
      if(*text == wanted) return (char *)text;
      if(*text == '\0') return NULL;
      ++text;
   }
}

static void ut_set_sequence(const char *text)
{
   size_t index = 0;
   while(text[index] != '\0' && index + 1 < sizeof(szCurrentSequence))
   {
      szCurrentSequence[index] = text[index];
      ++index;
   }
   szCurrentSequence[index] = '\0';
}
