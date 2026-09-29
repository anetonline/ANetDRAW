static void reports_the_current_session_state(void)
{
   bSyncActive = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSyncSessionActive());
   bSyncActive = TRUE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODSyncSessionActive());
}

static const UTTestCase ut_cases[] = {
   {"session state", reports_the_current_session_state}
};
