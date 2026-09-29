#define UT_CUSTOM_MOCK_AllocConsole
#define UT_CUSTOM_MOCK_CreateFileA
#define UT_CUSTOM_MOCK_FreeConsole
#define UT_CUSTOM_MOCK_GetConsoleCP
#define UT_CUSTOM_MOCK_GetConsoleCursorInfo
#define UT_CUSTOM_MOCK_GetConsoleMode
#define UT_CUSTOM_MOCK_GetConsoleOutputCP
#define UT_CUSTOM_MOCK_GetConsoleScreenBufferInfo
#define UT_CUSTOM_MOCK_ODConsoleShutdown
#define UT_CUSTOM_MOCK_SetConsoleCP
#define UT_CUSTOM_MOCK_SetConsoleMode
#define UT_CUSTOM_MOCK_SetConsoleOutputCP

static HANDLE ut_input = (HANDLE)(UINT_PTR)1;
static HANDLE ut_output = (HANDLE)(UINT_PTR)2;
static unsigned ut_create_calls;
static unsigned ut_alloc_calls;
static unsigned ut_free_calls;
static unsigned ut_shutdown_calls;
static unsigned ut_failure_step;
static BOOL ut_console_missing;
static unsigned ut_alloc_failures;
static BOOL ut_fail_after_alloc;

BOOL WINAPI utm_AllocConsole(void)
{
   ++ut_alloc_calls;
   if(ut_alloc_failures != 0)
   {
      --ut_alloc_failures;
      return(FALSE);
   }
   return(TRUE);
}

BOOL WINAPI utm_FreeConsole(void)
{
   ++ut_free_calls;
   return(TRUE);
}

HANDLE WINAPI utm_CreateFileA(LPCSTR name, DWORD access, DWORD sharing,
   LPSECURITY_ATTRIBUTES security, DWORD creation, DWORD flags,
   HANDLE template_file)
{
   ++ut_create_calls;
   UT_ASSERT_EQ_UINT(GENERIC_READ | GENERIC_WRITE, access);
   UT_ASSERT_EQ_UINT(FILE_SHARE_READ | FILE_SHARE_WRITE, sharing);
   UT_ASSERT_NULL(security); UT_ASSERT_EQ_UINT(OPEN_EXISTING, creation);
   UT_ASSERT_EQ_UINT(0, flags); UT_ASSERT_NULL(template_file);
   if(strcmp("CONIN$", name) == 0)
   {
      if(ut_console_missing && ut_alloc_calls == 0)
         return(INVALID_HANDLE_VALUE);
      if(ut_failure_step == 1
         && (ut_alloc_calls == 0 || ut_fail_after_alloc))
         return(INVALID_HANDLE_VALUE);
      return(ut_input);
   }
   UT_ASSERT_EQ_INT(0, strcmp("CONOUT$", name));
   if(ut_failure_step == 2
      && (ut_alloc_calls == 0 || ut_fail_after_alloc))
      return(INVALID_HANDLE_VALUE);
   return(ut_output);
}

BOOL WINAPI utm_GetConsoleMode(HANDLE handle, LPDWORD mode)
{
   if(handle == ut_input)
   {
      *mode = 0x1234;
      return(ut_failure_step != 3
         || (ut_alloc_calls != 0 && !ut_fail_after_alloc));
   }
   UT_ASSERT_EQ_PTR(ut_output, handle);
   *mode = 0x5678;
   return(ut_failure_step != 4
      || (ut_alloc_calls != 0 && !ut_fail_after_alloc));
}

BOOL WINAPI utm_GetConsoleCursorInfo(HANDLE handle,
   PCONSOLE_CURSOR_INFO info)
{
   UT_ASSERT_EQ_PTR(ut_output, handle);
   info->dwSize = 25; info->bVisible = TRUE;
   return(ut_failure_step != 5
      || (ut_alloc_calls != 0 && !ut_fail_after_alloc));
}

BOOL WINAPI utm_GetConsoleScreenBufferInfo(HANDLE handle,
   PCONSOLE_SCREEN_BUFFER_INFO info)
{
   UT_ASSERT_EQ_PTR(ut_output, handle);
   memset(info, 0, sizeof(*info));
   info->dwSize.X = 80; info->dwSize.Y = 25;
   info->srWindow.Right = 79; info->srWindow.Bottom = 24;
   return(ut_failure_step != 6
      || (ut_alloc_calls != 0 && !ut_fail_after_alloc));
}

UINT WINAPI utm_GetConsoleCP(void) { return(65001); }
UINT WINAPI utm_GetConsoleOutputCP(void) { return(1252); }
BOOL WINAPI utm_SetConsoleCP(UINT value) { UT_ASSERT_EQ_UINT(437, value); return(TRUE); }
BOOL WINAPI utm_SetConsoleOutputCP(UINT value) { UT_ASSERT_EQ_UINT(437, value); return(TRUE); }
BOOL WINAPI utm_SetConsoleMode(HANDLE handle, DWORD mode)
{
   UT_ASSERT_EQ_PTR(ut_input, handle);
   UT_ASSERT((mode & ENABLE_WINDOW_INPUT) != 0);
   return(TRUE);
}

void utm_ODConsoleShutdown(void) { ++ut_shutdown_calls; }

static void reset_fixture(void)
{
   bConsoleActive = FALSE; hConsoleInput = hConsoleOutput = INVALID_HANDLE_VALUE;
   bConsoleOwned = FALSE;
   ut_create_calls = ut_alloc_calls = ut_free_calls = ut_shutdown_calls = 0;
   ut_failure_step = 0;
   ut_console_missing = FALSE;
   ut_alloc_failures = 0;
   ut_fail_after_alloc = FALSE;
}

static void initializes_and_preserves_console_state(void)
{
   reset_fixture();
   UT_ASSERT(utt_ODConsoleInitialize());
   UT_ASSERT(bConsoleActive);
   UT_ASSERT_EQ_UINT(0, wRepeatCount);
   UT_ASSERT_EQ_UINT(65001, nSavedInputCodePage);
   UT_ASSERT_EQ_UINT(1252, nSavedOutputCodePage);
   UT_ASSERT_EQ_UINT(80, SavedBufferSize.X);
   UT_ASSERT_EQ_UINT(0, ut_alloc_calls);
   UT_ASSERT(!bConsoleOwned);
}

static void allocates_a_console_when_none_is_attached(void)
{
   reset_fixture(); ut_console_missing = TRUE;
   UT_ASSERT(utt_ODConsoleInitialize());
   UT_ASSERT(bConsoleActive);
   UT_ASSERT(bConsoleOwned);
   UT_ASSERT_EQ_UINT(1, ut_alloc_calls);
   UT_ASSERT_EQ_UINT(3, ut_create_calls);
}

static void reports_console_allocation_and_reopen_failures(void)
{
   reset_fixture(); ut_console_missing = TRUE; ut_alloc_failures = 2;
   UT_ASSERT(!utt_ODConsoleInitialize());
   UT_ASSERT_EQ_UINT(2, ut_alloc_calls);
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
   UT_ASSERT_EQ_UINT(0, ut_shutdown_calls);
   UT_ASSERT(!bConsoleOwned);

   reset_fixture(); ut_console_missing = TRUE; ut_failure_step = 1;
   ut_fail_after_alloc = TRUE;
   UT_ASSERT(!utt_ODConsoleInitialize());
   UT_ASSERT_EQ_UINT(1, ut_alloc_calls);
   UT_ASSERT_EQ_UINT(1, ut_shutdown_calls);
}

static void replaces_an_inaccessible_attached_console(void)
{
   reset_fixture(); ut_console_missing = TRUE; ut_alloc_failures = 1;
   UT_ASSERT(utt_ODConsoleInitialize());
   UT_ASSERT(bConsoleActive);
   UT_ASSERT(bConsoleOwned);
   UT_ASSERT_EQ_UINT(2, ut_alloc_calls);
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
}

static void active_console_is_idempotent(void)
{
   reset_fixture(); bConsoleActive = TRUE;
   UT_ASSERT(utt_ODConsoleInitialize());
   UT_ASSERT_EQ_UINT(0, ut_create_calls);
}

static void replaces_an_unusable_attached_console(void)
{
   unsigned step;
   for(step = 2; step <= 6; ++step)
   {
      reset_fixture(); ut_failure_step = step;
      UT_ASSERT(utt_ODConsoleInitialize());
      UT_ASSERT(bConsoleActive);
      UT_ASSERT(bConsoleOwned);
      UT_ASSERT_EQ_UINT(1, ut_shutdown_calls);
      UT_ASSERT_EQ_UINT(1, ut_free_calls);
      UT_ASSERT_EQ_UINT(1, ut_alloc_calls);
   }
}

static void rejects_each_unavailable_allocated_console_resource(void)
{
   unsigned step;
   for(step = 1; step <= 6; ++step)
   {
      reset_fixture(); ut_console_missing = TRUE; ut_failure_step = step;
      ut_fail_after_alloc = TRUE;
      UT_ASSERT(!utt_ODConsoleInitialize());
      UT_ASSERT(!bConsoleActive);
      UT_ASSERT_EQ_UINT(1, ut_alloc_calls);
   }
}

static void reports_failed_attached_console_replacement(void)
{
   reset_fixture();
   ut_failure_step = 2;
   ut_alloc_failures = 1;
   UT_ASSERT(!utt_ODConsoleInitialize());
   UT_ASSERT_EQ_UINT(1, ut_alloc_calls);
   UT_ASSERT_EQ_UINT(1, ut_shutdown_calls);
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
}

static const UTTestCase ut_cases[] = {
   {"success", initializes_and_preserves_console_state},
   {"allocate console", allocates_a_console_when_none_is_attached},
   {"allocation failures", reports_console_allocation_and_reopen_failures},
   {"replace inaccessible attachment",
      replaces_an_inaccessible_attached_console},
   {"already active", active_console_is_idempotent},
   {"replace unusable attachment", replaces_an_unusable_attached_console},
   {"allocated resource failures",
      rejects_each_unavailable_allocated_console_resource},
   {"replacement allocation failure",
      reports_failed_attached_console_replacement}
};
