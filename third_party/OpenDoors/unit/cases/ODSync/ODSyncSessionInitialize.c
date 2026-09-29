static void initializes_and_reuses_session_state(void)
{
   bSyncActive = FALSE;
   nAPILevel = 7;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODSyncSessionInitialize());
   UT_ASSERT(bSyncActive);
   UT_ASSERT_EQ_UINT(0, nAPILevel);

   nAPILevel = 1;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODSyncSessionInitialize());
   UT_ASSERT_EQ_UINT(1, nAPILevel);
}

static const UTTestCase ut_cases[] = {
   {"session state", initializes_and_reuses_session_state}
};
