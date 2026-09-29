#include <setjmp.h>
#include <stdarg.h>

#define UT_CUSTOM_MOCK_OutputDebugStringA
#define UT_CUSTOM_MOCK___acrt_iob_func
#define UT_CUSTOM_MOCK_exit
#define UT_CUSTOM_MOCK_fputs
#define UT_CUSTOM_MOCK_sprintf

static jmp_buf ut_exit_target;
static const char *ut_expected_function;
static const char *ut_expected_interface;
static const char *ut_expected_actual;
static const char *ut_expected_replacement;
static unsigned ut_debug_calls;
static unsigned ut_stderr_calls;
static FILE *ut_stderr_stream = (FILE *)(void *)&ut_stderr_calls;

FILE *utm___acrt_iob_func(unsigned int index)
{
   UT_ASSERT_EQ_UINT(2, index);
   return(ut_stderr_stream);
}

int utm_sprintf(char *destination, const char *format, ...)
{
   va_list arguments;
   const char *function;
   const char *interface_name;
   const char *actual;
   const char *replacement;
   const char *target;

   UT_ASSERT_NOT_NULL(format);
   va_start(arguments, format);
   function = va_arg(arguments, const char *);
   interface_name = va_arg(arguments, const char *);
   actual = va_arg(arguments, const char *);
   replacement = va_arg(arguments, const char *);
   target = va_arg(arguments, const char *);
   va_end(arguments);
   UT_ASSERT_EQ_INT(0, strcmp(ut_expected_function, function));
   UT_ASSERT_EQ_INT(0, strcmp(ut_expected_interface, interface_name));
   UT_ASSERT_EQ_INT(0, strcmp(ut_expected_actual, actual));
   UT_ASSERT_EQ_INT(0, strcmp(ut_expected_replacement, replacement));
   UT_ASSERT_NOT_NULL(target);
   strcpy(destination, "diagnostic");
   return(10);
}

int utm_fputs(const char *text, FILE *stream)
{
   UT_ASSERT_EQ_INT(0, strcmp("diagnostic", text));
   UT_ASSERT_EQ_PTR(ut_stderr_stream, stream);
   ++ut_stderr_calls;
   return(0);
}

void WINAPI utm_OutputDebugStringA(LPCSTR text)
{
   UT_ASSERT_EQ_INT(0, strcmp("diagnostic", text));
   ++ut_debug_calls;
}

void utm_exit(int status)
{
   UT_ASSERT_EQ_INT(EXIT_FAILURE, status);
   longjmp(ut_exit_target, 1);
}

static void invoke_mismatch(tODWindowsSubsystem expected,
   tODWindowsSubsystem actual)
{
   memset(&od_control, 0, sizeof(od_control));
   ut_debug_calls = ut_stderr_calls = 0;
   if(setjmp(ut_exit_target) == 0)
      utt_ODCommandLineInterfaceMismatch(ut_expected_function,
         expected, actual);
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_debug_calls);
   UT_ASSERT_EQ_UINT(1, ut_stderr_calls);
}

static void reports_gui_parser_in_console(void)
{
   ut_expected_function = "od_parse_cmd_line";
   ut_expected_interface = "Windows GUI";
   ut_expected_actual = "Windows console";
   ut_expected_replacement = "od_parse_cmd_line_cons(argc, argv)";
   invoke_mismatch(kODWindowsSubsystemGUI, kODWindowsSubsystemConsole);
}

static void reports_console_parser_in_gui(void)
{
   ut_expected_function = "od_parse_cmd_line_cons";
   ut_expected_interface = "Windows console";
   ut_expected_actual = "Windows GUI";
   ut_expected_replacement = "od_parse_cmd_line(command_line)";
   invoke_mismatch(kODWindowsSubsystemConsole, kODWindowsSubsystemGUI);
}

static void describes_unknown_subsystem(void)
{
   ut_expected_function = "od_parse_cmd_line_cons";
   ut_expected_interface = "Windows console";
   ut_expected_actual = "an unknown";
   ut_expected_replacement = "od_parse_cmd_line(command_line)";
   invoke_mismatch(kODWindowsSubsystemConsole, kODWindowsSubsystemUnknown);
}

static const UTTestCase ut_cases[] = {
   {"GUI parser mismatch", reports_gui_parser_in_console},
   {"console parser mismatch", reports_console_parser_in_gui},
   {"unknown subsystem", describes_unknown_subsystem}
};
