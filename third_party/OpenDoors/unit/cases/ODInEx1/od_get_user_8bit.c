#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed

static BOOL ut_call_allowed;

BOOL utm_ODSyncPublicCallAllowed(void)
{
   return(ut_call_allowed);
}

static void returns_the_recorded_setting(void)
{
   ut_call_allowed = TRUE;
   bUserEightBit = FALSE;
   UT_ASSERT(!utt_od_get_user_8bit());

   bUserEightBit = TRUE;
   UT_ASSERT(utt_od_get_user_8bit());
}

static void rejects_a_terminal_session(void)
{
   ut_call_allowed = FALSE;
   bUserEightBit = TRUE;
   UT_ASSERT(!utt_od_get_user_8bit());
}

static const UTTestCase ut_cases[] = {
   {"recorded setting", returns_the_recorded_setting},
   {"terminal session", rejects_a_terminal_session}
};
