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
   utt_pdef_opendoors(PEROP_INITIALIZE);
   ut_public_call_allowed = TRUE;
}

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) || defined(ODPLAT_WIN32)
#define UT_CUSTOM_MOCK_ODScrnDisplayString
#define UT_CUSTOM_MOCK_ODScrnPrintf
#define UT_CUSTOM_MOCK_ODScrnPutText
#define UT_CUSTOM_MOCK_ODScrnSetAttribute
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos

#include <stdarg.h>
#include <string.h>

static unsigned ut_attribute_calls;
static unsigned ut_cursor_calls;
static unsigned ut_string_calls;
static unsigned ut_printf_calls;
static unsigned ut_puttext_calls;
static BOOL ut_saw_ansi;
static BOOL ut_saw_avatar;
static BOOL ut_saw_sysop_next;
static BOOL ut_saw_want_chat;
static BOOL ut_saw_no_keyboard;
static BOOL ut_saw_large_node;

static void reset_personality(void)
{
   memset(&od_control, 0, sizeof(od_control));
   od_control.od_status_line[0] = "status zero";
   od_control.od_status_line[1] = "%s|%s|%lu";
   od_control.od_status_line[2] = "status two";
   od_control.od_time_left = "%d";
   od_control.od_help_text = "help one";
   od_control.od_help_text2 = "help two";
   od_control.od_sysop_next = "NEXT";
   od_control.od_want_chat = "WANT CHAT";
   od_control.od_no_keyboard = "NO KEYBOARD";
   strcpy(od_control.user_name, "User");
   strcpy(od_control.user_location, "Place");
   ut_attribute_calls = 0;
   ut_cursor_calls = 0;
   ut_string_calls = 0;
   ut_printf_calls = 0;
   ut_puttext_calls = 0;
   ut_saw_ansi = FALSE;
   ut_saw_avatar = FALSE;
   ut_saw_sysop_next = FALSE;
   ut_saw_want_chat = FALSE;
   ut_saw_no_keyboard = FALSE;
   ut_saw_large_node = FALSE;
}

void ODCALL utm_ODScrnSetAttribute(BYTE attribute)
{
   UT_ASSERT(attribute == 0x70 || attribute == 0xf0);
   ++ut_attribute_calls;
}

void ODCALL utm_ODScrnSetCursorPos(BYTE column, BYTE row)
{
   UT_ASSERT(column >= 1 && column <= 80);
   UT_ASSERT(row == 24 || row == 25);
   ++ut_cursor_calls;
}

void ODCALL utm_ODScrnDisplayString(const char *text)
{
   UT_ASSERT_NOT_NULL(text);
   if(strcmp(text, "[ANSI]") == 0 || strcmp(text, "[ANSI] ") == 0)
      ut_saw_ansi = TRUE;
   if(strcmp(text, "[AVT]") == 0 || strcmp(text, "[AVT] ") == 0)
      ut_saw_avatar = TRUE;
   if(text == od_control.od_sysop_next) ut_saw_sysop_next = TRUE;
   if(text == od_control.od_want_chat) ut_saw_want_chat = TRUE;
   if(text == od_control.od_no_keyboard) ut_saw_no_keyboard = TRUE;
   if(strcmp(text, "?]") == 0) ut_saw_large_node = TRUE;
   ++ut_string_calls;
}

INT ODVCALL utm_ODScrnPrintf(char *format, ...)
{
   va_list arguments;
   va_start(arguments, format);
   if(format == od_control.od_status_line[1])
   {
      UT_ASSERT_EQ_INT(0, strcmp(od_control.user_name,
         va_arg(arguments, char *)));
      UT_ASSERT_EQ_INT(0, strcmp(od_control.user_location,
         va_arg(arguments, char *)));
      UT_ASSERT_EQ_UINT(od_control.od_connect_speed,
         va_arg(arguments, unsigned long));
   }
   else if(strcmp(format, "%d]") == 0)
   {
      UT_ASSERT_EQ_INT(od_control.od_node, va_arg(arguments, int));
   }
   else if(strcmp(format, "%u") == 0)
   {
      UT_ASSERT_EQ_UINT(od_control.user_security,
         va_arg(arguments, unsigned int));
   }
   else if(format == od_control.od_time_left)
   {
      UT_ASSERT_EQ_INT(od_control.user_timelimit,
         va_arg(arguments, int));
   }
   else
   {
      UT_ASSERT(FALSE);
   }
   va_end(arguments);
   ++ut_printf_calls;
   return 0;
}

BOOL ODCALL utm_ODScrnPutText(BYTE left, BYTE top, BYTE right, BYTE bottom,
   void *buffer)
{
   const unsigned char *bytes = (const unsigned char *)buffer;
   UT_ASSERT_EQ_INT(80, left);
   UT_ASSERT_EQ_INT(25, top);
   UT_ASSERT_EQ_INT(80, right);
   UT_ASSERT_EQ_INT(25, bottom);
   UT_ASSERT_EQ_INT(' ', bytes[0]);
   UT_ASSERT_EQ_INT(0x70, bytes[1]);
   ++ut_puttext_calls;
   return TRUE;
}

static void displays_each_optional_primary_indicator(void)
{
   reset_personality();
   od_control.od_node = 42;
   od_control.od_connect_speed = 57600;
   od_control.user_security = 100;
   od_control.user_timelimit = 60;
   od_control.user_ansi = TRUE;
   od_control.user_avatar = TRUE;
   od_control.sysop_next = TRUE;
   od_control.user_wantchat = TRUE;
   od_control.od_user_keyboard_on = FALSE;
   utt_pdef_opendoors(PEROP_DISPLAY1);
   UT_ASSERT_EQ_UINT(1, ut_puttext_calls);
   UT_ASSERT_EQ_UINT(4, ut_printf_calls);
   UT_ASSERT(ut_saw_ansi);
   UT_ASSERT(ut_saw_avatar);
   UT_ASSERT(ut_saw_sysop_next);
   UT_ASSERT(ut_saw_want_chat);
   UT_ASSERT(ut_saw_no_keyboard);
   UT_ASSERT(!ut_saw_large_node);
}

static void displays_large_node_and_omits_optional_indicators(void)
{
   reset_personality();
   od_control.od_node = 1000;
   od_control.od_user_keyboard_on = TRUE;
   utt_pdef_opendoors(PEROP_DISPLAY1);
   UT_ASSERT(ut_saw_large_node);
   UT_ASSERT(!ut_saw_ansi);
   UT_ASSERT(!ut_saw_avatar);
   UT_ASSERT(!ut_saw_sysop_next);
   UT_ASSERT(!ut_saw_want_chat);
   UT_ASSERT(!ut_saw_no_keyboard);
   UT_ASSERT_EQ_UINT(3, ut_printf_calls);
}

static void displays_help(void)
{
   reset_personality();
   utt_pdef_opendoors(PEROP_DISPLAY8);
   UT_ASSERT_EQ_UINT(1, ut_attribute_calls);
   UT_ASSERT_EQ_UINT(2, ut_cursor_calls);
   UT_ASSERT_EQ_UINT(2, ut_string_calls);
   UT_ASSERT_EQ_UINT(1, ut_puttext_calls);
}

static void updates_both_states_of_each_indicator(void)
{
   reset_personality();
   od_control.user_timelimit = 12;
   od_control.sysop_next = TRUE;
   od_control.user_ansi = TRUE;
   od_control.user_avatar = TRUE;
   od_control.od_user_keyboard_on = FALSE;
   od_control.user_wantchat = TRUE;
   utt_pdef_opendoors(PEROP_UPDATE1);
   UT_ASSERT(ut_saw_sysop_next);
   UT_ASSERT(ut_saw_ansi);
   UT_ASSERT(ut_saw_avatar);
   UT_ASSERT(ut_saw_no_keyboard);
   UT_ASSERT(ut_saw_want_chat);

   reset_personality();
   od_control.od_user_keyboard_on = TRUE;
   utt_pdef_opendoors(PEROP_UPDATE1);
   UT_ASSERT(!ut_saw_sysop_next);
   UT_ASSERT(!ut_saw_ansi);
   UT_ASSERT(!ut_saw_avatar);
   UT_ASSERT(!ut_saw_no_keyboard);
   UT_ASSERT(!ut_saw_want_chat);
}

static void initializes_the_standard_key_map(void)
{
   reset_personality();
   utt_pdef_opendoors(PEROP_INITIALIZE);
   UT_ASSERT_EQ_INT(0x2300, od_control.key_hangup);
   UT_ASSERT_EQ_INT(0x2000, od_control.key_drop2bbs);
   UT_ASSERT_EQ_INT(0x2400, od_control.key_dosshell);
   UT_ASSERT_EQ_INT(0x2e00, od_control.key_chat);
   UT_ASSERT_EQ_INT(0x3100, od_control.key_sysopnext);
   UT_ASSERT_EQ_INT(0x2600, od_control.key_lockout);
   UT_ASSERT_EQ_INT(0x3b00, od_control.key_status[0]);
   UT_ASSERT_EQ_INT(0x4300, od_control.key_status[7]);
   UT_ASSERT_EQ_INT(0x4400, od_control.key_status[8]);
   UT_ASSERT_EQ_INT(0x2500, od_control.key_keyboardoff);
   UT_ASSERT_EQ_INT(0x4800, od_control.key_moretime);
   UT_ASSERT_EQ_INT(0x5000, od_control.key_lesstime);
   UT_ASSERT_EQ_INT(-1, od_control.od_page_statusline);
}

static void ignores_an_unknown_operation(void)
{
   reset_personality();
   utt_pdef_opendoors(0xff);
   UT_ASSERT_EQ_UINT(0, ut_attribute_calls);
   UT_ASSERT_EQ_UINT(0, ut_cursor_calls);
   UT_ASSERT_EQ_UINT(0, ut_string_calls);
}

#ifdef ODPLAT_WIN32
static void ignores_gui_subsystem(void)
{
   reset_personality(); ut_subsystem = kODWindowsSubsystemGUI;
   utt_pdef_opendoors(PEROP_DISPLAY1);
   UT_ASSERT_EQ_UINT(0, ut_string_calls);
   ut_subsystem = kODWindowsSubsystemConsole;
}
#endif

static const UTTestCase ut_cases[] = {
   {"primary indicators", displays_each_optional_primary_indicator},
   {"primary omitted indicators", displays_large_node_and_omits_optional_indicators},
   {"help display", displays_help},
   {"indicator updates", updates_both_states_of_each_indicator},
   {"key-map initialization", initializes_the_standard_key_map},
   {"unknown operation", ignores_an_unknown_operation},
#ifdef ODPLAT_WIN32
   {"GUI subsystem", ignores_gui_subsystem},
#endif
   {"terminal session", rejects_a_terminal_session}
};

#else

static void accepts_every_operation_as_a_noop(void)
{
   utt_pdef_opendoors(PEROP_DISPLAY1);
   utt_pdef_opendoors(PEROP_DISPLAY8);
   utt_pdef_opendoors(PEROP_UPDATE1);
   utt_pdef_opendoors(PEROP_INITIALIZE);
   utt_pdef_opendoors(0xff);
}

static const UTTestCase ut_cases[] = {
   {"non-DOS no-op", accepts_every_operation_as_a_noop},
   {"terminal session", rejects_a_terminal_session}
};

#endif
