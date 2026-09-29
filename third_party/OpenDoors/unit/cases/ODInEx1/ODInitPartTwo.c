#include <setjmp.h>

#ifdef ODPLAT_DOS32
#define UT_PERSONALITY_CALL ODCALL
#else
#define UT_PERSONALITY_CALL
#endif

#define UT_CUSTOM_MOCK_GetModuleHandleA
#define UT_CUSTOM_MOCK_ODAtExitCallback
#define UT_CUSTOM_MOCK_ODComAlloc
#define UT_CUSTOM_MOCK_ODComGetMethod
#define UT_CUSTOM_MOCK_ODComOpen
#define UT_CUSTOM_MOCK_ODComOpenFromExistingHandle
#define UT_CUSTOM_MOCK_ODComSetDTR
#define UT_CUSTOM_MOCK_ODComSetFIFO
#define UT_CUSTOM_MOCK_ODComSetFlowControl
#define UT_CUSTOM_MOCK_ODComSetIRQ
#define UT_CUSTOM_MOCK_ODComSetIdleFunction
#define UT_CUSTOM_MOCK_ODComSetPort
#define UT_CUSTOM_MOCK_ODComSetPortAddress
#define UT_CUSTOM_MOCK_ODComSetPreferredMethod
#define UT_CUSTOM_MOCK_ODComSetRXBuf
#define UT_CUSTOM_MOCK_ODComSetSpeed
#define UT_CUSTOM_MOCK_ODComSetTXBuf
#define UT_CUSTOM_MOCK_ODFrameStart
#define UT_CUSTOM_MOCK_ODInitApplyUserInfo
#define UT_CUSTOM_MOCK_ODInitError
#define UT_CUSTOM_MOCK_ODInitSelectTerminalBaud
#define UT_CUSTOM_MOCK_ODKrnlInitialize
#define UT_CUSTOM_MOCK_ODKrnlRefreshUIState
#define UT_CUSTOM_MOCK_ODKrnlShutdown
#define UT_CUSTOM_MOCK_ODMultiResolvePersonality
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#define UT_CUSTOM_MOCK_ODReserveSessionInitialize
#define UT_CUSTOM_MOCK_ODScrnCreateWindow
#define UT_CUSTOM_MOCK_ODScrnDestroyWindow
#define UT_CUSTOM_MOCK_ODScrnDisplayString
#define UT_CUSTOM_MOCK_ODScrnInitialize
#define UT_CUSTOM_MOCK_ODScrnLocalInput
#define UT_CUSTOM_MOCK_ODScrnPublish
#define UT_CUSTOM_MOCK_ODScrnSetAttribute
#define UT_CUSTOM_MOCK_ODScrnSetBoundary
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos
#define UT_CUSTOM_MOCK_ODSessionScreenInitialize
#define UT_CUSTOM_MOCK_ODStringCopy
#define UT_CUSTOM_MOCK_atexit
#define UT_CUSTOM_MOCK_cfgetispeed
#define UT_CUSTOM_MOCK_cfgetospeed
#define UT_CUSTOM_MOCK_exit
#define UT_CUSTOM_MOCK_gethostname
#define UT_CUSTOM_MOCK_getpwuid
#define UT_CUSTOM_MOCK_getuid
#define UT_CUSTOM_MOCK_isatty
#define UT_CUSTOM_MOCK_od_clr_scr
#define UT_CUSTOM_MOCK_od_kernel
#define UT_CUSTOM_MOCK_od_set_statusline
#define UT_CUSTOM_MOCK_pdef_opendoors
#define UT_CUSTOM_MOCK_strcpy
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_tcgetattr

static int ut_port_token;
static tPortHandle ut_port = (tPortHandle)&ut_port_token;
static tODResult ut_alloc_result;
static tODResult ut_open_result;
static tODResult ut_existing_result;
static tODResult ut_kernel_result;
static tODResult ut_frame_result;
static tODResult ut_screen_result;
static tODResult ut_reserve_result;
static tComMethod ut_com_method;
static unsigned ut_alloc_calls;
static unsigned ut_open_calls;
static unsigned ut_existing_calls;
static unsigned ut_dtr_calls;
static unsigned ut_kernel_calls;
static unsigned ut_kernel_shutdown_calls;
static unsigned ut_refresh_calls;
static unsigned ut_clear_calls;
static unsigned ut_atexit_calls;
static unsigned ut_frame_calls;
static unsigned ut_publish_calls;
static unsigned ut_error_calls;
static unsigned ut_reserve_calls;
static unsigned ut_exit_calls;
static unsigned ut_log_calls;
static unsigned ut_preferred_calls;
static tComMethod ut_preferred_method;
static BYTE ut_flow;
static long ut_speed;
static BYTE ut_port_number;
static BYTE ut_irq;
static int ut_address;
static int ut_rx_size;
static int ut_tx_size;
static BYTE ut_fifo;
static BYTE ut_boundary_bottom;
static INT ut_session_width;
static INT ut_session_height;
static const char *ut_error_text;
static jmp_buf ut_exit_target;
static BOOL ut_exit_expected;
static BOOL ut_refresh_result;

#ifdef ODPLAT_NIX
static int ut_isatty_result;
static int ut_tcgetattr_result;
static unsigned ut_apply_user_calls;
static struct passwd ut_passwd;
#endif

#ifdef ODPLAT_WIN32
static int ut_module_token;
static HMODULE ut_module = (HMODULE)&ut_module_token;
static BOOL ut_first_module_missing;
static BOOL ut_cancel_during_frame_start;
static tODWindowsSubsystem ut_subsystem;
#endif

#if defined(OD_TEXTMODE) || defined(ODPLAT_WIN32)
static int ut_window_token;
static BOOL ut_window_available;
static unsigned ut_personality_calls;
static unsigned ut_status_calls;
static INT ut_status_value;
static BOOL ut_set_personality_result;
#endif

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0')
      ++length;
   return length;
}

char *utm_strcpy(char *destination, const char *source)
{
   char *result = destination;
   while((*destination++ = *source++) != '\0')
      ;
   return result;
}

tODResult utm_ODComAlloc(tPortHandle *port)
{
   ++ut_alloc_calls;
   if(ut_alloc_result == kODRCSuccess)
      *port = ut_port;
   return ut_alloc_result;
}

static void ut_assert_port(tPortHandle port)
{
   UT_ASSERT_EQ_PTR(ut_port, port);
}

tODResult utm_ODComSetPreferredMethod(tPortHandle port, tComMethod method)
{
   ut_assert_port(port);
   ++ut_preferred_calls;
   ut_preferred_method = method;
   return kODRCSuccess;
}

tODResult utm_ODComOpenFromExistingHandle(tPortHandle port,
   DWORD_PTR handle)
{
   ut_assert_port(port);
   UT_ASSERT(handle == od_control.od_open_handle);
   ++ut_existing_calls;
   return ut_existing_result;
}

tODResult utm_ODComSetFlowControl(tPortHandle port, BYTE flow)
{
   ut_assert_port(port);
   ut_flow = flow;
   return kODRCSuccess;
}

tODResult utm_ODComSetPort(tPortHandle port, BYTE number)
{
   ut_assert_port(port);
   ut_port_number = number;
   return kODRCSuccess;
}

tODResult utm_ODComSetPortAddress(tPortHandle port, int address)
{
   ut_assert_port(port);
   ut_address = address;
   return kODRCSuccess;
}

tODResult utm_ODComSetIRQ(tPortHandle port, BYTE irq)
{
   ut_assert_port(port);
   ut_irq = irq;
   return kODRCSuccess;
}

tODResult utm_ODComSetSpeed(tPortHandle port, long speed)
{
   ut_assert_port(port);
   ut_speed = speed;
   return kODRCSuccess;
}

tODResult utm_ODComSetRXBuf(tPortHandle port, int size)
{
   ut_assert_port(port);
   ut_rx_size = size;
   return kODRCSuccess;
}

tODResult utm_ODComSetTXBuf(tPortHandle port, int size)
{
   ut_assert_port(port);
   ut_tx_size = size;
   return kODRCSuccess;
}

tODResult utm_ODComSetFIFO(tPortHandle port, BYTE setting)
{
   ut_assert_port(port);
   ut_fifo = setting;
   return kODRCSuccess;
}

void ODCALL utm_od_kernel(void);

tODResult utm_ODComSetIdleFunction(tPortHandle port,
   void (ODCALL *callback)(void))
{
   ut_assert_port(port);
   UT_ASSERT(callback == utm_od_kernel);
   return kODRCSuccess;
}

tODResult utm_ODComOpen(tPortHandle port)
{
   ut_assert_port(port);
   ++ut_open_calls;
   return ut_open_result;
}

tODResult utm_ODComSetDTR(tPortHandle port, BOOL high)
{
   ut_assert_port(port);
   UT_ASSERT_EQ_INT(TRUE, high);
   ++ut_dtr_calls;
   return kODRCSuccess;
}

tODResult utm_ODComGetMethod(tPortHandle port, tComMethod *method)
{
   ut_assert_port(port);
   *method = ut_com_method;
   return kODRCSuccess;
}

void ODCALL utm_od_kernel(void)
{
}

tODResult utm_ODScrnInitialize(void)
{
   return ut_screen_result;
}

tODResult utm_ODReserveSessionInitialize(void)
{
   ++ut_reserve_calls;
   return ut_reserve_result;
}

void utm_ODScrnSetBoundary(BYTE left, BYTE top, BYTE right, BYTE bottom)
{
   UT_ASSERT_EQ_INT(1, left);
   UT_ASSERT_EQ_INT(1, top);
   UT_ASSERT_EQ_INT(80, right);
   ut_boundary_bottom = bottom;
}

void utm_ODSessionScreenInitialize(INT width, INT height)
{
   ut_session_width = width;
   ut_session_height = height;
}

#ifndef ODPLAT_WIN32
void utm_ODAtExitCallback(void);

int utm_atexit(void (*callback)(void))
{
   UT_ASSERT(callback == utm_ODAtExitCallback);
   ++ut_atexit_calls;
   return 0;
}

void utm_ODAtExitCallback(void)
{
}
#endif

void ODCALL utm_od_clr_scr(void)
{
   ++ut_clear_calls;
}

tODResult utm_ODKrnlInitialize(void)
{
   ++ut_kernel_calls;
   return ut_kernel_result;
}

BOOL utm_ODKrnlRefreshUIState(void)
{
   ++ut_refresh_calls;
   return ut_refresh_result;
}

void utm_ODKrnlShutdown(void)
{
   ++ut_kernel_shutdown_calls;
}

void utm_ODInitError(char *text)
{
   ++ut_error_calls;
   ut_error_text = text;
}

void utm_exit(int status)
{
   ++ut_exit_calls;
   UT_ASSERT_EQ_INT(od_control.od_errorlevel[1], status);
   if(!ut_exit_expected)
      UT_ASSERT(FALSE);
   longjmp(ut_exit_target, 1);
}

void utm_ODStringCopy(char *destination, const char *source, INT size)
{
   INT index = 0;
   while(index < size - 1 && source[index] != '\0')
   {
      destination[index] = source[index];
      ++index;
   }
   destination[index] = '\0';
}

#ifdef ODPLAT_NIX
int utm_isatty(int descriptor)
{
   UT_ASSERT_EQ_INT(STDIN_FILENO, descriptor);
   return ut_isatty_result;
}

int utm_tcgetattr(int descriptor, struct termios *term)
{
   UT_ASSERT_EQ_INT(STDIN_FILENO, descriptor);
   memset(term, 0, sizeof(*term));
   return ut_tcgetattr_result;
}

speed_t utm_cfgetispeed(const struct termios *term)
{
   (void)term;
   return B9600;
}

speed_t utm_cfgetospeed(const struct termios *term)
{
   (void)term;
   return B19200;
}

DWORD utm_ODInitSelectTerminalBaud(speed_t input, speed_t output)
{
   UT_ASSERT(input == B9600);
   UT_ASSERT(output == B19200);
   return 57600;
}

int utm_gethostname(char *name, size_t size)
{
   const char *source = "test-host";
   size_t index = 0;
   while(index + 1 < size && source[index] != '\0')
   {
      name[index] = source[index];
      ++index;
   }
   name[index] = '\0';
   return 0;
}

uid_t utm_getuid(void)
{
   return (uid_t)42;
}

struct passwd *utm_getpwuid(uid_t user)
{
   UT_ASSERT(user == (uid_t)42);
   return &ut_passwd;
}

void utm_ODInitApplyUserInfo(const struct passwd *user)
{
   UT_ASSERT_EQ_PTR(&ut_passwd, user);
   ++ut_apply_user_calls;
}
#endif

#ifdef ODPLAT_WIN32
HMODULE WINAPI utm_GetModuleHandleA(LPCSTR name)
{
   if(name != NULL && ut_first_module_missing)
      return NULL;
   return ut_module;
}

tODResult utm_ODFrameStart(HANDLE instance, tODThreadHandle *thread)
{
   UT_ASSERT_EQ_PTR(ut_module, instance);
   (void)thread;
   ++ut_frame_calls;
   if(ut_cancel_during_frame_start)
      bWindowsStartupCancelled = TRUE;
   return ut_frame_result;
}

void utm_ODScrnPublish(void)
{
   ++ut_publish_calls;
}
#endif

#if defined(OD_TEXTMODE) || defined(ODPLAT_WIN32)
void *utm_ODScrnCreateWindow(BYTE left, BYTE top, BYTE right, BYTE bottom,
   BYTE attribute, char *title, BYTE title_attribute)
{
   UT_ASSERT_EQ_INT(10, left);
   UT_ASSERT_EQ_INT(8, top);
   UT_ASSERT_EQ_INT(70, right);
   UT_ASSERT_EQ_INT(15, bottom);
   UT_ASSERT_EQ_INT(od_control.od_local_win_col, attribute);
   UT_ASSERT_EQ_PTR(od_control.od_prog_name, title);
   UT_ASSERT_EQ_INT(attribute, title_attribute);
   return ut_window_available ? &ut_window_token : NULL;
}

void utm_ODScrnDestroyWindow(void *window)
{
   UT_ASSERT_EQ_PTR(&ut_window_token, window);
}

void ODCALL utm_ODScrnSetCursorPos(BYTE column, BYTE row)
{
   (void)column;
   (void)row;
}

void ODCALL utm_ODScrnDisplayString(const char *text)
{
   UT_ASSERT_NOT_NULL(text);
}

void utm_ODScrnLocalInput(BYTE left, BYTE row, char *text, BYTE maximum)
{
   UT_ASSERT_EQ_INT(23, left);
   UT_ASSERT_EQ_INT(14, row);
   UT_ASSERT_EQ_INT(35, maximum);
   utm_strcpy(text, "Local User");
}

void ODCALL utm_ODScrnSetAttribute(BYTE attribute)
{
   UT_ASSERT_EQ_INT(7, attribute);
}

void ODCALL utm_pdef_opendoors(BYTE operation)
{
   UT_ASSERT_EQ_INT(20, operation);
   ++ut_personality_calls;
}

static void UT_PERSONALITY_CALL ut_test_personality(BYTE operation)
{
   UT_ASSERT_EQ_INT(20, operation);
   ++ut_personality_calls;
}

OD_PERSONALITY_CALLBACK *utm_ODMultiResolvePersonality(
   OD_PERSONALITY_PROC *personality)
{
#ifdef ODPLAT_WIN32
   if(personality == NULL) return(ut_test_personality);
#endif
   return((OD_PERSONALITY_CALLBACK *)personality);
}

#ifdef ODPLAT_WIN32
tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{
   return(ut_subsystem);
}
#endif

static BOOL ODCALL ut_set_personality(const char *name)
{
   UT_ASSERT_EQ_PTR(szDesiredPersonality, name);
   return ut_set_personality_result;
}

void ODCALL utm_od_set_statusline(INT setting)
{
   ++ut_status_calls;
   ut_status_value = setting;
}
#endif

#ifdef ODPLAT_DOS32
static void ODCALL ut_log_callback(void)
#else
static void ut_log_callback(void)
#endif
{
   ++ut_log_calls;
}

static void reset_part_two_fixture(void)
{
   unsigned index;
   memset(&od_control, 0, sizeof(od_control));
   od_control.baud = 9600;
   od_control.port = 1;
   od_control.od_com_flow_control = COM_DEFAULT_FLOW;
   od_control.od_com_fifo_trigger = 4;
   od_control.od_errorlevel[1] = 77;
   ut_alloc_result = kODRCSuccess;
   ut_open_result = kODRCSuccess;
   ut_existing_result = kODRCSuccess;
   ut_kernel_result = kODRCSuccess;
   ut_frame_result = kODRCSuccess;
   ut_screen_result = kODRCSuccess;
   ut_reserve_result = kODRCSuccess;
#ifdef ODPLAT_NIX
   ut_com_method = kComMethodStdIO;
#else
#ifdef ODPLAT_WIN32
   ut_com_method = kComMethodWin32;
#else
   ut_com_method = kComMethodFOSSIL;
#endif
#endif
   ut_alloc_calls = 0;
   ut_open_calls = 0;
   ut_existing_calls = 0;
   ut_dtr_calls = 0;
   ut_kernel_calls = 0;
   ut_kernel_shutdown_calls = 0;
   ut_refresh_calls = 0;
   ut_clear_calls = 0;
   ut_atexit_calls = 0;
   ut_frame_calls = 0;
   ut_publish_calls = 0;
   ut_error_calls = 0;
   ut_reserve_calls = 0;
   ut_exit_calls = 0;
   ut_log_calls = 0;
   ut_preferred_calls = 0;
   ut_preferred_method = kComMethodFOSSIL;
   ut_flow = 255;
   ut_speed = -1;
   ut_port_number = 255;
   ut_irq = 0;
   ut_address = 0;
   ut_rx_size = 0;
   ut_tx_size = 0;
   ut_fifo = 0;
   ut_boundary_bottom = 0;
   ut_session_width = 0;
   ut_session_height = 0;
   ut_error_text = NULL;
   ut_exit_expected = FALSE;
   ut_refresh_result = TRUE;
   hSerialPort = NULL;
   nForcedPort = -1;
   dwForcedBPS = 1;
   nInitialRemaining = 0;
   bPromptForUserName = FALSE;
   bSysopNameSet = FALSE;
   bTelnetSocket = FALSE;
   od_control.user_screenwidth = 80;
   od_control.user_screen_length = 23;
   szForcedSysopName[0] = '\0';
   for(index = 0; index < DIM(apszLogMessages); ++index)
      od_control.od_logfile_messages[index] = NULL;
#ifndef ODPLAT_WIN32
   bPreset = FALSE;
#endif
#ifdef ODPLAT_NIX
   ut_isatty_result = 0;
   ut_tcgetattr_result = -1;
   ut_apply_user_calls = 0;
   memset(&ut_passwd, 0, sizeof(ut_passwd));
#endif
#ifdef ODPLAT_WIN32
   ut_first_module_missing = FALSE;
   ut_cancel_during_frame_start = FALSE;
   ut_subsystem = kODWindowsSubsystemConsole;
   od_control.od_silent_mode = TRUE;
#endif
#if defined(OD_TEXTMODE) || defined(ODPLAT_WIN32)
   ut_window_available = FALSE;
   ut_personality_calls = 0;
   ut_status_calls = 0;
   ut_status_value = -1;
   ut_set_personality_result = TRUE;
   pfSetPersonality = NULL;
   pfCurrentPersonality = NULL;
   od_control.od_default_personality = NULL;
   bRAStatus = FALSE;
#endif
}

static void initializes_defaults_and_a_serial_session(void)
{
   reset_part_two_fixture();
   od_control.user_timelimit = 60;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_INT(0x0f, od_control.od_list_title_col);
   UT_ASSERT_EQ_INT(0x07, od_control.od_menu_highlight_col);
   UT_ASSERT_EQ_INT(0x0c, od_control.od_chat_color1);
   UT_ASSERT_EQ_INT(0x0f, od_control.od_chat_color2);
   UT_ASSERT_EQ_INT('`', od_control.od_color_delimiter);
   UT_ASSERT_EQ_INT(PAGE_USE_HOURS, od_control.od_okaytopage);
   UT_ASSERT_EQ_INT(200, od_control.od_inactivity);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_always_clear);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_clear_on_exit);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_no_ra_codes);
   UT_ASSERT(strncmp("\r\n", od_control.od_before_shell, 2) == 0);
   UT_ASSERT(strstr(od_control.od_after_shell, "\r\n")
      == od_control.od_after_shell);
   UT_ASSERT(strstr(od_control.od_before_chat, "\r\n")
      == od_control.od_before_chat);
   UT_ASSERT(strstr(od_control.od_no_response, "\r\n") != NULL);
   UT_ASSERT(strcmp("Sysop", od_control.sysop_name) == 0);
   UT_ASSERT(strcmp("~+++~  AT&D0|  ATO|", od_control.od_disable_dtr) == 0);
   UT_ASSERT_EQ_INT(60, nInitialRemaining);
   UT_ASSERT_EQ_UINT(1, ut_alloc_calls);
   UT_ASSERT_EQ_UINT(1, ut_open_calls);
   UT_ASSERT_EQ_UINT(1, ut_dtr_calls);
   UT_ASSERT_EQ_INT(1, ut_port_number);
   UT_ASSERT_EQ_INT(9600, ut_speed);
   UT_ASSERT_EQ_INT(256, ut_rx_size);
   UT_ASSERT_EQ_INT(3072, ut_tx_size);
   UT_ASSERT_EQ_INT(FIFO_ENABLE | FIFO_TRIGGER_4, ut_fifo);
   UT_ASSERT_EQ_INT(9600, od_control.od_connect_speed);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_list_stop);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_list_pause);
   UT_ASSERT(strcmp("DOOR.LOG", od_control.od_logfile_name) == 0);
   UT_ASSERT_EQ_UINT(1, ut_kernel_calls);
   UT_ASSERT_EQ_UINT(1, ut_reserve_calls);
#if defined(OD_TEXTMODE) || defined(ODPLAT_WIN32)
   UT_ASSERT_EQ_INT(23, ut_boundary_bottom);
   UT_ASSERT_EQ_INT(80, ut_session_width);
   UT_ASSERT_EQ_INT(23, ut_session_height);
#else
   UT_ASSERT_EQ_INT(25, ut_boundary_bottom);
   UT_ASSERT_EQ_INT(80, ut_session_width);
   UT_ASSERT_EQ_INT(25, ut_session_height);
#endif
}

static void preserves_configured_defaults_and_caps_session_time(void)
{
   unsigned index;
   reset_part_two_fixture();
   od_control.od_list_title_col = 1;
   od_control.od_continue_col = 2;
   od_control.od_list_name_col = 3;
   od_control.od_list_size_col = 4;
   od_control.od_list_comment_col = 5;
   od_control.od_list_offline_col = 6;
   od_control.od_local_win_col = 7;
   od_control.od_menu_title_col = 8;
   od_control.od_menu_border_col = 9;
   od_control.od_menu_text_col = 10;
   od_control.od_menu_key_col = 11;
   od_control.od_menu_highkey_col = 12;
   od_control.od_menu_highlight_col = 13;
   od_control.od_chat_color1 = 14;
   od_control.od_chat_color2 = 15;
   utm_strcpy(od_control.od_disable_dtr, "custom DTR");
   utm_strcpy(od_control.sysop_name, "Custom Sysop");
   utm_strcpy(od_control.od_logfile_name, "CUSTOM.LOG");
   od_control.user_timelimit = 200;
   od_control.od_maxtime = 100;
   od_control.od_connect_speed = 19200;
   for(index = 0; index < DIM(apszLogMessages); ++index)
      od_control.od_logfile_messages[index] = (char *)"custom";
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_INT(1, od_control.od_list_title_col);
   UT_ASSERT_EQ_INT(13, od_control.od_menu_highlight_col);
   UT_ASSERT_EQ_INT(14, od_control.od_chat_color1);
   UT_ASSERT_EQ_INT(15, od_control.od_chat_color2);
   UT_ASSERT(strcmp("custom DTR", od_control.od_disable_dtr) == 0);
   UT_ASSERT(strcmp("Custom Sysop", od_control.sysop_name) == 0);
   UT_ASSERT_EQ_INT(100, od_control.user_timelimit);
   UT_ASSERT_EQ_INT(100, od_control.od_maxtime_deduction);
   UT_ASSERT_EQ_INT(19200, od_control.od_connect_speed);
   UT_ASSERT(strcmp("CUSTOM.LOG", od_control.od_logfile_name) == 0);
}

static void exercises_maximum_time_decision_boundaries(void)
{
   reset_part_two_fixture();
   od_control.od_maxtime = 0;
   utt_ODInitPartTwo();

   reset_part_two_fixture();
   od_control.od_maxtime = 1441;
   utt_ODInitPartTwo();

   reset_part_two_fixture();
   od_control.od_maxtime = 100;
   od_control.user_timelimit = 100;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_INT(100, od_control.user_timelimit);
   UT_ASSERT_EQ_INT(0, od_control.od_maxtime_deduction);
}

static void clears_rip_and_invokes_the_log_component(void)
{
   reset_part_two_fixture();
   od_control.user_rip = TRUE;
   od_control.od_logfile = ut_log_callback;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_UINT(1, ut_clear_calls);
   UT_ASSERT_EQ_UINT(1, ut_log_calls);
}

static void run_expecting_fatal_exit(void)
{
   ut_exit_expected = TRUE;
   if(setjmp(ut_exit_target) == 0)
   {
      utt_ODInitPartTwo();
      UT_ASSERT(FALSE);
   }
   ut_exit_expected = FALSE;
   UT_ASSERT_EQ_UINT(1, ut_exit_calls);
   UT_ASSERT_EQ_UINT(1, ut_error_calls);
   UT_ASSERT_NOT_NULL(ut_error_text);
}

static void exercises_forced_local_name_and_bps_decisions(void)
{
   reset_part_two_fixture();
   od_control.od_force_local = TRUE;
   utm_strcpy(od_control.user_name, "Existing User");
   utt_ODInitPartTwo();
   UT_ASSERT(strcmp("Existing User", od_control.user_name) == 0);
   UT_ASSERT_EQ_INT(FALSE, bPromptForUserName);

   reset_part_two_fixture();
   od_control.od_force_local = TRUE;
   od_control.od_disable = DIS_NAME_PROMPT;
   bSysopNameSet = TRUE;
   utm_strcpy(szForcedSysopName, "Forced Sysop");
   utt_ODInitPartTwo();
   UT_ASSERT(strcmp("Forced Sysop", od_control.user_name) == 0);
   UT_ASSERT_EQ_INT(FALSE, bPromptForUserName);

   reset_part_two_fixture();
   od_control.od_force_local = TRUE;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_INT(TRUE, bPromptForUserName);

   reset_part_two_fixture();
   od_control.baud = 0;
   dwForcedBPS = 4800;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_INT(0, od_control.baud);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_INT(TRUE, od_control.od_silent_mode);
#else
   UT_ASSERT_EQ_INT(FALSE, od_control.od_silent_mode);
#endif

   reset_part_two_fixture();
   od_control.baud = 0;
   od_control.od_disable = DIS_LOCAL_OVERRIDE;
   dwForcedBPS = 4800;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_INT(4800, od_control.baud);

   reset_part_two_fixture();
   od_control.baud = 9600;
   dwForcedBPS = 4800;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_INT(4800, od_control.baud);
}

static void exercises_serial_configuration_matrix(void)
{
   static const INT flows[] = {
      COM_NO_FLOW, COM_RTSCTS_FLOW, COM_DEFAULT_FLOW, 99
   };
   static const INT triggers[] = {1, 8, 14, 3};
   unsigned index;

   reset_part_two_fixture();
   od_control.od_use_socket = TRUE;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_UINT(1, ut_preferred_calls);
   UT_ASSERT_EQ_INT(kComMethodSocket, ut_preferred_method);

   reset_part_two_fixture();
   od_control.od_use_socket = TRUE; bTelnetSocket = TRUE;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_UINT(1, ut_preferred_calls);
   UT_ASSERT_EQ_INT(kComMethodTelnetSocket, ut_preferred_method);

   for(index = 0; index < DIM(flows); ++index)
   {
      reset_part_two_fixture();
      od_control.od_com_flow_control = flows[index];
      utt_ODInitPartTwo();
      if(flows[index] == COM_NO_FLOW)
         UT_ASSERT_EQ_INT(FLOW_NONE, ut_flow);
      else if(flows[index] == COM_RTSCTS_FLOW)
         UT_ASSERT_EQ_INT(FLOW_RTSCTS, ut_flow);
   }

   for(index = 0; index < DIM(triggers); ++index)
   {
      reset_part_two_fixture();
      od_control.od_com_fifo_trigger = triggers[index];
      utt_ODInitPartTwo();
      if(triggers[index] == 3)
      {
         UT_ASSERT_EQ_INT(4, od_control.od_com_fifo_trigger);
         UT_ASSERT_EQ_INT(FIFO_ENABLE | FIFO_TRIGGER_4, ut_fifo);
      }
   }

   reset_part_two_fixture();
   od_control.od_use_socket = TRUE;
   od_control.od_no_fossil = TRUE;
   od_control.od_com_address = 1016;
   od_control.od_com_irq = 5;
   od_control.od_disable = DIS_BPS_SETTING;
   od_control.od_com_rx_buf = 1000;
   od_control.od_com_tx_buf = 2000;
   od_control.od_com_no_fifo = TRUE;
   nForcedPort = 3;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_UINT(2, ut_preferred_calls);
   UT_ASSERT_EQ_INT(kComMethodUART, ut_preferred_method);
   UT_ASSERT_EQ_INT(3, od_control.port);
   UT_ASSERT_EQ_INT(3, ut_port_number);
   UT_ASSERT_EQ_INT(1016, ut_address);
   UT_ASSERT_EQ_INT(5, ut_irq);
   UT_ASSERT_EQ_INT(SPEED_UNSPECIFIED, ut_speed);
   UT_ASSERT_EQ_INT(1000, ut_rx_size);
   UT_ASSERT_EQ_INT(2000, ut_tx_size);
   UT_ASSERT_EQ_INT(FIFO_DISABLE, ut_fifo);

   reset_part_two_fixture();
   od_control.od_com_irq = 15;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_INT(0, ut_irq);
}

static void exercises_serial_failure_results(void)
{
   static const tODResult open_results[] = {
      kODRCNoMemory, kODRCNoPortAddress, kODRCNoUART, kODRCGeneralFailure
   };
   unsigned index;

   reset_part_two_fixture();
   ut_reserve_result = kODRCFileAccessError;
   run_expecting_fatal_exit();
   UT_ASSERT_EQ_UINT(1, ut_reserve_calls);
   UT_ASSERT(strcmp("Unable to initialize shared reservations.",
      ut_error_text) == 0);

   reset_part_two_fixture();
   ut_alloc_result = kODRCNoMemory;
   run_expecting_fatal_exit();

#if defined(ODPLAT_NIX) || defined(ODPLAT_WIN32)
   reset_part_two_fixture();
   od_control.od_open_handle = (DWORD_PTR)123;
   ut_existing_result = kODRCGeneralFailure;
   run_expecting_fatal_exit();

   reset_part_two_fixture();
   od_control.od_open_handle = (DWORD_PTR)123;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_UINT(1, ut_existing_calls);
   UT_ASSERT_EQ_UINT(0, ut_open_calls);
#endif

   for(index = 0; index < DIM(open_results); ++index)
   {
      reset_part_two_fixture();
      ut_open_result = open_results[index];
      run_expecting_fatal_exit();
   }
}

static void exercises_reported_com_methods(void)
{
   static const tComMethod methods[] = {
      kComMethodFOSSIL, kComMethodUART, kComMethodWin32,
      kComMethodDoor32, kComMethodSocket, kComMethodStdIO
   };
   static const INT public_methods[] = {
      COM_FOSSIL, COM_INTERNAL, COM_WIN32,
      COM_DOOR32, COM_SOCKET, COM_STDIO
   };
   unsigned index;
   for(index = 0; index < DIM(methods); ++index)
   {
      reset_part_two_fixture();
      ut_com_method = methods[index];
      utt_ODInitPartTwo();
      UT_ASSERT_EQ_INT(public_methods[index], od_control.od_com_method);
   }

   reset_part_two_fixture();
   ut_com_method = (tComMethod)99;
   run_expecting_fatal_exit();
}

static void handles_kernel_failure_and_exit_registration(void)
{
   reset_part_two_fixture();
   bODInitialized = TRUE;
   ut_kernel_result = kODRCGeneralFailure;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_INT(FALSE, bODInitialized);
   UT_ASSERT_EQ_UINT(1, ut_error_calls);

#ifndef ODPLAT_WIN32
   reset_part_two_fixture();
   bPreset = TRUE;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_UINT(1, ut_atexit_calls);
   UT_ASSERT_EQ_INT(FALSE, bPreset);
#endif
}

static void rejects_local_screen_initialization_failure(void)
{
   reset_part_two_fixture();
   ut_screen_result = kODRCGeneralFailure;
   run_expecting_fatal_exit();
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_kernel_calls);
}

#ifdef ODPLAT_NIX
static void exercises_unix_local_account_and_terminal_paths(void)
{
   reset_part_two_fixture();
   bPromptForUserName = TRUE;
   ut_isatty_result = TRUE;
   ut_tcgetattr_result = -1;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_INT(19200, od_control.baud);
   UT_ASSERT_EQ_UINT(1, ut_apply_user_calls);

   reset_part_two_fixture();
   bPromptForUserName = TRUE;
   ut_isatty_result = TRUE;
   ut_tcgetattr_result = 0;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_INT(57600, od_control.baud);
   UT_ASSERT(strcmp("test-host", od_control.system_name) == 0);
   UT_ASSERT_EQ_UINT(1, ut_apply_user_calls);
}
#endif

#ifdef ODPLAT_WIN32
static void reset_windows_gui_fixture(void)
{
   reset_part_two_fixture();
   ut_subsystem = kODWindowsSubsystemGUI;
}

static void exercises_windows_frame_startup(void)
{
   reset_windows_gui_fixture();
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_UINT(0, ut_frame_calls);

   reset_windows_gui_fixture();
   ut_refresh_result = FALSE;
   bODInitialized = TRUE;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_UINT(1, ut_refresh_calls);
   UT_ASSERT_EQ_UINT(1, ut_kernel_shutdown_calls);
   UT_ASSERT_EQ_INT(FALSE, bODInitialized);
   UT_ASSERT_EQ_UINT(1, ut_error_calls);

   reset_windows_gui_fixture();
   od_control.od_silent_mode = FALSE;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_UINT(1, ut_refresh_calls);
   UT_ASSERT_EQ_UINT(1, ut_frame_calls);
   UT_ASSERT_EQ_UINT(1, ut_publish_calls);

   reset_windows_gui_fixture();
   od_control.od_silent_mode = FALSE;
   ut_first_module_missing = TRUE;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_UINT(1, ut_frame_calls);
   UT_ASSERT_EQ_UINT(1, ut_publish_calls);

   reset_windows_gui_fixture();
   od_control.od_silent_mode = FALSE;
   bPromptForUserName = TRUE;
   utm_strcpy(szWindowsStartupUserName, "Prompted User");
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_UINT(2, ut_refresh_calls);

   reset_windows_gui_fixture();
   od_control.od_silent_mode = FALSE;
   ut_frame_result = kODRCGeneralFailure;
   bODInitialized = TRUE;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_UINT(1, ut_kernel_shutdown_calls);
   UT_ASSERT_EQ_UINT(1, ut_publish_calls);
   UT_ASSERT_EQ_INT(FALSE, bODInitialized);
   UT_ASSERT_EQ_UINT(1, ut_error_calls);

   reset_windows_gui_fixture();
   od_control.od_silent_mode = FALSE;
   ut_frame_result = kODRCGeneralFailure;
   ut_cancel_during_frame_start = TRUE;
   ut_exit_expected = TRUE;
   if(setjmp(ut_exit_target) == 0)
   {
      utt_ODInitPartTwo();
      UT_ASSERT(FALSE);
   }
   ut_exit_expected = FALSE;
   UT_ASSERT_EQ_UINT(1, ut_kernel_shutdown_calls);
   UT_ASSERT_EQ_UINT(1, ut_exit_calls);
   UT_ASSERT_EQ_UINT(0, ut_error_calls);
}

static void exercises_windows_console_prompt_and_personality_paths(void)
{
   reset_part_two_fixture();
   od_control.user_screenwidth = 100;
   od_control.user_screen_length = 30;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_INT(100, ut_session_width);
   UT_ASSERT_EQ_INT(30, ut_session_height);

   reset_part_two_fixture();
   od_control.od_silent_mode = FALSE;
   bPromptForUserName = TRUE;
   ut_window_available = FALSE;
   utt_ODInitPartTwo();

   reset_part_two_fixture();
   od_control.od_silent_mode = FALSE;
   bPromptForUserName = TRUE;
   ut_window_available = TRUE;
   utt_ODInitPartTwo();
   UT_ASSERT(strcmp("Local User", od_control.user_name) == 0);
   UT_ASSERT(strcmp("Local User", od_control.user_handle) == 0);

   reset_part_two_fixture();
   bRAStatus = TRUE;
   btRAStatusToSet = 2;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_INT(2, ut_status_value);

   reset_part_two_fixture();
   od_control.od_default_personality = ut_test_personality;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_UINT(1, ut_personality_calls);

   reset_part_two_fixture();
   pfSetPersonality = ut_set_personality;
   ut_set_personality_result = TRUE;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_UINT(0, ut_personality_calls);

   reset_part_two_fixture();
   pfSetPersonality = ut_set_personality;
   ut_set_personality_result = FALSE;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_UINT(1, ut_personality_calls);
   UT_ASSERT_EQ_INT(0, ut_status_value);
}
#endif

#ifdef OD_TEXTMODE
static void exercises_dos_local_prompt_and_personality_paths(void)
{
   reset_part_two_fixture();
   bPromptForUserName = TRUE;
   ut_window_available = FALSE;
   utt_ODInitPartTwo();

   reset_part_two_fixture();
   bPromptForUserName = TRUE;
   ut_window_available = TRUE;
   utt_ODInitPartTwo();
   UT_ASSERT(strcmp("Local User", od_control.user_name) == 0);
   UT_ASSERT(strcmp("Local User", od_control.user_handle) == 0);

   reset_part_two_fixture();
   bRAStatus = TRUE;
   btRAStatusToSet = 2;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_INT(2, ut_status_value);

   reset_part_two_fixture();
   od_control.od_default_personality = ut_test_personality;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_UINT(1, ut_personality_calls);

   reset_part_two_fixture();
   pfSetPersonality = ut_set_personality;
   ut_set_personality_result = TRUE;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_UINT(0, ut_personality_calls);

   reset_part_two_fixture();
   pfSetPersonality = ut_set_personality;
   ut_set_personality_result = FALSE;
   utt_ODInitPartTwo();
   UT_ASSERT_EQ_UINT(1, ut_personality_calls);
   UT_ASSERT_EQ_INT(0, ut_status_value);
}
#endif

static const UTTestCase ut_cases[] = {
   {"defaults", initializes_defaults_and_a_serial_session},
   {"configured values", preserves_configured_defaults_and_caps_session_time},
   {"maximum-time boundaries", exercises_maximum_time_decision_boundaries},
   {"RIP and log callback", clears_rip_and_invokes_the_log_component},
   {"forced local and BPS", exercises_forced_local_name_and_bps_decisions},
   {"serial configuration", exercises_serial_configuration_matrix},
   {"serial failures", exercises_serial_failure_results},
   {"reported methods", exercises_reported_com_methods},
   {"screen failure", rejects_local_screen_initialization_failure},
   {"kernel and exit registration", handles_kernel_failure_and_exit_registration}
#ifdef ODPLAT_NIX
   ,{"UNIX local account", exercises_unix_local_account_and_terminal_paths}
#endif
#ifdef ODPLAT_WIN32
   ,{"Windows frame", exercises_windows_frame_startup}
   ,{"Windows console", exercises_windows_console_prompt_and_personality_paths}
#endif
#ifdef OD_TEXTMODE
   ,{"DOS local and personality", exercises_dos_local_prompt_and_personality_paths}
#endif
};
