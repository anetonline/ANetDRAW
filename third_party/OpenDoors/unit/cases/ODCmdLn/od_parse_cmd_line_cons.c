#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#define UT_CUSTOM_MOCK_ODCommandLineInterfaceMismatch
#define UT_CUSTOM_MOCK_ODParseCommandLineArguments

static tODWindowsSubsystem ut_subsystem;
static unsigned ut_mismatch_calls;
static unsigned ut_parse_calls;

tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{
   return(ut_subsystem);
}

void utm_ODCommandLineInterfaceMismatch(const char *function,
   tODWindowsSubsystem expected, tODWindowsSubsystem actual)
{
   UT_ASSERT_EQ_INT(0, strcmp("od_parse_cmd_line_cons", function));
   UT_ASSERT_EQ_INT(kODWindowsSubsystemConsole, expected);
   UT_ASSERT_EQ_INT(ut_subsystem, actual);
   ++ut_mismatch_calls;
}

void utm_ODParseCommandLineArguments(INT count, char **arguments)
{
   UT_ASSERT_EQ_INT(1, count);
   UT_ASSERT_NOT_NULL(arguments);
   ++ut_parse_calls;
}

static void selects_console_argument_parser(void)
{
   char *arguments[] = {"door"};
   ut_subsystem = kODWindowsSubsystemConsole;
   ut_mismatch_calls = ut_parse_calls = 0;
   utt_od_parse_cmd_line_cons(1, arguments);
   UT_ASSERT_EQ_UINT(1, ut_parse_calls);
   UT_ASSERT_EQ_UINT(0, ut_mismatch_calls);
}

static void diagnoses_gui_executable(void)
{
   char *arguments[] = {"door"};
   ut_subsystem = kODWindowsSubsystemGUI;
   ut_mismatch_calls = ut_parse_calls = 0;
   utt_od_parse_cmd_line_cons(1, arguments);
   UT_ASSERT_EQ_UINT(1, ut_mismatch_calls);
   UT_ASSERT_EQ_UINT(1, ut_parse_calls);
}

static const UTTestCase ut_cases[] = {
   {"console arguments", selects_console_argument_parser},
   {"subsystem mismatch", diagnoses_gui_executable}
};
