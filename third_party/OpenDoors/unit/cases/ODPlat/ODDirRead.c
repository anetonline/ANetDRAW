#define UT_CUSTOM_MOCK_ODStringCopy
static CONST char *ut_expected_copy_sources[2];
static unsigned ut_copy_calls;

static void ut_expect_copies(CONST char *first, CONST char *second)
{
   ut_expected_copy_sources[0] = first;
   ut_expected_copy_sources[1] = second;
   ut_copy_calls = 0;
}

void utm_ODStringCopy(char *destination, CONST char *source, INT size)
{
   INT index = 0;

   ++ut_copy_calls;
   UT_ASSERT(ut_copy_calls <= 2);
   if(ut_copy_calls <= 2)
      UT_ASSERT_EQ_PTR(ut_expected_copy_sources[ut_copy_calls - 1], source);
   UT_ASSERT_EQ_INT(DIR_FILENAME_SIZE, size);
   while(index + 1 < size && source[index] != '\0')
   {
      destination[index] = source[index];
      ++index;
   }
   destination[index] = '\0';
}

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) \
   || defined(ODPLAT_WIN32)
#define UT_CUSTOM_MOCK_DOSToCTime
static WORD ut_expected_date;
static WORD ut_expected_time;
static time_t ut_converted_time;

time_t utm_DOSToCTime(WORD date_value, WORD time_value)
{
   UT_ASSERT_EQ_UINT(ut_expected_date, date_value);
   UT_ASSERT_EQ_UINT(ut_expected_time, time_value);
   return(ut_converted_time);
}
#endif

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK_ODDirDOSFindNext
static INT ut_findnext_result;

INT utm_ODDirDOSFindNext(tDOSDirEntry *block)
{
   UT_ASSERT_NOT_NULL(block);
   return(ut_findnext_result);
}
#endif

#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_FileTimeToDosDateTime
#define UT_CUSTOM_MOCK_FindNextFileA
#define UT_CUSTOM_MOCK_ODDirWinMatchesAttributes
static BOOL ut_findnext_results[3];
static unsigned ut_findnext_count;
static BOOL ut_match_results[3];
static unsigned ut_match_count;

size_t utm_strlen(CONST char *text)
{
   CONST char *end = text;
   while(*end != '\0')
      ++end;
   return((size_t)(end - text));
}

BOOL WINAPI utm_FileTimeToDosDateTime(CONST FILETIME *file_time,
   LPWORD date_value, LPWORD time_value)
{
   UT_ASSERT_NOT_NULL(file_time);
   *date_value = ut_expected_date;
   *time_value = ut_expected_time;
   return(TRUE);
}

BOOL WINAPI utm_FindNextFileA(HANDLE handle, LPWIN32_FIND_DATAA data)
{
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, handle);
   UT_ASSERT_NOT_NULL(data);
   return(ut_findnext_results[ut_findnext_count++]);
}

BOOL utm_ODDirWinMatchesAttributes(tODDirInfo *directory)
{
   UT_ASSERT_NOT_NULL(directory);
   return(ut_match_results[ut_match_count++]);
}
#endif

#ifdef ODPLAT_NIX
#define UT_CUSTOM_MOCK_stat
#define UT_CUSTOM_MOCK_strrchr
#define UT_CUSTOM_MOCK_ODDirAttributesMatch
static int ut_stat_results[4];
static mode_t ut_stat_modes[4];
static off_t ut_stat_sizes[4];
static time_t ut_stat_times[4];
static unsigned ut_stat_count;
static BOOL ut_attribute_results[4];
static unsigned ut_attribute_count;

int utm_stat(CONST char *path, struct utm_stat *information)
{
   unsigned index = ut_stat_count++;
   UT_ASSERT_NOT_NULL(path);
   if(ut_stat_results[index] == 0)
   {
      information->st_mode = ut_stat_modes[index];
      information->st_size = ut_stat_sizes[index];
      information->st_mtime = ut_stat_times[index];
   }
   return(ut_stat_results[index]);
}

char *utm_strrchr(CONST char *text, int character)
{
   CONST char *last = NULL;
   CONST char *position = text;

   UT_ASSERT_EQ_INT(DIRSEP, character);
   do
   {
      if(*position == (char)character)
         last = position;
   } while(*position++ != '\0');
   return((char *)last);
}

BOOL utm_ODDirAttributesMatch(WORD entry_attributes, WORD search_attributes)
{
   UT_ASSERT_EQ_UINT(DIR_ATTRIB_HIDDEN | DIR_ATTRIB_DIREC,
      search_attributes);
   UT_ASSERT((entry_attributes & DIR_ATTRIB_NORMAL) == DIR_ATTRIB_NORMAL);
   return(ut_attribute_results[ut_attribute_count++]);
}
#endif

static void reports_an_already_exhausted_search(void)
{
   tODDirInfo directory;
   tODDirEntry entry;

   directory.bEOF = TRUE;
   UT_ASSERT_EQ_INT(kODRCEndOfFile,
      utt_ODDirRead(ODPTR2HANDLE(&directory, tODDirInfo), &entry));
}

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
static void reads_dos_entries_and_advances(void)
{
   tODDirInfo directory;
   tODDirEntry entry;

   directory.bEOF = FALSE;
   strcpy(directory.FindBlock.szFileName, "ENTRY.DAT");
   directory.FindBlock.btAttrib = DIR_ATTRIB_ARCH | DIR_ATTRIB_RDONLY;
   directory.FindBlock.dwFileSize = 1234;
   directory.FindBlock.wFileDate = 0x5821;
   directory.FindBlock.wFileTime = 0x6420;
   ut_expect_copies(directory.FindBlock.szFileName,
      directory.FindBlock.szFileName);
   ut_expected_date = directory.FindBlock.wFileDate;
   ut_expected_time = directory.FindBlock.wFileTime;
   ut_converted_time = (time_t)9876;
   ut_findnext_result = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODDirRead(ODPTR2HANDLE(&directory, tODDirInfo), &entry));
   UT_ASSERT_EQ_INT(FALSE, directory.bEOF);
   UT_ASSERT_EQ_UINT(DIR_ATTRIB_ARCH | DIR_ATTRIB_RDONLY, entry.wAttributes);
   UT_ASSERT_EQ_UINT(1234, entry.dwFileSize);
   UT_ASSERT_EQ_INT(9876, entry.LastWriteTime);
   UT_ASSERT_EQ_UINT(1, ut_copy_calls);

   ut_findnext_result = -1;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODDirRead(ODPTR2HANDLE(&directory, tODDirInfo), &entry));
   UT_ASSERT(directory.bEOF != FALSE);
}
#endif

#ifdef ODPLAT_WIN32
static void reads_windows_names_attributes_and_search_state(void)
{
   tODDirInfo directory;
   tODDirEntry entry;

   memset(&directory, 0, sizeof(directory));
   directory.hWindowsDir = (HANDLE)(DWORD_PTR)41;
   strcpy(directory.WindowsDirEntry.cFileName, "LongName.dat");
   directory.WindowsDirEntry.nFileSizeLow = 4321;
   ut_expect_copies(directory.WindowsDirEntry.cFileName, NULL);
   ut_expected_date = 0x5821;
   ut_expected_time = 0x6420;
   ut_converted_time = (time_t)9876;
   ut_findnext_count = 0;
   ut_findnext_results[0] = FALSE;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODDirRead(ODPTR2HANDLE(&directory, tODDirInfo), &entry));
   UT_ASSERT_EQ_INT(TRUE, directory.bEOF);
   UT_ASSERT_EQ_UINT(DIR_ATTRIB_NORMAL, entry.wAttributes);
   UT_ASSERT_EQ_UINT(4321, entry.dwFileSize);
   UT_ASSERT_EQ_INT(9876, entry.LastWriteTime);

   memset(&directory, 0, sizeof(directory));
   directory.hWindowsDir = (HANDLE)(DWORD_PTR)41;
   strcpy(directory.WindowsDirEntry.cAlternateFileName, "SHORT.DAT");
   directory.WindowsDirEntry.dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE
      | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN
      | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM;
   ut_expect_copies(directory.WindowsDirEntry.cAlternateFileName, NULL);
   ut_findnext_count = 0;
   ut_findnext_results[0] = TRUE;
   ut_findnext_results[1] = TRUE;
   ut_match_count = 0;
   ut_match_results[0] = FALSE;
   ut_match_results[1] = TRUE;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODDirRead(ODPTR2HANDLE(&directory, tODDirInfo), &entry));
   UT_ASSERT_EQ_INT(FALSE, directory.bEOF);
   UT_ASSERT_EQ_UINT(DIR_ATTRIB_NORMAL | DIR_ATTRIB_ARCH | DIR_ATTRIB_DIREC
      | DIR_ATTRIB_HIDDEN | DIR_ATTRIB_RDONLY | DIR_ATTRIB_SYSTEM,
      entry.wAttributes);
}
#endif

#ifdef ODPLAT_NIX
static void reads_and_filters_unix_entries(void)
{
   static char bad_name[] = "bad";
   static char good_name[] = "dir/GOOD";
   static char plain_name[] = "PLAIN";
   static char other_name[] = "OTHER";
   static char *paths[3];
   struct utm_stat root_information;
   tODDirInfo directory;
   tODDirEntry entry;

   UT_ASSERT_EQ_INT(0, lstat("/", &root_information));
   UT_ASSERT(S_ISDIR(root_information.st_mode));

   memset(&directory, 0, sizeof(directory));
   paths[0] = bad_name;
   paths[1] = good_name;
   directory.g.gl_pathv = paths;
   directory.g.gl_pathc = 2;
   directory.wAttributes = DIR_ATTRIB_HIDDEN | DIR_ATTRIB_DIREC;
   ut_stat_count = 0;
   ut_stat_results[0] = -1;
   ut_stat_results[1] = 0;
   ut_stat_modes[1] = root_information.st_mode & ~(S_IRUSR | S_IWUSR);
   ut_stat_sizes[1] = 222;
   ut_stat_times[1] = (time_t)333;
   ut_attribute_count = 0;
   ut_attribute_results[0] = TRUE;
   ut_expect_copies(good_name + 4, NULL);
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODDirRead(ODPTR2HANDLE(&directory, tODDirInfo), &entry));
   UT_ASSERT_EQ_INT(TRUE, directory.bEOF);
   UT_ASSERT_EQ_UINT(DIR_ATTRIB_NORMAL | DIR_ATTRIB_DIREC
      | DIR_ATTRIB_RDONLY | DIR_ATTRIB_SYSTEM, entry.wAttributes);
   UT_ASSERT_EQ_UINT(222, entry.dwFileSize);
   UT_ASSERT_EQ_INT(333, entry.LastWriteTime);

   memset(&directory, 0, sizeof(directory));
   paths[0] = bad_name;
   directory.g.gl_pathv = paths;
   directory.g.gl_pathc = 1;
   directory.wAttributes = DIR_ATTRIB_HIDDEN | DIR_ATTRIB_DIREC;
   ut_stat_count = 0;
   ut_stat_results[0] = -1;
   UT_ASSERT_EQ_INT(kODRCEndOfFile,
      utt_ODDirRead(ODPTR2HANDLE(&directory, tODDirInfo), &entry));

   memset(&directory, 0, sizeof(directory));
   paths[0] = plain_name;
   paths[1] = other_name;
   directory.g.gl_pathv = paths;
   directory.g.gl_pathc = 2;
   directory.wAttributes = DIR_ATTRIB_HIDDEN | DIR_ATTRIB_DIREC;
   ut_stat_count = 0;
   ut_stat_results[0] = 0;
   ut_stat_modes[0] = S_IRUSR | S_IWUSR;
   ut_stat_sizes[0] = 11;
   ut_stat_times[0] = (time_t)12;
   ut_attribute_count = 0;
   ut_attribute_results[0] = TRUE;
   ut_expect_copies(plain_name, NULL);
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODDirRead(ODPTR2HANDLE(&directory, tODDirInfo), &entry));
   UT_ASSERT_EQ_INT(FALSE, directory.bEOF);
   UT_ASSERT_EQ_UINT(DIR_ATTRIB_NORMAL, entry.wAttributes);

   memset(&directory, 0, sizeof(directory));
   paths[0] = plain_name;
   paths[1] = other_name;
   directory.g.gl_pathv = paths;
   directory.g.gl_pathc = 2;
   directory.wAttributes = DIR_ATTRIB_HIDDEN | DIR_ATTRIB_DIREC;
   ut_stat_count = 0;
   ut_stat_results[0] = 0;
   ut_stat_results[1] = 0;
   ut_stat_modes[0] = ut_stat_modes[1] = S_IRUSR | S_IWUSR;
   ut_attribute_count = 0;
   ut_attribute_results[0] = FALSE;
   ut_attribute_results[1] = FALSE;
   ut_expect_copies(plain_name, other_name);
   UT_ASSERT_EQ_INT(kODRCEndOfFile,
      utt_ODDirRead(ODPTR2HANDLE(&directory, tODDirInfo), &entry));
}
#endif

static const UTTestCase ut_cases[] = {
   {"already exhausted", reports_an_already_exhausted_search},
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   {"DOS entries", reads_dos_entries_and_advances}
#elif defined(ODPLAT_WIN32)
   {"Windows entries", reads_windows_names_attributes_and_search_state}
#else
   {"UNIX entries", reads_and_filters_unix_entries}
#endif
};
