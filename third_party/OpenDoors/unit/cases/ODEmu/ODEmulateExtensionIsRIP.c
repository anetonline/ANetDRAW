#define UT_CUSTOM_MOCK_tolower

int utm_tolower(int character)
{
   if(character >= 'A' && character <= 'Z')
      return character + ('a' - 'A');
   return character;
}

static void accepts_only_the_complete_case_insensitive_extension(void)
{
   UT_ASSERT_EQ_INT(FALSE, utt_ODEmulateExtensionIsRIP(NULL));
   UT_ASSERT_EQ_INT(TRUE, utt_ODEmulateExtensionIsRIP(".rip"));
   UT_ASSERT_EQ_INT(TRUE, utt_ODEmulateExtensionIsRIP(".RIP"));
   UT_ASSERT_EQ_INT(FALSE, utt_ODEmulateExtensionIsRIP(""));
   UT_ASSERT_EQ_INT(FALSE, utt_ODEmulateExtensionIsRIP(".ri"));
   UT_ASSERT_EQ_INT(FALSE, utt_ODEmulateExtensionIsRIP(".ripp"));
   UT_ASSERT_EQ_INT(FALSE, utt_ODEmulateExtensionIsRIP(".zip"));
}

static const UTTestCase ut_cases[] = {
   {"RIP extension", accepts_only_the_complete_case_insensitive_extension}
};
