#ifdef __WATCOMC__
#define UT_CUSTOM_MOCK__dos_findnext
static unsigned ut_findnext_result;
static tDOSDirEntry *ut_expected_block;

unsigned utm__dos_findnext(struct find_t *block)
{
   UT_ASSERT_EQ_PTR(ut_expected_block, block);
   return(ut_findnext_result);
}

static void maps_dos_results(void)
{
   tDOSDirEntry block;

   ut_expected_block = &block;
   ut_findnext_result = 0;
   UT_ASSERT_EQ_INT(0, utt_ODDirDOSFindNext(&block));
   ut_findnext_result = 18;
   UT_ASSERT_EQ_INT(-1, utt_ODDirDOSFindNext(&block));
}
#else
static void uses_the_dos_find_next_service(void)
{
   FILE *file;
   tDOSDirEntry block;

   file = fopen("UTFN1.TMP", "wb");
   UT_ASSERT_NOT_NULL(file);
   if(file != NULL)
      fclose(file);
   file = fopen("UTFN2.TMP", "wb");
   UT_ASSERT_NOT_NULL(file);
   if(file != NULL)
      fclose(file);
   UT_ASSERT_EQ_INT(0, findfirst("UTFN?.TMP", (struct ffblk *)&block, 0));
   UT_ASSERT_EQ_INT(0, utt_ODDirDOSFindNext(&block));
   UT_ASSERT_EQ_INT(-1, utt_ODDirDOSFindNext(&block));
   remove("UTFN1.TMP");
   remove("UTFN2.TMP");
}
#endif

static const UTTestCase ut_cases[] = {
#ifdef __WATCOMC__
   {"DOS results", maps_dos_results}
#else
   {"DOS find next", uses_the_dos_find_next_service}
#endif
};
