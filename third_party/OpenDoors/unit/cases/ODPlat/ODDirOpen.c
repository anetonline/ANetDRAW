#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_free
static tODDirInfo ut_directory;
static tODDirInfo ut_sentinel_directory;
static BOOL ut_allow_allocation;
static unsigned ut_free_calls;

static void ut_clear_directory(void)
{
   unsigned char *byte = (unsigned char *)&ut_directory;
   size_t remaining = sizeof(ut_directory);

   while(remaining != 0)
   {
      *byte++ = 0;
      --remaining;
   }
}

void *utm_malloc(size_t size)
{
   UT_ASSERT_EQ_UINT(sizeof(tODDirInfo), size);
   if(!ut_allow_allocation)
      return(NULL);
   ut_clear_directory();
   return(&ut_directory);
}

void utm_free(void *memory)
{
   ++ut_free_calls;
   UT_ASSERT_EQ_PTR(&ut_directory, memory);
}

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK_ODDirDOSFindFirst
static INT ut_find_result;

INT utm_ODDirDOSFindFirst(CONST char *path, tDOSDirEntry *block,
   WORD attributes)
{
   UT_ASSERT(strcmp("*.DAT", path) == 0);
   UT_ASSERT_EQ_PTR(&ut_directory.FindBlock, block);
   UT_ASSERT_EQ_UINT(DIR_ATTRIB_HIDDEN, attributes);
   return(ut_find_result);
}
#endif

#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_FindFirstFileA
#define UT_CUSTOM_MOCK_FindNextFileA
#define UT_CUSTOM_MOCK_FindClose
#define UT_CUSTOM_MOCK_ODDirWinMatchesAttributes
static HANDLE ut_findfirst_result;
static BOOL ut_findnext_results[3];
static unsigned ut_findnext_count;
static BOOL ut_match_results[3];
static unsigned ut_match_count;
static unsigned ut_findclose_calls;

HANDLE WINAPI utm_FindFirstFileA(LPCSTR path, LPWIN32_FIND_DATAA data)
{
   UT_ASSERT(strcmp("*.DAT", path) == 0);
   UT_ASSERT_EQ_PTR(&ut_directory.WindowsDirEntry, data);
   return(ut_findfirst_result);
}

BOOL WINAPI utm_FindNextFileA(HANDLE handle, LPWIN32_FIND_DATAA data)
{
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, handle);
   UT_ASSERT_EQ_PTR(&ut_directory.WindowsDirEntry, data);
   return(ut_findnext_results[ut_findnext_count++]);
}

BOOL WINAPI utm_FindClose(HANDLE handle)
{
   ++ut_findclose_calls;
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, handle);
   return(TRUE);
}

BOOL utm_ODDirWinMatchesAttributes(tODDirInfo *directory)
{
   UT_ASSERT_EQ_PTR(&ut_directory, directory);
   return(ut_match_results[ut_match_count++]);
}
#endif

#ifdef ODPLAT_NIX
#define UT_CUSTOM_MOCK_glob
#define UT_CUSTOM_MOCK_globfree
#define UT_CUSTOM_MOCK_memset
static INT ut_glob_result;
static size_t ut_glob_path_count;
static unsigned ut_globfree_calls;

void *utm_memset(void *memory, int value, size_t size)
{
   unsigned char *byte = (unsigned char *)memory;
   size_t remaining = size;

   UT_ASSERT_EQ_PTR(&ut_directory.g, memory);
   UT_ASSERT_EQ_INT(0, value);
   UT_ASSERT_EQ_UINT(sizeof(ut_directory.g), size);
   while(remaining != 0)
   {
      *byte++ = 0;
      --remaining;
   }
   return(memory);
}

int utm_glob(CONST char *pattern, int flags,
   int (*error_function)(CONST char *, int), glob_t *paths)
{
   UT_ASSERT(strcmp("*.DAT", pattern) == 0);
   UT_ASSERT_EQ_INT(GLOB_NOSORT, flags);
   UT_ASSERT_NULL(error_function);
   UT_ASSERT_EQ_PTR(&ut_directory.g, paths);
   paths->gl_pathc = ut_glob_path_count;
   return(ut_glob_result);
}

void utm_globfree(glob_t *paths)
{
   ++ut_globfree_calls;
   UT_ASSERT_EQ_PTR(&ut_directory.g, paths);
}
#endif

static void allocation_failure_leaves_handle_untouched(void)
{
   tODDirHandle handle = ODPTR2HANDLE(&ut_sentinel_directory, tODDirInfo);

   ut_allow_allocation = FALSE;
   ut_free_calls = 0;
   UT_ASSERT_EQ_INT(kODRCNoMemory,
      utt_ODDirOpen("*.DAT", DIR_ATTRIB_HIDDEN, &handle));
   UT_ASSERT_EQ_PTR(&ut_sentinel_directory,
      ODHANDLE2PTR(handle, tODDirInfo));
   UT_ASSERT_EQ_UINT(0, ut_free_calls);
}

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
static void handles_dos_search_results(void)
{
   tODDirHandle handle = ODPTR2HANDLE(&ut_sentinel_directory, tODDirInfo);

   ut_allow_allocation = TRUE;
   ut_free_calls = 0;
   ut_find_result = -1;
   UT_ASSERT_EQ_INT(kODRCNoMatch,
      utt_ODDirOpen("*.DAT", DIR_ATTRIB_HIDDEN, &handle));
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
   UT_ASSERT_EQ_PTR(&ut_sentinel_directory,
      ODHANDLE2PTR(handle, tODDirInfo));
   ut_find_result = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODDirOpen("*.DAT", DIR_ATTRIB_HIDDEN, &handle));
   UT_ASSERT_EQ_PTR(&ut_directory, ODHANDLE2PTR(handle, tODDirInfo));
   UT_ASSERT_EQ_INT(FALSE, ut_directory.bEOF);
}
#endif

#ifdef ODPLAT_WIN32
static void handles_windows_search_results(void)
{
   tODDirHandle handle = ODPTR2HANDLE(&ut_sentinel_directory, tODDirInfo);

   ut_allow_allocation = TRUE;
   ut_free_calls = 0;
   ut_findclose_calls = 0;
   ut_findfirst_result = INVALID_HANDLE_VALUE;
   UT_ASSERT_EQ_INT(kODRCNoMatch,
      utt_ODDirOpen("*.DAT", DIR_ATTRIB_HIDDEN, &handle));
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
   UT_ASSERT_EQ_PTR(&ut_sentinel_directory,
      ODHANDLE2PTR(handle, tODDirInfo));

   ut_findfirst_result = (HANDLE)(DWORD_PTR)41;
   ut_match_count = 0;
   ut_match_results[0] = FALSE;
   ut_findnext_count = 0;
   ut_findnext_results[0] = FALSE;
   UT_ASSERT_EQ_INT(kODRCNoMatch,
      utt_ODDirOpen("*.DAT", DIR_ATTRIB_HIDDEN, &handle));
   UT_ASSERT_EQ_UINT(2, ut_free_calls);
   UT_ASSERT_EQ_UINT(1, ut_findclose_calls);

   ut_match_count = 0;
   ut_match_results[0] = FALSE;
   ut_match_results[1] = FALSE;
   ut_match_results[2] = TRUE;
   ut_findnext_count = 0;
   ut_findnext_results[0] = TRUE;
   ut_findnext_results[1] = TRUE;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODDirOpen("*.DAT", DIR_ATTRIB_HIDDEN, &handle));
   UT_ASSERT_EQ_PTR(&ut_directory, ODHANDLE2PTR(handle, tODDirInfo));
   UT_ASSERT_EQ_UINT(DIR_ATTRIB_HIDDEN, ut_directory.wAttributes);

   ut_match_count = 0;
   ut_match_results[0] = TRUE;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODDirOpen("*.DAT", DIR_ATTRIB_HIDDEN, &handle));
}
#endif

#ifdef ODPLAT_NIX
static void handles_unix_glob_results(void)
{
   tODDirHandle handle = ODPTR2HANDLE(&ut_sentinel_directory, tODDirInfo);

   ut_allow_allocation = TRUE;
   ut_free_calls = 0;
   ut_globfree_calls = 0;
   ut_glob_result = GLOB_NOMATCH;
   ut_glob_path_count = 1;
   UT_ASSERT_EQ_INT(kODRCNoMatch,
      utt_ODDirOpen("*.DAT", DIR_ATTRIB_HIDDEN, &handle));
   ut_glob_result = 0;
   ut_glob_path_count = 0;
   UT_ASSERT_EQ_INT(kODRCNoMatch,
      utt_ODDirOpen("*.DAT", DIR_ATTRIB_HIDDEN, &handle));
   UT_ASSERT_EQ_UINT(2, ut_globfree_calls);
   UT_ASSERT_EQ_UINT(2, ut_free_calls);
   UT_ASSERT_EQ_PTR(&ut_sentinel_directory,
      ODHANDLE2PTR(handle, tODDirInfo));

   ut_glob_path_count = 2;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODDirOpen("*.DAT", DIR_ATTRIB_HIDDEN, &handle));
   UT_ASSERT_EQ_PTR(&ut_directory, ODHANDLE2PTR(handle, tODDirInfo));
   UT_ASSERT_EQ_UINT(0, ut_directory.pos);
   UT_ASSERT_EQ_UINT(DIR_ATTRIB_HIDDEN, ut_directory.wAttributes);
}
#endif

static const UTTestCase ut_cases[] = {
   {"allocation failure", allocation_failure_leaves_handle_untouched},
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   {"DOS search", handles_dos_search_results}
#elif defined(ODPLAT_WIN32)
   {"Windows search", handles_windows_search_results}
#else
   {"UNIX glob", handles_unix_glob_results}
#endif
};
