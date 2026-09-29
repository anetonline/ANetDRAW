#if defined(ODPLAT_DOS) && !defined(__TURBOC__)
#define UT_CUSTOM_MOCK_remove
static int ut_delete_result;

int utm_remove(const char *path)
{
   UT_ASSERT_EQ_INT(0, strcmp("DELETE.TMP", path));
   return(ut_delete_result);
}
#elif defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK_remove
static int ut_delete_result;

int utm_remove(const char *path)
{
   UT_ASSERT_EQ_INT(0, strcmp("DELETE.TMP", path));
   return(ut_delete_result);
}
#elif defined(ODPLAT_WIN32)
#define UT_CUSTOM_MOCK_DeleteFileA
static BOOL ut_delete_result;

BOOL WINAPI utm_DeleteFileA(LPCSTR path)
{
   UT_ASSERT_EQ_INT(0, strcmp("DELETE.TMP", path));
   return(ut_delete_result);
}
#elif defined(ODPLAT_NIX)
#define UT_CUSTOM_MOCK_unlink
static int ut_delete_result;

int utm_unlink(const char *path)
{
   UT_ASSERT_EQ_INT(0, strcmp("DELETE.TMP", path));
   return(ut_delete_result);
}
#endif

static void reports_delete_success_and_failure(void)
{
#if defined(ODPLAT_DOS) && defined(__TURBOC__)
   FILE *stream = fopen("DELETE.TMP", "wb");
   UT_ASSERT_NOT_NULL(stream);
   UT_ASSERT_EQ_INT(0, fclose(stream));
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODFileDelete("DELETE.TMP"));
   UT_ASSERT_EQ_INT(kODRCGeneralFailure, utt_ODFileDelete("DELETE.TMP"));
#else
# ifdef ODPLAT_WIN32
   ut_delete_result = TRUE;
# else
   ut_delete_result = 0;
# endif
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODFileDelete("DELETE.TMP"));

# ifdef ODPLAT_WIN32
   ut_delete_result = FALSE;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure, utt_ODFileDelete("DELETE.TMP"));
# elif defined(ODPLAT_NIX)
   ut_delete_result = -1;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure, utt_ODFileDelete("DELETE.TMP"));
# else
   ut_delete_result = -1;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure, utt_ODFileDelete("DELETE.TMP"));
# endif
#endif
}

static const UTTestCase ut_cases[] = {
   {"delete result", reports_delete_success_and_failure}
};
