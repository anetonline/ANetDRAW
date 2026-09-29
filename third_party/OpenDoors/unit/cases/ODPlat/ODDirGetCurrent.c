#define UT_CUSTOM_MOCK_strlen
size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0')
      ++length;
   return(length);
}

#ifdef ODPLAT_DOS
#define UT_CUSTOM_MOCK_strcpy
#define UT_CUSTOM_MOCK__getdrv
#define UT_CUSTOM_MOCK__getcd

char *utm_strcpy(char *destination, const char *source)
{
   char *result = destination;
   while((*destination++ = *source++) != '\0')
      ;
   return(result);
}

int utm__getdrv(void)
{
   return(2);
}

int ODSWAPCALL utm__getcd(int drive, char *directory)
{
   UT_ASSERT_EQ_INT(0, drive);
   utm_strcpy(directory, "WORK");
   return(0);
}
#elif defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK__getdcwd
static BOOL ut_get_current_succeeds;

char *utm__getdcwd(int drive, char *buffer, size_t size)
{
   UT_ASSERT_EQ_INT(0, drive);
   UT_ASSERT(size >= 4);
   if(!ut_get_current_succeeds)
      return(NULL);
   buffer[0] = 'C';
   buffer[1] = ':';
   buffer[2] = '\\';
   buffer[3] = '\0';
   return(buffer);
}
#elif defined(ODPLAT_WIN32)
#define UT_CUSTOM_MOCK_GetCurrentDirectoryA
DWORD WINAPI utm_GetCurrentDirectoryA(DWORD size, LPSTR buffer)
{
   UT_ASSERT(size >= 4);
   buffer[0] = 'C';
   buffer[1] = ':';
   buffer[2] = '\\';
   buffer[3] = '\0';
   return(3);
}
#elif defined(ODPLAT_NIX)
#define UT_CUSTOM_MOCK_getcwd
static BOOL ut_get_current_succeeds;

char *utm_getcwd(char *buffer, size_t size)
{
   UT_ASSERT(size >= 5);
   if(!ut_get_current_succeeds)
      return(NULL);
   buffer[0] = '/';
   buffer[1] = 't';
   buffer[2] = 'm';
   buffer[3] = 'p';
   buffer[4] = '\0';
   return(buffer);
}
#endif

static void returns_the_platform_current_directory(void)
{
   char path[32];
   memset(path, 'X', sizeof(path));
#ifdef ODPLAT_DOS
   utt_ODDirGetCurrent(path, sizeof(path));
   UT_ASSERT_EQ_INT(0, strcmp("C:\\WORK", path));
#elif defined(ODPLAT_DOS32)
   utt_ODDirGetCurrent(path, 3);
   UT_ASSERT_EQ_INT('\0', path[0]);
   ut_get_current_succeeds = TRUE;
   utt_ODDirGetCurrent(path, sizeof(path));
   UT_ASSERT_EQ_INT(0, strcmp("C:\\", path));
   ut_get_current_succeeds = FALSE;
   utt_ODDirGetCurrent(path, sizeof(path));
   UT_ASSERT_EQ_INT('\0', path[0]);
#elif defined(ODPLAT_WIN32)
   utt_ODDirGetCurrent(path, sizeof(path));
   UT_ASSERT_EQ_INT(0, strcmp("C:\\", path));
#elif defined(ODPLAT_NIX)
   ut_get_current_succeeds = TRUE;
   utt_ODDirGetCurrent(path, sizeof(path));
   UT_ASSERT_EQ_INT(0, strcmp("/tmp", path));
   ut_get_current_succeeds = FALSE;
   utt_ODDirGetCurrent(path, sizeof(path));
   UT_ASSERT_EQ_INT('\0', path[0]);
#endif
}

static const UTTestCase ut_cases[] = {
   {"current directory", returns_the_platform_current_directory}
};
