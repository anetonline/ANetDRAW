#ifdef __WATCOMC__
#define UT_CUSTOM_MOCK__dos_findfirst
static unsigned ut_findfirst_result;
static CONST char *ut_expected_path;
static unsigned ut_expected_attributes;
static tDOSDirEntry *ut_expected_block;

unsigned utm__dos_findfirst(CONST char *path, unsigned attributes,
   struct find_t *block)
{
   UT_ASSERT(strcmp(ut_expected_path, path) == 0);
   UT_ASSERT_EQ_UINT(ut_expected_attributes, attributes);
   UT_ASSERT_EQ_PTR(ut_expected_block, block);
   return(ut_findfirst_result);
}

static void maps_dos_results(void)
{
   tDOSDirEntry block;

   ut_expected_path = "*.DAT";
   ut_expected_attributes = DIR_ATTRIB_HIDDEN | DIR_ATTRIB_SYSTEM;
   ut_expected_block = &block;
   ut_findfirst_result = 0;
   UT_ASSERT_EQ_INT(0, utt_ODDirDOSFindFirst(ut_expected_path, &block,
      (WORD)ut_expected_attributes));
   ut_findfirst_result = 2;
   UT_ASSERT_EQ_INT(-1, utt_ODDirDOSFindFirst(ut_expected_path, &block,
      (WORD)ut_expected_attributes));
}
#else
static void uses_the_dos_find_first_service(void)
{
   FILE *file;
   tDOSDirEntry block;

   file = fopen("UTFF.TMP", "wb");
   UT_ASSERT_NOT_NULL(file);
   if(file != NULL)
      fclose(file);
   UT_ASSERT_EQ_INT(0, utt_ODDirDOSFindFirst("UTFF.TMP", &block, 0));
   UT_ASSERT(strcmp("UTFF.TMP", block.szFileName) == 0);
   UT_ASSERT_EQ_INT(-1, utt_ODDirDOSFindFirst("UTNONE.XXX", &block, 0));
   remove("UTFF.TMP");
}
#endif

static const UTTestCase ut_cases[] = {
#ifdef __WATCOMC__
   {"DOS results", maps_dos_results}
#else
   {"DOS find first", uses_the_dos_find_first_service}
#endif
};
