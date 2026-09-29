#define UT_CUSTOM_MOCK_GetCommandLineA
#define UT_CUSTOM_MOCK_calloc
#define UT_CUSTOM_MOCK_free
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_FreeLibrary
#define UT_CUSTOM_MOCK_LocalFree
#define UT_CUSTOM_MOCK_MultiByteToWideChar
#define UT_CUSTOM_MOCK_ODWindowsCommandLineToArgv
#define UT_CUSTOM_MOCK_WideCharToMultiByte
#endif
#define UT_CUSTOM_MOCK_isspace
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_realloc
#define UT_CUSTOM_MOCK_strdup
#define UT_CUSTOM_MOCK_strstr
#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed

static char *ut_arguments[4097];
static char ut_program[128];
static char ut_command[9000];
static char ut_long_command[9000];
static unsigned ut_strdup_calls;
static unsigned ut_free_calls;
static int ut_strdup_fail_call;
static BOOL ut_calloc_fails;
static BOOL ut_realloc_fails;
static BOOL ut_public_call_allowed;

BOOL utm_ODSyncPublicCallAllowed(void)
{
   if(!ut_public_call_allowed)
      od_control.od_error = ERR_GENERALFAILURE;
   return ut_public_call_allowed;
}
#ifdef ODPLAT_WIN32
static LPSTR ut_full_command;
typedef union
{
   void *alignment;
   unsigned char bytes[20000];
} tAllocationBlock;
static tAllocationBlock ut_malloc_blocks[5];
static WCHAR ut_wide_storage[10000];
static LPWSTR ut_wide_arguments[4098];
static unsigned ut_malloc_calls;
static unsigned ut_malloc_fail_call;
static unsigned ut_local_free_calls;
static unsigned ut_parser_calls;
static unsigned ut_parser_fail_call;
static unsigned ut_parser_empty_call;
static BOOL ut_parser_returns_module;
static unsigned ut_conversion_calls;
static unsigned ut_conversion_fail_call;
static unsigned ut_free_library_calls;
#endif

void *utm_calloc(size_t count, size_t size)
{
   UT_ASSERT_EQ_UINT(4097, count);
   UT_ASSERT_EQ_UINT(sizeof(char *), size);
   if(ut_calloc_fails)
      return NULL;
   memset(ut_arguments, 0, sizeof(ut_arguments));
   return ut_arguments;
}

void utm_free(void *memory)
{
   if(memory != NULL)
      ++ut_free_calls;
}

void *utm_realloc(void *memory, size_t size)
{
   UT_ASSERT_EQ_PTR(ut_arguments, memory);
   UT_ASSERT(size >= 2 * sizeof(char *));
   if(ut_realloc_fails)
      return NULL;
   return memory;
}

char *utm_strdup(const char *text)
{
   char *destination;
   ++ut_strdup_calls;
   if((int)ut_strdup_calls == ut_strdup_fail_call)
      return NULL;
   destination = ut_strdup_calls == 1 ? ut_program : ut_command;
   UT_ASSERT(strlen(text) < (ut_strdup_calls == 1 ? sizeof(ut_program) :
      sizeof(ut_command)));
   strcpy(destination, text);
   return destination;
}

int utm_isspace(int value)
{
   UT_ASSERT_EQ_INT((int)(unsigned char)value, value);
   return value == ' ' || value == '\t' || value == '\n' ||
      value == '\r' || value == '\f' || value == '\v';
}

#ifdef ODPLAT_WIN32
LPSTR WINAPI utm_GetCommandLineA(void)
{
   return ut_full_command;
}

void *utm_malloc(size_t size)
{
   unsigned call = ++ut_malloc_calls;
   UT_ASSERT(call <= sizeof(ut_malloc_blocks) / sizeof(ut_malloc_blocks[0]));
   UT_ASSERT(size <= sizeof(ut_malloc_blocks[0].bytes));
   return(call == ut_malloc_fail_call ? NULL : ut_malloc_blocks[call - 1].bytes);
}

int WINAPI utm_MultiByteToWideChar(UINT code_page, DWORD flags,
   LPCCH input, int input_length, LPWSTR output, int output_length)
{
   int length = 0;
   int result;
   UT_ASSERT_EQ_UINT(CP_ACP, code_page);
   UT_ASSERT_EQ_UINT(0, flags);
   UT_ASSERT_EQ_INT(-1, input_length);
   ++ut_conversion_calls;
   if(ut_conversion_calls == ut_conversion_fail_call)
      return(0);
   while(input[length] != '\0')
      ++length;
   ++length;
   result = length;
   if(output == NULL)
      return(result);
   UT_ASSERT(output_length >= length);
   while(length-- > 0)
      *output++ = (unsigned char)*input++;
   return(result);
}

int WINAPI utm_WideCharToMultiByte(UINT code_page, DWORD flags,
   LPCWCH input, int input_length, LPSTR output, int output_length,
   LPCCH default_character, LPBOOL used_default)
{
   int length = 0;
   int result;
   UT_ASSERT_EQ_UINT(CP_ACP, code_page);
   UT_ASSERT_EQ_UINT(0, flags);
   UT_ASSERT_EQ_INT(-1, input_length);
   UT_ASSERT_NULL(default_character);
   UT_ASSERT_NULL(used_default);
   ++ut_conversion_calls;
   if(ut_conversion_calls == ut_conversion_fail_call)
      return(0);
   while(input[length] != L'\0')
      ++length;
   ++length;
   result = length;
   if(output == NULL)
      return(result);
   UT_ASSERT(output_length >= length);
   while(length-- > 0)
      *output++ = (char)*input++;
   return(result);
}

static LPWSTR *ut_parse_wide(LPCWSTR command, INT *count)
{
   LPCWSTR current = command;
   LPWSTR output = ut_wide_storage;
   BOOL in_quotes;
   size_t slash_count;
   size_t index;
   *count = 0;
   while(*current != L'\0' && *count < 4097)
   {
      while(*current == L' ' || *current == L'\t')
         ++current;
      if(*current == L'\0')
         break;
      ut_wide_arguments[(*count)++] = output;
      in_quotes = FALSE;
      slash_count = 0;
      while(*current != L'\0')
      {
         WCHAR value = *current++;
         if(value == L'\\')
         {
            ++slash_count;
            continue;
         }
         if(value == L'\"')
         {
            for(index = 0; index < slash_count / 2; ++index)
               *output++ = L'\\';
            if((slash_count & 1) != 0)
               *output++ = L'\"';
            else
               in_quotes = !in_quotes;
            slash_count = 0;
            continue;
         }
         for(index = 0; index < slash_count; ++index)
            *output++ = L'\\';
         slash_count = 0;
         if(!in_quotes && (value == L' ' || value == L'\t'))
            break;
         *output++ = value;
      }
      for(index = 0; index < slash_count; ++index)
         *output++ = L'\\';
      *output++ = L'\0';
   }
   ut_wide_arguments[*count] = NULL;
   return(ut_wide_arguments);
}

LPWSTR *utm_ODWindowsCommandLineToArgv(LPCWSTR command, INT *count,
   HMODULE *module)
{
   ++ut_parser_calls;
   *module = ut_parser_returns_module ? (HMODULE)(UINT_PTR)0x1234 : NULL;
   if(ut_parser_calls == ut_parser_fail_call)
      return(NULL);
   memset(ut_wide_storage, 0, sizeof(ut_wide_storage));
   memset(ut_wide_arguments, 0, sizeof(ut_wide_arguments));
   if(ut_parser_calls == ut_parser_empty_call)
   {
      *count = 0;
      return(ut_wide_arguments);
   }
   return(ut_parse_wide(command, count));
}

HLOCAL WINAPI utm_LocalFree(HLOCAL memory)
{
   UT_ASSERT_EQ_PTR(ut_wide_arguments, memory);
   ++ut_local_free_calls;
   return(NULL);
}

BOOL WINAPI utm_FreeLibrary(HMODULE module)
{
   UT_ASSERT_EQ_PTR((HMODULE)(UINT_PTR)0x1234, module);
   ++ut_free_library_calls;
   return(TRUE);
}
#endif

static void reset_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
   memset(ut_program, 0, sizeof(ut_program));
   memset(ut_command, 0, sizeof(ut_command));
   ut_strdup_calls = 0;
   ut_free_calls = 0;
   ut_strdup_fail_call = 0;
   ut_calloc_fails = FALSE;
   ut_realloc_fails = FALSE;
   ut_public_call_allowed = TRUE;
#ifdef ODPLAT_WIN32
   ut_full_command = "door.exe";
   memset(ut_malloc_blocks, 0, sizeof(ut_malloc_blocks));
   ut_malloc_calls = 0;
   ut_malloc_fail_call = 0;
   ut_local_free_calls = 0;
   ut_parser_calls = 0;
   ut_parser_fail_call = 0;
   ut_parser_empty_call = 0;
   ut_parser_returns_module = FALSE;
   ut_conversion_calls = 0;
   ut_conversion_fail_call = 0;
   ut_free_library_calls = 0;
#endif
}

static void rejects_invalid_parameters_and_allocation_failures(void)
{
   INT count = 99;
   char **result;
#ifdef ODPLAT_WIN32
   unsigned conversion_failure;
#endif
   reset_fixture();
   result = utt_od_split_cmd_line(NULL, &count);
   UT_ASSERT_NULL(result);
   UT_ASSERT_EQ_INT(0, count);
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);

   reset_fixture();
   result = utt_od_split_cmd_line("value", NULL);
   UT_ASSERT_NULL(result);
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);

   reset_fixture();
   ut_calloc_fails = TRUE;
   result = utt_od_split_cmd_line("value", &count);
   UT_ASSERT_NULL(result);
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);

   reset_fixture();
#ifdef ODPLAT_WIN32
   ut_malloc_fail_call = 1;
   result = utt_od_split_cmd_line("value", &count);
   UT_ASSERT_NULL(result);
   UT_ASSERT_EQ_INT(0, count);
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);

   reset_fixture();
   ut_malloc_fail_call = 2;
   result = utt_od_split_cmd_line("value", &count);
   UT_ASSERT_NULL(result);
   UT_ASSERT_EQ_INT(0, count);

   reset_fixture();
   ut_malloc_fail_call = 3;
   result = utt_od_split_cmd_line("value", &count);
   UT_ASSERT_NULL(result);
   UT_ASSERT_EQ_INT(0, count);

   reset_fixture();
   ut_malloc_fail_call = 4;
   result = utt_od_split_cmd_line("value", &count);
   UT_ASSERT_NULL(result);
   UT_ASSERT_EQ_INT(0, count);

   reset_fixture();
   ut_parser_fail_call = 1;
   UT_ASSERT_NULL(utt_od_split_cmd_line("value", &count));
   UT_ASSERT_EQ_INT(0, count);

   reset_fixture();
   ut_parser_fail_call = 2;
   UT_ASSERT_NULL(utt_od_split_cmd_line("value", &count));
   UT_ASSERT_EQ_INT(0, count);

   reset_fixture();
   ut_parser_empty_call = 2;
   UT_ASSERT_NULL(utt_od_split_cmd_line("value", &count));
   UT_ASSERT_EQ_INT(0, count);

   for(conversion_failure = 1; conversion_failure <= 8;
      ++conversion_failure)
   {
      reset_fixture();
      ut_conversion_fail_call = conversion_failure;
      UT_ASSERT_NULL(utt_od_split_cmd_line("value", &count));
      UT_ASSERT_EQ_INT(0, count);
   }

   reset_fixture();
   ut_parser_returns_module = TRUE;
   ut_conversion_fail_call = 7;
   UT_ASSERT_NULL(utt_od_split_cmd_line("value", &count));
   UT_ASSERT_EQ_INT(0, count);
   UT_ASSERT_EQ_UINT(2, ut_local_free_calls);
   UT_ASSERT_EQ_UINT(2, ut_free_library_calls);
#else
   ut_strdup_fail_call = 1;
   result = utt_od_split_cmd_line("value", &count);
   UT_ASSERT_NULL(result);
   UT_ASSERT_EQ_INT(0, count);
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);

   reset_fixture();
   ut_strdup_fail_call = 2;
   result = utt_od_split_cmd_line("value", &count);
   UT_ASSERT_NULL(result);
   UT_ASSERT_EQ_INT(0, count);
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);
   UT_ASSERT_EQ_UINT(2, ut_free_calls);
#endif

#ifdef ODPLAT_WIN32
   reset_fixture();
   ut_full_command = NULL;
   result = utt_od_split_cmd_line("value", &count);
   UT_ASSERT_NOT_NULL(result);
   UT_ASSERT_EQ_INT(0, strcmp("", result[0]));
#endif
}

static void rejects_a_terminal_session(void)
{
   INT count = 99;
   reset_fixture();
   ut_public_call_allowed = FALSE;
   UT_ASSERT_NULL(utt_od_split_cmd_line("value", &count));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_INT(99, count);
}

static void splits_whitespace_and_handles_realloc_failure(void)
{
   INT count;
   char **result;
   reset_fixture();
   result = utt_od_split_cmd_line("one \t two", &count);
   UT_ASSERT_EQ_PTR(ut_arguments, result);
   UT_ASSERT_EQ_INT(3, count);
   UT_ASSERT_EQ_INT(0, strcmp("one", result[1]));
   UT_ASSERT_EQ_INT(0, strcmp("two", result[2]));

   reset_fixture();
   ut_realloc_fails = TRUE;
   result = utt_od_split_cmd_line("", &count);
   UT_ASSERT_EQ_PTR(ut_arguments, result);
   UT_ASSERT_EQ_INT(1, count);
}

static void stops_at_the_documented_argument_limit(void)
{
   INT count;
   char **result;
   unsigned position = 0;
   reset_fixture();
   while(position + 2 < sizeof(ut_long_command))
   {
      ut_long_command[position++] = 'x';
      ut_long_command[position++] = ' ';
   }
   ut_long_command[position] = '\0';
   result = utt_od_split_cmd_line(ut_long_command, &count);
   UT_ASSERT_NOT_NULL(result);
   UT_ASSERT_EQ_INT(4096, count);
}

static void preserves_high_bit_argument_bytes(void)
{
   char command[] = {(char)0x80, 'x', '\0'};
   INT count;
   char **result;
   reset_fixture();
   result = utt_od_split_cmd_line(command, &count);
   UT_ASSERT_NOT_NULL(result);
   UT_ASSERT_EQ_INT(2, count);
   UT_ASSERT_EQ_UINT(0x80, (unsigned char)result[1][0]);
   UT_ASSERT_EQ_INT('x', result[1][1]);
   UT_ASSERT_EQ_INT('\0', result[1][2]);
}

#ifdef ODPLAT_WIN32
static void parses_windows_quotes_and_empty_arguments(void)
{
   INT count;
   char **result;
   reset_fixture();
   ut_parser_returns_module = TRUE;
   result = utt_od_split_cmd_line(
      "plain \"two words\" \"\" quote\\\"inside", &count);
   UT_ASSERT_NOT_NULL(result);
   UT_ASSERT_EQ_INT(5, count);
   UT_ASSERT_EQ_INT(0, strcmp("plain", result[1]));
   UT_ASSERT_EQ_INT(0, strcmp("two words", result[2]));
   UT_ASSERT_EQ_INT(0, strcmp("", result[3]));
   UT_ASSERT_EQ_INT(0, strcmp("quote\"inside", result[4]));
   UT_ASSERT_NULL(result[5]);
}

static void obtains_program_name_from_the_full_command_line(void)
{
   INT count;
   char **result;
   reset_fixture();
   ut_full_command = "door.exe one two";
   result = utt_od_split_cmd_line("one two", &count);
   UT_ASSERT_NOT_NULL(result);
   UT_ASSERT_EQ_INT(0, strcmp("door.exe", result[0]));

   reset_fixture();
   ut_full_command = NULL;
   result = utt_od_split_cmd_line("one", &count);
   UT_ASSERT_EQ_INT(0, strcmp("", result[0]));

   reset_fixture();
   ut_full_command = "one";
   result = utt_od_split_cmd_line("one", &count);
   UT_ASSERT_EQ_INT(0, strcmp("one", result[0]));

   reset_fixture();
   ut_full_command = "door.exe unrelated";
   result = utt_od_split_cmd_line("missing", &count);
   UT_ASSERT_EQ_INT(0, strcmp("door.exe", result[0]));
}

static void reports_program_name_allocation_failure(void)
{
   INT count = 99;
   reset_fixture();
   ut_full_command = "door.exe one";
   ut_malloc_fail_call = 2;
   UT_ASSERT_NULL(utt_od_split_cmd_line("one", &count));
   UT_ASSERT_EQ_INT(0, count);
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);
}
#endif

static const UTTestCase ut_cases[] = {
   {"terminal session", rejects_a_terminal_session},
   {"invalid parameters and allocations", rejects_invalid_parameters_and_allocation_failures},
   {"split whitespace", splits_whitespace_and_handles_realloc_failure},
   {"high-bit argument", preserves_high_bit_argument_bytes},
   {"argument limit", stops_at_the_documented_argument_limit},
#ifdef ODPLAT_WIN32
   {"Windows grammar", parses_windows_quotes_and_empty_arguments},
   {"program name", obtains_program_name_from_the_full_command_line},
   {"program allocation", reports_program_name_allocation_failure},
#endif
};
