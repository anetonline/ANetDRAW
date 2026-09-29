#define UT_CUSTOM_MOCK_ODCoreSendRemoteByte
#define UT_CUSTOM_MOCK_ODAPISleep
#define UT_CUSTOM_MOCK_ODWaitForString
#ifdef OD_DIAGNOSTICS
#define UT_CUSTOM_MOCK_strcat
char *utm_strcat(char *destination, const char *source)
{
   size_t offset = strlen(destination);
   memcpy(destination + offset, source, strlen(source) + 1);
   return(destination);
}
#endif
static BYTE ut_sent[16];
static unsigned ut_sent_count;
static tODMilliSec ut_sleeps[16];
static unsigned ut_sleep_count;
static BOOL ut_wait_result;
static char ut_expected_response[64];
tODResult utm_ODCoreSendRemoteByte(BYTE value)
{
   UT_ASSERT(ut_sent_count < sizeof(ut_sent)); ut_sent[ut_sent_count++] = value;
   return(kODRCSuccess);
}
void utm_ODAPISleep(tODMilliSec milliseconds)
{
   UT_ASSERT(ut_sleep_count < sizeof(ut_sleeps) / sizeof(ut_sleeps[0]));
   ut_sleeps[ut_sleep_count++] = milliseconds;
}
BOOL utm_ODWaitForString(char *response, tODMilliSec timeout)
{
   UT_ASSERT(strcmp(response, ut_expected_response) == 0);
   UT_ASSERT(timeout == RESPONSE_TIMEOUT); return(ut_wait_result);
}

static void reset_once(void)
{
   memset(ut_sent, 0, sizeof(ut_sent)); memset(ut_sleeps, 0, sizeof(ut_sleeps));
   ut_sent_count = ut_sleep_count = 0; ut_wait_result = TRUE;
   ut_expected_response[0] = 0;
#ifdef OD_DIAGNOSTICS
   szDebugWorkString[0] = 0;
#endif
}

static void sends_literals_carriage_returns_and_pauses_then_waits(void)
{
   reset_once(); strcpy(ut_expected_response, "OK");
   UT_ASSERT(utt_ODSendModemCommandOnce("AT|~ OK "));
   UT_ASSERT_EQ_UINT(3, ut_sent_count); UT_ASSERT_EQ_INT('A', ut_sent[0]);
   UT_ASSERT_EQ_INT('T', ut_sent[1]); UT_ASSERT_EQ_INT('\r', ut_sent[2]);
   UT_ASSERT(ut_sleep_count >= 5);
}

static void ignores_response_pauses_and_reports_failed_wait(void)
{
   reset_once(); strcpy(ut_expected_response, "OK"); ut_wait_result = FALSE;
   UT_ASSERT(!utt_ODSendModemCommandOnce("A ~OK "));
   UT_ASSERT_EQ_UINT(1, ut_sent_count);
}

static void truncates_an_overlong_response(void)
{
   char command[MAX_RESPONSE_LEN + 4]; unsigned index;
   reset_once(); command[0] = 'A'; command[1] = ' ';
   for(index = 0; index < MAX_RESPONSE_LEN + 1; ++index)
      command[index + 2] = 'X';
   command[MAX_RESPONSE_LEN + 3] = 0;
   UT_ASSERT(utt_ODSendModemCommandOnce(command));
}

static const UTTestCase ut_cases[] = {
   {"command sequence", sends_literals_carriage_returns_and_pauses_then_waits},
   {"failed response", ignores_response_pauses_and_reports_failed_wait},
   {"response bound", truncates_an_overlong_response}
};
