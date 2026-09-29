#define UT_CUSTOM_MOCK_od_get_input
#define UT_CUSTOM_MOCK_od_kernel
#define UT_CUSTOM_MOCK_toupper

static tODInputEvent ut_events[16];
static unsigned ut_event_count;
static unsigned ut_event_index;
static tODMilliSec ut_timeouts[20];
static WORD ut_flags[20];
static unsigned ut_input_calls;
static unsigned ut_kernel_calls;
static tMenuItem ut_items[3];

BOOL ODCALL utm_od_get_input(tODInputEvent *event, tODMilliSec timeout,
   WORD flags)
{
   UT_ASSERT(ut_input_calls < sizeof(ut_timeouts) / sizeof(ut_timeouts[0]));
   ut_timeouts[ut_input_calls] = timeout;
   ut_flags[ut_input_calls] = flags;
   ++ut_input_calls;
   if(ut_event_index == ut_event_count)
      return FALSE;
   *event = ut_events[ut_event_index++];
   return TRUE;
}

void ODCALL utm_od_kernel(void)
{
   ++ut_kernel_calls;
}

int utm_toupper(int value)
{
   UT_ASSERT_EQ_INT((int)(unsigned char)value, value);
   if(value >= 'a' && value <= 'z')
      return value - ('a' - 'A');
   return value;
}

static void reset_input(void)
{
   memset(ut_events, 0, sizeof(ut_events));
   memset(ut_timeouts, 0, sizeof(ut_timeouts));
   memset(ut_flags, 0, sizeof(ut_flags));
   memset(ut_items, 0, sizeof(ut_items));
   strcpy(ut_items[0].szItemText, "Alpha");
   strcpy(ut_items[1].szItemText, "Bravo");
   strcpy(ut_items[2].szItemText, "Charlie");
   ut_items[0].btKeyIndex = 0;
   ut_items[1].btKeyIndex = 0;
   ut_items[2].btKeyIndex = 0;
   MenuLevelInfo[0].paMenuItems = ut_items;
   nCurrentLevel = 0;
   btCurrentNumMenuItems = 3;
   btCorrectItem = 0;
   nCommand = NO_COMMAND;
   wCurrentFlags = 0;
   bODInitialized = TRUE;
   ut_event_count = 0;
   ut_event_index = 0;
   ut_input_calls = 0;
   ut_kernel_calls = 0;
}

static void add_event(tODInputEventType type, char key)
{
   UT_ASSERT(ut_event_count < sizeof(ut_events) / sizeof(ut_events[0]));
   ut_events[ut_event_count].EventType = type;
   ut_events[ut_event_count].chKeyPress = key;
   ++ut_event_count;
}

static void waits_only_for_the_first_input_attempt(void)
{
   reset_input();
   add_event(EVENT_EXTENDED_KEY, OD_KEY_LEFT);
   utt_ODPopupCheckForKey(TRUE);
   UT_ASSERT_EQ_UINT(2, ut_input_calls);
   UT_ASSERT_EQ_UINT(OD_NO_TIMEOUT, ut_timeouts[0]);
   UT_ASSERT_EQ_UINT(0, ut_timeouts[1]);
   UT_ASSERT_EQ_UINT(GETIN_NORMAL, ut_flags[0]);
   UT_ASSERT_EQ_INT(NO_COMMAND, nCommand);

   reset_input();
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_UINT(1, ut_input_calls);
   UT_ASSERT_EQ_UINT(0, ut_timeouts[0]);
}

static void reports_a_session_which_ends_without_input(void)
{
   reset_input();
   bODInitialized = FALSE;
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(POPUP_ESCAPE, nCommand);
}

static void ignores_calls_after_a_command_is_already_selected(void)
{
   reset_input();
   nCommand = 7;
   utt_ODPopupCheckForKey(TRUE);
   UT_ASSERT_EQ_UINT(0, ut_input_calls);
   UT_ASSERT_EQ_UINT(0, ut_kernel_calls);
}

static void moves_up_and_down_with_wraparound(void)
{
   reset_input();
   add_event(EVENT_EXTENDED_KEY, OD_KEY_UP);
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(2, btCorrectItem);

   reset_input();
   btCorrectItem = 2;
   add_event(EVENT_EXTENDED_KEY, OD_KEY_UP);
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(1, btCorrectItem);

   reset_input();
   add_event(EVENT_EXTENDED_KEY, OD_KEY_DOWN);
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(1, btCorrectItem);

   reset_input();
   btCorrectItem = 2;
   add_event(EVENT_EXTENDED_KEY, OD_KEY_DOWN);
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(0, btCorrectItem);
}

static void handles_horizontal_arrows_only_for_pulldown_menus(void)
{
   reset_input();
   add_event(EVENT_EXTENDED_KEY, OD_KEY_LEFT);
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(NO_COMMAND, nCommand);

   reset_input();
   wCurrentFlags = MENU_PULLDOWN;
   add_event(EVENT_EXTENDED_KEY, OD_KEY_LEFT);
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(POPUP_LEFT, nCommand);

   reset_input();
   add_event(EVENT_EXTENDED_KEY, OD_KEY_RIGHT);
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(NO_COMMAND, nCommand);

   reset_input();
   wCurrentFlags = MENU_PULLDOWN;
   add_event(EVENT_EXTENDED_KEY, OD_KEY_RIGHT);
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(POPUP_RIGHT, nCommand);

   reset_input();
   add_event(EVENT_EXTENDED_KEY, OD_KEY_F1);
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(NO_COMMAND, nCommand);
}

static void ignores_unknown_event_types(void)
{
   reset_input();
   add_event((tODInputEventType)99, 'x');
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(NO_COMMAND, nCommand);
}

static void accepts_both_enter_characters(void)
{
   reset_input();
   btCorrectItem = 1;
   add_event(EVENT_CHARACTER, '\n');
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(2, nCommand);

   reset_input();
   add_event(EVENT_CHARACTER, '\r');
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(1, nCommand);
}

static void allows_escape_only_when_configured(void)
{
   reset_input();
   add_event(EVENT_CHARACTER, 27);
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(NO_COMMAND, nCommand);

   reset_input();
   wCurrentFlags = MENU_ALLOW_CANCEL;
   add_event(EVENT_CHARACTER, 27);
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(POPUP_ESCAPE, nCommand);
}

static void selects_hotkeys_case_insensitively_at_any_position(void)
{
   reset_input();
   add_event(EVENT_CHARACTER, 'a');
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(1, nCommand);

   reset_input();
   add_event(EVENT_CHARACTER, 'C');
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(3, nCommand);

   reset_input();
   add_event(EVENT_CHARACTER, 'x');
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(NO_COMMAND, nCommand);
}

static void selects_a_high_bit_hotkey_without_invalid_ctype_input(void)
{
   reset_input();
   ut_items[0].szItemText[0] = (char)0x80;
   add_event(EVENT_CHARACTER, (char)0x80);
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(1, nCommand);
}

static void maps_numeric_navigation_aliases(void)
{
   reset_input();
   wCurrentFlags = MENU_PULLDOWN;
   add_event(EVENT_CHARACTER, '4');
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(POPUP_LEFT, nCommand);

   reset_input();
   wCurrentFlags = MENU_PULLDOWN;
   add_event(EVENT_CHARACTER, '6');
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(POPUP_RIGHT, nCommand);

   reset_input();
   add_event(EVENT_CHARACTER, '8');
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(2, btCorrectItem);

   reset_input();
   add_event(EVENT_CHARACTER, '2');
   utt_ODPopupCheckForKey(FALSE);
   UT_ASSERT_EQ_INT(1, btCorrectItem);
}

static const UTTestCase ut_cases[] = {
   {"input timeout", waits_only_for_the_first_input_attempt},
   {"ended session", reports_a_session_which_ends_without_input},
   {"existing command", ignores_calls_after_a_command_is_already_selected},
   {"vertical arrows", moves_up_and_down_with_wraparound},
   {"horizontal arrows", handles_horizontal_arrows_only_for_pulldown_menus},
   {"unknown event", ignores_unknown_event_types},
   {"enter keys", accepts_both_enter_characters},
   {"escape", allows_escape_only_when_configured},
   {"hotkeys", selects_hotkeys_case_insensitively_at_any_position},
   {"high-bit hotkey", selects_a_high_bit_hotkey_without_invalid_ctype_input},
   {"numeric aliases", maps_numeric_navigation_aliases}
};
