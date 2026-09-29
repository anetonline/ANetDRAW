#ifdef ODPLAT_DOS32
#define UT_CUSTOM_MOCK_fputc
#define UT_CUSTOM_MOCK_fflush
static unsigned ut_fputc_calls;
static unsigned ut_flush_calls;
int utm_fputc(int character, FILE *stream)
{
   ++ut_fputc_calls;
   UT_ASSERT_EQ_INT('\a', character);
   UT_ASSERT_EQ_PTR(stdout, stream);
   return(character);
}
int utm_fflush(FILE *stream)
{
   ++ut_flush_calls;
   UT_ASSERT_EQ_PTR(stdout, stream);
   return(0);
}
#endif
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_MessageBeep
static unsigned ut_beep_calls;
BOOL WINAPI utm_MessageBeep(UINT type)
{
   ++ut_beep_calls;
   UT_ASSERT_EQ_UINT(0xffffffffUL, type);
   return(TRUE);
}
#endif

static void rings_the_platform_bell(void)
{
   utt_ODPlatRingBell();
#ifdef ODPLAT_DOS32
   UT_ASSERT_EQ_UINT(1, ut_fputc_calls);
   UT_ASSERT_EQ_UINT(1, ut_flush_calls);
#endif
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(1, ut_beep_calls);
#endif
}

static const UTTestCase ut_cases[] = {
   {"platform bell", rings_the_platform_bell}
};
