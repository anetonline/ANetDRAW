#define UT_CUSTOM_MOCK_LoadLibraryA
#define UT_CUSTOM_MOCK_GetProcAddress
#define UT_CUSTOM_MOCK_FreeLibrary
#define UT_CUSTOM_MOCK_ODWindowsCommandLineToArgvFallback

static WCHAR *ut_arguments[] = {L"result", NULL};
static HMODULE ut_module = (HMODULE)(UINT_PTR)0x1234;
static BOOL ut_load_succeeds;
static BOOL ut_proc_succeeds;
static BOOL ut_parse_succeeds;
static unsigned ut_free_calls;
static unsigned ut_fallback_calls;

static LPWSTR *WINAPI ut_command_line_to_argv(LPCWSTR command, INT *count)
{
   UT_ASSERT_EQ_INT(L'x', command[0]);
   if(!ut_parse_succeeds)
      return(NULL);
   *count = 1;
   return(ut_arguments);
}

HMODULE WINAPI utm_LoadLibraryA(LPCSTR name)
{
   UT_ASSERT_EQ_INT(0, strcmp("shell32.dll", name));
   return(ut_load_succeeds ? ut_module : NULL);
}

FARPROC WINAPI utm_GetProcAddress(HMODULE module, LPCSTR name)
{
   union
   {
      FARPROC generic;
      tODCommandLineToArgvW typed;
   } proc;
   UT_ASSERT_EQ_PTR(ut_module, module);
   UT_ASSERT_EQ_INT(0, strcmp("CommandLineToArgvW", name));
   proc.typed = ut_command_line_to_argv;
   return(ut_proc_succeeds ? proc.generic : NULL);
}

BOOL WINAPI utm_FreeLibrary(HMODULE module)
{
   UT_ASSERT_EQ_PTR(ut_module, module);
   ++ut_free_calls;
   return(TRUE);
}

LPWSTR *utm_ODWindowsCommandLineToArgvFallback(LPCWSTR command, INT *count)
{
   UT_ASSERT_EQ_INT(L'x', command[0]);
   ++ut_fallback_calls;
   *count = 1;
   return(ut_arguments);
}

static void reset_fixture(void)
{
   ut_load_succeeds = FALSE;
   ut_proc_succeeds = FALSE;
   ut_parse_succeeds = FALSE;
   ut_free_calls = 0;
   ut_fallback_calls = 0;
}

static void uses_the_runtime_api_when_available(void)
{
   HMODULE module = NULL;
   INT count = 0;
   reset_fixture();
   ut_load_succeeds = TRUE;
   ut_proc_succeeds = TRUE;
   ut_parse_succeeds = TRUE;
   UT_ASSERT_EQ_PTR(ut_arguments,
      utt_ODWindowsCommandLineToArgv(L"x", &count, &module));
   UT_ASSERT_EQ_PTR(ut_module, module);
   UT_ASSERT_EQ_INT(1, count);
   UT_ASSERT_EQ_UINT(0, ut_free_calls);
   UT_ASSERT_EQ_UINT(0, ut_fallback_calls);
}

static void falls_back_only_when_the_api_is_unavailable(void)
{
   HMODULE module = ut_module;
   INT count = 0;
   reset_fixture();
   UT_ASSERT_EQ_PTR(ut_arguments,
      utt_ODWindowsCommandLineToArgv(L"x", &count, &module));
   UT_ASSERT_NULL(module);
   UT_ASSERT_EQ_UINT(1, ut_fallback_calls);

   reset_fixture();
   ut_load_succeeds = TRUE;
   UT_ASSERT_EQ_PTR(ut_arguments,
      utt_ODWindowsCommandLineToArgv(L"x", &count, &module));
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
   UT_ASSERT_EQ_UINT(1, ut_fallback_calls);

   reset_fixture();
   ut_load_succeeds = TRUE;
   ut_proc_succeeds = TRUE;
   UT_ASSERT_NULL(utt_ODWindowsCommandLineToArgv(L"x", &count, &module));
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
   UT_ASSERT_EQ_UINT(0, ut_fallback_calls);
}

static const UTTestCase ut_cases[] = {
   {"runtime API", uses_the_runtime_api_when_available},
   {"fallback", falls_back_only_when_the_api_is_unavailable}
};
