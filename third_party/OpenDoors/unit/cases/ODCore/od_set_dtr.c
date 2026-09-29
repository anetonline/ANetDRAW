#define UT_CUSTOM_MOCK_ODComSetDTR
#define UT_CUSTOM_MOCK_ODCoreSetDTRResult
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_init

static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static unsigned ut_set_calls;
static BOOL ut_seen_high;
static tODResult ut_com_result;
static unsigned ut_result_calls;
static tODResult ut_seen_result;
static BOOL ut_init_succeeds;

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

tODResult utm_ODComSetDTR(tPortHandle port, BOOL high)
{
   UT_ASSERT_EQ_PTR(hSerialPort, port);
   ++ut_set_calls;
   ut_seen_high = high;
   return ut_com_result;
}

void utm_ODCoreSetDTRResult(tODResult result)
{
   ++ut_result_calls;
   ut_seen_result = result;
}

static void reset_dtr(void)
{
   bODInitialized = TRUE;
   od_control.baud = 9600;
   od_control.od_error = 0;
   hSerialPort = (tPortHandle)1;
   ut_init_calls = 0;
   ut_entries = 0;
   ut_exits = 0;
   ut_set_calls = 0;
   ut_seen_high = FALSE;
   ut_com_result = kODRCSuccess;
   ut_result_calls = 0;
   ut_seen_result = kODRCGeneralFailure;
   ut_init_succeeds = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_dtr(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   utt_od_set_dtr(TRUE);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void local_mode_rejects_the_request_after_initialization(void)
{
   reset_dtr();
   bODInitialized = FALSE;
   od_control.baud = 0;
   utt_od_set_dtr(TRUE);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_INT(ERR_NOREMOTE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_set_calls);
   UT_ASSERT_EQ_UINT(0, ut_result_calls);
}

static void remote_mode_passes_the_level_and_result(void)
{
   reset_dtr();
   ut_com_result = kODRCTimeout;
   utt_od_set_dtr(FALSE);
   UT_ASSERT_EQ_UINT(0, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(1, ut_set_calls);
   UT_ASSERT_EQ_INT(FALSE, ut_seen_high);
   UT_ASSERT_EQ_UINT(1, ut_result_calls);
   UT_ASSERT_EQ_INT(kODRCTimeout, ut_seen_result);

   reset_dtr();
   utt_od_set_dtr(TRUE);
   UT_ASSERT_EQ_INT(TRUE, ut_seen_high);
   UT_ASSERT_EQ_INT(kODRCSuccess, ut_seen_result);
}

static const UTTestCase ut_cases[] = {
   {"local DTR", local_mode_rejects_the_request_after_initialization},
   {"remote DTR", remote_mode_passes_the_level_and_result},
   {"terminal session", terminal_session_is_rejected}
};
