#define UT_CUSTOM_MOCK_free
static tODDirInfo ut_directory;
static unsigned ut_free_calls;

void utm_free(void *memory)
{
   ++ut_free_calls;
   UT_ASSERT_EQ_PTR(&ut_directory, memory);
}

#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_FindClose
static unsigned ut_close_calls;

BOOL WINAPI utm_FindClose(HANDLE handle)
{
   ++ut_close_calls;
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, handle);
   return(TRUE);
}
#endif

#ifdef ODPLAT_NIX
#define UT_CUSTOM_MOCK_globfree
static unsigned ut_close_calls;

void utm_globfree(glob_t *paths)
{
   ++ut_close_calls;
   UT_ASSERT_EQ_PTR(&ut_directory.g, paths);
}
#endif

static void releases_platform_and_directory_storage(void)
{
   memset(&ut_directory, 0, sizeof(ut_directory));
   ut_free_calls = 0;
#if defined(ODPLAT_WIN32) || defined(ODPLAT_NIX)
   ut_close_calls = 0;
#endif
#ifdef ODPLAT_WIN32
   ut_directory.hWindowsDir = (HANDLE)(DWORD_PTR)41;
#endif
   utt_ODDirClose(ODPTR2HANDLE(&ut_directory, tODDirInfo));
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
#if defined(ODPLAT_WIN32) || defined(ODPLAT_NIX)
   UT_ASSERT_EQ_UINT(1, ut_close_calls);
#endif
}

static const UTTestCase ut_cases[] = {
   {"close", releases_platform_and_directory_storage}
};
