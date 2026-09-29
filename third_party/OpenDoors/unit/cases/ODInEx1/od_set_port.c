#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed
static BOOL ut_call_allowed;
BOOL utm_ODSyncPublicCallAllowed(void)
{
   if(!ut_call_allowed) od_control.od_error = ERR_GENERALFAILURE;
   return(ut_call_allowed);
}

static void reset_port_state(void)
{
   memset(&od_control, 0, sizeof(od_control));
   bODInitialized = FALSE;
   nForcedPort = -1;
   ut_call_allowed = TRUE;
}

static void rejects_terminal_calls(void)
{
   reset_port_state();
   ut_call_allowed = FALSE;
   UT_ASSERT(!utt_od_set_port(1));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_INT(-1, nForcedPort);
}

static void rejects_calls_after_initialization(void)
{
   reset_port_state();
   bODInitialized = TRUE;
   od_control.port = 7;
   UT_ASSERT(!utt_od_set_port(0));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_INT(7, od_control.port);
   UT_ASSERT_EQ_INT(-1, nForcedPort);
}

static void rejects_ports_outside_the_byte_range(void)
{
   reset_port_state();
   UT_ASSERT(!utt_od_set_port(-1));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_INT(-1, nForcedPort);

   reset_port_state();
   UT_ASSERT(!utt_od_set_port(256));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_INT(-1, nForcedPort);
}

static void records_explicit_com1_and_the_largest_port(void)
{
   reset_port_state();
   UT_ASSERT(utt_od_set_port(0));
   UT_ASSERT_EQ_INT(0, od_control.port);
   UT_ASSERT_EQ_INT(0, nForcedPort);

   reset_port_state();
   UT_ASSERT(utt_od_set_port(255));
   UT_ASSERT_EQ_INT(255, od_control.port);
   UT_ASSERT_EQ_INT(255, nForcedPort);
}

static const UTTestCase ut_cases[] = {
   {"terminal session", rejects_terminal_calls},
   {"already initialized", rejects_calls_after_initialization},
   {"invalid range", rejects_ports_outside_the_byte_range},
   {"explicit ports", records_explicit_com1_and_the_largest_port}
};
