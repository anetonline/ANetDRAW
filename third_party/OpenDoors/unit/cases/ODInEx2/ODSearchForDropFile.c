#define UT_CUSTOM_MOCK_ODSearchInDir
#define UT_CUSTOM_MOCK_getenv
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_strcpy
static const char *ut_found_directory;
static INT ut_found_result;
static const char *ut_environment_directory;
static unsigned ut_search_calls;
static unsigned ut_getenv_calls;
size_t utm_strlen(const char *text)
{
   size_t length = 0; while(text[length]) ++length; return(length);
}
char *utm_strcpy(char *destination, const char *source)
{
   char *result = destination; while((*destination++ = *source++) != 0) {}
   return(result);
}
INT utm_ODSearchInDir(char **names, INT count, char *found, char *directory)
{
   UT_ASSERT_NOT_NULL(names); UT_ASSERT_EQ_INT(2, count); UT_ASSERT_NOT_NULL(found);
   ++ut_search_calls;
   if(ut_found_directory != NULL && strcmp(directory, ut_found_directory) == 0) {
      utm_strcpy(found, "selected"); return(ut_found_result);
   }
   return(-1);
}
char *utm_getenv(const char *name)
{
   ++ut_getenv_calls;
   if(ut_environment_directory != NULL && strcmp(name, "SBBSNODE") == 0)
      return((char *)ut_environment_directory);
   return(NULL);
}

static void reset_search(void)
{
   memset(&od_control, 0, sizeof(od_control)); ut_found_directory = NULL;
   ut_found_result = 1; ut_environment_directory = NULL;
   ut_search_calls = ut_getenv_calls = 0;
}

static void finds_configured_or_current_directory(void)
{
   char *names[2] = {"A", "B"}; char found[160] = ""; char directory[80] = "";
   reset_search(); utm_strcpy(od_control.info_path, "configured");
   ut_found_directory = "configured";
   UT_ASSERT_EQ_INT(1, utt_ODSearchForDropFile(names, 2, found, directory));
   UT_ASSERT(strcmp(directory, "configured") == 0); UT_ASSERT_EQ_UINT(1, ut_search_calls);
   reset_search(); utm_strcpy(od_control.info_path, "configured");
   ut_found_directory = "configured";
   UT_ASSERT_EQ_INT(1, utt_ODSearchForDropFile(names, 2, found, NULL));
   reset_search(); utm_strcpy(od_control.info_path, "configured");
   ut_found_directory = "." DIRSEP_STR;
   UT_ASSERT_EQ_INT(1, utt_ODSearchForDropFile(names, 2, found, NULL));
   UT_ASSERT_EQ_UINT(2, ut_search_calls);
   reset_search(); ut_found_directory = "." DIRSEP_STR;
   UT_ASSERT_EQ_INT(1, utt_ODSearchForDropFile(names, 2, found, directory));
   UT_ASSERT(strcmp(directory, "." DIRSEP_STR) == 0); UT_ASSERT_EQ_UINT(1, ut_search_calls);
}

static void searches_environment_directories_or_reports_absence(void)
{
   char *names[2] = {"A", "B"}; char found[160] = ""; char directory[80] = "";
   reset_search(); ut_environment_directory = "envdir";
   ut_found_directory = "envdir"; ut_found_result = 0;
   UT_ASSERT_EQ_INT(0, utt_ODSearchForDropFile(names, 2, found, directory));
   UT_ASSERT(strcmp(directory, "envdir") == 0); UT_ASSERT_EQ_UINT(6, ut_getenv_calls);
   reset_search(); ut_environment_directory = "envdir";
   ut_found_directory = "envdir"; ut_found_result = 0;
   UT_ASSERT_EQ_INT(0, utt_ODSearchForDropFile(names, 2, found, NULL));
   reset_search(); ut_environment_directory = "envdir";
   UT_ASSERT_EQ_INT(-1, utt_ODSearchForDropFile(names, 2, found, directory));
   UT_ASSERT_EQ_UINT(2, ut_search_calls); UT_ASSERT_EQ_UINT(6, ut_getenv_calls);
   reset_search();
   UT_ASSERT_EQ_INT(-1, utt_ODSearchForDropFile(names, 2, found, directory));
   UT_ASSERT_EQ_UINT(1, ut_search_calls); UT_ASSERT_EQ_UINT(6, ut_getenv_calls);
}

static const UTTestCase ut_cases[] = {
   {"primary directories", finds_configured_or_current_directory},
   {"environment", searches_environment_directories_or_reports_absence}
};
