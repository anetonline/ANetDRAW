#define UT_CUSTOM_MOCK_ODComCarrier
#define UT_CUSTOM_MOCK_ODComClose
#define UT_CUSTOM_MOCK_ODComFree
#define UT_CUSTOM_MOCK_ODComSetDTR
#define UT_CUSTOM_MOCK_ODDirChangeCurrent
#define UT_CUSTOM_MOCK_ODDropFileClose
#define UT_CUSTOM_MOCK_ODDropFileOpen
#define UT_CUSTOM_MOCK_ODDropFileRecordWriteFailure
#define UT_CUSTOM_MOCK_ODDropFileWrite
#define UT_CUSTOM_MOCK_ODExitInfoExtendedEndian
#define UT_CUSTOM_MOCK_ODExitInfoRA2Endian
#define UT_CUSTOM_MOCK_ODGetElapsedMinutes
#define UT_CUSTOM_MOCK_ODInQueueFree
#define UT_CUSTOM_MOCK_ODKrnlShutdown
#define UT_CUSTOM_MOCK_ODMakeFilename
#define UT_CUSTOM_MOCK_ODProcessExit
#define UT_CUSTOM_MOCK_ODReserveSessionShutdown
#define UT_CUSTOM_MOCK_ODScrnClear
#define UT_CUSTOM_MOCK_ODScrnRemoveMessage
#define UT_CUSTOM_MOCK_ODScrnSetAttribute
#define UT_CUSTOM_MOCK_ODScrnSetBoundary
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos
#define UT_CUSTOM_MOCK_ODScrnShowMessage
#define UT_CUSTOM_MOCK_ODScrnShutdown
#define UT_CUSTOM_MOCK_ODSessionScreenShutdown
#define UT_CUSTOM_MOCK_ODStringCToPascal
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_ODSyncAPIIsNested
#define UT_CUSTOM_MOCK_ODTextDropFileWrite
#define UT_CUSTOM_MOCK_ODWaitDrain
#define UT_CUSTOM_MOCK_ODWriteExitInfoPrimitive
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_strupr
#define UT_CUSTOM_MOCK_strupr
#define UT_CUSTOM_MOCK_time
#ifdef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_ODSyncAPIRelease
#define UT_CUSTOM_MOCK_ODSyncAPIReacquire
#define UT_CUSTOM_MOCK_ODThreadSleep
#endif
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODFrameShutdown
#define UT_CUSTOM_MOCK_ODFrameRequestShutdown
#define UT_CUSTOM_MOCK_ODInExDisableDTR
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#define UT_CUSTOM_MOCK_ODScrnPublish
#ifdef OD_DIAGNOSTICS
#define UT_CUSTOM_MOCK_ODDiagnosticMessage
#endif
#endif

static BOOL ut_drop_open;
static BOOL ut_elapsed_valid;
static BOOL ut_carrier;
static BOOL ut_primitive_write;
static DWORD ut_elapsed_minutes;
static time_t ut_now;
static time_t ut_time_step;
static unsigned ut_process_exit_calls;
static unsigned ut_api_entry_calls;
static unsigned ut_api_exit_calls;
static unsigned ut_before_exit_calls;
static unsigned ut_reserve_shutdown_calls;
static unsigned ut_log_close_calls;
static BOOL ut_nested_api;
static BOOL ut_init_succeeds;
static BOOL ut_init_ends_session;
static char ut_drop_info[25][2];
static tRA2ExitInfoRecord ut_ra2_record;
static tExitInfoRecord ut_exitinfo_record;
static tExtendedExitInfo ut_extended_record;
static unsigned ut_ra2_endian_calls;
static unsigned ut_extended_endian_calls;
#ifdef ODPLAT_WIN32
static tODWindowsSubsystem ut_subsystem;
static unsigned ut_frame_request_calls;
static unsigned ut_console_publish_calls;
#endif

void utm_ODExitInfoRA2Endian(tRA2ExitInfoRecord *record,
   BOOL from_little_endian)
{
   UT_ASSERT_EQ_PTR(&ut_ra2_record, record);
   UT_ASSERT_EQ_INT(FALSE, from_little_endian);
   ++ut_ra2_endian_calls;
}

void utm_ODExitInfoExtendedEndian(tExtendedExitInfo *record,
   BOOL from_little_endian)
{
   UT_ASSERT_EQ_PTR(&ut_extended_record, record);
   UT_ASSERT_EQ_INT(FALSE, from_little_endian);
   ++ut_extended_endian_calls;
}

void *utm_memcpy(void *destination, const void *source, size_t size)
{
   BYTE *out = destination; const BYTE *in = source; size_t index;
   for(index = 0; index < size; ++index) out[index] = in[index];
   return(destination);
}
void utm_free(void *memory) { (void)memory; }
char *utm_od_strupr(char *text)
{
   char *current = text;
   while(*current != '\0') { if(*current >= 'a' && *current <= 'z') *current -= ('a' - 'A'); ++current; }
   return(text);
}
char *utm_strupr(char *text) { return(utm_od_strupr(text)); }
char *utm_ODStringCToPascal(char *destination, BYTE maximum, char *source)
{
   size_t length = strlen(source); if(length > maximum) length = maximum;
   destination[0] = (char)length; utm_memcpy(destination + 1, source, length);
   return(destination);
}
void ODCALL utm_od_init(void)
{
   if(ut_init_succeeds)
      bODInitialized = TRUE;
   if(ut_init_ends_session)
      eODLifecycleState = kODLifecycleTerminal;
}
void utm_ODSyncAPIEntry(void) { ++ut_api_entry_calls; }
void utm_ODSyncAPIExit(void) { ++ut_api_exit_calls; }
BOOL utm_ODSyncAPIIsNested(void) { return(ut_nested_api); }
BOOL ODCALL utm_ODGetElapsedMinutes(DWORD *minutes, time_t start, time_t end)
{
   (void)start; (void)end; if(ut_elapsed_valid) *minutes = ut_elapsed_minutes;
   return(ut_elapsed_valid);
}
time_t utm_time(time_t *storage)
{ time_t result = ut_now; (void)storage; ut_now += ut_time_step; return(result); }
void *utm_ODScrnShowMessage(char *text, int flags) { (void)text; (void)flags; return(&ut_now); }
void utm_ODScrnRemoveMessage(void *window) { (void)window; }
void utm_ODDirChangeCurrent(char *path) { (void)path; }
tODResult utm_ODMakeFilename(char *out, const char *path, const char *name, INT maximum)
{ (void)out; (void)path; (void)name; (void)maximum; return(kODRCSuccess); }
BOOL ODCALL utm_ODDropFileOpen(tODDropFileWriter *writer, const char *path, const char *mode)
{ (void)path; (void)mode; writer->pFile = (FILE *)&ut_now; return(ut_drop_open); }
BOOL ODCALL utm_ODDropFileClose(tODDropFileWriter *writer) { (void)writer; return(TRUE); }
BOOL ODCALL utm_ODDropFileWrite(tODDropFileWriter *writer, const void *data, size_t size)
{ (void)writer; (void)data; (void)size; return(TRUE); }
static void utm_ODDropFileRecordWriteFailure(tODDropFileWriter *writer) { writer->nErrorCode = ERR_GENERALFAILURE; }
void utm_ODTextDropFileWrite(tODDropFileWriter *writer, const char *format, ...)
{ (void)writer; (void)format; }
INT utm_ODWriteExitInfoPrimitive(FILE *file, INT count)
{ (void)file; (void)count; return(ut_primitive_write); }
void utm_ODProcessExit(INT code) { (void)code; ++ut_process_exit_calls; }
void utm_ODReserveSessionShutdown(void) { ++ut_reserve_shutdown_calls; }
void utm_ODWaitDrain(tODMilliSec maximum) { (void)maximum; }
tODResult utm_ODComCarrier(tPortHandle port, BOOL *carrier)
{ (void)port; *carrier = ut_carrier; ut_carrier = FALSE; return(kODRCSuccess); }
tODResult utm_ODComSetDTR(tPortHandle port, BOOL high) { (void)port; (void)high; return(kODRCSuccess); }
tODResult utm_ODComClose(tPortHandle port) { (void)port; return(kODRCSuccess); }
tODResult utm_ODComFree(tPortHandle port) { (void)port; return(kODRCSuccess); }
void utm_ODInQueueFree(tODInQueueHandle queue) { (void)queue; }
void utm_ODKrnlShutdown(void) {}
void utm_ODSessionScreenShutdown(void) {}
void utm_ODScrnShutdown(void) {}
void utm_ODScrnSetBoundary(BYTE left, BYTE top, BYTE right, BYTE bottom)
{ (void)left; (void)top; (void)right; (void)bottom; }
void ODCALL utm_ODScrnSetAttribute(BYTE attribute) { (void)attribute; }
void utm_ODScrnClear(void) {}
void ODCALL utm_ODScrnSetCursorPos(BYTE column, BYTE row)
{ (void)column; (void)row; }
#ifdef OD_THREAD_SUPPORT
unsigned utm_ODSyncAPIRelease(void) { return(3); }
void utm_ODSyncAPIReacquire(unsigned level) { UT_ASSERT_EQ_UINT(3, level); }
void utm_ODThreadSleep(tODMilliSec milliseconds) { (void)milliseconds; }
#endif
#ifdef ODPLAT_WIN32
void utm_ODFrameShutdown(tODThreadHandle *thread) { (void)thread; }
BOOL utm_ODFrameRequestShutdown(tODThreadHandle thread)
{ (void)thread; ++ut_frame_request_calls; return(TRUE); }
void utm_ODInExDisableDTR(void) {}
tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{ return(ut_subsystem); }
void utm_ODScrnPublish(void) { ++ut_console_publish_calls; }
#ifdef OD_DIAGNOSTICS
void utm_ODDiagnosticMessage(const char *message, const char *title) { (void)message; (void)title; }
#endif
#endif

#ifdef ODPLAT_DOS32
static void ODCALL ut_before_exit(void)
#else
static void ut_before_exit(void)
#endif
{
   ++ut_before_exit_calls;
   utt_od_exit(99, TRUE);
}

static BOOL ODCALL ut_log_close(INT errorlevel)
{
   UT_ASSERT_EQ_INT(7, errorlevel); ++ut_log_close_calls; return(0);
}

static void reset_exit(void)
{
   unsigned index;
   memset(&od_control, 0, sizeof(od_control)); bODInitialized = TRUE;
   eODLifecycleState = kODLifecycleActive;
   bODExitPrologueComplete = FALSE;
   bODExitRequestedDuringInitialization = FALSE;
   bODPendingExitNoExit = FALSE;
   bPreOrExit = FALSE; szOriginalDir = NULL; dwFileBPS = 0;
   nInitialRemaining = 30; nStartupUnixTime = 0;
   memset(&ut_ra2_record, 0, sizeof(ut_ra2_record));
   memset(&ut_exitinfo_record, 0, sizeof(ut_exitinfo_record));
   memset(&ut_extended_record, 0, sizeof(ut_extended_record));
   pRA2ExitInfoRecord = &ut_ra2_record; pExitInfoRecord = &ut_exitinfo_record;
   pExtendedExitInfo = &ut_extended_record; pfLogClose = NULL;
   for(index = 0; index < 25; ++index)
   { ut_drop_info[index][0] = '\0'; apszDropFileInfo[index] = ut_drop_info[index]; }
   od_control.od_info_type = 255; od_control.od_noexit = TRUE;
   ut_drop_open = FALSE; ut_elapsed_valid = FALSE; ut_carrier = FALSE;
   ut_primitive_write = TRUE; ut_elapsed_minutes = 0; ut_now = 100;
   ut_time_step = 1;
   ut_process_exit_calls = ut_api_entry_calls = ut_api_exit_calls = 0;
   ut_before_exit_calls = ut_log_close_calls = 0;
   ut_reserve_shutdown_calls = 0;
   ut_ra2_endian_calls = ut_extended_endian_calls = 0;
   ut_nested_api = FALSE;
   ut_init_succeeds = TRUE;
   ut_init_ends_session = FALSE;
#ifdef ODPLAT_WIN32
   ut_subsystem = kODWindowsSubsystemGUI;
   ut_frame_request_calls = ut_console_publish_calls = 0;
#endif
}

static void defers_nested_noexit_teardown(void)
{
   reset_exit();
   ut_nested_api = TRUE;
   utt_od_exit(23, FALSE);
   UT_ASSERT_EQ_INT(kODLifecycleExitPending, eODLifecycleState);
   UT_ASSERT_EQ_INT(TRUE, bODInitialized);
   UT_ASSERT_EQ_INT(23, nODPendingExitErrorLevel);
   UT_ASSERT_EQ_INT(FALSE, bODPendingExitTermCall);
   UT_ASSERT_EQ_UINT(1, ut_api_entry_calls);
   UT_ASSERT_EQ_UINT(1, ut_api_exit_calls);
   UT_ASSERT_EQ_UINT(0, ut_reserve_shutdown_calls);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(1, ut_frame_request_calls);

   reset_exit();
   ut_subsystem = kODWindowsSubsystemConsole;
   ut_nested_api = TRUE;
   utt_od_exit(23, FALSE);
   UT_ASSERT_EQ_UINT(0, ut_frame_request_calls);
#endif
}

static void saves_an_initialization_exit_request_once(void)
{
   reset_exit();
   eODLifecycleState = kODLifecycleInitializing;
   utt_od_exit(23, FALSE);
   utt_od_exit(99, TRUE);
   UT_ASSERT(bODExitRequestedDuringInitialization);
   UT_ASSERT_EQ_INT(23, nODPendingExitErrorLevel);
   UT_ASSERT_EQ_INT(FALSE, bODPendingExitTermCall);
   UT_ASSERT_EQ_UINT(0, ut_api_entry_calls);
}

static void rejects_completed_and_failed_initialization_sessions(void)
{
   reset_exit();
   eODLifecycleState = kODLifecycleExitPending;
   utt_od_exit(23, FALSE);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_api_entry_calls);

   reset_exit();
   eODLifecycleState = kODLifecycleTerminal;
   utt_od_exit(23, FALSE);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_api_entry_calls);

   reset_exit();
   bODInitialized = FALSE;
   ut_init_succeeds = FALSE;
   utt_od_exit(23, FALSE);
   UT_ASSERT_EQ_UINT(0, ut_api_entry_calls);

   reset_exit();
   bODInitialized = FALSE;
   ut_init_ends_session = TRUE;
   utt_od_exit(23, FALSE);
   UT_ASSERT_EQ_UINT(0, ut_api_entry_calls);
}

static void resumes_teardown_after_a_completed_prologue(void)
{
   reset_exit();
   bODExitPrologueComplete = TRUE;
   bODPendingExitNoExit = TRUE;
   utt_od_exit(23, FALSE);
   UT_ASSERT_EQ_UINT(0, ut_before_exit_calls);
   UT_ASSERT_EQ_INT(kODLifecycleTerminal, eODLifecycleState);
   UT_ASSERT_EQ_UINT(1, ut_api_entry_calls);
   UT_ASSERT_EQ_UINT(1, ut_api_exit_calls);
   UT_ASSERT_EQ_UINT(1, ut_reserve_shutdown_calls);
}

static void resets_exit_state_without_terminating_process(void)
{
   reset_exit();
   utt_od_exit(7, FALSE);
   UT_ASSERT_EQ_INT(FALSE, bODInitialized);
   UT_ASSERT_EQ_UINT(1, ut_api_entry_calls);
   UT_ASSERT_EQ_UINT(1, ut_api_exit_calls);
   UT_ASSERT_EQ_UINT(0, ut_process_exit_calls);
   UT_ASSERT_EQ_UINT(1, ut_reserve_shutdown_calls);
}

static void covers_entry_hooks_time_and_process_exit(void)
{
   reset_exit(); bODInitialized = FALSE; od_control.od_noexit = FALSE;
   bPreOrExit = TRUE; od_control.od_before_exit = ut_before_exit;
   od_control.od_maxtime_deduction = 3; od_control.user_timelimit = 20;
   od_control.user_time_used = 5; dwFileBPS = 19200;
   ut_elapsed_valid = TRUE; ut_elapsed_minutes = 4;
   szOriginalDir = "OLD"; pfLogClose = ut_log_close;
   utt_od_exit(7, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_before_exit_calls);
   UT_ASSERT_EQ_UINT(1, ut_log_close_calls);
   UT_ASSERT_EQ_UINT(0, ut_process_exit_calls);
   UT_ASSERT(od_control.baud == 19200);
   UT_ASSERT_EQ_INT(8, od_control.user_time_used);

   reset_exit(); od_control.od_noexit = FALSE; bPreOrExit = FALSE;
   od_control.od_internal_debug = TRUE;
   utt_od_exit(7, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_process_exit_calls);
   UT_ASSERT_EQ_UINT(0, ut_api_exit_calls);
   reset_exit(); od_control.od_noexit = FALSE; bPreOrExit = FALSE;
   od_control.od_internal_debug = FALSE; utt_od_exit(7, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_process_exit_calls);
}

static void run_extended(BYTE type, BOOL term)
{
   reset_exit(); od_control.od_extended_info = TRUE;
   od_control.od_info_type = type; od_control.od_noexit = TRUE;
   od_control.user_sex = 'M'; od_control.user_menustackpointer = 1;
   od_control.user_rip = TRUE; ut_drop_open = TRUE;
   utt_od_exit(7, term);
   UT_ASSERT_NULL(pRA2ExitInfoRecord); UT_ASSERT_NULL(pExitInfoRecord);
   UT_ASSERT_NULL(pExtendedExitInfo);
   if(type == RA2EXITINFO)
      UT_ASSERT_EQ_UINT(1, ut_ra2_endian_calls);
   else if(type == RA1EXITINFO)
      UT_ASSERT_EQ_UINT(1, ut_extended_endian_calls);
}

static void covers_binary_drop_file_formats(void)
{
   bRAStatus = FALSE; run_extended(RA2EXITINFO, FALSE);
   bRAStatus = TRUE; reset_exit(); od_control.od_extended_info = TRUE;
   od_control.od_info_type = RA2EXITINFO; od_control.od_noexit = TRUE;
   od_control.user_sex = 'F'; ut_drop_open = TRUE; utt_od_exit(7, FALSE);

   ut_primitive_write = TRUE; run_extended(EXITINFO, FALSE);
   reset_exit(); od_control.od_extended_info = TRUE; od_control.od_info_type = EXITINFO;
   od_control.od_noexit = TRUE; ut_drop_open = TRUE; ut_primitive_write = FALSE;
   utt_od_exit(7, FALSE);
   run_extended(RA1EXITINFO, FALSE);
   reset_exit(); od_control.od_extended_info = TRUE; od_control.od_info_type = RA1EXITINFO;
   od_control.od_noexit = TRUE; ut_drop_open = TRUE; ut_primitive_write = FALSE;
   utt_od_exit(7, FALSE);
   run_extended(QBBS275EXITINFO, FALSE);
   reset_exit(); od_control.od_extended_info = TRUE;
   od_control.od_info_type = QBBS275EXITINFO; od_control.od_noexit = TRUE;
   od_control.user_menustackpointer = 0; od_control.user_rip = FALSE;
   ut_drop_open = TRUE; ut_primitive_write = FALSE; utt_od_exit(7, TRUE);
   reset_exit(); od_control.od_extended_info = TRUE; od_control.od_noexit = TRUE;
   od_control.od_info_type = 255; ut_drop_open = TRUE; utt_od_exit(7, FALSE);
   reset_exit(); od_control.od_extended_info = TRUE; od_control.od_noexit = TRUE;
   od_control.od_info_type = 255; ut_drop_open = FALSE; utt_od_exit(7, FALSE);
}

static void run_text(BYTE type)
{
   reset_exit(); od_control.od_info_type = type; od_control.od_noexit = TRUE;
   od_control.baud = 9600; dwFileBPS = 9600; od_control.user_ansi = TRUE;
   ut_drop_open = TRUE; utt_od_exit(7, FALSE);
}

static void covers_text_drop_file_formats(void)
{
   reset_exit(); od_control.od_info_type = DOORSYS_GAP; od_control.od_noexit = TRUE;
   ut_drop_open = FALSE; utt_od_exit(7, FALSE);
   reset_exit(); od_control.od_info_type = DOORSYS_GAP; od_control.od_noexit = TRUE;
   ut_drop_open = TRUE; btDoorSYSLock = 0; od_control.user_rip = TRUE;
   utt_od_exit(7, FALSE);
   reset_exit(); od_control.od_info_type = DOORSYS_GAP; od_control.od_noexit = TRUE;
   ut_drop_open = TRUE; btDoorSYSLock = 1; dwFileBPS = 9600; od_control.user_ansi = TRUE;
   utt_od_exit(7, FALSE);
   reset_exit(); od_control.od_info_type = DOORSYS_WILDCAT; od_control.od_noexit = TRUE;
   ut_drop_open = TRUE; btDoorSYSLock = 2; dwFileBPS = 9600;
   od_control.user_error_free = TRUE; utt_od_exit(7, FALSE);
   reset_exit(); od_control.od_info_type = DOORSYS_WILDCAT; od_control.od_noexit = TRUE;
   ut_drop_open = TRUE; dwFileBPS = 9600; od_control.user_error_free = FALSE;
   utt_od_exit(7, FALSE);
   reset_exit(); od_control.od_info_type = DOORSYS_GAP; od_control.od_noexit = TRUE;
   ut_drop_open = TRUE; btDoorSYSLock = 3; utt_od_exit(7, FALSE);
   run_text(DOORSYS_DRWY);
   reset_exit(); od_control.od_info_type = DOORSYS_DRWY; od_control.od_noexit = TRUE;
   ut_drop_open = TRUE; od_control.user_ansi = FALSE; utt_od_exit(7, FALSE);
   reset_exit(); od_control.od_info_type = DOORSYS_DRWY; od_control.od_noexit = TRUE;
   ut_drop_open = FALSE; utt_od_exit(7, FALSE);
   run_text(SFDOORSDAT);
   reset_exit(); od_control.od_info_type = SFDOORSDAT; od_control.od_noexit = TRUE;
   ut_drop_open = FALSE; utt_od_exit(7, FALSE);
   reset_exit(); od_control.od_info_type = SFDOORSDAT; od_control.od_noexit = TRUE;
   ut_drop_open = TRUE; dwFileBPS = 9600; od_control.sysop_next = TRUE;
   od_control.user_error_free = TRUE; ut_drop_info[15][0] = 'X';
   od_control.user_rip = TRUE; od_control.user_wantchat = TRUE; utt_od_exit(7, FALSE);
   reset_exit(); od_control.od_info_type = SFDOORSDAT; od_control.od_noexit = TRUE;
   ut_drop_open = TRUE; ut_drop_info[15][0] = 'X';
   od_control.user_rip = FALSE; od_control.user_wantchat = FALSE; utt_od_exit(7, FALSE);
   run_text(CHAINTXT);
   reset_exit(); od_control.od_info_type = CHAINTXT; od_control.od_noexit = TRUE;
   ut_drop_open = TRUE; utt_od_exit(7, FALSE);
   reset_exit(); od_control.od_info_type = CHAINTXT; od_control.od_noexit = TRUE;
   ut_drop_open = FALSE; utt_od_exit(7, FALSE);
   run_text(TRIBBSSYS);
   reset_exit(); od_control.od_info_type = TRIBBSSYS; od_control.od_noexit = TRUE;
   ut_drop_open = FALSE; utt_od_exit(7, FALSE);
   reset_exit(); od_control.od_info_type = TRIBBSSYS; od_control.od_noexit = TRUE;
   ut_drop_open = TRUE; od_control.user_expert = TRUE; od_control.user_ansi = FALSE;
   od_control.od_com_flow_control = COM_RTSCTS_FLOW;
   od_control.user_error_free = TRUE; od_control.user_rip = TRUE;
   utt_od_exit(7, FALSE);
}

static void covers_disconnect_and_screen_cleanup(void)
{
   reset_exit(); od_control.od_noexit = TRUE; dwFileBPS = 9600;
   ut_carrier = TRUE; od_control.od_hanging_up = "HANG"; utt_od_exit(7, TRUE);
   reset_exit(); od_control.od_noexit = TRUE; dwFileBPS = 9600;
   ut_carrier = TRUE; ut_time_step = 10; utt_od_exit(7, TRUE);
   reset_exit(); od_control.od_noexit = TRUE; dwFileBPS = 9600;
   od_control.od_clear_on_exit = TRUE;
#ifdef ODPLAT_WIN32
   ut_subsystem = kODWindowsSubsystemConsole;
#endif
   utt_od_exit(7, FALSE);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(1, ut_console_publish_calls);
   reset_exit(); od_control.od_noexit = TRUE;
   ut_subsystem = kODWindowsSubsystemConsole;
   od_control.od_clear_on_exit = FALSE;
   utt_od_exit(7, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_console_publish_calls);
#endif
}

static const UTTestCase ut_cases[] = {
   {"nested noexit", defers_nested_noexit_teardown},
   {"initialization exit", saves_an_initialization_exit_request_once},
   {"terminal guards", rejects_completed_and_failed_initialization_sessions},
   {"completed prologue", resumes_teardown_after_a_completed_prologue},
   {"shutdown without process exit", resets_exit_state_without_terminating_process},
   {"entry hooks time and process exit", covers_entry_hooks_time_and_process_exit},
   {"binary drop files", covers_binary_drop_file_formats},
   {"text drop files", covers_text_drop_file_formats},
   {"disconnect and screen cleanup", covers_disconnect_and_screen_cleanup}
};
