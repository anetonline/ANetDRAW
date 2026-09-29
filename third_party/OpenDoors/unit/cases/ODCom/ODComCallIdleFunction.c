static tPortInfo ut_port;
static unsigned ut_callback_calls;

static void ODCALL ut_idle(void)
{
   ++ut_callback_calls;
}

static void handles_absent_and_installed_callbacks(void)
{
   memset(&ut_port, 0, sizeof(ut_port));
   ut_callback_calls = 0;

   utt_ODComCallIdleFunction(&ut_port);
   UT_ASSERT_EQ_UINT(0, ut_callback_calls);

   ut_port.pfIdleCallback = ut_idle;
   utt_ODComCallIdleFunction(&ut_port);
   UT_ASSERT_EQ_UINT(1, ut_callback_calls);
}

static const UTTestCase ut_cases[] = {
   {"optional callback", handles_absent_and_installed_callbacks}
};
