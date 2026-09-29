#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_init

#ifdef OD_PERSONALITY_SUPPORT
#define UT_CUSTOM_MOCK_ODScrnSetBoundary
#define UT_CUSTOM_MOCK_od_set_statusline
#define UT_CUSTOM_MOCK_strcmp
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_strncpy
#define UT_CUSTOM_MOCK_strupr
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#define UT_CUSTOM_MOCK_ODPersonalityOpenDoors
#define UT_CUSTOM_MOCK_ODPersonalityOneRow
#define UT_CUSTOM_MOCK_ODPersonalityPCBoard
#define UT_CUSTOM_MOCK_ODPersonalityRemoteAccess
#define UT_CUSTOM_MOCK_ODPersonalityWildcat
#endif
#endif

#include <string.h>

static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static BOOL ut_init_succeeds;

void ODCALL utm_od_init(void)
{
   if(ut_init_succeeds) bODInitialized = TRUE;
   ++ut_init_calls;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

#ifdef OD_PERSONALITY_SUPPORT
static unsigned ut_status_calls;
static BYTE ut_status_values[2];
static unsigned ut_boundary_calls;
static BYTE ut_expected_bottom = 23;
static unsigned ut_standard_calls;
static unsigned ut_onerow_calls;
static unsigned ut_ra_calls;
static BYTE ut_standard_operation;
static BYTE ut_onerow_operation;
static BYTE ut_ra_operation;
#ifdef ODPLAT_WIN32
static tODWindowsSubsystem ut_subsystem = kODWindowsSubsystemConsole;
tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void) { return(ut_subsystem); }
static void ODPersonalityOpenDoors(BYTE operation)
{ ut_standard_operation = operation; ++ut_standard_calls; }
static void ODPersonalityOneRow(BYTE operation)
{ ut_onerow_operation = operation; ++ut_onerow_calls; }
static void ODPersonalityRemoteAccess(BYTE operation)
{ ut_ra_operation = operation; ++ut_ra_calls; }
static void ODPersonalityWildcat(BYTE operation)
{ (void)operation; UT_ASSERT(FALSE); }
static void ODPersonalityPCBoard(BYTE operation)
{ (void)operation; UT_ASSERT(FALSE); }
#endif

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

char *utm_strncpy(char *destination, const char *source, size_t count)
{
   size_t index;
   UT_ASSERT_EQ_UINT(32, count);
   for(index = 0; index < count && source[index] != '\0'; ++index)
      destination[index] = source[index];
   while(index < count) destination[index++] = '\0';
   return destination;
}

char *utm_strupr(char *text)
{
   char *position;
   for(position = text; *position != '\0'; ++position)
      if(*position >= 'a' && *position <= 'z')
         *position = (char)(*position - 'a' + 'A');
   return text;
}

int utm_strcmp(const char *left, const char *right)
{
   while(*left != '\0' && *left == *right)
   {
      ++left;
      ++right;
   }
   return (unsigned char)*left - (unsigned char)*right;
}

void ODCALL utm_od_set_statusline(INT status)
{
   UT_ASSERT(ut_status_calls < sizeof(ut_status_values));
   ut_status_values[ut_status_calls++] = status;
}

void utm_ODScrnSetBoundary(BYTE left, BYTE top, BYTE right,
   BYTE bottom)
{
   UT_ASSERT_EQ_INT(1, left);
   UT_ASSERT_EQ_INT(1, top);
   UT_ASSERT_EQ_INT(80, right);
   UT_ASSERT_EQ_INT(ut_expected_bottom, bottom);
   ++ut_boundary_calls;
}

void ODCALL pdef_opendoors(BYTE operation)
{
   ut_standard_operation = operation;
   ++ut_standard_calls;
}

void ODCALL pdef_od_onerow(BYTE operation)
{
   ut_onerow_operation = operation;
   ++ut_onerow_calls;
}

void ODCALL pdef_ra(BYTE operation)
{
   ut_ra_operation = operation;
   ++ut_ra_calls;
}

void ODCALL pdef_wildcat(BYTE operation) { (void)operation; UT_ASSERT(FALSE); }
void ODCALL pdef_pcboard(BYTE operation) { (void)operation; UT_ASSERT(FALSE); }

static void reset_selector(void)
{
   memset(&od_control, 0, sizeof(od_control));
   bODInitialized = TRUE;
   nCurrentPersonality = 255;
   pfCurrentPersonality = NULL;
   btCurrentStatusLine = 0;
   btOutputTop = 0;
   btOutputBottom = 0;
   ut_init_calls = 0;
   ut_entries = 0;
   ut_exits = 0;
   ut_status_calls = 0;
   ut_boundary_calls = 0;
   ut_expected_bottom = 23;
   ut_standard_calls = 0;
   ut_onerow_calls = 0;
   ut_ra_calls = 0;
   ut_standard_operation = 0;
   ut_onerow_operation = 0;
   ut_ra_operation = 0;
   ut_init_succeeds = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_selector(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT(!utt_od_set_personality("standard"));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void rejects_empty_and_unknown_names(void)
{
   reset_selector();
   bODInitialized = FALSE;
   UT_ASSERT(!utt_od_set_personality(""));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);

   reset_selector();
   UT_ASSERT(!utt_od_set_personality("missing"));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static void selects_first_personality_without_old_deinitialization(void)
{
   reset_selector();
   UT_ASSERT(utt_od_set_personality("standard"));
   UT_ASSERT_EQ_UINT(2, ut_status_calls);
   UT_ASSERT_EQ_INT(8, ut_status_values[0]);
   UT_ASSERT_EQ_INT(0, ut_status_values[1]);
   UT_ASSERT_EQ_UINT(1, ut_standard_calls);
   UT_ASSERT_EQ_INT(20, ut_standard_operation);
   UT_ASSERT_EQ_UINT(1, ut_boundary_calls);
   UT_ASSERT_EQ_INT(0, nCurrentPersonality);
   UT_ASSERT_EQ_PTR(aPersonalityInfo[0].pfPersonalityFunction,
      pfCurrentPersonality);
   UT_ASSERT_EQ_INT(255, btCurrentStatusLine);
   UT_ASSERT_EQ_INT(1, btOutputTop);
   UT_ASSERT_EQ_INT(23, btOutputBottom);
   UT_ASSERT_EQ_INT(-1, od_control.od_page_statusline);
}

static void leaves_current_personality_unchanged_on_repeat(void)
{
   reset_selector();
   nCurrentPersonality = 0;
   pfCurrentPersonality = aPersonalityInfo[0].pfPersonalityFunction;
   UT_ASSERT(utt_od_set_personality("STANDARD"));
   UT_ASSERT_EQ_UINT(0, ut_status_calls);
   UT_ASSERT_EQ_UINT(0, ut_standard_calls);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static void deinitializes_old_and_initializes_later_match(void)
{
   reset_selector();
   nCurrentPersonality = 0;
   pfCurrentPersonality = aPersonalityInfo[0].pfPersonalityFunction;
   UT_ASSERT(utt_od_set_personality("remoteaccess"));
   UT_ASSERT_EQ_UINT(1, ut_standard_calls);
   UT_ASSERT_EQ_INT(22, ut_standard_operation);
   UT_ASSERT_EQ_UINT(1, ut_ra_calls);
   UT_ASSERT_EQ_INT(20, ut_ra_operation);
   UT_ASSERT_EQ_INT(1, nCurrentPersonality);
   UT_ASSERT_EQ_PTR(aPersonalityInfo[1].pfPersonalityFunction,
      pfCurrentPersonality);
}

static void selects_the_one_row_builtin(void)
{
   reset_selector();
   ut_expected_bottom = 24;
   UT_ASSERT(utt_od_set_personality("od_onerow"));
   UT_ASSERT_EQ_UINT(2, ut_status_calls);
   UT_ASSERT_EQ_UINT(1, ut_onerow_calls);
   UT_ASSERT_EQ_INT(PEROP_INITIALIZE, ut_onerow_operation);
   UT_ASSERT_EQ_INT(4, nCurrentPersonality);
   UT_ASSERT_EQ_PTR(aPersonalityInfo[4].pfPersonalityFunction,
      pfCurrentPersonality);
   UT_ASSERT_EQ_INT(1, btOutputTop);
   UT_ASSERT_EQ_INT(24, btOutputBottom);
}

#ifdef ODPLAT_WIN32
static void rejects_gui_subsystem(void)
{
   reset_selector(); ut_subsystem = kODWindowsSubsystemGUI;
   UT_ASSERT(!utt_od_set_personality("standard"));
   UT_ASSERT_EQ_INT(ERR_UNSUPPORTED, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   ut_subsystem = kODWindowsSubsystemConsole;
}
#endif

static const UTTestCase ut_cases[] = {
   {"invalid names", rejects_empty_and_unknown_names},
   {"first selection", selects_first_personality_without_old_deinitialization},
   {"same selection", leaves_current_personality_unchanged_on_repeat},
   {"switch selection", deinitializes_old_and_initializes_later_match},
   {"one-row selection", selects_the_one_row_builtin},
#ifdef ODPLAT_WIN32
   {"GUI subsystem", rejects_gui_subsystem},
#endif
   {"terminal session", terminal_session_is_rejected}
};
#else
static void reports_unsupported_with_balanced_api_entry(void)
{
   ut_init_succeeds = TRUE;
   memset(&od_control, 0, sizeof(od_control));
   bODInitialized = FALSE;
   ut_init_calls = 0;
   ut_entries = 0;
   ut_exits = 0;
   UT_ASSERT(!utt_od_set_personality("anything"));
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_INT(ERR_UNSUPPORTED, od_control.od_error);

   bODInitialized = TRUE;
   od_control.od_error = 0;
   UT_ASSERT(!utt_od_set_personality("anything"));
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(2, ut_entries);
   UT_ASSERT_EQ_UINT(2, ut_exits);
   UT_ASSERT_EQ_INT(ERR_UNSUPPORTED, od_control.od_error);
}

static void terminal_session_is_rejected(void)
{
   memset(&od_control, 0, sizeof(od_control));
   bODInitialized = FALSE; ut_init_succeeds = FALSE;
   ut_entries = 0;
   UT_ASSERT(!utt_od_set_personality("anything"));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static const UTTestCase ut_cases[] = {
   {"unsupported", reports_unsupported_with_balanced_api_entry},
   {"terminal session", terminal_session_is_rejected}
};
#endif
