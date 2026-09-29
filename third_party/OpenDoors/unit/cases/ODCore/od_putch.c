#define UT_CUSTOM_MOCK_ODCoreSendRemoteByte
#define UT_CUSTOM_MOCK_ODScrnDisplayChar
#define UT_CUSTOM_MOCK_ODSessionScreenAvailable
#define UT_CUSTOM_MOCK_ODSessionScreenDisplayChar
#define UT_CUSTOM_MOCK_ODSessionScreenPresent
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_ODTimerElapsed
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_kernel

static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static BOOL ut_session_available;
static unsigned ut_session_calls;
static unsigned ut_present_calls;
static unsigned ut_local_calls;
static unsigned ut_remote_calls;
static unsigned char ut_seen_character;
static BOOL ut_timer_elapsed;
static unsigned ut_timer_calls;
static unsigned ut_kernel_calls;
static BOOL ut_init_succeeds;

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }
BOOL utm_ODSessionScreenAvailable(void) { return ut_session_available; }

void utm_ODSessionScreenDisplayChar(unsigned char value)
{
   ++ut_session_calls;
   ut_seen_character = value;
}

void utm_ODSessionScreenPresent(void) { ++ut_present_calls; }

void ODCALL utm_ODScrnDisplayChar(unsigned char value)
{
   ++ut_local_calls;
   ut_seen_character = value;
}

tODResult utm_ODCoreSendRemoteByte(BYTE value)
{
   ++ut_remote_calls;
   ut_seen_character = value;
   return kODRCSuccess;
}

BOOL utm_ODTimerElapsed(tODTimer *timer)
{
   UT_ASSERT_EQ_PTR(&RunKernelTimer, timer);
   ++ut_timer_calls;
   return ut_timer_elapsed;
}

void ODCALL utm_od_kernel(void) { ++ut_kernel_calls; }

static void reset_character(void)
{
   bODInitialized = TRUE;
   od_control.baud = 0;
   ut_init_calls = 0;
   ut_entries = 0;
   ut_exits = 0;
   ut_session_available = FALSE;
   ut_session_calls = 0;
   ut_present_calls = 0;
   ut_local_calls = 0;
   ut_remote_calls = 0;
   ut_seen_character = 0;
   ut_timer_elapsed = FALSE;
   ut_timer_calls = 0;
   ut_kernel_calls = 0;
   ut_init_succeeds = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_character(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   utt_od_putch('A');
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void local_mode_uses_the_platform_screen(void)
{
   reset_character();
   bODInitialized = FALSE;
   utt_od_putch('A');
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(1, ut_local_calls);
   UT_ASSERT_EQ_UINT(0, ut_session_calls);
   UT_ASSERT_EQ_UINT(0, ut_present_calls);
   UT_ASSERT_EQ_UINT(0, ut_remote_calls);
   UT_ASSERT_EQ_UINT('A', ut_seen_character);
   UT_ASSERT_EQ_UINT(1, ut_timer_calls);
}

static void remote_mode_updates_the_session_and_remote_connection(void)
{
   reset_character();
   od_control.baud = 9600;
   ut_session_available = TRUE;
   ut_timer_elapsed = TRUE;
   utt_od_putch((char)0xe1);
   UT_ASSERT_EQ_UINT(0, ut_init_calls);
   UT_ASSERT_EQ_UINT(0, ut_local_calls);
   UT_ASSERT_EQ_UINT(1, ut_session_calls);
   UT_ASSERT_EQ_UINT(1, ut_present_calls);
   UT_ASSERT_EQ_UINT(1, ut_remote_calls);
   UT_ASSERT_EQ_UINT(0xe1, ut_seen_character);
   UT_ASSERT_EQ_UINT(1, ut_kernel_calls);
}

static const UTTestCase ut_cases[] = {
   {"local character", local_mode_uses_the_platform_screen},
   {"remote character", remote_mode_updates_the_session_and_remote_connection},
   {"terminal session", terminal_session_is_rejected}
};
