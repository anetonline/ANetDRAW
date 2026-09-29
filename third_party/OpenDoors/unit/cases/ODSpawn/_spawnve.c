#ifdef ODPLAT_DOS
#define UT_CUSTOM_MOCK___get_errno_ptr
#define __get_errno_ptr utm___get_errno_ptr
#define UT_CUSTOM_MOCK_strrchr
#define UT_CUSTOM_MOCK_strchr
#define UT_CUSTOM_MOCK_strcpy
#define UT_CUSTOM_MOCK_strcat
#define UT_CUSTOM_MOCK_ODFileAccessMode
#define UT_CUSTOM_MOCK_doxspawn

static int ut_errno_value;
static int ut_access_results[2];
static unsigned ut_access_calls;
static unsigned ut_spawn_calls;
static char ut_spawn_path[80];
static const char *const ut_arguments[] = {"program", NULL};
static const char *const ut_environment[] = {"NAME=value", NULL};

int *utm___get_errno_ptr(void) { return(&ut_errno_value); }

char *utm_strrchr(const char *text, int character)
{
   const char *found = NULL;
   do
   {
      if(*text == character) found = text;
   } while(*text++ != '\0');
   return((char *)found);
}

char *utm_strchr(const char *text, int character)
{
   do
   {
      if(*text == character) return((char *)text);
   } while(*text++ != '\0');
   return(NULL);
}

char *utm_strcpy(char *destination, const char *source)
{
   char *result = destination;
   while((*destination++ = *source++) != '\0') { }
   return(result);
}

char *utm_strcat(char *destination, const char *source)
{
   char *result = destination;
   while(*destination != '\0') ++destination;
   while((*destination++ = *source++) != '\0') { }
   return(result);
}

BOOL utm_ODFileAccessMode(const char *path, int mode)
{
   unsigned call = ut_access_calls++;
   UT_ASSERT_EQ_INT(0, mode);
   if(ut_access_results[call] != 0) errno = ENOENT;
   return((BOOL)ut_access_results[call]);
}

static int utm_doxspawn(const char *path, const char *const arguments[],
   const char *const environment[])
{
   ++ut_spawn_calls;
   UT_ASSERT_EQ_PTR(ut_arguments, arguments);
   UT_ASSERT_EQ_PTR(ut_environment, environment);
   utm_strcpy(ut_spawn_path, path);
   return(7);
}

static void reset_spawnve(void)
{
   ut_access_results[0] = ut_access_results[1] = 1;
   ut_access_calls = ut_spawn_calls = 0;
   ut_spawn_path[0] = '\0';
   errno = 0;
}

static int call_spawnve(int mode, const char *path)
{
   return(utt__spawnve(mode, path, ut_arguments, ut_environment));
}

static void rejects_non_wait_mode(void)
{
   reset_spawnve();
   UT_ASSERT_EQ_INT(-1, call_spawnve(P_NOWAIT, "door"));
   UT_ASSERT_EQ_INT(EINVAL, errno);
   UT_ASSERT_EQ_UINT(0, ut_access_calls);
}

static void recognizes_extensions_after_each_path_separator_form(void)
{
   static const char *paths[] = {
      "door.com", "dir/door.com", "dir\\door.com",
      "left\\right/door.com", "left/right\\door.com"
   };
   unsigned index;
   for(index = 0; index < sizeof(paths) / sizeof(paths[0]); ++index)
   {
      reset_spawnve(); ut_access_results[0] = 0;
      UT_ASSERT_EQ_INT(7, call_spawnve(P_WAIT, paths[index]));
      UT_ASSERT(strcmp(paths[index], ut_spawn_path) == 0);
      UT_ASSERT_EQ_UINT(1, ut_access_calls);
   }
   reset_spawnve();
   UT_ASSERT_EQ_INT(-1, call_spawnve(P_WAIT, "door.com"));
   UT_ASSERT_EQ_INT(ENOENT, errno);
}

static void tries_com_then_exe_for_an_extensionless_path(void)
{
   reset_spawnve(); ut_access_results[0] = 0;
   UT_ASSERT_EQ_INT(7, call_spawnve(P_WAIT, "dir\\door"));
   UT_ASSERT(strcmp("dir\\door.com", ut_spawn_path) == 0);
   UT_ASSERT_EQ_UINT(1, ut_access_calls);

   reset_spawnve(); ut_access_results[1] = 0;
   UT_ASSERT_EQ_INT(7, call_spawnve(P_WAIT, "dir/door"));
   UT_ASSERT(strcmp("dir/door.exe", ut_spawn_path) == 0);
   UT_ASSERT_EQ_UINT(2, ut_access_calls);

   reset_spawnve();
   UT_ASSERT_EQ_INT(-1, call_spawnve(P_WAIT, "door"));
   UT_ASSERT_EQ_UINT(2, ut_access_calls);
   UT_ASSERT_EQ_INT(ENOENT, errno);
}

static const UTTestCase ut_cases[] = {
   {"invalid mode", rejects_non_wait_mode},
   {"explicit extension", recognizes_extensions_after_each_path_separator_form},
   {"implicit extensions", tries_com_then_exe_for_an_extensionless_path}
};
#endif
