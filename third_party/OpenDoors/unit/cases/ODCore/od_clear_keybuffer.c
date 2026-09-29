#define UT_CUSTOM_MOCK_ODComClearInbound
#define UT_CUSTOM_MOCK_ODInQueueEmpty
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_kernel

static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static unsigned ut_empty_calls;
static unsigned ut_clear_calls;
static unsigned ut_kernel_calls;
static BOOL ut_init_succeeds;

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

void utm_ODInQueueEmpty(tODInQueueHandle queue)
{
   UT_ASSERT_EQ_PTR(hODInputQueue, queue);
   ++ut_empty_calls;
}

tODResult utm_ODComClearInbound(tPortHandle port)
{
   UT_ASSERT_EQ_PTR(hSerialPort, port);
   ++ut_clear_calls;
   return kODRCSuccess;
}

void ODCALL utm_od_kernel(void) { ++ut_kernel_calls; }

static void reset_clear(void)
{
   bODInitialized = TRUE;
   od_control.baud = 0;
   hODInputQueue = (tODInQueueHandle)1;
   hSerialPort = (tPortHandle)2;
   ut_init_calls = 0;
   ut_entries = 0;
   ut_exits = 0;
   ut_empty_calls = 0;
   ut_clear_calls = 0;
   ut_kernel_calls = 0;
   ut_init_succeeds = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_clear(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   utt_od_clear_keybuffer();
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void local_mode_empties_only_the_common_input_queue(void)
{
   reset_clear();
   bODInitialized = FALSE;
   utt_od_clear_keybuffer();
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(1, ut_empty_calls);
   UT_ASSERT_EQ_UINT(0, ut_clear_calls);
   UT_ASSERT_EQ_UINT(1, ut_kernel_calls);
}

static void remote_mode_also_purges_the_communications_input(void)
{
   reset_clear();
   od_control.baud = 9600;
   utt_od_clear_keybuffer();
   UT_ASSERT_EQ_UINT(0, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_empty_calls);
   UT_ASSERT_EQ_UINT(1, ut_clear_calls);
}

static const UTTestCase ut_cases[] = {
   {"local purge", local_mode_empties_only_the_common_input_queue},
   {"remote purge", remote_mode_also_purges_the_communications_input},
   {"terminal session", terminal_session_is_rejected}
};
