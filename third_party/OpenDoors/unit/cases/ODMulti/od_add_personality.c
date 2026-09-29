#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed
#ifdef ODPLAT_WIN32
static void ODPersonalityOpenDoors(BYTE operation) { (void)operation; }
static void ODPersonalityOneRow(BYTE operation) { (void)operation; }
static void ODPersonalityPCBoard(BYTE operation) { (void)operation; }
static void ODPersonalityRemoteAccess(BYTE operation) { (void)operation; }
static void ODPersonalityWildcat(BYTE operation) { (void)operation; }
#endif
static BOOL ut_public_call_allowed = TRUE;

BOOL utm_ODSyncPublicCallAllowed(void)
{
   if(!ut_public_call_allowed) od_control.od_error = ERR_GENERALFAILURE;
   return(ut_public_call_allowed);
}

static void rejects_a_terminal_session(void)
{
   ut_public_call_allowed = FALSE;
   UT_ASSERT(!utt_od_add_personality("name", 1, 23, NULL));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   ut_public_call_allowed = TRUE;
}

#ifdef OD_PERSONALITY_SUPPORT
#define UT_CUSTOM_MOCK_strncpy
#define UT_CUSTOM_MOCK_strupr
#define UT_CUSTOM_MOCK_ODMultiResolvePersonality
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#endif

#include <string.h>

static unsigned ut_copy_calls;
static unsigned ut_upper_calls;

#ifdef ODPLAT_DOS32
#define UT_PERSONALITY_CALL ODCALL
#else
#define UT_PERSONALITY_CALL
#endif

void ODCALL pdef_opendoors(BYTE operation) { (void)operation; }
void ODCALL pdef_od_onerow(BYTE operation) { (void)operation; }
void ODCALL pdef_ra(BYTE operation) { (void)operation; }
void ODCALL pdef_wildcat(BYTE operation) { (void)operation; }
void ODCALL pdef_pcboard(BYTE operation) { (void)operation; }

char *utm_strncpy(char *destination, const char *source, size_t count)
{
   size_t index;
   UT_ASSERT_EQ_UINT(32, count);
   for(index = 0; index < count && source[index] != '\0'; ++index)
      destination[index] = source[index];
   while(index < count) destination[index++] = '\0';
   ++ut_copy_calls;
   return destination;
}

char *utm_strupr(char *text)
{
   char *position;
   for(position = text; *position != '\0'; ++position)
      if(*position >= 'a' && *position <= 'z')
         *position = (char)(*position - 'a' + 'A');
   ++ut_upper_calls;
   return text;
}

static void UT_PERSONALITY_CALL ut_personality(BYTE operation)
{
   (void)operation;
}

OD_PERSONALITY_CALLBACK *utm_ODMultiResolvePersonality(
   OD_PERSONALITY_PROC *personality)
{ return((OD_PERSONALITY_CALLBACK *)personality); }

#ifdef ODPLAT_WIN32
static tODWindowsSubsystem ut_subsystem = kODWindowsSubsystemConsole;
tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void) { return(ut_subsystem); }
#endif

static void stores_uppercase_truncated_metadata(void)
{
   const char name[] = "abcdefghijklmnopqrstuvwxyz123456789";
   UT_ASSERT(utt_od_add_personality(name, 3, 22, ut_personality));
   UT_ASSERT_EQ_INT(6, nPersonalities);
   UT_ASSERT_EQ_INT(0, strcmp("ABCDEFGHIJKLMNOPQRSTUVWXYZ123456",
      aPersonalityInfo[5].szName));
   UT_ASSERT_EQ_INT('\0', aPersonalityInfo[5].szName[32]);
   UT_ASSERT_EQ_INT(3, aPersonalityInfo[5].nStatusTopLine);
   UT_ASSERT_EQ_INT(22, aPersonalityInfo[5].nStatusBottomLine);
   UT_ASSERT_EQ_PTR(ut_personality,
      aPersonalityInfo[5].pfPersonalityFunction);
   UT_ASSERT_EQ_UINT(1, ut_copy_calls);
   UT_ASSERT_EQ_UINT(1, ut_upper_calls);
}

static void rejects_the_thirteenth_personality(void)
{
   unsigned index;
   for(index = 5; index < MAX_PERSONALITIES; ++index)
      UT_ASSERT(utt_od_add_personality("extra", 1, 23, ut_personality));
   UT_ASSERT_EQ_INT(MAX_PERSONALITIES, nPersonalities);
   UT_ASSERT(!utt_od_add_personality("overflow", 1, 23,
      ut_personality));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);
}

#ifdef ODPLAT_WIN32
static void rejects_gui_subsystem(void)
{
   memset(&od_control, 0, sizeof(od_control));
   ut_subsystem = kODWindowsSubsystemGUI;
   UT_ASSERT(!utt_od_add_personality("name", 1, 23, ut_personality));
   UT_ASSERT_EQ_INT(ERR_UNSUPPORTED, od_control.od_error);
   ut_subsystem = kODWindowsSubsystemConsole;
}
#endif

static const UTTestCase ut_cases[] = {
   {"stored metadata", stores_uppercase_truncated_metadata},
   {"capacity", rejects_the_thirteenth_personality},
#ifdef ODPLAT_WIN32
   {"GUI subsystem", rejects_gui_subsystem},
#endif
   {"terminal session", rejects_a_terminal_session}
};
#else
static void reports_unsupported_without_a_text_screen(void)
{
   memset(&od_control, 0, sizeof(od_control));
   UT_ASSERT(!utt_od_add_personality("name", 1, 23, NULL));
   UT_ASSERT_EQ_INT(ERR_UNSUPPORTED, od_control.od_error);
}

static const UTTestCase ut_cases[] = {
   {"unsupported", reports_unsupported_without_a_text_screen},
   {"terminal session", rejects_a_terminal_session}
};
#endif
