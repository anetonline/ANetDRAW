#define UT_CUSTOM_MOCK_strrchr

char *utm_strrchr(const char *text, int character)
{
   const char *found = NULL;
   while(*text != '\0') {
      if(*text == (char)character)
         found = text;
      ++text;
   }
   if(character == 0)
      found = text;
   return (char *)found;
}

static void locates_only_the_final_component_extension(void)
{
   const char *path;
   const char *extension;

   path = "plain";
   UT_ASSERT_NULL(utt_ODEmulatePathExtension(path));

   path = "name.ans";
   extension = utt_ODEmulatePathExtension(path);
   UT_ASSERT_EQ_PTR(path + 4, extension);
   UT_ASSERT(strcmp(".ans", extension) == 0);

   path = ".hidden";
   UT_ASSERT_EQ_PTR(path, utt_ODEmulatePathExtension(path));

   path = "dir.with.period/file";
   UT_ASSERT_NULL(utt_ODEmulatePathExtension(path));
   path = "dir.with.period/file.rip";
   UT_ASSERT(strcmp(".rip", utt_ODEmulatePathExtension(path)) == 0);

#ifndef ODPLAT_NIX
   path = "dir.with.period\\file";
   UT_ASSERT_NULL(utt_ODEmulatePathExtension(path));
   path = "dir.with.period\\file.avt";
   UT_ASSERT(strcmp(".avt", utt_ODEmulatePathExtension(path)) == 0);
#endif
}

static const UTTestCase ut_cases[] = {
   {"final component", locates_only_the_final_component_extension}
};
