#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed
static BOOL ut_public_call_allowed = TRUE;

BOOL utm_ODSyncPublicCallAllowed(void)
{
   return(ut_public_call_allowed);
}

static void returns_the_process_control_structure(void)
{
   UT_ASSERT_EQ_PTR(&od_control, utt_od_control_get());
}

static void rejects_a_terminal_session(void)
{
   ut_public_call_allowed = FALSE;
   UT_ASSERT_NULL(utt_od_control_get());
   ut_public_call_allowed = TRUE;
}

static const UTTestCase ut_cases[] = {
   {"control pointer", returns_the_process_control_structure},
   {"terminal session", rejects_a_terminal_session}
};
