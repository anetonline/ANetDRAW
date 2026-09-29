#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_get_answer
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_send_file

static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static unsigned ut_send_calls;
static unsigned ut_answer_calls;
static BOOL ut_send_result;
static char ut_send_hotkey;
static char ut_answer;
static const char *ut_expected_keys;
static BOOL ut_init_succeeds;

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

BOOL ODCALL utm_od_send_file(const char *name)
{
   ++ut_send_calls;
   UT_ASSERT(strcmp("menu", name) == 0);
   UT_ASSERT_EQ_PTR(ut_expected_keys, pszCurrentHotkeys);
   chHotkeyPressed = ut_send_hotkey;
   return ut_send_result;
}

char ODCALL utm_od_get_answer(const char *keys)
{
   ++ut_answer_calls;
   UT_ASSERT_EQ_PTR(ut_expected_keys, keys);
   return ut_answer;
}

static void reset_menu(void)
{
   memset(&od_control, 0, sizeof(od_control));
   bODInitialized = TRUE;
   pszCurrentHotkeys = NULL;
   chHotkeyPressed = '\0';
   ut_init_calls = ut_entries = ut_exits = 0;
   ut_send_calls = ut_answer_calls = 0;
   ut_send_result = TRUE;
   ut_send_hotkey = '\0';
   ut_answer = 'B';
   ut_expected_keys = "AB";
   ut_init_succeeds = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_menu(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT_EQ_INT(0, utt_od_hotkey_menu("menu", NULL, FALSE));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void rejects_a_null_hotkey_list_after_initializing(void)
{
   reset_menu();
   bODInitialized = FALSE;
   UT_ASSERT_EQ_INT('\0', utt_od_hotkey_menu("menu", NULL, FALSE));
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_send_calls);
   UT_ASSERT_NULL(pszCurrentHotkeys);
}

static void clears_shared_state_when_display_fails(void)
{
   reset_menu();
   ut_send_result = FALSE;
   UT_ASSERT_EQ_INT('\0',
      utt_od_hotkey_menu("menu", (char *)ut_expected_keys, FALSE));
   UT_ASSERT_EQ_UINT(1, ut_send_calls);
   UT_ASSERT_NULL(pszCurrentHotkeys);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static void returns_a_hotkey_pressed_during_display(void)
{
   reset_menu();
   ut_send_hotkey = 'A';
   UT_ASSERT_EQ_INT('A',
      utt_od_hotkey_menu("menu", (char *)ut_expected_keys, TRUE));
   UT_ASSERT_EQ_UINT(0, ut_answer_calls);
   UT_ASSERT_NULL(pszCurrentHotkeys);
}

static void waits_and_preserves_remote_output_when_connected(void)
{
   reset_menu();
   od_control.baud = 9600;
   UT_ASSERT_EQ_INT('B',
      utt_od_hotkey_menu("menu", (char *)ut_expected_keys, TRUE));
   UT_ASSERT_EQ_UINT(1, ut_answer_calls);
   UT_ASSERT_NULL(pszCurrentHotkeys);
}

static void waits_without_a_remote_connection(void)
{
   reset_menu();
   UT_ASSERT_EQ_INT('B',
      utt_od_hotkey_menu("menu", (char *)ut_expected_keys, TRUE));
   UT_ASSERT_EQ_UINT(1, ut_answer_calls);
}

static void returns_zero_without_waiting(void)
{
   reset_menu();
   UT_ASSERT_EQ_INT(0,
      utt_od_hotkey_menu("menu", (char *)ut_expected_keys, FALSE));
   UT_ASSERT_EQ_UINT(0, ut_answer_calls);
   UT_ASSERT_NULL(pszCurrentHotkeys);
}

static const UTTestCase ut_cases[] = {
   {"null keys", rejects_a_null_hotkey_list_after_initializing},
   {"display failure", clears_shared_state_when_display_fails},
   {"display hotkey", returns_a_hotkey_pressed_during_display},
   {"wait connected", waits_and_preserves_remote_output_when_connected},
   {"wait local", waits_without_a_remote_connection},
   {"no wait", returns_zero_without_waiting},
   {"terminal session", terminal_session_is_rejected}
};
