#define UT_CUSTOM_MOCK_strlen
size_t utm_strlen(const char *text)
{
   const char *end = text;
   while(*end != '\0') ++end;
   return((size_t)(end - text));
}
static void validates_printable_bounded_names(void)
{
   char long_name[OD_RESERVE_NAME_MAX + 2];
   char low_name[2] = {0x1f, '\0'};
   char high_name[2] = {0x7f, '\0'};
   size_t index;
   UT_ASSERT(!utt_ODReserveNameValid(NULL));
   UT_ASSERT(!utt_ODReserveNameValid(""));
   for(index = 0; index < sizeof(long_name) - 1; ++index)
      long_name[index] = 'x';
   long_name[sizeof(long_name) - 1] = '\0';
   UT_ASSERT(!utt_ODReserveNameValid(long_name));
   UT_ASSERT(!utt_ODReserveNameValid(low_name));
   UT_ASSERT(!utt_ODReserveNameValid(high_name));
   UT_ASSERT(utt_ODReserveNameValid("Visible Name!"));
}
static const UTTestCase ut_cases[] = {
   {"names", validates_printable_bounded_names}
};
