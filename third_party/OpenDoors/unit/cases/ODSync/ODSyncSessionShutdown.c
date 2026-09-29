static void clears_only_an_active_session(void)
{
   bSyncActive = FALSE;
   nAPILevel = 7;
   utt_ODSyncSessionShutdown();
   UT_ASSERT_EQ_UINT(7, nAPILevel);

   bSyncActive = TRUE;
   utt_ODSyncSessionShutdown();
   UT_ASSERT(!bSyncActive);
   UT_ASSERT_EQ_UINT(0, nAPILevel);
}

static const UTTestCase ut_cases[] = {
   {"session shutdown", clears_only_an_active_session}
};
