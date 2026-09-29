#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK_toupper
static int ut_expected_toupper;
int utm_toupper(int character)
{
   UT_ASSERT_EQ_INT(ut_expected_toupper, character);
   UT_ASSERT_EQ_INT((int)(unsigned char)character, character);
   return('C');
}
#endif

#ifdef ODPLAT_DOS
#define UT_CUSTOM_MOCK__setdrvcd
static int ut_expected_drive;
static unsigned ut_change_calls;

void ODSWAPCALL utm__setdrvcd(int drive, char *path)
{
   ++ut_change_calls;
   UT_ASSERT_EQ_INT(ut_expected_drive, drive);
   UT_ASSERT_NOT_NULL(path);
}
#elif defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK__chdrive
#define UT_CUSTOM_MOCK_chdir
static unsigned ut_drive_calls;
static unsigned ut_change_calls;

int utm__chdrive(int drive)
{
   ++ut_drive_calls;
   UT_ASSERT_EQ_INT(3, drive);
   return(0);
}

int utm_chdir(const char *path)
{
   ++ut_change_calls;
   UT_ASSERT_NOT_NULL(path);
   return(0);
}
#elif defined(ODPLAT_WIN32)
#define UT_CUSTOM_MOCK_SetCurrentDirectoryA
static unsigned ut_change_calls;

BOOL WINAPI utm_SetCurrentDirectoryA(LPCSTR path)
{
   ++ut_change_calls;
   UT_ASSERT_NOT_NULL(path);
   return(TRUE);
}
#elif defined(ODPLAT_NIX)
#define UT_CUSTOM_MOCK_chdir
static int ut_change_result;
static unsigned ut_change_calls;

int utm_chdir(const char *path)
{
   ++ut_change_calls;
   UT_ASSERT_NOT_NULL(path);
   return(ut_change_result);
}
#endif

static void changes_paths_with_and_without_drive_designators(void)
{
   char drive_path[] = "c:\\tmp";
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   char high_drive_path[] = {(char)0x80, ':', '\\', 't', 'm', 'p', '\0'};
#endif
   char relative_path[] = "tmp";
   ut_change_calls = 0;
#ifdef ODPLAT_DOS
   ut_expected_drive = 2;
   ut_expected_toupper = 'c';
   utt_ODDirChangeCurrent(drive_path);
   ut_expected_toupper = 0x80;
   utt_ODDirChangeCurrent(high_drive_path);
   ut_expected_drive = 0;
   utt_ODDirChangeCurrent(relative_path);
   UT_ASSERT_EQ_UINT(3, ut_change_calls);
#elif defined(ODPLAT_DOS32)
   ut_drive_calls = 0;
   ut_expected_toupper = 'c';
   utt_ODDirChangeCurrent(drive_path);
   ut_expected_toupper = 0x80;
   utt_ODDirChangeCurrent(high_drive_path);
   utt_ODDirChangeCurrent(relative_path);
   UT_ASSERT_EQ_UINT(2, ut_drive_calls);
   UT_ASSERT_EQ_UINT(3, ut_change_calls);
#elif defined(ODPLAT_WIN32)
   utt_ODDirChangeCurrent(drive_path);
   UT_ASSERT_EQ_UINT(1, ut_change_calls);
#elif defined(ODPLAT_NIX)
   ut_change_result = 0;
   utt_ODDirChangeCurrent(relative_path);
   ut_change_result = -1;
   utt_ODDirChangeCurrent(relative_path);
   UT_ASSERT_EQ_UINT(2, ut_change_calls);
#endif
}

static const UTTestCase ut_cases[] = {
   {"change directory", changes_paths_with_and_without_drive_designators}
};
