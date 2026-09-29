#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_ODWaitDiscard
#define UT_CUSTOM_MOCK_ODWaitNoCase
#define UT_CUSTOM_MOCK_od_clear_keybuffer
#define UT_CUSTOM_MOCK_od_disp
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_strlen

#define MOCK_API_ENTRY 100
#define MOCK_API_EXIT 101
#define MOCK_CLEAR 102
#define MOCK_DISCARD 103
#define MOCK_DISPLAY 104
#define MOCK_INIT 105
#define MOCK_WAIT 106

static char ut_wait_results[2];
static unsigned short ut_wait_index;
static BOOL ut_disconnect_on_discard;
static const char *ut_last_display;
static BOOL ut_init_succeeds;

static void reset_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
   bODInitialized = TRUE;
   ut_wait_results[0] = FALSE;
   ut_wait_results[1] = FALSE;
   ut_wait_index = 0;
   ut_disconnect_on_discard = FALSE;
   ut_last_display = NULL;
   ut_init_succeeds = TRUE;
}

void utm_ODSyncAPIEntry(void)
{
   ut_mock_called(MOCK_API_ENTRY);
}

void utm_ODSyncAPIExit(void)
{
   ut_mock_called(MOCK_API_EXIT);
}

static void utm_ODWaitDiscard(int bytes, tODMilliSec time)
{
   UT_ASSERT_EQ_INT(11, bytes);
   UT_ASSERT_EQ_UINT(660, time);
   ut_mock_called(MOCK_DISCARD);
   if(ut_disconnect_on_discard)
      bODInitialized = FALSE;
}

static char utm_ODWaitNoCase(char *text, tODMilliSec time)
{
   UT_ASSERT_NOT_NULL(text);
   UT_ASSERT_EQ_UINT(660, time);
   UT_ASSERT(ut_wait_index < 2);
   ut_mock_called(MOCK_WAIT);
   return ut_wait_results[ut_wait_index++];
}

void ODCALL utm_od_clear_keybuffer(void)
{
   ut_mock_called(MOCK_CLEAR);
}

void ODCALL utm_od_disp(const char *buffer, INT size, BOOL local_echo)
{
   UT_ASSERT_NOT_NULL(buffer);
   UT_ASSERT_EQ_INT((INT)utm_strlen(buffer), size);
   UT_ASSERT_EQ_INT(FALSE, local_echo);
   ut_last_display = buffer;
   ut_mock_called(MOCK_DISPLAY);
}

void ODCALL utm_od_init(void)
{
   ut_mock_called(MOCK_INIT);
   if(ut_init_succeeds) bODInitialized = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_fixture(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   utt_od_autodetect(0);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_API_ENTRY));
}

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0')
      ++length;
   return length;
}

static void local_mode_initializes_and_enables_ansi(void)
{
   reset_fixture();
   bODInitialized = FALSE;

   utt_od_autodetect(0);

   UT_ASSERT_EQ_UINT(3, ut_mock_call_count);
   UT_ASSERT_EQ_UINT(MOCK_INIT, ut_mock_calls[0]);
   UT_ASSERT_EQ_UINT(MOCK_API_ENTRY, ut_mock_calls[1]);
   UT_ASSERT_EQ_UINT(MOCK_API_EXIT, ut_mock_calls[2]);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_ansi);
   UT_ASSERT_EQ_INT(FALSE, od_control.user_rip);
}

static void known_remote_capabilities_skip_detection(void)
{
   reset_fixture();
   od_control.baud = 38400;
   od_control.user_ansi = TRUE;
   od_control.user_rip = TRUE;

   utt_od_autodetect(0);

   UT_ASSERT_EQ_UINT(2, ut_mock_call_count);
   UT_ASSERT_EQ_UINT(MOCK_API_ENTRY, ut_mock_calls[0]);
   UT_ASSERT_EQ_UINT(MOCK_API_EXIT, ut_mock_calls[1]);
}

static void successful_detection_enables_both_modes(void)
{
   reset_fixture();
   od_control.baud = 38400;
   ut_wait_results[0] = TRUE;
   ut_wait_results[1] = TRUE;

   utt_od_autodetect(0);

   UT_ASSERT_EQ_INT(TRUE, od_control.user_ansi);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_rip);
   UT_ASSERT_EQ_UINT(2, ut_wait_index);
   UT_ASSERT_EQ_UINT(4, ut_mock_count(MOCK_CLEAR));
   UT_ASSERT_EQ_UINT(2, ut_mock_count(MOCK_DISPLAY));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_DISCARD));
   UT_ASSERT(ut_last_display[1] == 0x1b);
   UT_ASSERT_EQ_UINT(MOCK_API_EXIT,
      ut_mock_calls[ut_mock_call_count - 1]);
}

static void failed_detection_leaves_modes_disabled(void)
{
   reset_fixture();
   od_control.baud = 38400;

   utt_od_autodetect(0);

   UT_ASSERT_EQ_INT(FALSE, od_control.user_ansi);
   UT_ASSERT_EQ_INT(FALSE, od_control.user_rip);
   UT_ASSERT_EQ_UINT(2, ut_wait_index);
   UT_ASSERT_EQ_UINT(4, ut_mock_count(MOCK_CLEAR));
   UT_ASSERT_EQ_UINT(2, ut_mock_count(MOCK_DISPLAY));
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_DISCARD));
}

static void disconnect_during_rip_discard_exits_without_final_clear(void)
{
   reset_fixture();
   od_control.baud = 38400;
   od_control.user_ansi = TRUE;
   ut_wait_results[0] = TRUE;
   ut_disconnect_on_discard = TRUE;

   utt_od_autodetect(0);

   UT_ASSERT_EQ_INT(TRUE, od_control.user_rip);
   UT_ASSERT_EQ_UINT(1, ut_wait_index);
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_CLEAR));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_DISCARD));
   UT_ASSERT_EQ_UINT(MOCK_API_EXIT,
      ut_mock_calls[ut_mock_call_count - 1]);
}

static const UTTestCase ut_cases[] = {
   {"local mode initializes and enables ANSI",
      local_mode_initializes_and_enables_ansi},
   {"known remote capabilities skip detection",
      known_remote_capabilities_skip_detection},
   {"successful detection enables both modes",
      successful_detection_enables_both_modes},
   {"failed detection leaves modes disabled",
      failed_detection_leaves_modes_disabled},
   {"disconnect during RIP discard exits early",
      disconnect_during_rip_discard_exits_without_final_clear},
   {"terminal session", terminal_session_is_rejected}
};
