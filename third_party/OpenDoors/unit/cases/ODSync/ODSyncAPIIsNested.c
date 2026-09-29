static void reports_api_nesting_on_every_platform(void)
{
   bSyncActive = FALSE;
   nAPILevel = 1;
   UT_ASSERT(!utt_ODSyncAPIIsNested());
   bSyncActive = TRUE;
   UT_ASSERT(utt_ODSyncAPIIsNested());
   nAPILevel = 0;
   UT_ASSERT(!utt_ODSyncAPIIsNested());
   nAPILevel = 1;
   UT_ASSERT(utt_ODSyncAPIIsNested());
}

static const UTTestCase ut_cases[] = {
   {"API nesting", reports_api_nesting_on_every_platform}
};
