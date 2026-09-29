#define UT_CUSTOM_MOCK_strcasecmp
#define UT_CUSTOM_MOCK_stricmp
#define UT_CUSTOM_MOCK_strlen

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

static int ut_casecmp(const char *left, const char *right)
{
   while(*left != '\0' || *right != '\0')
   {
      int l = *left++;
      int r = *right++;
      if(l >= 'a' && l <= 'z') l -= 'a' - 'A';
      if(r >= 'a' && r <= 'z') r -= 'a' - 'A';
      if(l != r) return l - r;
   }
   return 0;
}

#ifdef ODPLAT_NIX
int utm_strcasecmp(const char *left, const char *right)
{
   return ut_casecmp(left, right);
}
#else
int utm_stricmp(const char *left, const char *right)
{
   return ut_casecmp(left, right);
}
#endif

static void recognizes_case_insensitive_tail(void)
{
   UT_ASSERT_EQ_INT(TRUE, utt_ODStringHasTail("filename.RIP", ".rip"));
   UT_ASSERT_EQ_INT(TRUE, utt_ODStringHasTail("filename", ""));
   UT_ASSERT_EQ_INT(FALSE, utt_ODStringHasTail("filename.ans", ".rip"));
   UT_ASSERT_EQ_INT(FALSE, utt_ODStringHasTail("x", "longer"));
}

static const UTTestCase ut_cases[] = {
   {"tail matching", recognizes_case_insensitive_tail}
};
