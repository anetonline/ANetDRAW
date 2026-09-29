#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed

static BOOL ut_call_allowed;

BOOL utm_ODSyncPublicCallAllowed(void)
{
   return(ut_call_allowed);
}

static void records_a_normalized_boolean_setting(void)
{
   ut_call_allowed = TRUE;
   bUserEightBit = FALSE;
   UT_ASSERT(utt_od_set_user_8bit(7));
   UT_ASSERT_EQ_INT(TRUE, bUserEightBit);

   UT_ASSERT(utt_od_set_user_8bit(FALSE));
   UT_ASSERT_EQ_INT(FALSE, bUserEightBit);
}

static void rejects_a_terminal_session_without_changing_the_setting(void)
{
   ut_call_allowed = FALSE;
   bUserEightBit = TRUE;
   UT_ASSERT(!utt_od_set_user_8bit(FALSE));
   UT_ASSERT_EQ_INT(TRUE, bUserEightBit);
}

static const UTTestCase ut_cases[] = {
   {"record setting", records_a_normalized_boolean_setting},
   {"terminal session", rejects_a_terminal_session_without_changing_the_setting}
};
