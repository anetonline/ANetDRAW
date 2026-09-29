#define UT_CUSTOM_MOCK_ODSendModemCommandOnce
static BOOL ut_results[3];
static unsigned ut_calls;
BOOL utm_ODSendModemCommandOnce(char *command)
{
   UT_ASSERT(strcmp(command, "ATZ") == 0); UT_ASSERT(ut_calls < 3);
   return(ut_results[ut_calls++]);
}

static void succeeds_on_any_attempt_or_exhausts_retries(void)
{
   ut_calls = 0; ut_results[0] = TRUE;
   UT_ASSERT(utt_ODSendModemCommand("ATZ", 3)); UT_ASSERT_EQ_UINT(1, ut_calls);
   ut_calls = 0; ut_results[0] = FALSE; ut_results[1] = TRUE;
   UT_ASSERT(utt_ODSendModemCommand("ATZ", 3)); UT_ASSERT_EQ_UINT(2, ut_calls);
   ut_calls = 0; ut_results[0] = ut_results[1] = ut_results[2] = FALSE;
   UT_ASSERT(!utt_ODSendModemCommand("ATZ", 3)); UT_ASSERT_EQ_UINT(3, ut_calls);
}

static const UTTestCase ut_cases[] = {
   {"retries", succeeds_on_any_attempt_or_exhausts_retries}
};
