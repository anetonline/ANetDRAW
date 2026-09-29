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
   utt_pdef_pcboard(PEROP_INITIALIZE);
   ut_public_call_allowed = TRUE;
}

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) || defined(ODPLAT_WIN32)
#define UT_CUSTOM_MOCK_ODScrnDisplayChar
#define UT_CUSTOM_MOCK_ODScrnDisplayString
#define UT_CUSTOM_MOCK_ODScrnPrintf
#define UT_CUSTOM_MOCK_ODScrnSetAttribute
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos
#define UT_CUSTOM_MOCK_ODStatAddKey
#define UT_CUSTOM_MOCK_ODStatForceStatusUpdate
#define UT_CUSTOM_MOCK_sprintf
#define UT_CUSTOM_MOCK_toupper

#include <stdarg.h>
#include <string.h>

static unsigned ut_force_updates;
static unsigned ut_added_key_count;
static WORD ut_added_keys[2];
static BOOL ut_saw_upper_name;
static BOOL ut_saw_first_call;
static BOOL ut_saw_times_on;
static BOOL ut_saw_transfers;
static BOOL ut_saw_remote_baud;
static BOOL ut_saw_local_baud;
static unsigned char ut_mode_character;

static void reset_pcboard(void)
{
   memset(&od_control, 0, sizeof(od_control));
   strcpy(od_control.user_name, "Mixed Name");
   strcpy(od_control.user_location, "Mixed Place");
   strcpy(od_control.user_firstcall, "01-02-03");
   od_control.od_connect_speed = 57600;
   od_control.user_security = 100;
   od_control.user_numcalls = 7;
   od_control.user_uploads = 8;
   od_control.user_downloads = 9;
   od_control.user_timelimit = 60;
   ut_force_updates = 0;
   ut_added_key_count = 0;
   ut_saw_upper_name = FALSE;
   ut_saw_first_call = FALSE;
   ut_saw_times_on = FALSE;
   ut_saw_transfers = FALSE;
   ut_saw_remote_baud = FALSE;
   ut_saw_local_baud = FALSE;
   ut_mode_character = 0;
}

static void reset_display_observations(void)
{
   ut_saw_upper_name = FALSE;
   ut_saw_first_call = FALSE;
   ut_saw_times_on = FALSE;
   ut_saw_transfers = FALSE;
   ut_saw_remote_baud = FALSE;
   ut_saw_local_baud = FALSE;
   ut_mode_character = 0;
}

void ODCALL utm_ODScrnSetAttribute(BYTE attribute)
{
   UT_ASSERT_EQ_INT(0x70, attribute);
}

void ODCALL utm_ODScrnSetCursorPos(BYTE column, BYTE row)
{
   UT_ASSERT(column >= 1 && column <= 80);
   UT_ASSERT(row == 24 || row == 25);
}

void ODCALL utm_ODScrnDisplayString(const char *text)
{
   UT_ASSERT_NOT_NULL(text);
   if(strcmp(text, "MIXED NAME - MIXED PLACE") == 0)
      ut_saw_upper_name = TRUE;
   if(strcmp(text, "(Local) ") == 0) ut_saw_local_baud = TRUE;
}

void ODCALL utm_ODScrnDisplayChar(unsigned char character)
{
   UT_ASSERT(character == 'A' || character == 'G');
   ut_mode_character = character;
}

INT ODVCALL utm_ODScrnPrintf(char *format, ...)
{
   va_list arguments;
   va_start(arguments, format);
   if(strcmp(format, "(%lu) ") == 0)
   {
      UT_ASSERT_EQ_UINT(od_control.od_connect_speed,
         va_arg(arguments, unsigned long));
      ut_saw_remote_baud = TRUE;
   }
   else if(strcmp(format, " (%s)") == 0)
   {
      UT_ASSERT_EQ_INT(0, strcmp(od_control.user_firstcall,
         va_arg(arguments, char *)));
      ut_saw_first_call = TRUE;
   }
   else if(strcmp(format, "Sec(0)=%u  ") == 0)
   {
      UT_ASSERT_EQ_UINT(od_control.user_security,
         va_arg(arguments, unsigned int));
   }
   else if(strcmp(format, "Times On=%u  ") == 0)
   {
      UT_ASSERT_EQ_UINT(od_control.user_numcalls,
         va_arg(arguments, unsigned int));
      ut_saw_times_on = TRUE;
   }
   else if(strcmp(format, "Up:Dn=%lu:%lu") == 0)
   {
      UT_ASSERT_EQ_UINT(od_control.user_uploads,
         va_arg(arguments, unsigned long));
      UT_ASSERT_EQ_UINT(od_control.user_downloads,
         va_arg(arguments, unsigned long));
      ut_saw_transfers = TRUE;
   }
   else if(strcmp(format, "%4d") == 0)
   {
      UT_ASSERT_EQ_INT(od_control.user_timelimit,
         va_arg(arguments, int));
   }
   else
   {
      UT_ASSERT(FALSE);
   }
   va_end(arguments);
   return 0;
}

int utm_sprintf(char *buffer, const char *format, ...)
{
   va_list arguments;
   const char *name;
   const char *location;
   va_start(arguments, format);
   UT_ASSERT_EQ_INT(0, strcmp("%s - %s", format));
   name = va_arg(arguments, const char *);
   location = va_arg(arguments, const char *);
   UT_ASSERT_EQ_INT(0, strcmp(od_control.user_name, name));
   UT_ASSERT_EQ_INT(0, strcmp(od_control.user_location, location));
   strcpy(buffer, "Mixed Name - Mixed Place");
   va_end(arguments);
   return (int)strlen(buffer);
}

int utm_toupper(int character)
{
   if(character >= 'a' && character <= 'z') return character - 'a' + 'A';
   return character;
}

void ODCALL utm_ODStatAddKey(WORD key)
{
   UT_ASSERT(ut_added_key_count < sizeof(ut_added_keys) / sizeof(ut_added_keys[0]));
   ut_added_keys[ut_added_key_count++] = key;
}

void ODCALL utm_ODStatForceStatusUpdate(void)
{
   ++ut_force_updates;
}

static void display_primary(BYTE info_type, BOOL extended, BOOL ansi,
   BOOL avatar, BOOL rip, unsigned long baud)
{
   reset_display_observations();
   od_control.od_info_type = info_type;
   od_control.od_extended_info = extended;
   od_control.user_ansi = ansi;
   od_control.user_avatar = avatar;
   od_control.user_rip = rip;
   od_control.baud = baud;
   utt_pdef_pcboard(PEROP_DISPLAY1);
   UT_ASSERT(ut_saw_upper_name);
   UT_ASSERT_EQ_INT((ansi || avatar || rip) ? 'G' : 'A', ut_mode_character);
   UT_ASSERT_EQ_INT(baud != 0, ut_saw_remote_baud);
   UT_ASSERT_EQ_INT(baud == 0, ut_saw_local_baud);
}

static void covers_display_information_sources_and_modes(void)
{
   reset_pcboard();
   display_primary(0, FALSE, FALSE, FALSE, FALSE, 0);
   UT_ASSERT(!ut_saw_first_call && !ut_saw_times_on && !ut_saw_transfers);

   display_primary(RA1EXITINFO, FALSE, TRUE, FALSE, FALSE, 1);
   UT_ASSERT(ut_saw_first_call && !ut_saw_times_on && !ut_saw_transfers);
   display_primary(RA2EXITINFO, FALSE, FALSE, TRUE, FALSE, 1);
   UT_ASSERT(ut_saw_first_call);
   display_primary(CHAINTXT, FALSE, FALSE, FALSE, TRUE, 1);
   UT_ASSERT(!ut_saw_first_call && ut_saw_times_on && !ut_saw_transfers);
   display_primary(SFDOORSDAT, FALSE, FALSE, FALSE, FALSE, 1);
   UT_ASSERT(!ut_saw_times_on && ut_saw_transfers);
   display_primary(DOORSYS_GAP, FALSE, FALSE, FALSE, FALSE, 1);
   UT_ASSERT(ut_saw_times_on && ut_saw_transfers);
   display_primary(DOORSYS_WILDCAT, FALSE, FALSE, FALSE, FALSE, 1);
   UT_ASSERT(ut_saw_first_call && ut_saw_times_on && ut_saw_transfers);
   display_primary(0, TRUE, FALSE, FALSE, FALSE, 1);
   UT_ASSERT(!ut_saw_first_call && ut_saw_times_on && ut_saw_transfers);
}

static void displays_secondary_status_and_time_update(void)
{
   reset_pcboard();
   utt_pdef_pcboard(PEROP_DISPLAY2);
   UT_ASSERT_EQ_INT(0x2300, od_control.key_status[0]);
   UT_ASSERT_EQ_INT(0, od_control.key_status[1]);
   utt_pdef_pcboard(PEROP_UPDATE1);
}

static void initializes_the_pcboard_key_map(void)
{
   reset_pcboard();
   utt_pdef_pcboard(PEROP_INITIALIZE);
   UT_ASSERT_EQ_INT(0x4200, od_control.key_hangup);
   UT_ASSERT_EQ_INT(0x2d00, od_control.key_drop2bbs);
   UT_ASSERT_EQ_INT(0x3f00, od_control.key_dosshell);
   UT_ASSERT_EQ_INT(0x4400, od_control.key_chat);
   UT_ASSERT_EQ_INT(0x3100, od_control.key_sysopnext);
   UT_ASSERT_EQ_INT(0x3c00, od_control.key_lockout);
   UT_ASSERT_EQ_INT(0x2300, od_control.key_status[1]);
   UT_ASSERT_EQ_INT(0x2500, od_control.key_keyboardoff);
   UT_ASSERT_EQ_INT(0, od_control.key_moretime);
   UT_ASSERT_EQ_INT(0, od_control.key_lesstime);
   UT_ASSERT_EQ_INT(0, od_control.od_page_statusline);
   UT_ASSERT_EQ_UINT(2, ut_added_key_count);
   UT_ASSERT_EQ_INT(0x6900, ut_added_keys[0]);
   UT_ASSERT_EQ_INT(0x6800, ut_added_keys[1]);
}

static void add_time_covers_each_boundary(void)
{
   reset_pcboard();
   od_control.od_last_hot = 0x6900;
   od_control.user_timelimit = 1435;
   utt_pdef_pcboard(PEROP_CUSTOMKEY);
   UT_ASSERT_EQ_INT(1440, od_control.user_timelimit);
   UT_ASSERT_EQ_UINT(1, ut_force_updates);

   od_control.od_last_hot = 0x6900;
   od_control.user_timelimit = 1436;
   utt_pdef_pcboard(PEROP_CUSTOMKEY);
   UT_ASSERT_EQ_INT(1436, od_control.user_timelimit);
   UT_ASSERT_EQ_UINT(1, ut_force_updates);

   od_control.od_last_hot = 0x6900;
   od_control.user_timelimit = 4;
   utt_pdef_pcboard(PEROP_CUSTOMKEY);
   UT_ASSERT_EQ_INT(5, od_control.user_timelimit);
   UT_ASSERT_EQ_UINT(2, ut_force_updates);
   UT_ASSERT_EQ_INT(0, od_control.od_last_hot);
}

static void subtract_time_covers_each_boundary(void)
{
   reset_pcboard();
   od_control.od_last_hot = 0x6800;
   od_control.user_timelimit = 6;
   utt_pdef_pcboard(PEROP_CUSTOMKEY);
   UT_ASSERT_EQ_INT(1, od_control.user_timelimit);
   UT_ASSERT_EQ_UINT(1, ut_force_updates);

   od_control.od_last_hot = 0x6800;
   od_control.user_timelimit = 5;
   utt_pdef_pcboard(PEROP_CUSTOMKEY);
   UT_ASSERT_EQ_INT(4, od_control.user_timelimit);
   UT_ASSERT_EQ_UINT(2, ut_force_updates);

   od_control.od_last_hot = 0x6800;
   od_control.user_timelimit = 1;
   utt_pdef_pcboard(PEROP_CUSTOMKEY);
   UT_ASSERT_EQ_INT(1, od_control.user_timelimit);
   UT_ASSERT_EQ_UINT(2, ut_force_updates);
}

static void unknown_operations_and_hotkeys_are_ignored(void)
{
   reset_pcboard();
   utt_pdef_pcboard(0xff);
   od_control.od_last_hot = 0x1234;
   utt_pdef_pcboard(PEROP_CUSTOMKEY);
   UT_ASSERT_EQ_INT(0x1234, od_control.od_last_hot);
   UT_ASSERT_EQ_UINT(0, ut_force_updates);
}
#ifdef ODPLAT_WIN32
static void ignores_gui_subsystem(void)
{
   reset_pcboard(); ut_subsystem = kODWindowsSubsystemGUI;
   utt_pdef_pcboard(PEROP_DISPLAY1);
   UT_ASSERT(!ut_saw_upper_name);
   ut_subsystem = kODWindowsSubsystemConsole;
}
#endif

static const UTTestCase ut_cases[] = {
   {"display information sources", covers_display_information_sources_and_modes},
   {"secondary display and update", displays_secondary_status_and_time_update},
   {"key-map initialization", initializes_the_pcboard_key_map},
   {"add-time boundaries", add_time_covers_each_boundary},
   {"subtract-time boundaries", subtract_time_covers_each_boundary},
   {"unknown operations", unknown_operations_and_hotkeys_are_ignored},
#ifdef ODPLAT_WIN32
   {"GUI subsystem", ignores_gui_subsystem},
#endif
   {"terminal session", rejects_a_terminal_session}
};

#else

static void accepts_every_operation_as_a_noop(void)
{
   utt_pdef_pcboard(PEROP_DISPLAY1);
   utt_pdef_pcboard(PEROP_DISPLAY2);
   utt_pdef_pcboard(PEROP_UPDATE1);
   utt_pdef_pcboard(PEROP_INITIALIZE);
   utt_pdef_pcboard(PEROP_CUSTOMKEY);
   utt_pdef_pcboard(0xff);
}

static const UTTestCase ut_cases[] = {
   {"non-DOS no-op", accepts_every_operation_as_a_noop},
   {"terminal session", rejects_a_terminal_session}
};

#endif
