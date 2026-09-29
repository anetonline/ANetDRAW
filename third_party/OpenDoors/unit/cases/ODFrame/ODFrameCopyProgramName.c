#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODKrnlGetUIState
#define UT_CUSTOM_MOCK_ODStringCopy
static unsigned ut_copy_calls;
void utm_ODKrnlGetUIState(tODUIState *state)
{ memset(state, 0, sizeof(*state)); strcpy(state->szProgramName, "OpenDoors"); }
void utm_ODStringCopy(char *destination, const char *source, INT size)
{
   size_t length;
   UT_ASSERT(strcmp(source, "OpenDoors") == 0); ++ut_copy_calls;
   length = strlen(source); if(length >= (size_t)size) length = (size_t)size - 1;
   memcpy(destination, source, length); destination[length] = '\0';
}
static void copies_the_cached_program_name(void)
{
   char destination[5]; ut_copy_calls = 0;
   utt_ODFrameCopyProgramName(destination, sizeof(destination));
   UT_ASSERT(strcmp(destination, "Open") == 0); UT_ASSERT_EQ_UINT(1, ut_copy_calls);
}
static const UTTestCase ut_cases[] = {{"cached copy", copies_the_cached_program_name}};
#endif
