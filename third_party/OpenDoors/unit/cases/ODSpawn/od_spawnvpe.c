#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_time
#define UT_CUSTOM_MOCK_ODWaitDrain
#define UT_CUSTOM_MOCK_ODComClose
#define UT_CUSTOM_MOCK_ODComOpen
#define UT_CUSTOM_MOCK_ODDWordDivide
#define UT_CUSTOM_MOCK_ODInQueueResetLastActivity
#define UT_CUSTOM_MOCK_od_clear_keybuffer
#if defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK_spawnvpe
#else
#define UT_CUSTOM_MOCK__spawnvpe
#endif
#ifdef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_ODSyncAPIRelease
#define UT_CUSTOM_MOCK_ODSyncAPIReacquire
#endif
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODWindowsQuoteArguments
#define UT_CUSTOM_MOCK_ODScrnShowMessage
#define UT_CUSTOM_MOCK_ODScrnRemoveMessage
#define UT_CUSTOM_MOCK_ODInExDisableDTR
#define UT_CUSTOM_MOCK_free
#endif
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_ODScrnGetTextInfo
#define UT_CUSTOM_MOCK_ODScrnSetBoundary
#define UT_CUSTOM_MOCK_ODScrnGetText
#define UT_CUSTOM_MOCK_ODScrnSetAttribute
#define UT_CUSTOM_MOCK_ODScrnClear
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos
#define UT_CUSTOM_MOCK_ODScrnPutText
#ifdef ODPLAT_DOS
#define UT_CUSTOM_MOCK_strcpy
#define UT_CUSTOM_MOCK__getdrv
#define UT_CUSTOM_MOCK__getcd
#define UT_CUSTOM_MOCK__setdrvcd
#else
#define UT_CUSTOM_MOCK_ODDirGetCurrent
#define UT_CUSTOM_MOCK_ODDirChangeCurrent
#endif

#endif

#ifdef ODPLAT_WIN32
#define TEST_SPAWNVPE(mode, path, arguments, environment) \
   utt_ODSpawnVPEInternal(mode, path, arguments, environment, TRUE)
#else
#define TEST_SPAWNVPE(mode, path, arguments, environment) \
   utt_ODSpawnVPEInternal(mode, path, arguments, environment)
#endif

static const char *ut_arguments[] = {"program", "argument", NULL};
static const char *ut_environment[] = {"NAME=value", NULL};
static INT16 ut_spawn_result;
static BOOL ut_wait_aborts;
static BOOL ut_init_succeeds;
static time_t ut_times[2];
static unsigned ut_time_index;
static unsigned ut_api_entry_calls, ut_api_exit_calls;
static unsigned ut_spawn_calls, ut_close_calls, ut_open_calls;
static unsigned ut_wait_calls, ut_reset_activity_calls, ut_clear_calls;
#ifdef OD_THREAD_SUPPORT
static unsigned ut_release_calls, ut_reacquire_calls;
#endif
#ifdef ODPLAT_WIN32
static const char *ut_empty_arguments[] = {NULL};
static const char *ut_quoted_arguments[] = {"<program>", "<argument>", NULL};
static BOOL ut_quote_fails;
static BOOL ut_expect_quoted_arguments;
static unsigned ut_quote_calls;
static unsigned ut_windows_free_calls;
#endif

void utm_ODSyncAPIEntry(void) { ++ut_api_entry_calls; }
void utm_ODSyncAPIExit(void) { ++ut_api_exit_calls; }
void ODCALL utm_od_init(void)
{
   if(ut_init_succeeds) bODInitialized = TRUE;
}
time_t utm_time(time_t *storage)
{ UT_ASSERT_NULL(storage); UT_ASSERT(ut_time_index < 2); return(ut_times[ut_time_index++]); }
void utm_ODWaitDrain(tODMilliSec maximum)
{ UT_ASSERT(maximum == 10000); ++ut_wait_calls; if(ut_wait_aborts) bODInitialized = FALSE; }
tODResult utm_ODComClose(tPortHandle port)
{ UT_ASSERT_EQ_PTR(hSerialPort, port); ++ut_close_calls; return(kODRCSuccess); }
tODResult utm_ODComOpen(tPortHandle port)
{ UT_ASSERT_EQ_PTR(hSerialPort, port); ++ut_open_calls; return(kODRCSuccess); }
BOOL utm_ODDWordDivide(DWORD *quotient, DWORD *remainder, DWORD dividend, DWORD divisor)
{
   UT_ASSERT_NOT_NULL(quotient); UT_ASSERT_NULL(remainder); UT_ASSERT(divisor == 60);
   *quotient = dividend / divisor; return(TRUE);
}
void utm_ODInQueueResetLastActivity(tODInQueueHandle queue)
{ UT_ASSERT_EQ_PTR(hODInputQueue, queue); ++ut_reset_activity_calls; }
void ODCALL utm_od_clear_keybuffer(void) { ++ut_clear_calls; }

static INT16 ut_spawn_primitive(INT16 mode, const char *path,
   const char *const arguments[], const char *const environment[])
{
   ++ut_spawn_calls; UT_ASSERT(mode == P_WAIT || mode == P_NOWAIT);
   UT_ASSERT(strcmp(path, "program") == 0);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_PTR(ut_expect_quoted_arguments ? ut_quoted_arguments :
      ut_arguments, arguments);
#else
   UT_ASSERT_EQ_PTR(ut_arguments, arguments);
#endif
   UT_ASSERT_EQ_PTR(ut_environment, environment); return(ut_spawn_result);
}
#ifdef ODPLAT_DOS32
int utm_spawnvpe(int mode, const char *path, const char *const arguments[],
   const char *const environment[])
{ return(ut_spawn_primitive((INT16)mode, path, arguments, environment)); }
#else
#ifdef ODPLAT_WIN32
intptr_t utm__spawnvpe(int mode, const char *path, const char *const arguments[],
   const char *const environment[])
{ return((intptr_t)ut_spawn_primitive((INT16)mode, path, arguments, environment)); }
#else
int utm__spawnvpe(int mode, const char *path, const char *const arguments[],
   const char *const environment[])
{ return(ut_spawn_primitive((INT16)mode, path, arguments, environment)); }
#endif
#endif

#ifdef OD_THREAD_SUPPORT
unsigned utm_ODSyncAPIRelease(void) { ++ut_release_calls; return(4); }
void utm_ODSyncAPIReacquire(unsigned level)
{ UT_ASSERT_EQ_UINT(4, level); ++ut_reacquire_calls; }
#endif

#ifdef ODPLAT_WIN32
static unsigned ut_show_calls, ut_remove_calls, ut_disable_dtr_calls;
char **utm_ODWindowsQuoteArguments(const char *const arguments[])
{
   ++ut_quote_calls;
   UT_ASSERT_EQ_PTR(ut_arguments, arguments);
   return(ut_quote_fails ? NULL : (char **)ut_quoted_arguments);
}
void utm_free(void *memory)
{
   UT_ASSERT_EQ_PTR(ut_quoted_arguments, memory);
   ++ut_windows_free_calls;
}
void *utm_ODScrnShowMessage(char *text, int flags)
{ UT_ASSERT(strcmp(text, "Running sub-program...") == 0); UT_ASSERT_EQ_INT(0, flags); ++ut_show_calls; return(&ut_show_calls); }
void utm_ODScrnRemoveMessage(void *window)
{ UT_ASSERT_EQ_PTR(&ut_show_calls, window); ++ut_remove_calls; }
void utm_ODInExDisableDTR(void) { ++ut_disable_dtr_calls; }
#endif

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
static BYTE ut_screen[4000];
static char ut_directory[256];
static unsigned ut_malloc_calls, ut_malloc_fail_at, ut_free_calls;
static unsigned ut_screen_clear_calls, ut_screen_put_calls;
static unsigned ut_boundary_calls, ut_attribute_calls, ut_cursor_calls;
static BYTE ut_boundary_left, ut_boundary_top;
static BYTE ut_boundary_right, ut_boundary_bottom;
static BYTE ut_attribute, ut_cursor_column, ut_cursor_row;
void *utm_malloc(size_t size)
{
   unsigned call = ++ut_malloc_calls;
   if(call == ut_malloc_fail_at) return(NULL);
   if(call == 1) { UT_ASSERT_EQ_UINT(sizeof(ut_screen), size); return(ut_screen); }
   UT_ASSERT_EQ_UINT(sizeof(ut_directory), size); return(ut_directory);
}
void utm_free(void *memory)
{ UT_ASSERT(memory == ut_screen || memory == ut_directory); ++ut_free_calls; }
void utm_ODScrnGetTextInfo(tODScrnTextInfo *info)
{ info->winleft = 2; info->wintop = 3; info->winright = 70; info->winbottom = 20;
  info->attribute = 6; info->curx = 7; info->cury = 8; }
void utm_ODScrnSetBoundary(BYTE left, BYTE top, BYTE right, BYTE bottom)
{
   ++ut_boundary_calls; ut_boundary_left = left; ut_boundary_top = top;
   ut_boundary_right = right; ut_boundary_bottom = bottom;
}
BOOL ODCALL utm_ODScrnGetText(BYTE left, BYTE top, BYTE right, BYTE bottom, void *buffer)
{ UT_ASSERT_EQ_INT(1, left); UT_ASSERT_EQ_INT(1, top); UT_ASSERT_EQ_INT(80, right);
  UT_ASSERT_EQ_INT(25, bottom); UT_ASSERT_EQ_PTR(ut_screen, buffer); return(TRUE); }
BOOL ODCALL utm_ODScrnPutText(BYTE left, BYTE top, BYTE right, BYTE bottom, void *buffer)
{ UT_ASSERT_EQ_INT(1, left); UT_ASSERT_EQ_INT(1, top); UT_ASSERT_EQ_INT(80, right);
  UT_ASSERT_EQ_INT(25, bottom); UT_ASSERT_EQ_PTR(ut_screen, buffer);
  ++ut_screen_put_calls; return(TRUE); }
void ODCALL utm_ODScrnSetAttribute(BYTE attribute)
{ ++ut_attribute_calls; ut_attribute = attribute; }
void utm_ODScrnClear(void) { ++ut_screen_clear_calls; }
void ODCALL utm_ODScrnSetCursorPos(BYTE column, BYTE row)
{ ++ut_cursor_calls; ut_cursor_column = column; ut_cursor_row = row; }
#ifdef ODPLAT_DOS
char *utm_strcpy(char *destination, const char *source)
{ char *out = destination; while((*out++ = *source++) != '\0') {} return(destination); }
int ODSWAPCALL utm__getdrv(void) { return(2); }
int ODSWAPCALL utm__getcd(int drive, char *directory)
{ UT_ASSERT_EQ_INT(0, drive); utm_strcpy(directory, "DOOR"); return(0); }
void ODSWAPCALL utm__setdrvcd(int drive, char *directory)
{ UT_ASSERT_EQ_INT(2, drive); UT_ASSERT(strcmp(directory, "C:\\DOOR") == 0); }
#else
void utm_ODDirGetCurrent(char *directory, INT maximum)
{ UT_ASSERT_EQ_INT(256, maximum); strcpy(directory, "C:\\DOOR"); }
void utm_ODDirChangeCurrent(char *directory)
{ UT_ASSERT(strcmp(directory, "C:\\DOOR") == 0); }
#endif
#endif

static void reset_spawnvpe(void)
{
   memset(&od_control, 0, sizeof(od_control)); bODInitialized = TRUE;
   ut_init_succeeds = TRUE;
   bIsShell = FALSE; nNextTimeDeductTime = 0; od_control.user_timelimit = 20;
   ut_spawn_result = 7; ut_wait_aborts = FALSE;
   ut_times[0] = 100; ut_times[1] = 220; ut_time_index = 0;
   ut_api_entry_calls = ut_api_exit_calls = ut_spawn_calls = 0;
   ut_close_calls = ut_open_calls = ut_wait_calls = 0;
   ut_reset_activity_calls = ut_clear_calls = 0;
#ifdef OD_THREAD_SUPPORT
   ut_release_calls = ut_reacquire_calls = 0;
#endif
#ifdef ODPLAT_WIN32
   ut_show_calls = ut_remove_calls = ut_disable_dtr_calls = 0;
   ut_quote_fails = FALSE;
   ut_expect_quoted_arguments = TRUE;
   ut_quote_calls = 0;
   ut_windows_free_calls = 0;
#endif
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   ut_malloc_calls = ut_malloc_fail_at = ut_free_calls = 0;
   ut_screen_clear_calls = ut_screen_put_calls = 0;
   ut_boundary_calls = ut_attribute_calls = ut_cursor_calls = 0;
   ut_boundary_left = ut_boundary_top = 0;
   ut_boundary_right = ut_boundary_bottom = 0;
   ut_attribute = ut_cursor_column = ut_cursor_row = 0;
#endif
}

static void initializes_and_propagates_primitive_result(void)
{
   reset_spawnvpe(); bODInitialized = FALSE; ut_spawn_result = -1;
   UT_ASSERT_EQ_INT(-1, TEST_SPAWNVPE(P_WAIT, "program", ut_arguments, ut_environment));
   UT_ASSERT_EQ_UINT(1, ut_api_entry_calls); UT_ASSERT_EQ_UINT(1, ut_api_exit_calls);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(1, ut_windows_free_calls);
#endif
}

static void rejects_a_terminal_session_after_initialization(void)
{
   reset_spawnvpe();
   bODInitialized = FALSE;
   ut_init_succeeds = FALSE;
   UT_ASSERT_EQ_INT(-1,
      TEST_SPAWNVPE(P_WAIT, "program", ut_arguments, ut_environment));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_api_entry_calls);
   UT_ASSERT_EQ_UINT(0, ut_spawn_calls);
}

#if !defined(ODPLAT_DOS) && !defined(ODPLAT_DOS32)
static void covers_waiting_and_nonwaiting_modes(void)
{
   reset_spawnvpe();
   UT_ASSERT_EQ_INT(7, TEST_SPAWNVPE(P_NOWAIT, "program", ut_arguments, ut_environment));
   UT_ASSERT_EQ_UINT(0, ut_wait_calls); UT_ASSERT_EQ_UINT(0, ut_reset_activity_calls);
   reset_spawnvpe(); ut_wait_aborts = TRUE;
   UT_ASSERT_EQ_INT(-1, TEST_SPAWNVPE(P_WAIT, "program", ut_arguments, ut_environment));
   UT_ASSERT_EQ_UINT(0, ut_spawn_calls); UT_ASSERT_EQ_UINT(1, ut_api_exit_calls);
   reset_spawnvpe(); od_control.baud = 9600;
   UT_ASSERT_EQ_INT(7, TEST_SPAWNVPE(P_WAIT, "program", ut_arguments, ut_environment));
   UT_ASSERT_EQ_UINT(1, ut_close_calls); UT_ASSERT_EQ_UINT(1, ut_open_calls);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(1, ut_disable_dtr_calls); UT_ASSERT_EQ_UINT(1, ut_remove_calls);
#endif
}
#endif

#ifdef ODPLAT_WIN32
static void rejects_invalid_arguments_and_quote_failure(void)
{
   reset_spawnvpe();
   UT_ASSERT_EQ_INT(-1,
      TEST_SPAWNVPE(P_WAIT, "program", NULL, ut_environment));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_spawn_calls);

   reset_spawnvpe();
   UT_ASSERT_EQ_INT(-1,
      TEST_SPAWNVPE(P_WAIT, "program", ut_empty_arguments, ut_environment));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_spawn_calls);

   reset_spawnvpe();
   ut_quote_fails = TRUE;
   UT_ASSERT_EQ_INT(-1,
      TEST_SPAWNVPE(P_WAIT, "program", ut_arguments, ut_environment));
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_spawn_calls);
   UT_ASSERT_EQ_UINT(0, ut_windows_free_calls);
}

static void preserves_preformatted_windows_arguments(void)
{
   reset_spawnvpe();
   ut_expect_quoted_arguments = FALSE;
   UT_ASSERT_EQ_INT(7, utt_ODSpawnVPEInternal(P_WAIT, "program",
      ut_arguments, ut_environment, FALSE));
   UT_ASSERT_EQ_UINT(0, ut_quote_calls);
   UT_ASSERT_EQ_UINT(0, ut_windows_free_calls);

   reset_spawnvpe();
   ut_expect_quoted_arguments = FALSE;
   ut_wait_aborts = TRUE;
   UT_ASSERT_EQ_INT(-1, utt_ODSpawnVPEInternal(P_WAIT, "program",
      ut_arguments, ut_environment, FALSE));
   UT_ASSERT_EQ_UINT(0, ut_quote_calls);
   UT_ASSERT_EQ_UINT(0, ut_windows_free_calls);
}
#endif

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
static void rejects_invalid_mode_and_allocation_failures(void)
{
   reset_spawnvpe();
   UT_ASSERT_EQ_INT(-1, TEST_SPAWNVPE(P_NOWAIT, "program", ut_arguments, ut_environment));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   reset_spawnvpe(); ut_malloc_fail_at = 1;
   UT_ASSERT_EQ_INT(-1, TEST_SPAWNVPE(P_WAIT, "program", ut_arguments, ut_environment));
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error); UT_ASSERT_EQ_UINT(0, ut_free_calls);
   reset_spawnvpe(); ut_malloc_fail_at = 2;
   UT_ASSERT_EQ_INT(-1, TEST_SPAWNVPE(P_WAIT, "program", ut_arguments, ut_environment));
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error); UT_ASSERT_EQ_UINT(1, ut_free_calls);
}

static void covers_dos_screen_wait_and_serial_paths(void)
{
   reset_spawnvpe(); od_control.od_clear_on_exit = TRUE;
   UT_ASSERT_EQ_INT(7, TEST_SPAWNVPE(P_WAIT, "program", ut_arguments, ut_environment));
   UT_ASSERT_EQ_UINT(1, ut_screen_clear_calls); UT_ASSERT_EQ_UINT(1, ut_cursor_calls);
   UT_ASSERT_EQ_UINT(2, ut_free_calls);
   reset_spawnvpe(); ut_wait_aborts = TRUE;
   UT_ASSERT_EQ_INT(-1, TEST_SPAWNVPE(P_WAIT, "program", ut_arguments, ut_environment));
   UT_ASSERT_EQ_UINT(0, ut_spawn_calls); UT_ASSERT_EQ_UINT(2, ut_free_calls);
   UT_ASSERT_EQ_UINT(1, ut_screen_put_calls);
   UT_ASSERT_EQ_UINT(2, ut_boundary_calls);
   UT_ASSERT_EQ_INT(2, ut_boundary_left); UT_ASSERT_EQ_INT(3, ut_boundary_top);
   UT_ASSERT_EQ_INT(70, ut_boundary_right); UT_ASSERT_EQ_INT(20, ut_boundary_bottom);
   UT_ASSERT_EQ_UINT(2, ut_attribute_calls); UT_ASSERT_EQ_INT(6, ut_attribute);
   UT_ASSERT_EQ_UINT(2, ut_cursor_calls);
   UT_ASSERT_EQ_INT(7, ut_cursor_column); UT_ASSERT_EQ_INT(8, ut_cursor_row);
   reset_spawnvpe(); od_control.od_silent_mode = TRUE; ut_wait_aborts = TRUE;
   UT_ASSERT_EQ_INT(-1, TEST_SPAWNVPE(P_WAIT, "program", ut_arguments, ut_environment));
   UT_ASSERT_EQ_UINT(0, ut_spawn_calls); UT_ASSERT_EQ_UINT(2, ut_free_calls);
   UT_ASSERT_EQ_UINT(0, ut_screen_put_calls);
   UT_ASSERT_EQ_UINT(1, ut_boundary_calls);
   UT_ASSERT_EQ_UINT(1, ut_attribute_calls); UT_ASSERT_EQ_UINT(1, ut_cursor_calls);
   reset_spawnvpe(); od_control.baud = 9600;
   UT_ASSERT_EQ_INT(7, TEST_SPAWNVPE(P_WAIT, "program", ut_arguments, ut_environment));
   UT_ASSERT_EQ_UINT(1, ut_close_calls); UT_ASSERT_EQ_UINT(1, ut_open_calls);
}
#endif

static void covers_time_accounting_modes(void)
{
   reset_spawnvpe();
   UT_ASSERT_EQ_INT(7, TEST_SPAWNVPE(P_WAIT, "program", ut_arguments, ut_environment));
   UT_ASSERT_EQ_INT(18, od_control.user_timelimit);
   reset_spawnvpe(); bIsShell = TRUE;
   UT_ASSERT_EQ_INT(7, TEST_SPAWNVPE(P_WAIT, "program", ut_arguments, ut_environment));
   UT_ASSERT(nNextTimeDeductTime == 120);
   reset_spawnvpe(); od_control.od_spawn_freeze_time = TRUE;
   UT_ASSERT_EQ_INT(7, TEST_SPAWNVPE(P_WAIT, "program", ut_arguments, ut_environment));
   UT_ASSERT(nNextTimeDeductTime == 120);
}
static const UTTestCase ut_cases[] = {
   {"initialize/result", initializes_and_propagates_primitive_result},
   {"terminal session", rejects_a_terminal_session_after_initialization},
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   {"mode and allocation failures", rejects_invalid_mode_and_allocation_failures},
   {"DOS screen wait and serial", covers_dos_screen_wait_and_serial_paths},
#endif
#if !defined(ODPLAT_DOS) && !defined(ODPLAT_DOS32)
   {"wait modes", covers_waiting_and_nonwaiting_modes},
#endif
#ifdef ODPLAT_WIN32
   {"arguments and quote failure", rejects_invalid_arguments_and_quote_failure},
   {"preformatted arguments", preserves_preformatted_windows_arguments},
#endif
   {"time accounting", covers_time_accounting_modes}
};
