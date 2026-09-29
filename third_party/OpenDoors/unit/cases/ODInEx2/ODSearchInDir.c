#define UT_CUSTOM_MOCK_ODMakeFilename
#define UT_CUSTOM_MOCK_ODDirOpen
#define UT_CUSTOM_MOCK_ODDirRead
#define UT_CUSTOM_MOCK_ODDirClose
#define UT_CUSTOM_MOCK_ODFileAccessMode
static BOOL ut_open[3];
static BOOL ut_access_denied[3];
static time_t ut_times[3];
static unsigned ut_close_calls;
static int ut_handles[3];
static const char *ut_names[3] = {"ZERO", "ONE", "TWO"};
static int ut_name_index(const char *path)
{
   int index; for(index = 0; index < 3; ++index)
      if(strstr(path, ut_names[index]) != NULL) return(index);
   return(-1);
}
tODResult utm_ODMakeFilename(char *output, CONST char *path,
   CONST char *name, INT size)
{
   size_t needed = strlen(path) + strlen(name) + 1;
   UT_ASSERT_NOT_NULL(output); UT_ASSERT_NOT_NULL(path); UT_ASSERT_NOT_NULL(name);
   UT_ASSERT((size_t)size >= needed); strcpy(output, path); strcat(output, name);
   return(kODRCSuccess);
}
tODResult utm_ODDirOpen(CONST char *path, WORD attributes, tODDirHandle *handle)
{
   int index = ut_name_index(path); UT_ASSERT(index >= 0);
   UT_ASSERT_EQ_UINT(DIR_ATTRIB_NORMAL | DIR_ATTRIB_ARCH, attributes);
   UT_ASSERT_NOT_NULL(handle); *handle = ODPTR2HANDLE(&ut_handles[index], int);
   return(ut_open[index] ? kODRCSuccess : kODRCGeneralFailure);
}
tODResult utm_ODDirRead(tODDirHandle handle, tODDirEntry *entry)
{
   int index;
   for(index = 0; index < 3; ++index)
      if(handle == ODPTR2HANDLE(&ut_handles[index], int)) break;
   UT_ASSERT(index < 3);
   memset(entry, 0, sizeof(*entry)); entry->LastWriteTime = ut_times[index];
   return(kODRCSuccess);
}
void utm_ODDirClose(tODDirHandle handle)
{
   UT_ASSERT_NOT_NULL(handle); ++ut_close_calls;
}
BOOL utm_ODFileAccessMode(const char *path, int mode)
{
   int index = ut_name_index(path); UT_ASSERT(index >= 0); UT_ASSERT_EQ_INT(4, mode);
   return(ut_access_denied[index]);
}
static void reset_directory(void)
{
   memset(ut_open, 0, sizeof(ut_open));
   memset(ut_access_denied, 0, sizeof(ut_access_denied));
   memset(ut_times, 0, sizeof(ut_times)); ut_close_calls = 0;
}
static void reports_no_matching_readable_file(void)
{
   char found[160] = "unchanged"; char *names[3] = {"ZERO", "ONE", "TWO"};
   reset_directory();
   UT_ASSERT_EQ_INT(-1, utt_ODSearchInDir(names, 3, found, "dir/"));
   UT_ASSERT(strcmp(found, "unchanged") == 0); UT_ASSERT_EQ_UINT(0, ut_close_calls);
   reset_directory(); ut_open[0] = TRUE; ut_access_denied[0] = TRUE;
   UT_ASSERT_EQ_INT(-1, utt_ODSearchInDir(names, 3, found, "dir/"));
   UT_ASSERT_EQ_UINT(1, ut_close_calls);
}
static void skips_generic_dorinfo_after_node_specific_match(void)
{
   char found[160]; char *names[3] = {"ZERO", "ONE", "TWO"};
   reset_directory(); ut_open[1] = TRUE; ut_times[1] = 100;
   UT_ASSERT_EQ_INT(1, utt_ODSearchInDir(names, 3, found, "dir/"));
   UT_ASSERT(strcmp(found, "dir/ONE") == 0); UT_ASSERT_EQ_UINT(1, ut_close_calls);
}
static void keeps_same_session_priority_and_accepts_newer_file(void)
{
   char found[160]; char *names[3] = {"ZERO", "ONE", "TWO"};
   reset_directory(); ut_open[0] = ut_open[1] = ut_open[2] = TRUE;
   ut_times[0] = 100; ut_times[1] = 105; ut_times[2] = 120;
   UT_ASSERT_EQ_INT(2, utt_ODSearchInDir(names, 3, found, "dir/"));
   UT_ASSERT(strcmp(found, "dir/TWO") == 0); UT_ASSERT_EQ_UINT(3, ut_close_calls);
   reset_directory(); ut_open[0] = ut_open[1] = TRUE;
   ut_times[0] = 100; ut_times[1] = 120; ut_access_denied[1] = TRUE;
   UT_ASSERT_EQ_INT(0, utt_ODSearchInDir(names, 3, found, "dir/"));
}
static const UTTestCase ut_cases[] = {
   {"not found", reports_no_matching_readable_file},
   {"DORINFO priority", skips_generic_dorinfo_after_node_specific_match},
   {"timestamps", keeps_same_session_priority_and_accepts_newer_file}
};
