#define UT_CUSTOM_MOCK_od_log_open
#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed

static unsigned ut_open_calls;
static BOOL ut_public_call_allowed = TRUE;

BOOL utm_ODSyncPublicCallAllowed(void)
{
   return(ut_public_call_allowed);
}

BOOL ODCALL utm_od_log_open(void)
{
   ++ut_open_calls;
   return FALSE;
}

static void enables_by_opening_log(void)
{
   ut_open_calls = 0;
   utt_ODLogEnable();
   UT_ASSERT_EQ_UINT(1, ut_open_calls);
}

static void rejects_a_terminal_session(void)
{
   ut_open_calls = 0;
   ut_public_call_allowed = FALSE;
   utt_ODLogEnable();
   UT_ASSERT_EQ_UINT(0, ut_open_calls);
   ut_public_call_allowed = TRUE;
}

static const UTTestCase ut_cases[] = {
   {"opens log", enables_by_opening_log},
   {"terminal session", rejects_a_terminal_session}
};
