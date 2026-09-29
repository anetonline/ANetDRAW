#define UT_CUSTOM_MOCK_strncpy

char *utm_strncpy(char *destination, const char *source, size_t count)
{
   size_t index = 0;
   while(index < count && source[index] != '\0')
   {
      destination[index] = source[index];
      ++index;
   }
   while(index < count) destination[index++] = '\0';
   return destination;
}

static void copies_and_always_terminates(void)
{
   char destination[5];
   memset(destination, 'x', sizeof(destination));
   utt_ODStringCopy(destination, "ab", sizeof(destination));
   UT_ASSERT_EQ_INT(0, strcmp("ab", destination));
   utt_ODStringCopy(destination, "abcdef", sizeof(destination));
   UT_ASSERT_EQ_INT(0, strcmp("abcd", destination));
}

static const UTTestCase ut_cases[] = {
   {"bounded copy", copies_and_always_terminates}
};
