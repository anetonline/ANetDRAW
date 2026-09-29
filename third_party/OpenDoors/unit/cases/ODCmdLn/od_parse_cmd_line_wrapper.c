#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#define UT_CUSTOM_MOCK_ODCommandLineInterfaceMismatch
#define UT_CUSTOM_MOCK_ODParseCommandLineArguments
#define UT_CUSTOM_MOCK_od_split_cmd_line
#define UT_CUSTOM_MOCK_od_free_split_cmd_line

static tODWindowsSubsystem ut_subsystem;
static unsigned ut_mismatch_calls;
static unsigned ut_parse_calls;
static unsigned ut_free_calls;
static BOOL ut_split_succeeds;
static char *ut_arguments[] = {"door", "-LOCAL"};

tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{
   return(ut_subsystem);
}

void utm_ODCommandLineInterfaceMismatch(const char *function,
   tODWindowsSubsystem expected, tODWindowsSubsystem actual)
{
   UT_ASSERT_EQ_INT(0, strcmp("od_parse_cmd_line", function));
   UT_ASSERT_EQ_INT(kODWindowsSubsystemGUI, expected);
   UT_ASSERT_EQ_INT(ut_subsystem, actual);
   ++ut_mismatch_calls;
}

char ** ODCALL utm_od_split_cmd_line(const char *command, INT *count)
{
   UT_ASSERT_EQ_INT(0, strcmp("raw", command));
   if(!ut_split_succeeds) return(NULL);
   *count = 2;
   return(ut_arguments);
}

void utm_ODParseCommandLineArguments(INT count, char **arguments)
{
   UT_ASSERT_EQ_INT(2, count);
   UT_ASSERT_EQ_PTR(ut_arguments, arguments);
   ++ut_parse_calls;
}

void ODCALL utm_od_free_split_cmd_line(char **arguments)
{
   UT_ASSERT_EQ_PTR(ut_arguments, arguments);
   ++ut_free_calls;
}

static void reset_fixture(void)
{
   ut_subsystem = kODWindowsSubsystemGUI;
   ut_mismatch_calls = 0;
   ut_parse_calls = 0;
   ut_free_calls = 0;
   ut_split_succeeds = TRUE;
}

static void parses_and_releases_gui_arguments(void)
{
   reset_fixture();
   utt_od_parse_cmd_line("raw");
   UT_ASSERT_EQ_UINT(1, ut_parse_calls);
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
   UT_ASSERT_EQ_UINT(0, ut_mismatch_calls);
}

static void stops_when_splitting_fails(void)
{
   reset_fixture();
   ut_split_succeeds = FALSE;
   utt_od_parse_cmd_line("raw");
   UT_ASSERT_EQ_UINT(0, ut_parse_calls);
   UT_ASSERT_EQ_UINT(0, ut_free_calls);
}

static void diagnoses_console_executable(void)
{
   reset_fixture();
   ut_subsystem = kODWindowsSubsystemConsole;
   utt_od_parse_cmd_line("raw");
   UT_ASSERT_EQ_UINT(1, ut_mismatch_calls);
}

static const UTTestCase ut_cases[] = {
   {"GUI arguments", parses_and_releases_gui_arguments},
   {"split failure", stops_when_splitting_fails},
   {"subsystem mismatch", diagnoses_console_executable}
};
#else
#define UT_CUSTOM_MOCK_ODParseCommandLineArguments
static unsigned ut_parse_calls;
void utm_ODParseCommandLineArguments(INT count, char **arguments)
{
   UT_ASSERT_EQ_INT(1, count); UT_ASSERT_NOT_NULL(arguments); ++ut_parse_calls;
}
static void forwards_arguments(void)
{
   char *arguments[] = {"door"};
   ut_parse_calls = 0; utt_od_parse_cmd_line(1, arguments);
   UT_ASSERT_EQ_UINT(1, ut_parse_calls);
}
static const UTTestCase ut_cases[] = {{"arguments", forwards_arguments}};
#endif
