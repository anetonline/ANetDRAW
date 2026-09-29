#define UT_CUSTOM_MOCK_CloseHandle
#define UT_CUSTOM_MOCK_FreeConsole
#define UT_CUSTOM_MOCK_SetConsoleCP
#define UT_CUSTOM_MOCK_SetConsoleCursorInfo
#define UT_CUSTOM_MOCK_SetConsoleMode
#define UT_CUSTOM_MOCK_SetConsoleOutputCP
#define UT_CUSTOM_MOCK_SetConsoleScreenBufferSize
#define UT_CUSTOM_MOCK_SetConsoleWindowInfo
#define UT_CUSTOM_MOCK_free

static unsigned ut_close_calls;
static unsigned ut_free_console_calls;
static unsigned ut_restore_calls;
static void *ut_freed;

BOOL WINAPI utm_CloseHandle(HANDLE handle)
{
   UT_ASSERT(handle == hConsoleInput || handle == hConsoleOutput);
   ++ut_close_calls; return(TRUE);
}
BOOL WINAPI utm_FreeConsole(void)
{
   ++ut_free_console_calls;
   return(TRUE);
}
BOOL WINAPI utm_SetConsoleMode(HANDLE handle, DWORD mode)
{
   (void)handle; (void)mode; ++ut_restore_calls; return(TRUE);
}
BOOL WINAPI utm_SetConsoleCP(UINT value) { (void)value; ++ut_restore_calls; return(TRUE); }
BOOL WINAPI utm_SetConsoleOutputCP(UINT value) { (void)value; ++ut_restore_calls; return(TRUE); }
BOOL WINAPI utm_SetConsoleCursorInfo(HANDLE handle,
   const CONSOLE_CURSOR_INFO *info)
{
   (void)handle; (void)info; ++ut_restore_calls; return(TRUE);
}
BOOL WINAPI utm_SetConsoleScreenBufferSize(HANDLE handle, COORD size)
{
   (void)handle; (void)size; ++ut_restore_calls; return(TRUE);
}
BOOL WINAPI utm_SetConsoleWindowInfo(HANDLE handle, BOOL absolute,
   const SMALL_RECT *window)
{
   (void)handle; (void)absolute; (void)window; ++ut_restore_calls; return(TRUE);
}
void utm_free(void *memory) { ut_freed = memory; }

static void restores_active_console_and_releases_storage(void)
{
   static CHAR_INFO cells[2];
   hConsoleInput = (HANDLE)(UINT_PTR)1;
   hConsoleOutput = (HANDLE)(UINT_PTR)2;
   bConsoleActive = TRUE; bConsoleOwned = FALSE; wRepeatCount = 3;
   pConsoleCells = cells; nConsoleCellCapacity = 2;
   ut_close_calls = ut_restore_calls = ut_free_console_calls = 0;
   ut_freed = NULL;
   utt_ODConsoleShutdown();
   UT_ASSERT_EQ_UINT(2, ut_close_calls);
   UT_ASSERT_EQ_UINT(8, ut_restore_calls);
   UT_ASSERT_EQ_PTR(cells, ut_freed);
   UT_ASSERT(!bConsoleActive);
   UT_ASSERT_EQ_UINT(0, wRepeatCount);
   UT_ASSERT_NULL(pConsoleCells);
   UT_ASSERT_EQ_UINT(0, nConsoleCellCapacity);
   UT_ASSERT_EQ_UINT(0, ut_free_console_calls);
}

static void releases_a_console_allocated_by_opendoors(void)
{
   hConsoleInput = (HANDLE)(UINT_PTR)1;
   hConsoleOutput = (HANDLE)(UINT_PTR)2;
   bConsoleActive = TRUE; bConsoleOwned = TRUE;
   pConsoleCells = NULL; nConsoleCellCapacity = 0;
   ut_close_calls = ut_restore_calls = ut_free_console_calls = 0;
   ut_freed = (void *)1;
   utt_ODConsoleShutdown();
   UT_ASSERT_EQ_UINT(2, ut_close_calls);
   UT_ASSERT_EQ_UINT(8, ut_restore_calls);
   UT_ASSERT_EQ_UINT(1, ut_free_console_calls);
   UT_ASSERT(!bConsoleOwned);
}

static void closes_only_valid_partial_handle(void)
{
   hConsoleInput = INVALID_HANDLE_VALUE;
   hConsoleOutput = (HANDLE)(UINT_PTR)2;
   bConsoleActive = FALSE; bConsoleOwned = FALSE; pConsoleCells = NULL;
   ut_close_calls = ut_restore_calls = ut_free_console_calls = 0;
   ut_freed = (void *)1;
   utt_ODConsoleShutdown();
   UT_ASSERT_EQ_UINT(1, ut_close_calls);
   UT_ASSERT_EQ_UINT(0, ut_restore_calls);
   UT_ASSERT_NULL(ut_freed);

   hConsoleInput = (HANDLE)(UINT_PTR)1;
   hConsoleOutput = INVALID_HANDLE_VALUE;
   ut_close_calls = 0;
   utt_ODConsoleShutdown();
   UT_ASSERT_EQ_UINT(1, ut_close_calls);
}

static const UTTestCase ut_cases[] = {
   {"active restore", restores_active_console_and_releases_storage},
   {"owned console", releases_a_console_allocated_by_opendoors},
   {"partial initialization", closes_only_valid_partial_handle}
};
