#define UT_CUSTOM_MOCK_od_disp_str
#define UT_CUSTOM_MOCK_ODKrnlForceOpenDoorsShutdown

static unsigned ut_display_calls;
static unsigned ut_callback_calls;
static unsigned ut_force_calls;
static BYTE ut_force_reason;

void ODCALL utm_od_disp_str(const char *message)
{
   ++ut_display_calls;
   UT_ASSERT(strcmp("notice", message) == 0);
}
void utm_ODKrnlForceOpenDoorsShutdown(BYTE reason)
{
   ++ut_force_calls;
   ut_force_reason = reason;
}
#ifdef ODPLAT_DOS32
static void ODCALL ut_message_callback(char *message)
#else
static void ut_message_callback(char *message)
#endif
{
   ++ut_callback_calls;
   UT_ASSERT(strcmp("notice", message) == 0);
}

static void reset_delivery(void)
{
   ut_display_calls = ut_callback_calls = ut_force_calls = 0;
   ut_force_reason = 0;
   bODInitialized = TRUE;
}

static void displays_or_calls_back_immediately(void)
{
   reset_delivery();
   od_control.od_time_msg_func = NULL;
   UT_ASSERT(!utt_ODKrnlDeliverTimeMessage("notice", 0, TRUE));
   UT_ASSERT_EQ_UINT(1, ut_display_calls);

   reset_delivery();
   od_control.od_time_msg_func = ut_message_callback;
   UT_ASSERT(!utt_ODKrnlDeliverTimeMessage("notice", 0, TRUE));
   UT_ASSERT_EQ_UINT(1, ut_callback_calls);
}

static void stops_when_the_callback_exits_or_a_reason_forces_shutdown(void)
{
   reset_delivery();
   od_control.od_time_msg_func = ut_message_callback;
   bODInitialized = FALSE;
   UT_ASSERT(utt_ODKrnlDeliverTimeMessage("notice", 0, TRUE));
   UT_ASSERT_EQ_UINT(0, ut_force_calls);

   reset_delivery();
   od_control.od_time_msg_func = NULL;
   UT_ASSERT(utt_ODKrnlDeliverTimeMessage("notice", 7, TRUE));
   UT_ASSERT_EQ_UINT(1, ut_force_calls);
   UT_ASSERT_EQ_UINT(7, ut_force_reason);
}

static void callback_policy_does_not_defer_application_flow_delivery(void)
{
   reset_delivery();
   od_control.od_time_msg_func = ut_message_callback;
   UT_ASSERT(!utt_ODKrnlDeliverTimeMessage("notice", 0, FALSE));
   UT_ASSERT_EQ_UINT(1, ut_callback_calls);

   reset_delivery();
   od_control.od_time_msg_func = NULL;
   UT_ASSERT(!utt_ODKrnlDeliverTimeMessage("notice", 0, FALSE));
   UT_ASSERT_EQ_UINT(1, ut_display_calls);
}

static const UTTestCase ut_cases[] = {
   {"immediate", displays_or_calls_back_immediately},
   {"stop", stops_when_the_callback_exits_or_a_reason_forces_shutdown},
   {"callback policy", callback_policy_does_not_defer_application_flow_delivery}
};
