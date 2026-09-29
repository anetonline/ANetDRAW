#define UT_CUSTOM_MOCK_od_get_input
#define UT_CUSTOM_MOCK_ODEditUpdateCursorPos
#define UT_CUSTOM_MOCK_ODEditGotoPreviousLine
#define UT_CUSTOM_MOCK_ODEditBufferGetTotalLines
#define UT_CUSTOM_MOCK_ODEditGotoNextLine
#define UT_CUSTOM_MOCK_ODEditCursorLeft
#define UT_CUSTOM_MOCK_ODEditBufferGetLineLength
#define UT_CUSTOM_MOCK_ODEditScrollArea
#define UT_CUSTOM_MOCK_ODEditPastEndOfCurLine
#define UT_CUSTOM_MOCK_ODEditBufferMakeSpace
#define UT_CUSTOM_MOCK_ODEditDeleteCurrentChar
#define UT_CUSTOM_MOCK_od_putch
#define UT_CUSTOM_MOCK_ODEditDeleteCurrentLine
#define UT_CUSTOM_MOCK_ODEditEnterText
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_memset

#define UT_MAX_EVENTS 8
#define UT_MAX_ENTERED 8

static tODInputEvent ut_events[UT_MAX_EVENTS];
static unsigned ut_event_count;
static unsigned ut_event_index;
static unsigned ut_cursor_updates;
static unsigned ut_previous_calls;
static unsigned ut_next_calls;
static UINT ut_total_lines;
static BOOL ut_cursor_left_result;
static unsigned ut_cursor_left_calls;
static UINT ut_line_length;
static unsigned ut_line_length_calls;
static INT ut_scroll_distances[4];
static unsigned ut_scroll_calls;
static BOOL ut_past_end;
static unsigned ut_past_end_calls;
static tODResult ut_make_space_result;
static unsigned ut_make_space_calls;
static unsigned ut_delete_char_calls;
static unsigned ut_delete_line_calls;
static unsigned ut_beep_calls;
static tODResult ut_enter_result;
static char ut_entered_text[UT_MAX_ENTERED];
static BOOL ut_entered_insert;
static unsigned ut_enter_calls;
static tODEditMenuResult ut_menu_result;
static unsigned ut_menu_calls;

BOOL ODCALL utm_od_get_input(tODInputEvent *event, tODMilliSec timeout,
   WORD flags)
{
   UT_ASSERT_NOT_NULL(event);
   UT_ASSERT(timeout == OD_NO_TIMEOUT);
   UT_ASSERT_EQ_INT(GETIN_NORMAL, flags);
   if(ut_event_index >= ut_event_count)
      return(FALSE);
   *event = ut_events[ut_event_index++];
   return(TRUE);
}

void utm_ODEditUpdateCursorPos(tEditInstance *instance)
{
   UT_ASSERT_NOT_NULL(instance);
   ++ut_cursor_updates;
}

void utm_ODEditGotoPreviousLine(tEditInstance *instance)
{
   ++ut_previous_calls;
   UT_ASSERT(instance->unCurrentLine > 0);
   --instance->unCurrentLine;
}

UINT utm_ODEditBufferGetTotalLines(tEditInstance *instance)
{
   UT_ASSERT_NOT_NULL(instance);
   return(ut_total_lines);
}

void utm_ODEditGotoNextLine(tEditInstance *instance)
{
   ++ut_next_calls;
   UT_ASSERT(instance->unCurrentLine + 1 < ut_total_lines);
   ++instance->unCurrentLine;
}

BOOL utm_ODEditCursorLeft(tEditInstance *instance)
{
   ++ut_cursor_left_calls;
   if(ut_cursor_left_result && instance->unCurrentColumn > 0)
      --instance->unCurrentColumn;
   return(ut_cursor_left_result);
}

UINT utm_ODEditBufferGetLineLength(tEditInstance *instance, UINT line)
{
   ++ut_line_length_calls;
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT(line < ut_total_lines);
   return(ut_line_length);
}

BOOL utm_ODEditScrollArea(tEditInstance *instance, INT distance)
{
   UT_ASSERT(ut_scroll_calls < 4);
   ut_scroll_distances[ut_scroll_calls++] = distance;
   if(distance < 0)
      instance->unLineScrolledToTop -= (UINT)(-distance);
   else
      instance->unLineScrolledToTop += (UINT)distance;
   return(TRUE);
}

BOOL utm_ODEditPastEndOfCurLine(tEditInstance *instance)
{
   ++ut_past_end_calls;
   UT_ASSERT_NOT_NULL(instance);
   return(ut_past_end);
}

tODResult utm_ODEditBufferMakeSpace(tEditInstance *instance, UINT line,
   UINT column, UINT count)
{
   ++ut_make_space_calls;
   UT_ASSERT_EQ_UINT(instance->unCurrentLine, line);
   UT_ASSERT_EQ_UINT(instance->unCurrentColumn, column);
   UT_ASSERT_EQ_UINT(0, count);
   return(ut_make_space_result);
}

void utm_ODEditDeleteCurrentChar(tEditInstance *instance)
{
   UT_ASSERT_NOT_NULL(instance);
   ++ut_delete_char_calls;
}

void ODCALL utm_od_putch(char character)
{
   UT_ASSERT_EQ_INT('\a', character);
   ++ut_beep_calls;
}

void utm_ODEditDeleteCurrentLine(tEditInstance *instance)
{
   UT_ASSERT_NOT_NULL(instance);
   ++ut_delete_line_calls;
}

tODResult utm_ODEditEnterText(tEditInstance *instance, char *text,
   BOOL insert)
{
   unsigned index;
   UT_ASSERT_NOT_NULL(instance);
   ++ut_enter_calls;
   ut_entered_insert = insert;
   index = 0;
   while(text[index] != '\0' && index + 1 < UT_MAX_ENTERED)
   {
      ut_entered_text[index] = text[index];
      ++index;
   }
   ut_entered_text[index] = '\0';
   return(ut_enter_result);
}

size_t utm_strlen(const char *text)
{
   size_t length;
   length = 0;
   while(text[length] != '\0') ++length;
   return(length);
}

void *utm_memset(void *destination, int value, size_t count)
{
   unsigned char *bytes;
   size_t index;
   bytes = (unsigned char *)destination;
   for(index = 0; index < count; ++index)
      bytes[index] = (unsigned char)value;
   return(destination);
}

#ifdef ODPLAT_DOS32
static tODEditMenuResult ODCALL ut_menu_callback(void *unused)
#else
static tODEditMenuResult ut_menu_callback(void *unused)
#endif
{
   UT_ASSERT_NULL(unused);
   ++ut_menu_calls;
   return(ut_menu_result);
}

static void add_event(tODInputEventType type, char key)
{
   UT_ASSERT(ut_event_count < UT_MAX_EVENTS);
   ut_events[ut_event_count].EventType = type;
   ut_events[ut_event_count].bFromRemote = TRUE;
   ut_events[ut_event_count].chKeyPress = key;
   ++ut_event_count;
}

static void reset_loop(tEditInstance *instance, tODEditOptions *options)
{
   unsigned index;
   utm_memset(instance, 0, sizeof(*instance));
   utm_memset(options, 0, sizeof(*options));
   instance->pUserOptions = options;
   instance->unAreaWidth = 10;
   instance->unAreaHeight = 5;
   instance->unLinesInBuffer = 3;
   instance->bInsertMode = TRUE;
   instance->bWordWrapLongLines = TRUE;
   instance->unTabStopSize = 8;
   instance->pszLineBreak = "\n";
   instance->pszParagraphBreak = "\r";
   ut_event_count = 0;
   ut_event_index = 0;
   ut_cursor_updates = 0;
   ut_previous_calls = 0;
   ut_next_calls = 0;
   ut_total_lines = 3;
   ut_cursor_left_result = FALSE;
   ut_cursor_left_calls = 0;
   ut_line_length = 3;
   ut_line_length_calls = 0;
   ut_scroll_calls = 0;
   for(index = 0; index < 4; ++index) ut_scroll_distances[index] = 0;
   ut_past_end = FALSE;
   ut_past_end_calls = 0;
   ut_make_space_result = kODRCSuccess;
   ut_make_space_calls = 0;
   ut_delete_char_calls = 0;
   ut_delete_line_calls = 0;
   ut_beep_calls = 0;
   ut_enter_result = kODRCSuccess;
   ut_entered_text[0] = '\0';
   ut_entered_insert = FALSE;
   ut_enter_calls = 0;
   ut_menu_result = EDIT_MENU_DO_NOTHING;
   ut_menu_calls = 0;
   od_control.od_error = 0;
}

static void exits_successfully_when_input_ends(void)
{
   tEditInstance instance;
   tODEditOptions options;
   reset_loop(&instance, &options);
   UT_ASSERT_EQ_INT(OD_MULTIEDIT_SUCCESS, utt_ODEditMainLoop(&instance));
   UT_ASSERT_EQ_UINT(1, ut_cursor_updates);
}

static void handles_vertical_and_left_navigation_boundaries(void)
{
   tEditInstance instance;
   tODEditOptions options;

   reset_loop(&instance, &options); add_event(EVENT_EXTENDED_KEY, OD_KEY_UP);
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_UINT(0, ut_previous_calls);
   reset_loop(&instance, &options); instance.unCurrentLine = 1;
   add_event(EVENT_EXTENDED_KEY, OD_KEY_UP); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(1, ut_previous_calls);

   reset_loop(&instance, &options); instance.unCurrentLine = 2;
   add_event(EVENT_EXTENDED_KEY, OD_KEY_DOWN); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(0, ut_next_calls);
   reset_loop(&instance, &options); instance.unCurrentLine = 1;
   add_event(EVENT_EXTENDED_KEY, OD_KEY_DOWN); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(1, ut_next_calls);

   reset_loop(&instance, &options); instance.unCurrentColumn = 2;
   add_event(EVENT_EXTENDED_KEY, OD_KEY_LEFT); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(1, ut_cursor_updates);
   reset_loop(&instance, &options); instance.unCurrentColumn = 2;
   ut_cursor_left_result = TRUE; add_event(EVENT_EXTENDED_KEY, OD_KEY_LEFT);
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_UINT(2, ut_cursor_updates);
}

static void handles_right_navigation_in_both_wrap_modes(void)
{
   tEditInstance instance;
   tODEditOptions options;

   reset_loop(&instance, &options); instance.unCurrentColumn = 1;
   add_event(EVENT_EXTENDED_KEY, OD_KEY_RIGHT); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(2, instance.unCurrentColumn);

   reset_loop(&instance, &options); instance.unCurrentColumn = 3;
   add_event(EVENT_EXTENDED_KEY, OD_KEY_RIGHT); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(1, ut_next_calls); UT_ASSERT_EQ_UINT(0, instance.unCurrentColumn);

   reset_loop(&instance, &options); instance.unCurrentLine = 2;
   instance.unCurrentColumn = 3; add_event(EVENT_EXTENDED_KEY, OD_KEY_RIGHT);
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_UINT(0, ut_next_calls);

   reset_loop(&instance, &options); instance.bWordWrapLongLines = FALSE;
   instance.unCurrentColumn = 8; add_event(EVENT_EXTENDED_KEY, OD_KEY_RIGHT);
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_UINT(9, instance.unCurrentColumn);

   reset_loop(&instance, &options); instance.bWordWrapLongLines = FALSE;
   instance.unCurrentColumn = 9; add_event(EVENT_EXTENDED_KEY, OD_KEY_RIGHT);
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_UINT(9, instance.unCurrentColumn);
}

static void handles_home_end_insert_and_unknown_extended_keys(void)
{
   tEditInstance instance;
   tODEditOptions options;
   reset_loop(&instance, &options); instance.unCurrentColumn = 4;
   add_event(EVENT_EXTENDED_KEY, OD_KEY_HOME); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(0, instance.unCurrentColumn);
   reset_loop(&instance, &options); add_event(EVENT_EXTENDED_KEY, OD_KEY_END);
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_UINT(ut_line_length, instance.unCurrentColumn);
   reset_loop(&instance, &options); add_event(EVENT_EXTENDED_KEY, OD_KEY_INSERT);
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_INT(FALSE, instance.bInsertMode);
   reset_loop(&instance, &options); add_event(EVENT_EXTENDED_KEY, OD_KEY_F1);
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_UINT(1, ut_cursor_updates);
   reset_loop(&instance, &options);
   add_event((tODInputEventType)99, 0);
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_UINT(1, ut_cursor_updates);
}

static void handles_page_up_paths_and_distance_limits(void)
{
   tEditInstance instance;
   tODEditOptions options;

   reset_loop(&instance, &options); instance.unLineScrolledToTop = 2;
   instance.unCurrentLine = 2; add_event(EVENT_EXTENDED_KEY, OD_KEY_PGUP);
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_INT(-2, ut_scroll_distances[0]);

   reset_loop(&instance, &options); instance.unLineScrolledToTop = 8;
   instance.unCurrentLine = 8; add_event(EVENT_EXTENDED_KEY, OD_KEY_PGUP);
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_INT(-4, ut_scroll_distances[0]);

   reset_loop(&instance, &options); instance.unCurrentLine = 2;
   add_event(EVENT_EXTENDED_KEY, OD_KEY_PGUP); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(0, instance.unCurrentLine);

   reset_loop(&instance, &options); add_event(EVENT_EXTENDED_KEY, OD_KEY_PGUP);
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_UINT(1, ut_cursor_updates);
}

static void handles_page_down_paths_and_distance_limits(void)
{
   tEditInstance instance;
   tODEditOptions options;

   reset_loop(&instance, &options); ut_total_lines = 10;
   instance.unLinesInBuffer = 10; instance.unCurrentLine = 0;
   add_event(EVENT_EXTENDED_KEY, OD_KEY_PGDN); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_INT(4, ut_scroll_distances[0]); UT_ASSERT_EQ_UINT(4, instance.unCurrentLine);

   reset_loop(&instance, &options); ut_total_lines = 10;
   instance.unLinesInBuffer = 10; instance.unLineScrolledToTop = 7;
   instance.unCurrentLine = 8; add_event(EVENT_EXTENDED_KEY, OD_KEY_PGDN);
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_INT(2, ut_scroll_distances[0]);
   UT_ASSERT_EQ_UINT(9, instance.unCurrentLine);

   reset_loop(&instance, &options); instance.unLineScrolledToTop = 2;
   add_event(EVENT_EXTENDED_KEY, OD_KEY_PGDN); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(0, ut_scroll_calls);
}

static void handles_all_delete_outcomes(void)
{
   tEditInstance instance;
   tODEditOptions options;

   reset_loop(&instance, &options); ut_past_end = FALSE;
   add_event(EVENT_EXTENDED_KEY, OD_KEY_DELETE); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(1, ut_delete_char_calls); UT_ASSERT_EQ_UINT(0, ut_make_space_calls);

   reset_loop(&instance, &options); ut_past_end = TRUE;
   add_event(EVENT_EXTENDED_KEY, OD_KEY_DELETE); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(1, ut_delete_char_calls);

   reset_loop(&instance, &options); ut_past_end = TRUE;
   ut_make_space_result = kODRCSafeFailure;
   add_event(EVENT_EXTENDED_KEY, OD_KEY_DELETE); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(1, ut_beep_calls);

   reset_loop(&instance, &options); ut_past_end = TRUE;
   ut_make_space_result = kODRCUnrecoverableFailure;
   add_event(EVENT_EXTENDED_KEY, OD_KEY_DELETE);
   UT_ASSERT_EQ_INT(OD_MULTIEDIT_ERROR, utt_ODEditMainLoop(&instance));
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);
}

static void handles_control_keys_and_menu_results(void)
{
   tEditInstance instance;
   tODEditOptions options;

   reset_loop(&instance, &options); add_event(EVENT_CHARACTER, 25);
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_UINT(1, ut_delete_line_calls);

   reset_loop(&instance, &options); add_event(EVENT_CHARACTER, 26);
   UT_ASSERT_EQ_INT(OD_MULTIEDIT_SUCCESS, utt_ODEditMainLoop(&instance));

   reset_loop(&instance, &options); options.pfMenuCallback = ut_menu_callback;
   ut_menu_result = EDIT_MENU_EXIT_EDITOR; add_event(EVENT_CHARACTER, 27);
   UT_ASSERT_EQ_INT(OD_MULTIEDIT_SUCCESS, utt_ODEditMainLoop(&instance));

   reset_loop(&instance, &options); options.pfMenuCallback = ut_menu_callback;
   ut_menu_result = EDIT_MENU_DO_NOTHING; add_event(EVENT_CHARACTER, 26);
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_UINT(2, ut_cursor_updates);

   reset_loop(&instance, &options); options.pfMenuCallback = ut_menu_callback;
   ut_menu_result = (tODEditMenuResult)99; add_event(EVENT_CHARACTER, 26);
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_UINT(2, ut_cursor_updates);
}

static void handles_backspace_paths(void)
{
   tEditInstance instance;
   tODEditOptions options;

   reset_loop(&instance, &options); instance.unCurrentColumn = 2;
   ut_cursor_left_result = TRUE; ut_past_end = FALSE;
   add_event(EVENT_CHARACTER, '\b'); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(1, ut_delete_char_calls);

   reset_loop(&instance, &options); instance.unCurrentColumn = 2;
   ut_cursor_left_result = TRUE; ut_past_end = TRUE;
   add_event(EVENT_CHARACTER, '\b'); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(0, ut_delete_char_calls); UT_ASSERT_EQ_UINT(2, ut_cursor_updates);

   reset_loop(&instance, &options); instance.unCurrentColumn = 0;
   ut_cursor_left_result = FALSE; add_event(EVENT_CHARACTER, '\b');
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_UINT(0, ut_delete_char_calls);
}

static void handles_insert_mode_tabs_and_failure(void)
{
   tEditInstance instance;
   tODEditOptions options;

   reset_loop(&instance, &options); instance.unCurrentColumn = 3;
   add_event(EVENT_CHARACTER, '\t'); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(1, ut_enter_calls); UT_ASSERT_EQ_INT(TRUE, ut_entered_insert);
   UT_ASSERT_EQ_INT(0, strcmp("     ", ut_entered_text));

   reset_loop(&instance, &options); ut_enter_result = kODRCUnrecoverableFailure;
   add_event(EVENT_CHARACTER, '\t');
   UT_ASSERT_EQ_INT(OD_MULTIEDIT_ERROR, utt_ODEditMainLoop(&instance));
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);
}

static void handles_overwrite_mode_tab_paths(void)
{
   tEditInstance instance;
   tODEditOptions options;

   reset_loop(&instance, &options); instance.bInsertMode = FALSE;
   instance.unCurrentColumn = 0; ut_line_length = 9;
   add_event(EVENT_CHARACTER, '\t'); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(8, instance.unCurrentColumn);

   reset_loop(&instance, &options); instance.bInsertMode = FALSE;
   instance.bWordWrapLongLines = FALSE; instance.unCurrentColumn = 0;
   add_event(EVENT_CHARACTER, '\t'); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(8, instance.unCurrentColumn);

   reset_loop(&instance, &options); instance.bInsertMode = FALSE;
   instance.unCurrentColumn = 0; ut_line_length = 3;
   add_event(EVENT_CHARACTER, '\t'); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(1, ut_next_calls); UT_ASSERT_EQ_UINT(0, instance.unCurrentColumn);

   reset_loop(&instance, &options); instance.bInsertMode = FALSE;
   instance.unCurrentLine = 2; instance.unCurrentColumn = 0; ut_line_length = 3;
   add_event(EVENT_CHARACTER, '\t'); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(0, ut_next_calls);
}

static void handles_enter_break_selection_and_overwrite_navigation(void)
{
   tEditInstance instance;
   tODEditOptions options;

   reset_loop(&instance, &options); add_event(EVENT_CHARACTER, '\r');
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_INT(0, strcmp("\n", ut_entered_text));

   reset_loop(&instance, &options); instance.pszLineBreak = "";
   add_event(EVENT_CHARACTER, '\r'); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_INT(0, strcmp("\r", ut_entered_text));

   reset_loop(&instance, &options); instance.pszLineBreak = NULL;
   instance.pszParagraphBreak = ""; add_event(EVENT_CHARACTER, '\r');
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_INT(0, strcmp(DEFAULT_LINE_BREAK, ut_entered_text));

   reset_loop(&instance, &options); instance.pszLineBreak = NULL;
   instance.pszParagraphBreak = NULL; add_event(EVENT_CHARACTER, '\r');
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_INT(0, strcmp(DEFAULT_LINE_BREAK, ut_entered_text));

   reset_loop(&instance, &options); instance.bInsertMode = FALSE;
   instance.unCurrentLine = 2; instance.unCurrentColumn = 1;
   add_event(EVENT_CHARACTER, '\r'); utt_ODEditMainLoop(&instance);
   UT_ASSERT_EQ_UINT(ut_line_length, instance.unCurrentColumn);

   reset_loop(&instance, &options); instance.bInsertMode = FALSE;
   instance.unCurrentLine = 1; add_event(EVENT_CHARACTER, '\r');
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_UINT(1, ut_next_calls);
   UT_ASSERT_EQ_UINT(0, instance.unCurrentColumn);

   reset_loop(&instance, &options); ut_enter_result = kODRCUnrecoverableFailure;
   add_event(EVENT_CHARACTER, '\r');
   UT_ASSERT_EQ_INT(OD_MULTIEDIT_ERROR, utt_ODEditMainLoop(&instance));
}

static void handles_printable_and_ignored_characters(void)
{
   tEditInstance instance;
   tODEditOptions options;

   reset_loop(&instance, &options); add_event(EVENT_CHARACTER, 'A');
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_INT(0, strcmp("A", ut_entered_text));
   UT_ASSERT_EQ_INT(TRUE, ut_entered_insert);

   reset_loop(&instance, &options); instance.bInsertMode = FALSE;
   ut_past_end = TRUE; add_event(EVENT_CHARACTER, 'B');
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_INT(TRUE, ut_entered_insert);

   reset_loop(&instance, &options); instance.bInsertMode = FALSE;
   ut_past_end = FALSE; add_event(EVENT_CHARACTER, 'C');
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_INT(FALSE, ut_entered_insert);

   reset_loop(&instance, &options); add_event(EVENT_CHARACTER, 1);
   utt_ODEditMainLoop(&instance); UT_ASSERT_EQ_UINT(0, ut_enter_calls);

   reset_loop(&instance, &options); ut_enter_result = kODRCUnrecoverableFailure;
   add_event(EVENT_CHARACTER, 'D');
   UT_ASSERT_EQ_INT(OD_MULTIEDIT_ERROR, utt_ODEditMainLoop(&instance));
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);
}

static const UTTestCase ut_cases[] = {
   {"input end", exits_successfully_when_input_ends},
   {"vertical and left", handles_vertical_and_left_navigation_boundaries},
   {"right", handles_right_navigation_in_both_wrap_modes},
   {"simple extended", handles_home_end_insert_and_unknown_extended_keys},
   {"page up", handles_page_up_paths_and_distance_limits},
   {"page down", handles_page_down_paths_and_distance_limits},
   {"delete", handles_all_delete_outcomes},
   {"control and menu", handles_control_keys_and_menu_results},
   {"backspace", handles_backspace_paths},
   {"insert tab", handles_insert_mode_tabs_and_failure},
   {"overwrite tab", handles_overwrite_mode_tab_paths},
   {"enter", handles_enter_break_selection_and_overwrite_navigation},
   {"characters", handles_printable_and_ignored_characters}
};
