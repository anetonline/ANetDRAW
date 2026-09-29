#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
static tODWindowsSubsystem ut_subsystem = kODWindowsSubsystemConsole;
tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{ return(ut_subsystem); }
#endif
static BOOL ut_public_call_allowed = TRUE;
BOOL utm_ODSyncPublicCallAllowed(void)
{ return(ut_public_call_allowed); }

static void rejects_a_terminal_session(void)
{
   ut_public_call_allowed = FALSE;
   utt_pdef_od_onerow(PEROP_INITIALIZE);
   ut_public_call_allowed = TRUE;
}

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) || defined(ODPLAT_WIN32)
#define UT_CUSTOM_MOCK_ODScrnDisplayString
#define UT_CUSTOM_MOCK_ODScrnPrintf
#define UT_CUSTOM_MOCK_ODScrnPutText
#define UT_CUSTOM_MOCK_ODScrnSetAttribute
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos
#define UT_CUSTOM_MOCK_sprintf

#include <stdarg.h>
#include <string.h>

static char ut_row[81];
static BYTE ut_column;
static BYTE ut_attribute;
static unsigned ut_puttext_calls;

static void ut_write(const char *text)
{
   while(*text != '\0' && ut_column <= 80)
      ut_row[ut_column++ - 1] = *text++;
}

static void reset_personality(void)
{
   memset(&od_control, 0, sizeof(od_control));
   memset(ut_row, '?', 80);
   ut_row[80] = '\0';
   strcpy(od_control.user_name, "User");
   strcpy(od_control.user_location, "Place");
   od_control.od_time_left = "%d mins";
   od_control.od_help_text = "F8 HELP";
   od_control.od_help_text2 = "F9 HELP";
   od_control.od_sysop_next = "[SN] ";
   od_control.od_want_chat = "[Want-Chat]";
   od_control.od_no_keyboard = "[Keyboard]";
   od_control.od_user_keyboard_on = TRUE;
   ut_column = 1;
   ut_attribute = 0;
   ut_puttext_calls = 0;
}

static void assert_at(unsigned column, const char *expected)
{
   UT_ASSERT_EQ_INT(0, strncmp(ut_row + column - 1, expected,
      strlen(expected)));
}

void ODCALL utm_ODScrnSetAttribute(BYTE attribute)
{
   UT_ASSERT(attribute == 0x70 || attribute == 0xf0);
   ut_attribute = attribute;
}

void ODCALL utm_ODScrnSetCursorPos(BYTE column, BYTE row)
{
   UT_ASSERT(column >= 1 && column <= 80);
   UT_ASSERT_EQ_INT(25, row);
   ut_column = column;
}

void ODCALL utm_ODScrnDisplayString(const char *text)
{
   UT_ASSERT_NOT_NULL(text);
   ut_write(text);
}

INT ODVCALL utm_ODScrnPrintf(char *format, ...)
{
   va_list arguments;
   char text[16];
   unsigned value;
   unsigned length;
   va_start(arguments, format);
   if(format == od_control.od_time_left)
   {
      UT_ASSERT_EQ_INT(60, va_arg(arguments, int));
      ut_write("60 mins");
   }
   else if(strcmp(format, "%3u") == 0)
   {
      value = va_arg(arguments, unsigned int);
      UT_ASSERT(value < 1000);
      text[0] = value >= 100 ? (char)('0' + value / 100) : ' ';
      text[1] = value >= 10 ? (char)('0' + (value / 10) % 10) : ' ';
      text[2] = (char)('0' + value % 10);
      text[3] = '\0';
      ut_write(text);
   }
   else if(strcmp(format, "%.10s") == 0 || strcmp(format, "%.11s") == 0)
   {
      const char *source = va_arg(arguments, const char *);
      length = strcmp(format, "%.10s") == 0 ? 10 : 11;
      while(length-- != 0 && *source != '\0')
         ut_row[ut_column++ - 1] = *source++;
   }
   else
      UT_ASSERT(FALSE);
   va_end(arguments);
   return(0);
}

BOOL ODCALL utm_ODScrnPutText(BYTE left, BYTE top, BYTE right, BYTE bottom,
   void *buffer)
{
   const unsigned char *cell = (const unsigned char *)buffer;
   UT_ASSERT_EQ_INT(80, left);
   UT_ASSERT_EQ_INT(25, top);
   UT_ASSERT_EQ_INT(80, right);
   UT_ASSERT_EQ_INT(25, bottom);
   UT_ASSERT_EQ_INT(' ', cell[0]);
   UT_ASSERT_EQ_INT(0x70, cell[1]);
   ut_row[79] = (char)cell[0];
   ++ut_puttext_calls;
   return(TRUE);
}

int utm_sprintf(char *output, const char *format, ...)
{
   va_list arguments;
   const char *name;
   const char *location;
   char *position = output;
   UT_ASSERT_EQ_INT(0, strcmp("%s of %s", format));
   va_start(arguments, format);
   name = va_arg(arguments, const char *);
   location = va_arg(arguments, const char *);
   while(*name != '\0') *position++ = *name++;
   *position++ = ' ';
   *position++ = 'o';
   *position++ = 'f';
   *position++ = ' ';
   while(*location != '\0') *position++ = *location++;
   *position = '\0';
   va_end(arguments);
   return((int)(position - output));
}

static void displays_the_compact_layout(void)
{
   reset_personality();
   od_control.od_node = 42;
   od_control.user_timelimit = 60;
   od_control.user_ansi = TRUE;
   od_control.user_avatar = TRUE;
   od_control.sysop_next = TRUE;
   utt_pdef_od_onerow(PEROP_DISPLAY1);
   assert_at(1, "User of Place");
   assert_at(34, "Time: ");
   assert_at(40, "60 mins");
   assert_at(50, "[ANSI]");
   assert_at(56, "[AVT]");
   assert_at(61, "[SN] ");
   assert_at(66, "[Node: 42]");
   assert_at(76, "F8/9");
   UT_ASSERT(strstr(ut_row, "Security") == NULL);
   UT_ASSERT(strstr(ut_row, "BPS") == NULL);
   UT_ASSERT_EQ_INT(0x70, ut_attribute);
   UT_ASSERT_EQ_UINT(1, ut_puttext_calls);
}

static void truncates_identity_before_the_time_field(void)
{
   reset_personality();
   strcpy(od_control.user_name, "12345678901234567890123456789012345");
   strcpy(od_control.user_location, "Location");
   od_control.user_timelimit = 60;
   utt_pdef_od_onerow(PEROP_DISPLAY1);
   assert_at(1, "123456789012345678901234567890123");
   assert_at(34, "Time: ");
}

static void displays_alerts_in_the_identity_field(void)
{
   reset_personality();
   od_control.user_timelimit = 60;
   od_control.od_user_keyboard_on = FALSE;
   utt_pdef_od_onerow(PEROP_DISPLAY1);
   assert_at(1, "[Keyboard]");
   assert_at(12, "           ");

   reset_personality();
   od_control.user_timelimit = 60;
   od_control.user_wantchat = TRUE;
   utt_pdef_od_onerow(PEROP_DISPLAY1);
   assert_at(1, "[Want-Chat]");

   reset_personality();
   od_control.user_timelimit = 60;
   od_control.od_user_keyboard_on = FALSE;
   od_control.user_wantchat = TRUE;
   utt_pdef_od_onerow(PEROP_UPDATE1);
   assert_at(1, "[Keyboard]");
   assert_at(12, "[Want-Chat]");
}

static void clears_optional_fields_and_formats_large_nodes(void)
{
   reset_personality();
   od_control.od_node = 1000;
   od_control.user_timelimit = 60;
   utt_pdef_od_onerow(PEROP_DISPLAY1);
   assert_at(50, "                ");
   assert_at(66, "[Node:  ?]");
}

static void displays_each_help_line(void)
{
   reset_personality();
   utt_pdef_od_onerow(PEROP_DISPLAY7);
   assert_at(1, "F8 HELP");
   assert_at(8, "                                                                         ");
   reset_personality();
   utt_pdef_od_onerow(PEROP_DISPLAY8);
   assert_at(1, "F9 HELP");
}

static void initializes_the_one_row_key_map(void)
{
   unsigned index;
   reset_personality();
   utt_pdef_od_onerow(PEROP_INITIALIZE);
   UT_ASSERT_EQ_INT(0x2300, od_control.key_hangup);
   UT_ASSERT_EQ_INT(0x2000, od_control.key_drop2bbs);
   UT_ASSERT_EQ_INT(0x2400, od_control.key_dosshell);
   UT_ASSERT_EQ_INT(0x2e00, od_control.key_chat);
   UT_ASSERT_EQ_INT(0x3100, od_control.key_sysopnext);
   UT_ASSERT_EQ_INT(0x2600, od_control.key_lockout);
   UT_ASSERT_EQ_INT(0x3b00, od_control.key_status[0]);
   for(index = 1; index < 6; ++index)
      UT_ASSERT_EQ_INT(0, od_control.key_status[index]);
   UT_ASSERT_EQ_INT(0x4200, od_control.key_status[6]);
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
   utt_pdef_od_onerow(0xff);
   UT_ASSERT_EQ_INT('?', ut_row[0]);
}

#ifdef ODPLAT_WIN32
static void ignores_gui_subsystem(void)
{
   reset_personality();
   ut_subsystem = kODWindowsSubsystemGUI;
   utt_pdef_od_onerow(PEROP_DISPLAY1);
   UT_ASSERT_EQ_INT('?', ut_row[0]);
   ut_subsystem = kODWindowsSubsystemConsole;
}
#endif

static const UTTestCase ut_cases[] = {
   {"compact layout", displays_the_compact_layout},
   {"identity truncation", truncates_identity_before_the_time_field},
   {"identity alerts", displays_alerts_in_the_identity_field},
   {"optional fields", clears_optional_fields_and_formats_large_nodes},
   {"help lines", displays_each_help_line},
   {"key map", initializes_the_one_row_key_map},
   {"unknown operation", ignores_an_unknown_operation},
#ifdef ODPLAT_WIN32
   {"GUI subsystem", ignores_gui_subsystem},
#endif
   {"terminal session", rejects_a_terminal_session}
};

#else

static void accepts_every_operation_as_a_noop(void)
{
   utt_pdef_od_onerow(PEROP_DISPLAY1);
   utt_pdef_od_onerow(PEROP_DISPLAY7);
   utt_pdef_od_onerow(PEROP_DISPLAY8);
   utt_pdef_od_onerow(PEROP_UPDATE1);
   utt_pdef_od_onerow(PEROP_INITIALIZE);
   utt_pdef_od_onerow(0xff);
}

static const UTTestCase ut_cases[] = {
   {"non-DOS no-op", accepts_every_operation_as_a_noop},
   {"terminal session", rejects_a_terminal_session}
};

#endif
