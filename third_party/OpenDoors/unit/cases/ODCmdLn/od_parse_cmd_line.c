#include <setjmp.h>
#include <stdarg.h>

#define UT_CUSTOM_MOCK_MessageBoxA
#define UT_CUSTOM_MOCK_ODAdvanceToNextArg
#define UT_CUSTOM_MOCK_ODGetCommandLineParameter
#define UT_CUSTOM_MOCK_ODGetNextArgName
#define UT_CUSTOM_MOCK_ODParseOpenHandle
#define UT_CUSTOM_MOCK_atoi
#define UT_CUSTOM_MOCK_atol
#define UT_CUSTOM_MOCK_exit
#define UT_CUSTOM_MOCK_fputs
#define UT_CUSTOM_MOCK_od_free_split_cmd_line
#define UT_CUSTOM_MOCK_od_split_cmd_line
#define UT_CUSTOM_MOCK_puts
#define UT_CUSTOM_MOCK_sprintf
#define UT_CUSTOM_MOCK_strcasecmp
#define UT_CUSTOM_MOCK_stricmp
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_strncasecmp
#define UT_CUSTOM_MOCK_strnicmp
#define UT_CUSTOM_MOCK_strncpy
#define UT_CUSTOM_MOCK_strtol
#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed

#ifdef ODPLAT_DOS32
#define UT_CALLBACK ODCALL
#else
#define UT_CALLBACK
#endif

static jmp_buf ut_exit_target;
static char **ut_windows_arguments;
static INT ut_windows_argument_count;
static BOOL ut_split_fails;
static BOOL ut_parse_handle_result;
static DWORD_PTR ut_parse_handle_value;
static BOOL ut_special_n_value;
static unsigned ut_help_callback_count;
static unsigned ut_help_output_count;
static unsigned ut_message_count;
static unsigned ut_free_split_count;
static unsigned ut_flag_count;
static unsigned ut_handler_count;
static BOOL ut_flag_result;
static char ut_handler_keyword[32];
static char ut_handler_options[80];
static BOOL ut_public_call_allowed;

BOOL utm_ODSyncPublicCallAllowed(void)
{
   if(!ut_public_call_allowed)
      od_control.od_error = ERR_GENERALFAILURE;
   return ut_public_call_allowed;
}

static int ut_casecmp(const char *left, const char *right, size_t maximum)
{
   size_t index = 0;
   while(index < maximum)
   {
      unsigned char l = (unsigned char)left[index];
      unsigned char r = (unsigned char)right[index];
      if(l >= 'a' && l <= 'z') l = (unsigned char)(l - ('a' - 'A'));
      if(r >= 'a' && r <= 'z') r = (unsigned char)(r - ('a' - 'A'));
      if(l != r || l == 0) return (int)l - (int)r;
      ++index;
   }
   return 0;
}

static long ut_decimal(const char *text)
{
   long value = 0;
   while(*text >= '0' && *text <= '9')
   {
      value = value * 10 + (*text - '0');
      ++text;
   }
   return value;
}

static tCommandLineParameter ut_parameter(const char *argument)
{
   static const struct {
      const char *name;
      tCommandLineParameter value;
   } parameters[] = {
      {"CONFIG", kParamConfigFile}, {"LOCAL", kParamLocal},
      {"BPS", kParamBPS}, {"PORT", kParamPort}, {"NODE", kParamNode},
      {"HELP", kParamHelp}, {"PERSONALITY", kParamPersonality},
      {"MAXTIME", kParamMaxTime}, {"ADDRESS", kParamAddress},
      {"IRQ", kParamIRQ}, {"NOFOSSIL", kParamNoFOSSIL},
      {"NOFIFO", kParamNoFIFO}, {"DROPFILE", kParamDropFile},
      {"USERNAME", kParamUserName}, {"TIMELEFT", kParamTimeLeft},
      {"SECURITY", kParamSecurity}, {"LOCATION", kParamLocation},
      {"GRAPHICS", kParamGraphics}, {"BBSNAME", kParamBBSName},
      {"HANDLE", kParamPortHandle}, {"SOCKET", kParamSocketDescriptor},
      {"SILENT", kParamSilentMode}, {"CP437UTF8", kParamCP436UTF8}
   };
   unsigned index;
   if(ut_casecmp(argument, "-INVALID", (size_t)-1) == 0)
      return (tCommandLineParameter)99;
   if(argument[0] != '-' && argument[0] != '/') return kParamOption;
   ++argument;
   for(index = 0; index < sizeof(parameters) / sizeof(parameters[0]); ++index)
   {
      if(ut_casecmp(argument, parameters[index].name, (size_t)-1) == 0)
         return parameters[index].value;
   }
   return kParamUnknown;
}

void utm_ODAdvanceToNextArg(INT *current, INT count, char *option)
{
   (void)option;
   UT_ASSERT(*current + 1 < count);
   ++*current;
}

tCommandLineParameter utm_ODGetCommandLineParameter(char *argument)
{
   return ut_parameter(argument);
}

void utm_ODGetNextArgName(INT *current, INT count, char **arguments,
   char *result, size_t size)
{
   size_t index = 0;
   UT_ASSERT(*current + 1 < count);
   ++*current;
   while(arguments[*current][index] != '\0' && index + 1 < size)
   {
      result[index] = arguments[*current][index];
      ++index;
   }
   result[index] = '\0';
}

BOOL utm_ODParseOpenHandle(const char *text, DWORD_PTR *value)
{
   (void)text;
   if(ut_parse_handle_result) *value = ut_parse_handle_value;
   return ut_parse_handle_result;
}

int utm_atoi(const char *text)
{
   if(ut_special_n_value && ut_casecmp(text, "N", (size_t)-1) == 0)
      return 1;
   return (int)ut_decimal(text);
}

long utm_atol(const char *text)
{
   return ut_decimal(text);
}

long utm_strtol(const char *text, char **end, int base)
{
   long value = 0;
   (void)end;
   UT_ASSERT_EQ_INT(16, base);
   while(*text != '\0')
   {
      int digit = *text >= '0' && *text <= '9' ? *text - '0' :
         (*text >= 'a' && *text <= 'f' ? *text - 'a' + 10 : 0);
      value = value * 16 + digit;
      ++text;
   }
   return value;
}

char *utm_strncpy(char *destination, const char *source, size_t count)
{
   size_t index = 0;
   while(index < count && source[index] != '\0')
   {
      destination[index] = source[index];
      ++index;
   }
   while(index < count) destination[index++] = '\0';
   return destination;
}

#ifdef ODPLAT_NIX
int utm_strcasecmp(const char *left, const char *right)
{
   return ut_casecmp(left, right, (size_t)-1);
}
int utm_strncasecmp(const char *left, const char *right, size_t count)
{
   return ut_casecmp(left, right, count);
}
#else
int utm_stricmp(const char *left, const char *right)
{
   return ut_casecmp(left, right, (size_t)-1);
}
int utm_strnicmp(const char *left, const char *right, size_t count)
{
   return ut_casecmp(left, right, count);
}
#endif

void utm_exit(int status)
{
   UT_ASSERT(status == 0 || status == 1);
   longjmp(ut_exit_target, status + 1);
}

#ifndef ODPLAT_WIN32
int utm_puts(const char *text)
{
   UT_ASSERT_NOT_NULL(text);
   ++ut_help_output_count;
   return 0;
}
int utm_fputs(const char *text, FILE *stream)
{
   UT_ASSERT_NOT_NULL(text);
   (void)stream;
   ++ut_help_output_count;
   return 0;
}
#endif

#ifdef ODPLAT_WIN32
char ** ODCALL utm_od_split_cmd_line(const char *command, INT *count)
{
   (void)command;
   if(ut_split_fails) return NULL;
   *count = ut_windows_argument_count;
   return ut_windows_arguments;
}
void ODCALL utm_od_free_split_cmd_line(char **arguments)
{
   UT_ASSERT_EQ_PTR(ut_windows_arguments, arguments);
   ++ut_free_split_count;
}
int WINAPI utm_MessageBoxA(HWND window, LPCSTR text, LPCSTR title, UINT type)
{
   UT_ASSERT_NULL(window);
   UT_ASSERT_NOT_NULL(text);
   UT_ASSERT_NOT_NULL(title);
   UT_ASSERT_EQ_UINT(MB_ICONINFORMATION | MB_OK, type);
   ++ut_message_count;
   return IDOK;
}
int utm_sprintf(char *destination, const char *format, ...)
{
   (void)format;
   strcpy(destination, "Command Line Options");
   return (int)utm_strlen(destination);
}
size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}
#endif

static void UT_CALLBACK ut_help_callback(void)
{
   ++ut_help_callback_count;
}

static BOOL UT_CALLBACK ut_flag_callback(const char *keyword)
{
   UT_ASSERT_EQ_INT(0, strcmp("-UNKNOWN", keyword));
   ++ut_flag_count;
   return ut_flag_result;
}

static void UT_CALLBACK ut_command_callback(char *keyword, char *options)
{
   ++ut_handler_count;
   strcpy(ut_handler_keyword, keyword);
   strcpy(ut_handler_options, options);
}

static void reset_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
   bParsedCmdLine = FALSE;
   nForcedPort = 0;
   wPreSetInfo = 0;
   ut_split_fails = FALSE;
   ut_parse_handle_result = TRUE;
   ut_parse_handle_value = 123;
   ut_special_n_value = FALSE;
   ut_help_callback_count = 0;
   ut_help_output_count = 0;
   ut_message_count = 0;
   ut_free_split_count = 0;
   ut_flag_count = 0;
   ut_handler_count = 0;
   ut_flag_result = FALSE;
   ut_handler_keyword[0] = '\0';
   ut_handler_options[0] = '\0';
   ut_public_call_allowed = TRUE;
}

static void invoke_parser(INT count, char **arguments)
{
   utt_ODParseCommandLineArguments(count, arguments);
}

static void parses_standard_value_and_flag_options(void)
{
   char *arguments[] = {
      "door", "-CONFIG", "custom.cfg", "-LOCAL", "-BPS", "57600",
      "-PORT", "COM2", "-NODE", "7", "-MAXTIME", "30",
      "-ADDRESS", "3f8", "-IRQ", "4", "-NOFOSSIL", "-NOFIFO",
      "-DROPFILE", "drop/path", "-USERNAME", "Ada", "-TIMELEFT", "20",
      "-SECURITY", "100", "-LOCATION", "Earth", "-GRAPHICS", "1",
      "-BBSNAME", "MyBBS", "-HANDLE", "123", "-SILENT",
      "-PERSONALITY", "ignored", "plain", "-CP437UTF8"
   };
   reset_fixture();
   invoke_parser((INT)(sizeof(arguments) / sizeof(arguments[0])), arguments);
   UT_ASSERT_EQ_INT(TRUE, bParsedCmdLine);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_force_local);
   UT_ASSERT_EQ_UINT(57600, od_control.baud);
   UT_ASSERT_EQ_INT(1, od_control.port);
   UT_ASSERT_EQ_UINT(PRESET_BPS | PRESET_PORT, wPreSetInfo);
   UT_ASSERT_EQ_INT(7, od_control.od_node);
   UT_ASSERT_EQ_INT(30, od_control.od_maxtime);
   UT_ASSERT_EQ_UINT(0x3f8, od_control.od_com_address);
   UT_ASSERT_EQ_INT(4, od_control.od_com_irq);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_no_fossil);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_com_no_fifo);
   UT_ASSERT_EQ_INT(0, strcmp("drop/path", od_control.info_path));
   UT_ASSERT_EQ_INT(0, strcmp("Ada", od_control.user_name));
   UT_ASSERT_EQ_INT(20, od_control.user_timelimit);
   UT_ASSERT_EQ_INT(100, od_control.user_security);
   UT_ASSERT_EQ_INT(0, strcmp("Earth", od_control.user_location));
   UT_ASSERT_EQ_INT(TRUE, od_control.user_ansi);
   UT_ASSERT_EQ_INT(0, strcmp("MyBBS", od_control.system_name));
   UT_ASSERT_EQ_UINT(123, od_control.od_open_handle);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_silent_mode);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_cp437_to_utf8_out);
   UT_ASSERT_EQ_INT(0, strcmp("custom.cfg", od_control.od_config_filename));
}

static void covers_port_graphics_handle_and_socket_alternatives(void)
{
   char *numeric_port[] = {"door", "-PORT", "0"};
   char *graphics_missing[] = {"door", "-GRAPHICS"};
   char *graphics_zero[] = {"door", "-GRAPHICS", "0"};
   char *graphics_n[] = {"door", "-GRAPHICS", "N"};
   char *bad_handle[] = {"door", "-HANDLE", "bad"};
   char *socket[] = {"door", "-SOCKET", "456"};
   reset_fixture();
   invoke_parser(3, numeric_port);
   UT_ASSERT_EQ_INT(0, od_control.port);
   reset_fixture();
   invoke_parser(2, graphics_missing);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_ansi);
   reset_fixture();
   invoke_parser(3, graphics_zero);
   UT_ASSERT_EQ_INT(FALSE, od_control.user_ansi);
   reset_fixture();
   ut_special_n_value = TRUE;
   invoke_parser(3, graphics_n);
   UT_ASSERT_EQ_INT(FALSE, od_control.user_ansi);
   reset_fixture();
   ut_parse_handle_result = FALSE;
   invoke_parser(3, bad_handle);
#if defined(ODPLAT_WIN32) || defined(ODPLAT_NIX)
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
#endif
   reset_fixture();
   ut_parse_handle_value = 456;
   invoke_parser(3, socket);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_use_socket);
   UT_ASSERT_EQ_UINT(456, od_control.od_open_handle);
}

static void dispatches_unknown_options_to_callbacks(void)
{
   char *arguments[] = {"door", "-UNKNOWN", "value"};
   char *invalid[] = {"door", "-INVALID"};
   reset_fixture();
   od_control.od_cmd_line_flag_handler = ut_flag_callback;
   ut_flag_result = TRUE;
   invoke_parser(3, arguments);
   UT_ASSERT_EQ_UINT(1, ut_flag_count);
   UT_ASSERT_EQ_UINT(0, ut_handler_count);

   reset_fixture();
   od_control.od_cmd_line_flag_handler = ut_flag_callback;
   od_control.od_cmd_line_handler = ut_command_callback;
   invoke_parser(3, arguments);
   UT_ASSERT_EQ_UINT(1, ut_flag_count);
   UT_ASSERT_EQ_UINT(1, ut_handler_count);
   UT_ASSERT_EQ_INT(0, strcmp("-UNKNOWN", ut_handler_keyword));
   UT_ASSERT_EQ_INT(0, strcmp("value", ut_handler_options));

   reset_fixture();
   invoke_parser(2, arguments);
   UT_ASSERT_EQ_UINT(0, ut_flag_count);
   UT_ASSERT_EQ_UINT(0, ut_handler_count);

   reset_fixture();
   invoke_parser(2, invalid);
}

static void help_callback_and_builtin_help_exit(void)
{
   char *arguments[] = {"door", "-HELP"};
   reset_fixture();
   od_control.od_cmd_line_help_func = ut_help_callback;
   if(setjmp(ut_exit_target) == 0) invoke_parser(2, arguments);
   UT_ASSERT_EQ_UINT(1, ut_help_callback_count);

   reset_fixture();
   od_control.od_cmd_line_help = "custom help";
   strcpy(od_control.od_prog_name, "Door");
   if(setjmp(ut_exit_target) == 0) invoke_parser(2, arguments);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(1, ut_message_count);
#else
   UT_ASSERT_EQ_UINT(2, ut_help_output_count);
#endif

   reset_fixture();
   if(setjmp(ut_exit_target) == 0) invoke_parser(2, arguments);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(1, ut_message_count);
#else
   UT_ASSERT(ut_help_output_count > 2);
#endif
}

static void rejects_missing_argument_vector_or_failed_split(void)
{
   reset_fixture();
   utt_ODParseCommandLineArguments(0, NULL);
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_INT(FALSE, bParsedCmdLine);
}

static void rejects_a_terminal_session(void)
{
   char *arguments[] = {"door"};
   reset_fixture();
   ut_public_call_allowed = FALSE;
   invoke_parser(1, arguments);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_INT(FALSE, bParsedCmdLine);
}

static const UTTestCase ut_cases[] = {
   {"terminal session", rejects_a_terminal_session},
   {"standard options", parses_standard_value_and_flag_options},
   {"option alternatives", covers_port_graphics_handle_and_socket_alternatives},
   {"unknown callbacks", dispatches_unknown_options_to_callbacks},
   {"help", help_callback_and_builtin_help_exit},
   {"invalid argument vector", rejects_missing_argument_vector_or_failed_split}
};
