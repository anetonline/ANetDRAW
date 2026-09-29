#include <stdarg.h>

#define UT_CUSTOM_MOCK_strlen
#ifdef ODPLAT_NIX
#define UT_CUSTOM_MOCK_fwrite
#endif
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK_printf
#endif
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_MessageBoxA
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#define UT_CUSTOM_MOCK___acrt_iob_func
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_fprintf
#define UT_CUSTOM_MOCK_fputs
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_sprintf
#endif

static unsigned ut_length_calls;

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   ++ut_length_calls;
   while(text[length] != '\0') ++length;
   return length;
}

#ifdef ODPLAT_NIX
static unsigned ut_write_calls;
static const void *ut_written_text;
static size_t ut_written_size;

size_t utm_fwrite(const void *buffer, size_t size, size_t count, FILE *stream)
{
   (void)stream;
   ++ut_write_calls;
   ut_written_text = buffer;
   ut_written_size = size;
   UT_ASSERT_EQ_UINT(1, count);
   return count;
}

static void writes_the_exact_error_to_standard_error(void)
{
   char message[] = "Initialization failed";
   ut_length_calls = ut_write_calls = 0;
   ut_written_text = NULL;
   ut_written_size = 0;
   utt_ODInitError(message);
   UT_ASSERT_EQ_UINT(1, ut_length_calls);
   UT_ASSERT_EQ_UINT(1, ut_write_calls);
   UT_ASSERT_EQ_PTR(message, ut_written_text);
   UT_ASSERT_EQ_UINT(sizeof(message) - 1, ut_written_size);
}
#endif

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
static unsigned ut_print_calls;
static const char *ut_print_formats[2];

int utm_printf(const char *format, ...)
{
   va_list arguments;
   UT_ASSERT(ut_print_calls < 2);
   ut_print_formats[ut_print_calls++] = format;
   va_start(arguments, format);
   if(ut_print_calls == 1)
   {
      UT_ASSERT_EQ_PTR(od_control.od_prog_name, va_arg(arguments, char *));
      UT_ASSERT(strcmp("Initialization failed",
         va_arg(arguments, char *)) == 0);
   }
   va_end(arguments);
   return 0;
}

static void prints_command_help_only_after_parsing(void)
{
   ut_print_calls = 0;
   bParsedCmdLine = FALSE;
   utt_ODInitError("Initialization failed");
   UT_ASSERT_EQ_UINT(1, ut_print_calls);

   ut_print_calls = 0;
   bParsedCmdLine = TRUE;
   utt_ODInitError("Initialization failed");
   UT_ASSERT_EQ_UINT(2, ut_print_calls);
   UT_ASSERT(strstr(ut_print_formats[1], "-HELP") != NULL);
}
#endif

#ifdef ODPLAT_WIN32
static char ut_message_storage[256];
static BOOL ut_malloc_fails;
static unsigned ut_malloc_calls;
static unsigned ut_free_calls;
static unsigned ut_format_calls;
static unsigned ut_message_calls;
static const char *ut_message_text;
static tODWindowsSubsystem ut_subsystem;
static unsigned ut_console_error_calls;
static unsigned ut_console_help_calls;
static FILE *ut_stderr_stream = (FILE *)(void *)&ut_console_error_calls;

FILE *utm___acrt_iob_func(unsigned int index)
{
   UT_ASSERT_EQ_UINT(2, index);
   return(ut_stderr_stream);
}

tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{
   return(ut_subsystem);
}

int utm_fprintf(FILE *stream, const char *format, ...)
{
   va_list arguments;
   va_start(arguments, format);
   UT_ASSERT_EQ_PTR(ut_stderr_stream, stream);
   UT_ASSERT_EQ_INT(0, strcmp("%s: %s\n", format));
   UT_ASSERT_EQ_PTR(od_control.od_prog_name, va_arg(arguments, char *));
   UT_ASSERT_EQ_INT(0, strcmp("Initialization failed",
      va_arg(arguments, char *)));
   va_end(arguments);
   ++ut_console_error_calls;
   return(0);
}

int utm_fputs(const char *text, FILE *stream)
{
   UT_ASSERT_EQ_PTR(ut_stderr_stream, stream);
   UT_ASSERT(strstr(text, "-HELP") != NULL);
   ++ut_console_help_calls;
   return(0);
}

void *utm_malloc(size_t size)
{
   ++ut_malloc_calls;
   UT_ASSERT_EQ_UINT(utm_strlen("Initialization failed") + 80, size);
   return ut_malloc_fails ? NULL : ut_message_storage;
}

void utm_free(void *memory)
{
   ++ut_free_calls;
   UT_ASSERT_EQ_PTR(ut_message_storage, memory);
}

int utm_sprintf(char *output, const char *format, ...)
{
   const char *source;
   va_list arguments;
   char *current = output;
   ++ut_format_calls;
   UT_ASSERT_EQ_PTR(ut_message_storage, output);
   va_start(arguments, format);
   source = va_arg(arguments, const char *);
   va_end(arguments);
   while(*source != '\0') *current++ = *source++;
   source = "\nUse the -HELP command line option for help, or -LOCAL for local mode.";
   while(*source != '\0') *current++ = *source++;
   *current = '\0';
   return (int)(current - output);
}

int WINAPI utm_MessageBoxA(HWND window, LPCSTR text, LPCSTR caption,
   UINT type)
{
   (void)window;
   ++ut_message_calls;
   ut_message_text = text;
   UT_ASSERT_EQ_PTR(od_control.od_prog_name, caption);
   UT_ASSERT_EQ_UINT(MB_ICONSTOP | MB_OK | MB_TASKMODAL, type);
   return IDOK;
}

static void reset_windows_error(void)
{
   ut_malloc_fails = FALSE;
   ut_malloc_calls = 0;
   ut_free_calls = 0;
   ut_format_calls = 0;
   ut_message_calls = 0;
   ut_message_text = NULL;
   ut_length_calls = 0;
   ut_subsystem = kODWindowsSubsystemGUI;
   ut_console_error_calls = ut_console_help_calls = 0;
   memset(ut_message_storage, 0, sizeof(ut_message_storage));
}

static void displays_the_original_message_without_parsed_arguments(void)
{
   char message[] = "Initialization failed";
   reset_windows_error();
   bParsedCmdLine = FALSE;
   utt_ODInitError(message);
   UT_ASSERT_EQ_UINT(0, ut_malloc_calls);
   UT_ASSERT_EQ_UINT(1, ut_message_calls);
   UT_ASSERT_EQ_PTR(message, ut_message_text);
}

static void allocation_failure_falls_back_to_the_original_message(void)
{
   char message[] = "Initialization failed";
   reset_windows_error();
   bParsedCmdLine = TRUE;
   ut_malloc_fails = TRUE;
   utt_ODInitError(message);
   UT_ASSERT_EQ_UINT(1, ut_malloc_calls);
   UT_ASSERT_EQ_UINT(1, ut_message_calls);
   UT_ASSERT_EQ_PTR(message, ut_message_text);
   UT_ASSERT_EQ_UINT(0, ut_free_calls);
}

static void appends_help_to_an_allocated_message_and_releases_it(void)
{
   reset_windows_error();
   bParsedCmdLine = TRUE;
   utt_ODInitError("Initialization failed");
   UT_ASSERT_EQ_UINT(1, ut_malloc_calls);
   UT_ASSERT_EQ_UINT(1, ut_format_calls);
   UT_ASSERT_EQ_UINT(1, ut_message_calls);
   UT_ASSERT_EQ_PTR(ut_message_storage, ut_message_text);
   UT_ASSERT(strstr(ut_message_storage, "-HELP") != NULL);
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
}

static void writes_console_errors_and_optional_help(void)
{
   reset_windows_error();
   ut_subsystem = kODWindowsSubsystemConsole;
   bParsedCmdLine = FALSE;
   utt_ODInitError("Initialization failed");
   UT_ASSERT_EQ_UINT(1, ut_console_error_calls);
   UT_ASSERT_EQ_UINT(0, ut_console_help_calls);
   UT_ASSERT_EQ_UINT(0, ut_message_calls);

   reset_windows_error();
   ut_subsystem = kODWindowsSubsystemConsole;
   bParsedCmdLine = TRUE;
   utt_ODInitError("Initialization failed");
   UT_ASSERT_EQ_UINT(1, ut_console_error_calls);
   UT_ASSERT_EQ_UINT(1, ut_console_help_calls);
   UT_ASSERT_EQ_UINT(0, ut_message_calls);
}
#endif

static const UTTestCase ut_cases[] = {
#ifdef ODPLAT_NIX
   {"standard error", writes_the_exact_error_to_standard_error}
#else
#ifdef ODPLAT_WIN32
   {"unparsed message", displays_the_original_message_without_parsed_arguments},
   {"allocation failure", allocation_failure_falls_back_to_the_original_message},
   {"message with help", appends_help_to_an_allocated_message_and_releases_it},
   {"console message", writes_console_errors_and_optional_help}
#else
   {"DOS output", prints_command_help_only_after_parsing}
#endif
#endif
};
