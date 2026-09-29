#define UT_CUSTOM_MOCK_ODDirOpen
#define UT_CUSTOM_MOCK_ODDirClose
#define UT_CUSTOM_MOCK_fopen
#define UT_CUSTOM_MOCK_fclose

static tODResult ut_directory_result;
static CONST char *ut_expected_path;
static CONST char *ut_expected_mode;
static BOOL ut_fopen_succeeds;
static unsigned ut_close_directory_calls;
static unsigned ut_close_file_calls;
static int ut_file_token;
static tODDirInfo ut_directory_token;

static BOOL ut_strings_equal(CONST char *left, CONST char *right)
{
   while(*left != '\0' && *left == *right)
   {
      ++left;
      ++right;
   }
   return(*left == *right);
}

tODResult utm_ODDirOpen(CONST char *path, WORD attributes,
   tODDirHandle *handle)
{
   UT_ASSERT(ut_strings_equal(ut_expected_path, path));
   UT_ASSERT_EQ_UINT(DIR_ATTRIB_ARCH | DIR_ATTRIB_RDONLY | DIR_ATTRIB_DIREC,
      attributes);
   if(ut_directory_result == kODRCSuccess)
      *handle = ODPTR2HANDLE(&ut_directory_token, tODDirInfo);
   return(ut_directory_result);
}

void utm_ODDirClose(tODDirHandle handle)
{
   ++ut_close_directory_calls;
   UT_ASSERT_EQ_PTR(&ut_directory_token,
      ODHANDLE2PTR(handle, tODDirInfo));
}

FILE *utm_fopen(CONST char *path, CONST char *mode)
{
   UT_ASSERT(ut_strings_equal(ut_expected_path, path));
   UT_ASSERT(ut_strings_equal(ut_expected_mode, mode));
   if(ut_fopen_succeeds)
      return((FILE *)&ut_file_token);
   return(NULL);
}

int utm_fclose(FILE *file)
{
   ++ut_close_file_calls;
   UT_ASSERT_EQ_PTR(&ut_file_token, file);
   return(0);
}

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK_strlen
size_t utm_strlen(CONST char *text)
{
   CONST char *end = text;

   while(*end != '\0')
      ++end;
   return((size_t)(end - text));
}
#endif

#if (defined(ODPLAT_DOS) && defined(__WATCOMC__)) \
   || defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK__dos_getfileattr
static unsigned ut_attributes_result;

unsigned utm__dos_getfileattr(CONST char *path, unsigned *attributes)
{
   UT_ASSERT(ut_strings_equal(ut_expected_path, path));
   UT_ASSERT_NOT_NULL(attributes);
   *attributes = 0;
   return(ut_attributes_result);
}
#endif

static void reports_directory_lookup_failure(void)
{
   ut_expected_path = "MISSING.DAT";
   ut_directory_result = kODRCNoMatch;
   ut_close_directory_calls = 0;
   UT_ASSERT_EQ_INT(TRUE, utt_ODFileAccessMode(ut_expected_path, 0));
   UT_ASSERT_EQ_UINT(0, ut_close_directory_calls);
}

static void reports_existing_file_for_mode_zero(void)
{
   ut_expected_path = "EXISTS.DAT";
   ut_directory_result = kODRCSuccess;
   ut_close_directory_calls = 0;
   UT_ASSERT_EQ_INT(FALSE, utt_ODFileAccessMode(ut_expected_path, 0));
   UT_ASSERT_EQ_UINT(1, ut_close_directory_calls);
}

static void maps_access_modes_and_file_results(void)
{
   static CONST int modes[] = {2, 4, 6};
   static CONST char *mode_strings[] = {"a", "r", "r+"};
   unsigned index;

   ut_expected_path = "ACCESS.DAT";
   ut_directory_result = kODRCSuccess;
   ut_close_directory_calls = 0;
   ut_close_file_calls = 0;
   for(index = 0; index < sizeof(modes) / sizeof(modes[0]); ++index)
   {
      ut_expected_mode = mode_strings[index];
      ut_fopen_succeeds = FALSE;
      UT_ASSERT_EQ_INT(TRUE,
         utt_ODFileAccessMode(ut_expected_path, modes[index]));
      ut_fopen_succeeds = TRUE;
      UT_ASSERT_EQ_INT(FALSE,
         utt_ODFileAccessMode(ut_expected_path, modes[index]));
   }
   UT_ASSERT_EQ_UINT(6, ut_close_directory_calls);
   UT_ASSERT_EQ_UINT(3, ut_close_file_calls);
}

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
static void handles_dos_root_paths(void)
{
   ut_directory_result = kODRCNoMatch;
#ifdef __TURBOC__
   UT_ASSERT_EQ_INT(FALSE, utt_ODFileAccessMode("C:\\", 0));
   UT_ASSERT_EQ_INT(TRUE, utt_ODFileAccessMode("A:\\", 0));
#else
   ut_expected_path = "C:\\";
   ut_attributes_result = 0;
   UT_ASSERT_EQ_INT(FALSE, utt_ODFileAccessMode(ut_expected_path, 0));
   ut_attributes_result = 3;
   UT_ASSERT_EQ_INT(TRUE, utt_ODFileAccessMode(ut_expected_path, 0));
#endif
   UT_ASSERT_EQ_INT(TRUE, utt_ODFileAccessMode("C:\\", 2));
   UT_ASSERT_EQ_INT(TRUE, utt_ODFileAccessMode("\\", 4));

   ut_expected_path = "C:X";
   UT_ASSERT_EQ_INT(TRUE, utt_ODFileAccessMode(ut_expected_path, 0));
   ut_expected_path = "CX\\";
   UT_ASSERT_EQ_INT(TRUE, utt_ODFileAccessMode(ut_expected_path, 0));
   ut_expected_path = "XX";
   UT_ASSERT_EQ_INT(TRUE, utt_ODFileAccessMode(ut_expected_path, 0));
   ut_expected_path = "X";
   UT_ASSERT_EQ_INT(TRUE, utt_ODFileAccessMode(ut_expected_path, 0));
}
#endif

static const UTTestCase ut_cases[] = {
   {"directory failure", reports_directory_lookup_failure},
   {"mode zero", reports_existing_file_for_mode_zero},
   {"access modes", maps_access_modes_and_file_results},
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   {"DOS roots", handles_dos_root_paths}
#endif
};
