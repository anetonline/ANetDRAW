#define UT_CUSTOM_MOCK_ODComCarrier
#define UT_CUSTOM_MOCK_ODCoreCarrierResult
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_init

static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static unsigned ut_carrier_calls;
static tODResult ut_com_result;
static BOOL ut_com_carrier;
static unsigned ut_result_calls;
static tODResult ut_seen_result;
static BOOL ut_seen_carrier;
static BOOL ut_public_result;
static BOOL ut_init_succeeds;

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

tODResult utm_ODComCarrier(tPortHandle port, BOOL *carrier)
{
   UT_ASSERT_EQ_PTR(hSerialPort, port);
   UT_ASSERT(carrier != NULL);
   ++ut_carrier_calls;
   *carrier = ut_com_carrier;
   return ut_com_result;
}

BOOL utm_ODCoreCarrierResult(tODResult result, BOOL carrier)
{
   ++ut_result_calls;
   ut_seen_result = result;
   ut_seen_carrier = carrier;
   return ut_public_result;
}

static void reset_carrier(void)
{
   bODInitialized = TRUE;
   od_control.baud = 9600;
   od_control.od_error = 0;
   hSerialPort = (tPortHandle)1;
   ut_init_calls = 0;
   ut_entries = 0;
   ut_exits = 0;
   ut_carrier_calls = 0;
   ut_com_result = kODRCSuccess;
   ut_com_carrier = TRUE;
   ut_result_calls = 0;
   ut_seen_result = kODRCGeneralFailure;
   ut_seen_carrier = FALSE;
   ut_public_result = TRUE;
   ut_init_succeeds = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_carrier(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT(!utt_od_carrier());
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void local_mode_reports_no_remote_connection(void)
{
   reset_carrier();
   bODInitialized = FALSE;
   od_control.baud = 0;
   UT_ASSERT_EQ_INT(FALSE, utt_od_carrier());
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_INT(ERR_NOREMOTE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_carrier_calls);
   UT_ASSERT_EQ_UINT(0, ut_result_calls);
}

static void remote_mode_maps_the_communications_result(void)
{
   reset_carrier();
   ut_com_result = kODRCTimeout;
   ut_com_carrier = FALSE;
   ut_public_result = TRUE;
   UT_ASSERT_EQ_INT(TRUE, utt_od_carrier());
   UT_ASSERT_EQ_UINT(0, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(1, ut_carrier_calls);
   UT_ASSERT_EQ_UINT(1, ut_result_calls);
   UT_ASSERT_EQ_INT(kODRCTimeout, ut_seen_result);
   UT_ASSERT_EQ_INT(FALSE, ut_seen_carrier);
}

static const UTTestCase ut_cases[] = {
   {"local mode", local_mode_reports_no_remote_connection},
   {"remote result", remote_mode_maps_the_communications_result},
   {"terminal session", terminal_session_is_rejected}
};
