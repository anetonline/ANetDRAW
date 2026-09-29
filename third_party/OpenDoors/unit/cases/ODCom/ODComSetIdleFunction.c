static tPortInfo ut_port;
static unsigned ut_callback_calls;

static void ODCALL ut_idle_callback(void)
{
   ++ut_callback_calls;
}

static void stores_and_clears_the_callback(void)
{
   memset(&ut_port, 0, sizeof(ut_port));
   ut_callback_calls = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComSetIdleFunction(
      ODPTR2HANDLE(&ut_port, tPortInfo), ut_idle_callback));
   UT_ASSERT_EQ_PTR(ut_idle_callback, ut_port.pfIdleCallback);
   (*ut_port.pfIdleCallback)();
   UT_ASSERT_EQ_UINT(1, ut_callback_calls);
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComSetIdleFunction(
      ODPTR2HANDLE(&ut_port, tPortInfo), NULL));
   UT_ASSERT_NULL(ut_port.pfIdleCallback);
}

static const UTTestCase ut_cases[] = {
   {"callback", stores_and_clears_the_callback}
};
