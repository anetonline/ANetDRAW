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
   utt_pdef_wildcat(PEROP_INITIALIZE);
   ut_public_call_allowed = TRUE;
}

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) || defined(ODPLAT_WIN32)
#define UT_CUSTOM_MOCK_ODScrnDisplayChar
#define UT_CUSTOM_MOCK_ODScrnDisplayString
#define UT_CUSTOM_MOCK_ODScrnPrintf
#define UT_CUSTOM_MOCK_ODScrnPutText
#define UT_CUSTOM_MOCK_ODScrnSetAttribute
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos
#define UT_CUSTOM_MOCK_ODStatAddKey
#define UT_CUSTOM_MOCK_ODStatForceStatusUpdate
#define UT_CUSTOM_MOCK_ODStatGetUserAge
#define UT_CUSTOM_MOCK_ODStatRemoveKey
#define UT_CUSTOM_MOCK_od_exit
#define UT_CUSTOM_MOCK_sprintf
#define UT_CUSTOM_MOCK_strlen

#include <stdarg.h>
#include <string.h>

#define UT_WILDCAT_KEY_COUNT 14

static WORD ut_added_keys[UT_WILDCAT_KEY_COUNT];
static WORD ut_removed_keys[UT_WILDCAT_KEY_COUNT];
static unsigned ut_added_count;
static unsigned ut_removed_count;
static unsigned ut_force_updates;
static unsigned ut_exit_calls;
static BOOL ut_saw_phone;
static BOOL ut_saw_age;
static BOOL ut_saw_since;
static BOOL ut_saw_page;
static BOOL ut_saw_keyboard;
static BOOL ut_saw_next;
static const char *ut_time_format;
static unsigned ut_firstcall_length;
static const int ut_time_limits[] = {9, 10, 100, 1000};
static const char *ut_time_formats[] = {"   %d", "  %d", " %d", "%d"};
static const WORD ut_custom_keys[UT_WILDCAT_KEY_COUNT] = {
   0x4200, 0x4800, 0x5000, 0x7800, 0x7900, 0x7a00, 0x7b00,
   0x7c00, 0x7d00, 0x7e00, 0x7f00, 0x8000, 0x3f00, 0x3e00
};
static const WORD ut_removed_custom_keys[UT_WILDCAT_KEY_COUNT] = {
   0x4200, 0x4800, 0x5000, 0x7800, 0x7900, 0x7a00, 0x7b00,
   0x7c00, 0x7d00, 0x7e00, 0x7f00, 0x8000, 0x3e00, 0x3f00
};
static const WORD ut_hangup_keys[] = {
   0x7800, 0x7900, 0x7a00, 0x7b00, 0x7c00,
   0x7d00, 0x7e00, 0x7f00, 0x8000
};

static void reset_wildcat(void)
{
   memset(&od_control, 0, sizeof(od_control));
   memset(szStatusText, 0, 80);
   strcpy(od_control.user_name, "Wild User");
   strcpy(od_control.user_homephone, "555-0100");
   strcpy(od_control.user_firstcall, "01-02-03");
   od_control.od_connect_speed = 57600;
   od_control.user_security = 100;
   od_control.user_timelimit = 60;
   ut_added_count = 0;
   ut_removed_count = 0;
   ut_force_updates = 0;
   ut_exit_calls = 0;
   ut_saw_phone = FALSE;
   ut_saw_age = FALSE;
   ut_saw_since = FALSE;
   ut_saw_page = FALSE;
   ut_saw_keyboard = FALSE;
   ut_saw_next = FALSE;
   ut_time_format = NULL;
   ut_firstcall_length = 8;
}

static void reset_display_observations(void)
{
   ut_saw_phone = FALSE;
   ut_saw_age = FALSE;
   ut_saw_since = FALSE;
   ut_saw_page = FALSE;
   ut_saw_keyboard = FALSE;
   ut_saw_next = FALSE;
   ut_time_format = NULL;
}

void ODCALL utm_ODScrnSetAttribute(BYTE attribute)
{
   UT_ASSERT(attribute == 0x70 || attribute == 0x71);
}

void ODCALL utm_ODScrnSetCursorPos(BYTE column, BYTE row)
{
   UT_ASSERT(column >= 1 && column <= 80);
   UT_ASSERT(row == 24 || row == 25);
}

void ODCALL utm_ODScrnDisplayString(const char *text)
{
   UT_ASSERT_NOT_NULL(text);
   if(text == od_control.user_homephone) ut_saw_phone = TRUE;
   if(strcmp(text, "Age: ") == 0 || strcmp(text, "42") == 0)
      ut_saw_age = TRUE;
   if(strcmp(text, "Since: ") == 0) ut_saw_since = TRUE;
   if(strcmp(text, "Page Bell ") == 0) ut_saw_page = TRUE;
   if(strcmp(text, "Kybd ") == 0) ut_saw_keyboard = TRUE;
   if(strcmp(text, "Local-Next") == 0) ut_saw_next = TRUE;
}

void ODCALL utm_ODScrnDisplayChar(unsigned char character)
{
   UT_ASSERT((character >= '0' && character <= '9') || character == '/');
}

INT ODVCALL utm_ODScrnPrintf(char *format, ...)
{
   va_list arguments;
   va_start(arguments, format);
   if(strcmp(format, "%34.34s") == 0)
   {
      UT_ASSERT_EQ_PTR(szStatusText, va_arg(arguments, char *));
   }
   else if(strcmp(format, "%lu") == 0)
   {
      UT_ASSERT_EQ_UINT(od_control.od_connect_speed,
         va_arg(arguments, unsigned long));
   }
   else if(strcmp(format, "%u") == 0)
   {
      UT_ASSERT_EQ_UINT(od_control.user_security,
         va_arg(arguments, unsigned int));
   }
   else if(strcmp(format, "   %d") == 0 || strcmp(format, "  %d") == 0 ||
           strcmp(format, " %d") == 0 || strcmp(format, "%d") == 0)
   {
      UT_ASSERT_EQ_INT(od_control.user_timelimit, va_arg(arguments, int));
      ut_time_format = format;
   }
   else
   {
      UT_ASSERT(FALSE);
   }
   va_end(arguments);
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
   return TRUE;
}

int utm_sprintf(char *buffer, const char *format, ...)
{
   va_list arguments;
   const char *name;
   va_start(arguments, format);
   UT_ASSERT_EQ_INT(0, strcmp("(%s), ", format));
   name = va_arg(arguments, const char *);
   UT_ASSERT_EQ_INT(0, strcmp(od_control.user_name, name));
   strcpy(buffer, "(Wild User), ");
   va_end(arguments);
   return (int)(sizeof("(Wild User), ") - 1);
}

size_t utm_strlen(const char *text)
{
   UT_ASSERT_EQ_PTR(od_control.user_firstcall, text);
   return ut_firstcall_length;
}

void ODCALL utm_ODStatGetUserAge(char *age)
{
   strcpy(age, "42");
}

void ODCALL utm_ODStatAddKey(WORD key)
{
   UT_ASSERT(ut_added_count < UT_WILDCAT_KEY_COUNT);
   ut_added_keys[ut_added_count++] = key;
}

void ODCALL utm_ODStatRemoveKey(WORD key)
{
   UT_ASSERT(ut_removed_count < UT_WILDCAT_KEY_COUNT);
   ut_removed_keys[ut_removed_count++] = key;
}

void ODCALL utm_ODStatForceStatusUpdate(void)
{
   ++ut_force_updates;
}

void ODCALL utm_od_exit(INT error_level, BOOL hangup)
{
   UT_ASSERT_EQ_INT(2, error_level);
   UT_ASSERT(hangup);
   ++ut_exit_calls;
}

static void display_primary(BYTE info_type, BOOL extended,
   unsigned firstcall_length)
{
   reset_display_observations();
   od_control.od_info_type = info_type;
   od_control.od_extended_info = extended;
   ut_firstcall_length = firstcall_length;
   utt_pdef_wildcat(PEROP_DISPLAY1);
}

static void covers_every_display_information_source(void)
{
   reset_wildcat();
   display_primary(0, FALSE, 7);
   UT_ASSERT(!ut_saw_phone && !ut_saw_age && !ut_saw_since);

   display_primary(0, TRUE, 7);
   UT_ASSERT(ut_saw_phone && !ut_saw_age && !ut_saw_since);
   display_primary(SFDOORSDAT, FALSE, 7);
   UT_ASSERT(ut_saw_phone && !ut_saw_age && !ut_saw_since);
   display_primary(DOORSYS_GAP, FALSE, 7);
   UT_ASSERT(ut_saw_phone && !ut_saw_age && !ut_saw_since);

   display_primary(RA1EXITINFO, FALSE, 8);
   UT_ASSERT(!ut_saw_phone && ut_saw_age && ut_saw_since);
   display_primary(RA1EXITINFO, FALSE, 7);
   UT_ASSERT(ut_saw_age && !ut_saw_since);
   display_primary(RA2EXITINFO, FALSE, 8);
   UT_ASSERT(ut_saw_age && ut_saw_since);
   display_primary(DOORSYS_WILDCAT, FALSE, 8);
   UT_ASSERT(ut_saw_phone && ut_saw_age && ut_saw_since);
}

static void covers_each_time_width_and_indicator_state(void)
{
   unsigned index;
   reset_wildcat();
   for(index = 0;
       index < sizeof(ut_time_limits) / sizeof(ut_time_limits[0]);
       ++index)
   {
      reset_display_observations();
      od_control.user_timelimit = ut_time_limits[index];
      od_control.od_okaytopage = index == 0;
      od_control.od_user_keyboard_on = index == 0;
      od_control.sysop_next = index == 0;
      utt_pdef_wildcat(PEROP_UPDATE1);
      UT_ASSERT_EQ_INT(0, strcmp(ut_time_formats[index], ut_time_format));
      UT_ASSERT_EQ_INT(index == 0, ut_saw_page);
      UT_ASSERT_EQ_INT(index == 0, ut_saw_keyboard);
      UT_ASSERT_EQ_INT(index == 0, ut_saw_next);
   }
}

static void initializes_and_deinitializes_every_custom_key(void)
{
   unsigned index;
   reset_wildcat();
   utt_pdef_wildcat(PEROP_INITIALIZE);
   UT_ASSERT_EQ_INT(0, od_control.key_hangup);
   UT_ASSERT_EQ_INT(0x4400, od_control.key_drop2bbs);
   UT_ASSERT_EQ_INT(0x2000, od_control.key_dosshell);
   UT_ASSERT_EQ_INT(0x4100, od_control.key_chat);
   UT_ASSERT_EQ_INT(0x3b00, od_control.key_sysopnext);
   UT_ASSERT_EQ_INT(0x8100, od_control.key_lockout);
   UT_ASSERT_EQ_INT(0x2500, od_control.key_keyboardoff);
   UT_ASSERT_EQ_INT(-1, od_control.od_page_statusline);
   UT_ASSERT_EQ_UINT(UT_WILDCAT_KEY_COUNT, ut_added_count);
   for(index = 0; index < UT_WILDCAT_KEY_COUNT; ++index)
      UT_ASSERT_EQ_INT(ut_custom_keys[index], ut_added_keys[index]);

   utt_pdef_wildcat(PEROP_DEINITIALIZE);
   UT_ASSERT_EQ_UINT(UT_WILDCAT_KEY_COUNT, ut_removed_count);
   for(index = 0; index < UT_WILDCAT_KEY_COUNT; ++index)
      UT_ASSERT_EQ_INT(ut_removed_custom_keys[index], ut_removed_keys[index]);
}

static void custom_chat_and_time_keys_update_the_session(void)
{
   reset_wildcat();
   od_control.od_chat_active = TRUE;
   od_control.od_last_hot = 0x4200;
   utt_pdef_wildcat(PEROP_CUSTOMKEY);
   UT_ASSERT(!od_control.od_chat_active);

   od_control.user_timelimit = 1435;
   od_control.od_last_hot = 0x4800;
   utt_pdef_wildcat(PEROP_CUSTOMKEY);
   UT_ASSERT_EQ_INT(1440, od_control.user_timelimit);
   UT_ASSERT_EQ_UINT(1, ut_force_updates);
   od_control.user_timelimit = 1436;
   od_control.od_last_hot = 0x4800;
   utt_pdef_wildcat(PEROP_CUSTOMKEY);
   UT_ASSERT_EQ_INT(1436, od_control.user_timelimit);
   UT_ASSERT_EQ_UINT(1, ut_force_updates);

   od_control.user_timelimit = 10;
   od_control.od_last_hot = 0x5000;
   utt_pdef_wildcat(PEROP_CUSTOMKEY);
   UT_ASSERT_EQ_INT(5, od_control.user_timelimit);
   UT_ASSERT_EQ_UINT(2, ut_force_updates);
}

static void every_hangup_key_requests_exit(void)
{
   unsigned index;
   reset_wildcat();
   for(index = 0;
       index < sizeof(ut_hangup_keys) / sizeof(ut_hangup_keys[0]);
       ++index)
   {
      od_control.od_last_hot = ut_hangup_keys[index];
      utt_pdef_wildcat(PEROP_CUSTOMKEY);
   }
   UT_ASSERT_EQ_UINT(sizeof(ut_hangup_keys) / sizeof(ut_hangup_keys[0]),
      ut_exit_calls);
}

static void both_page_keys_toggle_each_direction(void)
{
   reset_wildcat();
   od_control.od_okaytopage = FALSE;
   od_control.od_last_hot = 0x3f00;
   utt_pdef_wildcat(PEROP_CUSTOMKEY);
   UT_ASSERT(od_control.od_okaytopage);
   od_control.od_last_hot = 0x3e00;
   utt_pdef_wildcat(PEROP_CUSTOMKEY);
   UT_ASSERT(!od_control.od_okaytopage);
   UT_ASSERT_EQ_UINT(2, ut_force_updates);
}

static void unknown_operations_and_hotkeys_are_ignored(void)
{
   reset_wildcat();
   utt_pdef_wildcat(0xff);
   od_control.od_last_hot = 0x1234;
   utt_pdef_wildcat(PEROP_CUSTOMKEY);
   UT_ASSERT_EQ_INT(0x1234, od_control.od_last_hot);
}
#ifdef ODPLAT_WIN32
static void ignores_gui_subsystem(void)
{
   reset_wildcat(); ut_subsystem = kODWindowsSubsystemGUI;
   utt_pdef_wildcat(PEROP_DISPLAY1);
   UT_ASSERT_EQ_UINT(0, ut_added_count);
   ut_subsystem = kODWindowsSubsystemConsole;
}
#endif

static const UTTestCase ut_cases[] = {
   {"display information sources", covers_every_display_information_source},
   {"time widths and indicators", covers_each_time_width_and_indicator_state},
   {"initialize and deinitialize", initializes_and_deinitializes_every_custom_key},
   {"chat and time hotkeys", custom_chat_and_time_keys_update_the_session},
   {"hangup hotkeys", every_hangup_key_requests_exit},
   {"page hotkeys", both_page_keys_toggle_each_direction},
   {"unknown operations", unknown_operations_and_hotkeys_are_ignored},
#ifdef ODPLAT_WIN32
   {"GUI subsystem", ignores_gui_subsystem},
#endif
   {"terminal session", rejects_a_terminal_session}
};

#else

static void accepts_every_operation_as_a_noop(void)
{
   utt_pdef_wildcat(PEROP_DISPLAY1);
   utt_pdef_wildcat(PEROP_UPDATE1);
   utt_pdef_wildcat(PEROP_INITIALIZE);
   utt_pdef_wildcat(PEROP_CUSTOMKEY);
   utt_pdef_wildcat(PEROP_DEINITIALIZE);
   utt_pdef_wildcat(0xff);
}

static const UTTestCase ut_cases[] = {
   {"non-DOS no-op", accepts_every_operation_as_a_noop},
   {"terminal session", rejects_a_terminal_session}
};

#endif
