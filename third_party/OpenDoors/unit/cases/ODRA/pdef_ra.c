#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
static tODWindowsSubsystem ut_subsystem = kODWindowsSubsystemConsole;
tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{ return(ut_subsystem); }
#endif
static BOOL ut_public_call_allowed = TRUE;
BOOL utm_ODSyncPublicCallAllowed(void)
{
   return(ut_public_call_allowed);
}

static void rejects_a_terminal_session(void)
{
   ut_public_call_allowed = FALSE;
   utt_pdef_ra(PEROP_INITIALIZE);
   ut_public_call_allowed = TRUE;
}

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) || defined(ODPLAT_WIN32)
#define UT_CUSTOM_MOCK_ODRADisplayDate
#define UT_CUSTOM_MOCK_ODRADisplayFlags
#define UT_CUSTOM_MOCK_ODRADisplayPageInfo
#define UT_CUSTOM_MOCK_ODRADisplayTime
#define UT_CUSTOM_MOCK_ODScrnDisplayString
#define UT_CUSTOM_MOCK_ODScrnPrintf
#define UT_CUSTOM_MOCK_ODScrnPutText
#define UT_CUSTOM_MOCK_ODScrnSetAttribute
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos
#define UT_CUSTOM_MOCK_ODStatGetUserAge
#define UT_CUSTOM_MOCK_strcpy
#define UT_CUSTOM_MOCK_strlen

static unsigned ut_string_calls;
static unsigned ut_printf_calls;
static unsigned ut_puttext_calls;
static unsigned ut_attribute_calls;
static unsigned ut_cursor_calls;
static unsigned ut_page_calls;
static unsigned ut_time_calls;
static unsigned ut_date_calls;
static unsigned ut_flag_calls;
static unsigned ut_age_calls;

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

char *utm_strcpy(char *output, const char *input)
{
   char *result = output;
   while((*output++ = *input++) != '\0') { }
   return result;
}

void ODCALL utm_ODScrnDisplayString(const char *text)
{
   UT_ASSERT(text != NULL);
   ++ut_string_calls;
}

INT ODVCALL utm_ODScrnPrintf(char *format, ...)
{
   UT_ASSERT(format != NULL);
   ++ut_printf_calls;
   return 0;
}

BOOL ODCALL utm_ODScrnPutText(BYTE left, BYTE top, BYTE right, BYTE bottom,
   void *buffer)
{
   UT_ASSERT_EQ_UINT(80, left);
   UT_ASSERT_EQ_UINT(25, top);
   UT_ASSERT_EQ_UINT(80, right);
   UT_ASSERT_EQ_UINT(25, bottom);
   UT_ASSERT(buffer != NULL);
   ++ut_puttext_calls;
   return TRUE;
}

void ODCALL utm_ODScrnSetAttribute(BYTE attribute)
{
   UT_ASSERT(attribute == 0x70 || attribute == 0x99);
   ++ut_attribute_calls;
}

void ODCALL utm_ODScrnSetCursorPos(BYTE column, BYTE row)
{
   UT_ASSERT(column >= 1 && column <= 80);
   UT_ASSERT(row == 24 || row == 25);
   ++ut_cursor_calls;
}

void utm_ODRADisplayPageInfo(void)
{
   ++ut_page_calls;
}

void utm_ODRADisplayTime(void)
{
   ++ut_time_calls;
}

void utm_ODRADisplayDate(char *date)
{
   UT_ASSERT(date != NULL);
   ++ut_date_calls;
}

void utm_ODRADisplayFlags(BYTE flags)
{
   (void)flags;
   ++ut_flag_calls;
}

void ODCALL utm_ODStatGetUserAge(char *age)
{
   UT_ASSERT(age != NULL);
   age[0] = '?';
   age[1] = '\0';
   ++ut_age_calls;
}

static void reset_ra(void)
{
   memset(&od_control, 0, sizeof(od_control));
   utm_strcpy(od_control.user_name, "User");
   utm_strcpy(od_control.user_reasonforchat, "Reason");
   ut_string_calls = 0;
   ut_printf_calls = 0;
   ut_puttext_calls = 0;
   ut_attribute_calls = 0;
   ut_cursor_calls = 0;
   ut_page_calls = 0;
   ut_time_calls = 0;
   ut_date_calls = 0;
   ut_flag_calls = 0;
   ut_age_calls = 0;
}

static void display_one_covers_optional_indicators(void)
{
   reset_ra();
   od_control.od_node = 999;
   od_control.od_user_keyboard_on = FALSE;
   od_control.user_ansi = TRUE;
   od_control.user_avatar = TRUE;
   od_control.sysop_next = TRUE;
   od_control.user_wantchat = TRUE;
   utt_pdef_ra(PEROP_DISPLAY1);
   UT_ASSERT_EQ_UINT(1, ut_page_calls);
   UT_ASSERT(ut_string_calls > 5);

   reset_ra();
   od_control.od_node = 1000;
   od_control.od_user_keyboard_on = TRUE;
   utt_pdef_ra(PEROP_DISPLAY1);
   UT_ASSERT_EQ_UINT(1, ut_page_calls);
}

static void show_display_two(BYTE info_type, BOOL extended)
{
   reset_ra();
   od_control.od_info_type = info_type;
   od_control.od_extended_info = extended;
   utt_pdef_ra(PEROP_DISPLAY2);
}

static void display_two_covers_each_information_source(void)
{
   show_display_two(NO_DOOR_FILE, TRUE);
   show_display_two(NO_DOOR_FILE, FALSE);
   show_display_two(SFDOORSDAT, FALSE);
   show_display_two(DOORSYS_GAP, FALSE);
   show_display_two(CHAINTXT, FALSE);
   show_display_two(DOORSYS_WILDCAT, FALSE);
   show_display_two(RA1EXITINFO, FALSE);
   show_display_two(RA2EXITINFO, FALSE);
   UT_ASSERT(ut_cursor_calls > 0);
}

static void show_display_three(BYTE info_type, BOOL extended)
{
   reset_ra();
   od_control.od_info_type = info_type;
   od_control.od_extended_info = extended;
   utt_pdef_ra(PEROP_DISPLAY3);
}

static void display_three_covers_transfer_and_flag_sources(void)
{
   show_display_three(NO_DOOR_FILE, TRUE);
   UT_ASSERT_EQ_UINT(4, ut_flag_calls);
   show_display_three(NO_DOOR_FILE, FALSE);
   show_display_three(SFDOORSDAT, FALSE);
   show_display_three(DOORSYS_GAP, FALSE);
   show_display_three(DOORSYS_WILDCAT, FALSE);
}

static void show_display_four(BYTE info_type, BOOL extended, BYTE event_status)
{
   reset_ra();
   od_control.od_info_type = info_type;
   od_control.od_extended_info = extended;
   od_control.event_status = event_status;
   utt_pdef_ra(PEROP_DISPLAY4);
   UT_ASSERT_EQ_UINT(1, ut_time_calls);
}

static void display_four_covers_event_states(void)
{
   show_display_four(NO_DOOR_FILE, TRUE, ES_ENABLED);
   show_display_four(NO_DOOR_FILE, TRUE, 0);
   show_display_four(DOORSYS_WILDCAT, FALSE, 0);
   show_display_four(NO_DOOR_FILE, FALSE, 0);
}

static void show_display_five(BYTE info_type, BOOL extended)
{
   reset_ra();
   od_control.od_info_type = info_type;
   od_control.od_extended_info = extended;
   utt_pdef_ra(PEROP_DISPLAY5);
}

static void display_five_covers_message_credit_and_handle_sources(void)
{
   show_display_five(NO_DOOR_FILE, TRUE);
   show_display_five(NO_DOOR_FILE, FALSE);
   show_display_five(EXITINFO, TRUE);
   show_display_five(CHAINTXT, FALSE);
   show_display_five(RA1EXITINFO, TRUE);
   show_display_five(RA2EXITINFO, TRUE);
   show_display_five(DOORSYS_WILDCAT, FALSE);
}

static void show_display_six(BYTE info_type, BOOL wants_chat,
   const char *reason)
{
   reset_ra();
   od_control.od_info_type = info_type;
   od_control.user_wantchat = wants_chat;
   utm_strcpy(od_control.user_reasonforchat, reason);
   utt_pdef_ra(PEROP_DISPLAY6);
}

static void display_six_covers_comments_and_chat_reason(void)
{
   show_display_six(NO_DOOR_FILE, FALSE, "Reason");
   show_display_six(NO_DOOR_FILE, TRUE, "");
   show_display_six(NO_DOOR_FILE, TRUE, "Reason");
   show_display_six(RA1EXITINFO, FALSE, "");
   show_display_six(RA2EXITINFO, FALSE, "");
   show_display_six(DOORSYS_WILDCAT, FALSE, "");
}

static void covers_static_and_help_displays(void)
{
   reset_ra();
   utt_pdef_ra(PEROP_DISPLAY7);
   UT_ASSERT_EQ_UINT(1, ut_puttext_calls);
   reset_ra();
   utt_pdef_ra(PEROP_DISPLAY8);
   UT_ASSERT(ut_string_calls >= 2);
}

static void update_one_covers_each_indicator_transition(void)
{
   reset_ra();
   od_control.od_user_keyboard_on = TRUE;
   od_control.user_ansi = TRUE;
   od_control.user_avatar = TRUE;
   od_control.sysop_next = TRUE;
   od_control.user_wantchat = TRUE;
   utt_pdef_ra(PEROP_UPDATE1);

   od_control.od_user_keyboard_on = FALSE;
   od_control.user_ansi = FALSE;
   od_control.user_avatar = FALSE;
   od_control.sysop_next = FALSE;
   od_control.user_wantchat = FALSE;
   utt_pdef_ra(PEROP_UPDATE1);

   od_control.od_user_keyboard_on = TRUE;
   utt_pdef_ra(PEROP_UPDATE1);
   UT_ASSERT_EQ_UINT(3, ut_page_calls);
}

static void update_four_refreshes_the_clock(void)
{
   reset_ra();
   utt_pdef_ra(PEROP_UPDATE4);
   UT_ASSERT_EQ_UINT(1, ut_time_calls);
}

static void initialize_assigns_the_remoteaccess_keys(void)
{
   reset_ra();
   utt_pdef_ra(PEROP_INITIALIZE);
   UT_ASSERT_EQ_UINT(0x2300, od_control.key_hangup);
   UT_ASSERT_EQ_UINT(0x2000, od_control.key_drop2bbs);
   UT_ASSERT_EQ_UINT(0x2400, od_control.key_dosshell);
   UT_ASSERT_EQ_UINT(0x2e00, od_control.key_chat);
   UT_ASSERT_EQ_UINT(0x3100, od_control.key_sysopnext);
   UT_ASSERT_EQ_UINT(0x2600, od_control.key_lockout);
   UT_ASSERT_EQ_UINT(0x3b00, od_control.key_status[0]);
   UT_ASSERT_EQ_UINT(0x4400, od_control.key_status[8]);
   UT_ASSERT_EQ_UINT(0x2500, od_control.key_keyboardoff);
   UT_ASSERT_EQ_UINT(0x4800, od_control.key_moretime);
   UT_ASSERT_EQ_UINT(0x5000, od_control.key_lesstime);
   UT_ASSERT_EQ_UINT(5, od_control.od_page_statusline);
}

static void ignores_an_unknown_operation(void)
{
   reset_ra();
   utt_pdef_ra(0xff);
   UT_ASSERT_EQ_UINT(0, ut_string_calls);
}
#ifdef ODPLAT_WIN32
static void ignores_gui_subsystem(void)
{
   reset_ra(); ut_subsystem = kODWindowsSubsystemGUI;
   utt_pdef_ra(PEROP_DISPLAY1);
   UT_ASSERT_EQ_UINT(0, ut_string_calls);
   ut_subsystem = kODWindowsSubsystemConsole;
}
#endif

static const UTTestCase ut_cases[] = {
   {"display one", display_one_covers_optional_indicators},
   {"display two", display_two_covers_each_information_source},
   {"display three", display_three_covers_transfer_and_flag_sources},
   {"display four", display_four_covers_event_states},
   {"display five", display_five_covers_message_credit_and_handle_sources},
   {"display six", display_six_covers_comments_and_chat_reason},
   {"display seven and eight", covers_static_and_help_displays},
   {"update one", update_one_covers_each_indicator_transition},
   {"update four", update_four_refreshes_the_clock},
   {"initialize", initialize_assigns_the_remoteaccess_keys},
   {"unknown operation", ignores_an_unknown_operation},
#ifdef ODPLAT_WIN32
   {"GUI subsystem", ignores_gui_subsystem},
#endif
   {"terminal session", rejects_a_terminal_session}
};
#else
static void accepts_every_operation_as_a_no_op(void)
{
   utt_pdef_ra(PEROP_INITIALIZE);
   utt_pdef_ra(PEROP_DISPLAY1);
   utt_pdef_ra(0xff);
}

static const UTTestCase ut_cases[] = {
   {"non-DOS no-op", accepts_every_operation_as_a_no_op},
   {"terminal session", rejects_a_terminal_session}
};
#endif
