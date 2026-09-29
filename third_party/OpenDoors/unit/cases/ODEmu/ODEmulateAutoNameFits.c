#define UT_CUSTOM_MOCK_strlen

static char ut_name[sizeof(szODWorkString) + 1];

size_t utm_strlen(const char *text)
{
   const char *end = text;
   while(*end != '\0')
      ++end;
   return (size_t)(end - text);
}

static void accepts_the_exact_limit_and_rejects_one_more_byte(void)
{
   size_t limit = sizeof(szODWorkString) - sizeof(szRIPExtension);
   memset(ut_name, 'x', sizeof(ut_name));
   ut_name[limit] = '\0';
   UT_ASSERT_EQ_INT(TRUE, utt_ODEmulateAutoNameFits(ut_name));

   ut_name[limit] = 'x';
   ut_name[limit + 1] = '\0';
   UT_ASSERT_EQ_INT(FALSE, utt_ODEmulateAutoNameFits(ut_name));
}

static const UTTestCase ut_cases[] = {
   {"boundary", accepts_the_exact_limit_and_rejects_one_more_byte}
};
