static void reports_only_a_threaded_active_api_level(void)
{
#ifdef OD_THREAD_SUPPORT
   bSyncActive = FALSE;
   nAPILevel = 1;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSyncAPILevelActive());

   bSyncActive = TRUE;
   nAPILevel = 0;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSyncAPILevelActive());
   nAPILevel = 1;
   UT_ASSERT_EQ_INT(TRUE, utt_ODSyncAPILevelActive());
#else
   UT_ASSERT_EQ_INT(FALSE, utt_ODSyncAPILevelActive());
#endif
}

static const UTTestCase ut_cases[] = {
   {"threaded API activity", reports_only_a_threaded_active_api_level}
};
