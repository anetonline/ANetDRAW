#define UT_CUSTOM_MOCK_od_set_attrib
#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed

static unsigned ut_calls;
static INT ut_attribute;
static BOOL ut_public_call_allowed = TRUE;

BOOL utm_ODSyncPublicCallAllowed(void)
{
   return(ut_public_call_allowed);
}

void ODCALL utm_od_set_attrib(INT attribute)
{
   ++ut_calls;
   ut_attribute = attribute;
}

static void combines_the_pc_foreground_and_background_nibbles(void)
{
   ut_calls = 0;
   ut_attribute = 0;
   utt_od_set_color(5, 2);
   UT_ASSERT_EQ_UINT(1, ut_calls);
   UT_ASSERT_EQ_INT(0x25, ut_attribute);
}

static void rejects_a_terminal_session(void)
{
   ut_calls = 0;
   ut_public_call_allowed = FALSE;
   utt_od_set_color(5, 2);
   UT_ASSERT_EQ_UINT(0, ut_calls);
   ut_public_call_allowed = TRUE;
}

static const UTTestCase ut_cases[] = {
   {"combined attribute", combines_the_pc_foreground_and_background_nibbles},
   {"terminal session", rejects_a_terminal_session}
};
