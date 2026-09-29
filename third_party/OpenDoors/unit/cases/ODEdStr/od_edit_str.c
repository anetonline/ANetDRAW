#define UT_CUSTOM_MOCK_ODEditAsCharForPos
#define UT_CUSTOM_MOCK_ODEditDisplayPermaliteral
#define UT_CUSTOM_MOCK_ODEditGetWindowSize
#define UT_CUSTOM_MOCK_ODEditIsCharValidForPos
#define UT_CUSTOM_MOCK_ODStatEndArrowUse
#define UT_CUSTOM_MOCK_ODStatStartArrowUse
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_disp_str
#define UT_CUSTOM_MOCK_od_get_input
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_putch
#define UT_CUSTOM_MOCK_od_repeat
#define UT_CUSTOM_MOCK_od_set_attrib
#define UT_CUSTOM_MOCK_od_set_cursor
#define UT_CUSTOM_MOCK_strcpy
#define UT_CUSTOM_MOCK_strlen

#define UT_MAX_EVENTS 64

static tODInputEvent ut_events[UT_MAX_EVENTS];
static BOOL ut_event_results[UT_MAX_EVENTS];
static unsigned ut_event_count;
static unsigned ut_event_index;
static unsigned ut_entries;
static unsigned ut_exits;
static unsigned ut_arrow_starts;
static unsigned ut_arrow_ends;
static INT ut_window_width;
static INT ut_window_height;
static char ut_invalid_character;
static INT ut_invalid_position;
static char ut_convert_character;
static char ut_converted_character;
static INT ut_convert_position;
static unsigned ut_init_calls;
static unsigned ut_permaliteral_calls;
static unsigned ut_cursor_calls;
static unsigned ut_attrib_calls;
static unsigned ut_disp_calls;
static unsigned ut_repeat_calls;
static unsigned ut_putch_calls;
static BOOL ut_init_succeeds;

void ODCALL utm_od_init(void)
{
   if(ut_init_succeeds) bODInitialized = TRUE;
   ++ut_init_calls;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }
void utm_ODStatStartArrowUse(void) { ++ut_arrow_starts; }
void utm_ODStatEndArrowUse(void) { ++ut_arrow_ends; }

void utm_ODEditGetWindowSize(INT *width, INT *height)
{
   *width = ut_window_width;
   *height = ut_window_height;
}

BOOL utm_ODEditIsCharValidForPos(char entered, INT position)
{
   if(entered == ut_invalid_character &&
      (ut_invalid_position < 0 || position == ut_invalid_position))
      return FALSE;
   return TRUE;
}

char utm_ODEditAsCharForPos(char entered, INT position)
{
   if(entered == ut_convert_character &&
      (ut_convert_position < 0 || position == ut_convert_position))
      return ut_converted_character;
   return entered;
}

void utm_ODEditDisplayPermaliteral(WORD flags)
{
   (void)flags;
   ++ut_permaliteral_calls;
}
void ODCALL utm_od_set_attrib(INT attribute)
{
   (void)attribute;
   ++ut_attrib_calls;
}
void ODCALL utm_od_set_cursor(INT row, INT column)
{
   (void)row;
   (void)column;
   ++ut_cursor_calls;
}
void ODCALL utm_od_disp_str(const char *text)
{
   (void)text;
   ++ut_disp_calls;
}
void ODCALL utm_od_repeat(char character, BYTE count)
{
   (void)character;
   (void)count;
   ++ut_repeat_calls;
}
void ODCALL utm_od_putch(char character)
{
   (void)character;
   ++ut_putch_calls;
}

char *utm_strcpy(char *destination, const char *source)
{
   char *result = destination;
   while((*destination++ = *source++) != '\0') { }
   return result;
}

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

BOOL ODCALL utm_od_get_input(tODInputEvent *event, tODMilliSec timeout,
   WORD flags)
{
   unsigned index = ut_event_index++;
   (void)timeout;
   (void)flags;
   UT_ASSERT(index < ut_event_count);
   if(ut_event_results[index]) *event = ut_events[index];
   return ut_event_results[index];
}

static void reset_editor(void)
{
   memset(&od_control, 0, sizeof(od_control));
   memset(ut_events, 0, sizeof(ut_events));
   memset(ut_event_results, 0, sizeof(ut_event_results));
   bODInitialized = TRUE;
   ut_event_count = 0;
   ut_event_index = 0;
   ut_entries = 0;
   ut_exits = 0;
   ut_arrow_starts = 0;
   ut_arrow_ends = 0;
   ut_window_width = 80;
   ut_window_height = 25;
   ut_invalid_character = '\0';
   ut_invalid_position = -1;
   ut_convert_character = '\0';
   ut_converted_character = '\0';
   ut_convert_position = -1;
   ut_init_calls = 0;
   ut_permaliteral_calls = 0;
   ut_cursor_calls = 0;
   ut_attrib_calls = 0;
   ut_disp_calls = 0;
   ut_repeat_calls = 0;
   ut_putch_calls = 0;
   ut_init_succeeds = TRUE;
}

static void terminal_session_is_rejected(void)
{
   char input[8] = "";
   reset_editor(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT_EQ_UINT(0,
      utt_od_edit_str(input, "?", 1, 1, 7, 15, '.', EDIT_FLAG_NO_REDRAW));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void queue_failure(void)
{
   UT_ASSERT(ut_event_count < UT_MAX_EVENTS);
   ut_event_results[ut_event_count++] = FALSE;
}

static void queue_event(INT type, char key)
{
   unsigned index = ut_event_count++;
   UT_ASSERT(index < UT_MAX_EVENTS);
   ut_event_results[index] = TRUE;
   ut_events[index].EventType = type;
   ut_events[index].chKeyPress = key;
}

static void queue_character(char key)
{
   queue_event(EVENT_CHARACTER, key);
}

static void queue_extended(char key)
{
   queue_event(EVENT_EXTENDED_KEY, key);
}

static void input_failure_cancels_without_redraw(void)
{
   char input[4] = "";
   reset_editor();
   queue_failure();
   UT_ASSERT_EQ_INT(EDIT_RETURN_CANCEL,
      utt_od_edit_str(input, "?", 1, 1, 7, 15, '.', EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp("", input) == 0);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(1, ut_arrow_starts);
   UT_ASSERT_EQ_UINT(1, ut_arrow_ends);
}

static void rejects_invalid_parameters_after_initializing(void)
{
   char input[4] = "";
   reset_editor();
   bODInitialized = FALSE;
   UT_ASSERT_EQ_INT(EDIT_RETURN_ERROR,
      utt_od_edit_str(NULL, "?", 1, 1, 1, 2, '.', 0));
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);

   reset_editor();
   UT_ASSERT_EQ_INT(EDIT_RETURN_ERROR,
      utt_od_edit_str(input, NULL, 1, 1, 1, 2, '.', 0));
   reset_editor();
   UT_ASSERT_EQ_INT(EDIT_RETURN_ERROR,
      utt_od_edit_str(input, "?", 0, 1, 1, 2, '.', 0));
   reset_editor();
   UT_ASSERT_EQ_INT(EDIT_RETURN_ERROR,
      utt_od_edit_str(input, "?", 1, 0, 1, 2, '.', 0));
}

static void rejects_empty_and_oversized_formats(void)
{
   char input[82] = "";
   char controls[82];
   char literals[84];
   unsigned index;
   reset_editor();
   UT_ASSERT_EQ_INT(EDIT_RETURN_ERROR,
      utt_od_edit_str(input, "  '' \"\" ", 1, 1, 1, 2, '.', 0));

   for(index = 0; index < 81; ++index) controls[index] = '?';
   controls[81] = '\0';
   reset_editor();
   UT_ASSERT_EQ_INT(EDIT_RETURN_ERROR,
      utt_od_edit_str(input, controls, 1, 1, 1, 2, '.', 0));

   literals[0] = '\'';
   for(index = 0; index < 81; ++index) literals[index + 1] = 'x';
   literals[82] = '\'';
   literals[83] = '\0';
   reset_editor();
   UT_ASSERT_EQ_INT(EDIT_RETURN_ERROR,
      utt_od_edit_str(input, literals, 1, 1, 1, 2, '.', 0));
}

static void parses_both_literal_delimiters_and_spaces(void)
{
   char input[8] = "";
   reset_editor();
   queue_failure();
   UT_ASSERT_EQ_INT(EDIT_RETURN_CANCEL,
      utt_od_edit_str(input, " ? 'x' \"y\" ", 1, 1, 1, 2, '.',
         EDIT_FLAG_NO_REDRAW));
   UT_ASSERT_EQ_UINT(1, ut_arrow_starts);
}

static void rejects_each_out_of_window_condition(void)
{
   char input[8] = "";
   reset_editor();
   ut_window_height = 1;
   UT_ASSERT_EQ_INT(EDIT_RETURN_ERROR,
      utt_od_edit_str(input, "?", 2, 1, 1, 2, '.', 0));

   reset_editor();
   od_control.user_avatar = TRUE;
   ut_window_width = 300;
   ut_window_height = 300;
   UT_ASSERT_EQ_INT(EDIT_RETURN_ERROR,
      utt_od_edit_str(input, "?", 256, 1, 1, 2, '.', 0));

   reset_editor();
   ut_window_width = 3;
   UT_ASSERT_EQ_INT(EDIT_RETURN_ERROR,
      utt_od_edit_str(input, "?", 1, 3, 1, 2, '.', 0));

   reset_editor();
   ut_window_width = 4;
   UT_ASSERT_EQ_INT(EDIT_RETURN_ERROR,
      utt_od_edit_str(input, "???", 1, 1, 1, 2, '.', 0));

   reset_editor();
   od_control.user_avatar = TRUE;
   ut_window_width = 300;
   queue_failure();
   UT_ASSERT_EQ_INT(EDIT_RETURN_CANCEL,
      utt_od_edit_str(input, "?", 1, 254, 1, 2, '.',
         EDIT_FLAG_NO_REDRAW));
}

static void redraws_existing_normal_password_and_permaliteral_fields(void)
{
   char input[8] = "long";
   reset_editor();
   queue_failure();
   UT_ASSERT_EQ_INT(EDIT_RETURN_CANCEL,
      utt_od_edit_str(input, "??", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING));
   UT_ASSERT(strcmp(input, "lo") == 0);
   UT_ASSERT(ut_disp_calls > 0);
   UT_ASSERT(ut_repeat_calls > 0);

   utm_strcpy(input, "a");
   reset_editor();
   queue_failure();
   UT_ASSERT_EQ_INT(EDIT_RETURN_CANCEL,
      utt_od_edit_str(input, "?'x'", 1, 1, 1, 2, '*',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_PASSWORD_MODE |
         EDIT_FLAG_PERMALITERAL | EDIT_FLAG_SHOW_SIZE));
   UT_ASSERT(ut_permaliteral_calls > 0);
   UT_ASSERT(ut_repeat_calls > 0);

   utm_strcpy(input, "old");
   reset_editor();
   queue_failure();
   UT_ASSERT_EQ_INT(EDIT_RETURN_CANCEL,
      utt_od_edit_str(input, "???", 1, 1, 1, 2, '.', 0));
   UT_ASSERT(strcmp(input, "") == 0);
}

static void returns_each_field_navigation_result(void)
{
   char input[4] = "a";
   reset_editor();
   queue_extended(OD_KEY_UP);
   UT_ASSERT_EQ_INT(EDIT_RETURN_PREVIOUS,
      utt_od_edit_str(input, "?", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_FIELD_MODE |
         EDIT_FLAG_NO_REDRAW));

   utm_strcpy(input, "a");
   reset_editor();
   queue_extended(OD_KEY_SHIFTTAB);
   UT_ASSERT_EQ_INT(EDIT_RETURN_PREVIOUS,
      utt_od_edit_str(input, "?", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_FIELD_MODE |
         EDIT_FLAG_NO_REDRAW));

   utm_strcpy(input, "a");
   reset_editor();
   queue_extended(OD_KEY_DOWN);
   UT_ASSERT_EQ_INT(EDIT_RETURN_NEXT,
      utt_od_edit_str(input, "?", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_FIELD_MODE |
         EDIT_FLAG_NO_REDRAW));

   utm_strcpy(input, "a");
   reset_editor();
   queue_character(9);
   UT_ASSERT_EQ_INT(EDIT_RETURN_NEXT,
      utt_od_edit_str(input, "?", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_FIELD_MODE |
         EDIT_FLAG_NO_REDRAW));
}

static void handles_all_nonreturning_navigation_keys(void)
{
   char input[8] = "ab";
   reset_editor();
   queue_extended(OD_KEY_UP);
   queue_extended(OD_KEY_DOWN);
   queue_extended(OD_KEY_RIGHT);
   queue_extended(OD_KEY_LEFT);
   queue_extended(OD_KEY_LEFT);
   queue_extended(OD_KEY_LEFT);
   queue_extended(OD_KEY_HOME);
   queue_extended(OD_KEY_RIGHT);
   queue_extended(OD_KEY_HOME);
   queue_extended(OD_KEY_END);
   queue_extended(OD_KEY_END);
   queue_extended(OD_KEY_INSERT);
   queue_extended((char)0x7f);
   queue_event(99, 'x');
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "???", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "ab") == 0);
}

static void moves_left_across_permanent_literals(void)
{
   char input[8] = "a-x";
   reset_editor();
   queue_extended(OD_KEY_LEFT);
   queue_extended(OD_KEY_LEFT);
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "?'-'?", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_PERMALITERAL |
         EDIT_FLAG_NO_REDRAW));
}

static void inserts_missing_literals_and_replays_the_entered_character(void)
{
   char input[8] = "a";
   reset_editor();
   ut_invalid_character = 'q';
   ut_invalid_position = 1;
   queue_character('q');
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "?'x'?", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "axq") == 0);

   utm_strcpy(input, "a-");
   reset_editor();
   ut_invalid_character = 'q';
   ut_invalid_position = 1;
   queue_extended(OD_KEY_LEFT);
   queue_character('q');
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "?'-'?", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "a-q") == 0);
}

static void deletes_at_cursor_and_with_backspace(void)
{
   char input[8] = "abc";
   reset_editor();
   ut_convert_character = 'b';
   ut_converted_character = 'B';
   ut_convert_position = 0;
   queue_extended(OD_KEY_HOME);
   queue_extended(OD_KEY_DELETE);
   queue_extended(OD_KEY_END);
   queue_extended(OD_KEY_DELETE);
   queue_character(8);
   queue_character(8);
   queue_character(8);
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "???", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "") == 0);

   utm_strcpy(input, "abc");
   reset_editor();
   queue_extended(OD_KEY_HOME);
   queue_extended(OD_KEY_RIGHT);
   queue_character(8);
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "???", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "bc") == 0);
}

static void backspace_skips_permanent_literals(void)
{
   char input[8] = "ax";
   reset_editor();
   queue_character(8);
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "?'x'", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_PERMALITERAL |
         EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "") == 0);

   utm_strcpy(input, "x");
   reset_editor();
   queue_character(8);
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "'x'?", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_PERMALITERAL |
         EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "x") == 0);
}

static void control_y_and_auto_delete_clear_existing_text(void)
{
   char input[8] = "abc";
   reset_editor();
   ut_invalid_character = 25;
   ut_invalid_position = 0;
   queue_character(25);
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "***", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "") == 0);

   utm_strcpy(input, "a");
   reset_editor();
   queue_character(25);
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "?", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "") == 0);

   reset_editor();
   ut_invalid_character = 25;
   ut_invalid_position = 0;
   queue_character(25);
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "*", 1, 1, 1, 2, '.',
         EDIT_FLAG_NO_REDRAW));

   utm_strcpy(input, "abc");
   reset_editor();
   queue_character('x');
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "???", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_AUTO_DELETE |
         EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "x") == 0);

   utm_strcpy(input, "abc");
   reset_editor();
   queue_extended(OD_KEY_HOME);
   queue_character('x');
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "???", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_AUTO_DELETE |
         EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "abc") == 0);
}

static void escape_is_ignored_or_restores_the_original_value(void)
{
   char input[8] = "old";
   reset_editor();
   queue_character(27);
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "???", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_NO_REDRAW));

   utm_strcpy(input, "old");
   reset_editor();
   queue_character(25);
   queue_character(27);
   UT_ASSERT_EQ_INT(EDIT_RETURN_CANCEL,
      utt_od_edit_str(input, "???", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_ALLOW_CANCEL |
         EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "old") == 0);
   UT_ASSERT(ut_disp_calls > 0);
}

static void fill_and_auto_enter_wait_until_the_field_is_complete(void)
{
   char input[8] = "";
   reset_editor();
   queue_character(13);
   queue_character('a');
   queue_character('b');
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "??", 1, 1, 1, 2, '.',
         EDIT_FLAG_FILL_STRING | EDIT_FLAG_AUTO_ENTER |
         EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "ab") == 0);
}

static void acceptance_rejects_invalid_content_then_converts_it(void)
{
   char input[8] = "x";
   reset_editor();
   ut_invalid_character = 'x';
   ut_invalid_position = 0;
   queue_character(13);
   queue_character(8);
   queue_character('a');
   queue_character(26);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "?", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "a") == 0);

   utm_strcpy(input, "a");
   reset_editor();
   ut_convert_character = 'a';
   ut_converted_character = 'A';
   ut_convert_position = 0;
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "?", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "A") == 0);
   UT_ASSERT(ut_disp_calls > 0);
}

static void covers_remaining_window_and_display_flag_combinations(void)
{
   char input[8] = "";
   reset_editor();
   od_control.user_avatar = TRUE;
   ut_window_width = 200;
   queue_failure();
   UT_ASSERT_EQ_INT(EDIT_RETURN_CANCEL,
      utt_od_edit_str(input, "?", 1, 1, 1, 2, '.',
         EDIT_FLAG_NO_REDRAW));

   reset_editor();
   queue_failure();
   UT_ASSERT_EQ_INT(EDIT_RETURN_CANCEL,
      utt_od_edit_str(input, "?", 1, 1, 1, 2, '.', EDIT_FLAG_SHOW_SIZE));
}

static void revisits_existing_permaliterals_at_zero_and_middle_positions(void)
{
   char input[8] = "ax";
   reset_editor();
   queue_extended(OD_KEY_LEFT);
   queue_extended((char)0x7f);
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "?'x'?", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_PERMALITERAL |
         EDIT_FLAG_NO_REDRAW));

   utm_strcpy(input, "x");
   reset_editor();
   queue_extended(OD_KEY_HOME);
   queue_extended((char)0x7f);
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "'x'?", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_PERMALITERAL |
         EDIT_FLAG_NO_REDRAW));

   input[0] = '\0';
   reset_editor();
   queue_extended(OD_KEY_LEFT);
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "'x'?", 1, 1, 1, 2, '.',
         EDIT_FLAG_PERMALITERAL | EDIT_FLAG_NO_REDRAW));
}

static void covers_permaliteral_middle_backspace_and_mismatched_literal(void)
{
   char input[8] = "a-x";
   reset_editor();
   queue_extended(OD_KEY_HOME);
   queue_extended(OD_KEY_RIGHT);
   queue_character(8);
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "?'-'?", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_PERMALITERAL |
         EDIT_FLAG_NO_REDRAW));

   utm_strcpy(input, "a!");
   reset_editor();
   ut_invalid_character = 'q';
   ut_invalid_position = 1;
   queue_extended(OD_KEY_LEFT);
   queue_character('q');
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "?'x'?", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_NO_REDRAW));
}

static void adds_a_character_before_an_automatic_literal(void)
{
   char input[8] = "";
   reset_editor();
   queue_character('a');
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "?'x'?", 1, 1, 1, 2, '.',
         EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "ax") == 0);
}

static void cursor_validation_can_reject_or_password_convert(void)
{
   char input[8] = "abc";
   reset_editor();
   ut_invalid_character = 'b';
   ut_invalid_position = 0;
   queue_extended(OD_KEY_HOME);
   queue_extended(OD_KEY_DELETE);
   queue_character('x');
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "???", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_NO_REDRAW));

   utm_strcpy(input, "abc");
   reset_editor();
   ut_convert_character = 'b';
   ut_converted_character = 'B';
   ut_convert_position = 0;
   queue_extended(OD_KEY_HOME);
   queue_extended(OD_KEY_DELETE);
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "???", 1, 1, 1, 2, '*',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_PASSWORD_MODE |
         EDIT_FLAG_NO_REDRAW));
   UT_ASSERT_EQ_INT('B', input[0]);
}

static void cannot_append_beyond_a_full_field(void)
{
   char input[8] = "abc";
   reset_editor();
   queue_character('x');
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "???", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "abc") == 0);
}

static void leave_blank_with_no_literal_remains_empty(void)
{
   char input[4] = "";
   reset_editor();
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "?", 1, 1, 1, 2, '.',
         EDIT_FLAG_LEAVE_BLANK | EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "") == 0);
}

static void leave_blank_removes_only_leading_literals(void)
{
   char input[8] = "";
   reset_editor();
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "'-'?", 1, 1, 1, 2, '.',
         EDIT_FLAG_PERMALITERAL | EDIT_FLAG_LEAVE_BLANK |
         EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "") == 0);

   utm_strcpy(input, "-a");
   reset_editor();
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "'-'?", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_PERMALITERAL |
         EDIT_FLAG_LEAVE_BLANK | EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "-a") == 0);
}

static void final_redraw_covers_background_and_password_modes(void)
{
   char input[8] = "a";
   reset_editor();
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "???", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_KEEP_BLANK));

   utm_strcpy(input, "-a");
   reset_editor();
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "'-'??", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_PERMALITERAL |
         EDIT_FLAG_KEEP_BLANK));
   UT_ASSERT(ut_permaliteral_calls >= 2);

   utm_strcpy(input, "a");
   reset_editor();
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "???", 1, 1, 1, 2, '*',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_PASSWORD_MODE));
}

static void ignores_invalid_nonliteral_input(void)
{
   char input[4] = "";
   reset_editor();
   ut_invalid_character = '!';
   ut_invalid_position = 0;
   queue_character('!');
   queue_character('a');
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "?", 1, 1, 1, 2, '.',
         EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "a") == 0);
}

static void inserts_overwrites_and_rejects_full_insertions(void)
{
   char input[8] = "ac";
   reset_editor();
   queue_extended(OD_KEY_HOME);
   queue_extended(OD_KEY_RIGHT);
   queue_character('b');
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "???", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "abc") == 0);

   utm_strcpy(input, "abc");
   reset_editor();
   queue_extended(OD_KEY_HOME);
   queue_extended(OD_KEY_RIGHT);
   queue_character('x');
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "???", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "abc") == 0);

   utm_strcpy(input, "abc");
   reset_editor();
   queue_extended(OD_KEY_HOME);
   queue_extended(OD_KEY_RIGHT);
   queue_extended(OD_KEY_INSERT);
   queue_character('x');
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "???", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "axc") == 0);
}

static void exercises_password_insert_and_overwrite_output(void)
{
   char input[8] = "ac";
   reset_editor();
   queue_extended(OD_KEY_HOME);
   queue_extended(OD_KEY_RIGHT);
   queue_character('b');
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "???", 1, 1, 1, 2, '*',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_PASSWORD_MODE |
         EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "abc") == 0);

   utm_strcpy(input, "abc");
   reset_editor();
   queue_extended(OD_KEY_HOME);
   queue_extended(OD_KEY_INSERT);
   queue_character('x');
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "???", 1, 1, 1, 2, '*',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_PASSWORD_MODE |
         EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "xbc") == 0);
}

static void strict_and_permanent_modes_disable_insert_and_delete(void)
{
   char input[8] = "ab";
   reset_editor();
   queue_extended(OD_KEY_HOME);
   queue_extended(OD_KEY_INSERT);
   queue_extended(OD_KEY_DELETE);
   queue_character('x');
   queue_character(8);
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "??", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_STRICT_INPUT |
         EDIT_FLAG_NO_REDRAW));
   UT_ASSERT(strcmp(input, "xb") == 0);

   utm_strcpy(input, "a-x");
   reset_editor();
   queue_extended(OD_KEY_HOME);
   queue_extended(OD_KEY_INSERT);
   queue_extended(OD_KEY_DELETE);
   queue_character(13);
   UT_ASSERT_EQ_INT(EDIT_RETURN_ACCEPT,
      utt_od_edit_str(input, "?'-'?", 1, 1, 1, 2, '.',
         EDIT_FLAG_EDIT_STRING | EDIT_FLAG_PERMALITERAL |
         EDIT_FLAG_NO_REDRAW));
}

static const UTTestCase ut_cases[] = {
   {"input failure", input_failure_cancels_without_redraw},
   {"invalid parameters", rejects_invalid_parameters_after_initializing},
   {"invalid formats", rejects_empty_and_oversized_formats},
   {"format literals", parses_both_literal_delimiters_and_spaces},
   {"window limits", rejects_each_out_of_window_condition},
   {"initial redraw", redraws_existing_normal_password_and_permaliteral_fields},
   {"field navigation", returns_each_field_navigation_result},
   {"navigation keys", handles_all_nonreturning_navigation_keys},
   {"permaliteral left", moves_left_across_permanent_literals},
   {"literal replay", inserts_missing_literals_and_replays_the_entered_character},
   {"invalid input", ignores_invalid_nonliteral_input},
   {"insert overwrite", inserts_overwrites_and_rejects_full_insertions},
   {"password edits", exercises_password_insert_and_overwrite_output},
   {"strict edits", strict_and_permanent_modes_disable_insert_and_delete},
   {"delete and backspace", deletes_at_cursor_and_with_backspace},
   {"permaliteral backspace", backspace_skips_permanent_literals},
   {"line clearing", control_y_and_auto_delete_clear_existing_text},
   {"escape", escape_is_ignored_or_restores_the_original_value},
   {"fill and auto enter", fill_and_auto_enter_wait_until_the_field_is_complete},
   {"accept validation", acceptance_rejects_invalid_content_then_converts_it},
   {"leave blank", leave_blank_removes_only_leading_literals},
   {"final redraw", final_redraw_covers_background_and_password_modes},
   {"display combinations", covers_remaining_window_and_display_flag_combinations},
   {"permaliteral revisit", revisits_existing_permaliterals_at_zero_and_middle_positions},
   {"permaliteral edge edits", covers_permaliteral_middle_backspace_and_mismatched_literal},
   {"automatic literal", adds_a_character_before_an_automatic_literal},
   {"cursor validation", cursor_validation_can_reject_or_password_convert},
   {"full append", cannot_append_beyond_a_full_field},
   {"empty leave blank", leave_blank_with_no_literal_remains_empty},
   {"terminal session", terminal_session_is_rejected}
};
