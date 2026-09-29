#define UT_CUSTOM_MOCK_ODDirAttributesMatch
static WORD ut_expected_entry_attributes;
static WORD ut_expected_search_attributes;
static BOOL ut_match_result;

BOOL utm_ODDirAttributesMatch(WORD entry_attributes, WORD search_attributes)
{
   UT_ASSERT_EQ_UINT(ut_expected_entry_attributes, entry_attributes);
   UT_ASSERT_EQ_UINT(ut_expected_search_attributes, search_attributes);
   return(ut_match_result);
}

static void maps_no_windows_attributes(void)
{
   tODDirInfo directory;

   memset(&directory, 0, sizeof(directory));
   directory.wAttributes = DIR_ATTRIB_HIDDEN;
   ut_expected_entry_attributes = DIR_ATTRIB_NORMAL;
   ut_expected_search_attributes = DIR_ATTRIB_HIDDEN;
   ut_match_result = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODDirWinMatchesAttributes(&directory));
}

static void maps_all_windows_attributes(void)
{
   tODDirInfo directory;

   memset(&directory, 0, sizeof(directory));
   directory.WindowsDirEntry.dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE
      | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY
      | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY;
   directory.wAttributes = DIR_ATTRIB_HIDDEN | DIR_ATTRIB_SYSTEM
      | DIR_ATTRIB_DIREC;
   ut_expected_entry_attributes = DIR_ATTRIB_NORMAL | DIR_ATTRIB_ARCH
      | DIR_ATTRIB_HIDDEN | DIR_ATTRIB_RDONLY | DIR_ATTRIB_SYSTEM
      | DIR_ATTRIB_DIREC;
   ut_expected_search_attributes = directory.wAttributes;
   ut_match_result = TRUE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODDirWinMatchesAttributes(&directory));
}

static const UTTestCase ut_cases[] = {
   {"no attributes", maps_no_windows_attributes},
   {"all attributes", maps_all_windows_attributes}
};
