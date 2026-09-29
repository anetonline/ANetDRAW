#define UT_CUSTOM_MOCK_malloc

static tPortInfo ut_port;
static BYTE ut_tx_queue[32];
static BYTE ut_rx_queue[32];
static unsigned ut_malloc_calls;
static unsigned ut_malloc_fail_call;

#ifdef __TURBOC__
static void ut_turbo_fossil_enabled(BOOL enabled)
{
   ASM mov ah, 0xfe
   ASM mov al, enabled
   ASM int 0x14
}
#endif

void *utm_malloc(size_t size)
{
   ++ut_malloc_calls;
   if(ut_malloc_calls == ut_malloc_fail_call)
      return(NULL);
   if(ut_malloc_calls == 1)
   {
      UT_ASSERT_EQ_UINT(ut_port.nTransmitBufferSize, size);
      return(ut_tx_queue);
   }
   UT_ASSERT_EQ_UINT(ut_port.nReceiveBufferSize, size);
   return(ut_rx_queue);
}

#if defined(__WATCOMC__) && !defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK_int86
static BOOL ut_fossil_detected;
int utm_int86(int interrupt_number, const union REGS *input,
   union REGS *output)
{
   UT_ASSERT_EQ_INT(0x14, interrupt_number);
   UT_ASSERT_EQ_INT(4, input->h.ah);
   UT_ASSERT_NOT_NULL(output);
   *output = *input;
   output->x.ax = ut_fossil_detected ? 6484 : 0;
   return(0);
}
#endif

#ifdef ODPLAT_DOS32
#define UT_CUSTOM_MOCK_OD32FossilDetect
#define UT_CUSTOM_MOCK_OD32FossilBufferAllocate
#define UT_CUSTOM_MOCK_OD32FossilSetFlow
#define UT_CUSTOM_MOCK_OD32FossilInitialize
static BOOL ut_fossil_detected;
static unsigned ut_fossil_allocate_calls;
static BYTE ut_fossil_flow;
static unsigned ut_fossil_initialize_calls;
static BYTE ut_fossil_settings;
BOOL utm_OD32FossilDetect(BYTE port)
{
   UT_ASSERT_EQ_INT(2, port);
   return(ut_fossil_detected);
}
BOOL utm_OD32FossilBufferAllocate(tOD32FossilBuffer *buffer, WORD size)
{
   UT_ASSERT_NOT_NULL(buffer);
   UT_ASSERT_EQ_UINT(4096, size);
   ++ut_fossil_allocate_calls;
   return(TRUE);
}
void utm_OD32FossilSetFlow(BYTE port, BYTE flow)
{
   UT_ASSERT_EQ_INT(2, port);
   ut_fossil_flow = flow;
}
void utm_OD32FossilInitialize(BYTE port, BYTE settings)
{
   UT_ASSERT_EQ_INT(2, port);
   ++ut_fossil_initialize_calls;
   ut_fossil_settings = settings;
}
#endif

#ifdef INCLUDE_UART_COM
#define UT_CUSTOM_MOCK_UTInp
#define UT_CUSTOM_MOCK_UTOutp
#define UT_CUSTOM_MOCK_UTDisable
#define UT_CUSTOM_MOCK_UTEnable
#define UT_CUSTOM_MOCK_ODComInternalResetTX
#define UT_CUSTOM_MOCK_ODComInternalResetRX
#define UT_CUSTOM_MOCK_ODComGetVect
#define UT_CUSTOM_MOCK_ODComSetVect
#define UT_CUSTOM_MOCK_ODComInternalISR
#define UT_CUSTOM_MOCK_ODDWordDivide
static unsigned ut_ier_reads;
static BYTE ut_old_ier;
static BYTE ut_uart_probe;
static BYTE ut_modem_status;
static BYTE ut_fifo_probe;
static BYTE ut_old_mcr;
static unsigned ut_port_writes;
static unsigned ut_disable_calls;
static unsigned ut_enable_calls;
static unsigned ut_reset_tx_calls;
static unsigned ut_reset_rx_calls;
static unsigned ut_getvect_calls;
static unsigned ut_setvect_calls;
static DWORD ut_divide_quotient;
static DWORD ut_divide_remainder;
static unsigned ut_divide_calls;

#ifdef __TURBOC__
int
#else
unsigned
#endif
utm_UTInp(unsigned port)
{
   if(port == (unsigned)nIntEnableRegAddr)
   {
      ++ut_ier_reads;
      return(ut_ier_reads == 1 ? ut_old_ier : ut_uart_probe);
   }
   if(port == (unsigned)nModemStatusRegAddr) return(ut_modem_status);
   if(port == (unsigned)nIntIDRegAddr) return(ut_fifo_probe);
   if(port == (unsigned)nModemCtrlRegAddr) return(ut_old_mcr);
   if(port == (unsigned)nI8259MaskRegAddr) return(0xff);
   if(port == (unsigned)nLineCtrlRegAddr) return(0);
   UT_ASSERT(FALSE);
   return(0);
}

#ifdef __TURBOC__
int
#else
unsigned
#endif
utm_UTOutp(unsigned port, unsigned value)
{
   (void)port;
   ++ut_port_writes;
   return(value);
}
void utm_UTDisable(void) { ++ut_disable_calls; }
void utm_UTEnable(void) { ++ut_enable_calls; }
void utm_ODComInternalResetTX(void) { ++ut_reset_tx_calls; }
void utm_ODComInternalResetRX(void) { ++ut_reset_rx_calls; }
void (INTERRUPT far *utm_ODComGetVect(BYTE vector))(void)
{
   UT_ASSERT_EQ_INT(btIntVector, vector);
   ++ut_getvect_calls;
   return(NULL);
}
void utm_ODComSetVect(BYTE vector, void (INTERRUPT far *handler)(void))
{
   UT_ASSERT_EQ_INT(btIntVector, vector);
   UT_ASSERT_NOT_NULL(handler);
   ++ut_setvect_calls;
}
static void INTERRUPT utm_ODComInternalISR(void)
{
}
BOOL utm_ODDWordDivide(DWORD *quotient, DWORD *remainder, DWORD dividend,
   DWORD divisor)
{
   UT_ASSERT_NOT_NULL(quotient);
   UT_ASSERT_NOT_NULL(remainder);
   UT_ASSERT_EQ_UINT(115200UL, dividend);
   UT_ASSERT(divisor == ut_port.lSpeed);
   *quotient = ut_divide_quotient;
   *remainder = ut_divide_remainder;
   ++ut_divide_calls;
   return(TRUE);
}
#endif

#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_LoadLibraryA
#define UT_CUSTOM_MOCK_GetProcAddress
#define UT_CUSTOM_MOCK_FreeLibrary
#define UT_CUSTOM_MOCK_CreateFileA
#define UT_CUSTOM_MOCK_SetupComm
#define UT_CUSTOM_MOCK_GetCommState
#define UT_CUSTOM_MOCK_SetCommState
#define UT_CUSTOM_MOCK_CloseHandle
#define UT_CUSTOM_MOCK_sprintf

#undef LoadLibraryA
#undef GetProcAddress
#undef FreeLibrary
WINBASEAPI HMODULE WINAPI LoadLibraryA(LPCSTR name);
WINBASEAPI FARPROC WINAPI GetProcAddress(HMODULE module, LPCSTR name);
WINBASEAPI BOOL WINAPI FreeLibrary(HMODULE module);

static BOOL ut_load_failure;
static const char *ut_missing_export;
static unsigned ut_load_calls;
static unsigned ut_getproc_calls;
static unsigned ut_free_library_calls;
static HANDLE ut_create_result;
static BOOL ut_setup_result;
static BOOL ut_get_state_result;
static BOOL ut_set_state_result;
static unsigned ut_close_handle_calls;
static DCB ut_written_dcb;

HMODULE WINAPI utm_LoadLibraryA(LPCSTR name)
{
   ++ut_load_calls;
   UT_ASSERT_EQ_INT(0, strcmp("DOOR32.DLL", name));
   if(ut_load_failure) return(NULL);
   return(LoadLibraryA(name));
}
FARPROC WINAPI utm_GetProcAddress(HMODULE module, LPCSTR name)
{
   ++ut_getproc_calls;
   UT_ASSERT_NOT_NULL(module);
   if(ut_missing_export != NULL && strcmp(ut_missing_export, name) == 0)
      return(NULL);
   return(GetProcAddress(module, name));
}
BOOL WINAPI utm_FreeLibrary(HMODULE module)
{
   ++ut_free_library_calls;
   return(FreeLibrary(module));
}
HANDLE WINAPI utm_CreateFileA(LPCSTR name, DWORD access, DWORD sharing,
   LPSECURITY_ATTRIBUTES security, DWORD creation, DWORD attributes,
   HANDLE template_file)
{
   UT_ASSERT_EQ_INT(0, strcmp("COM3", name));
   UT_ASSERT_EQ_UINT(GENERIC_READ | GENERIC_WRITE, access);
   UT_ASSERT_EQ_UINT(0, sharing);
   UT_ASSERT_NULL(security);
   UT_ASSERT_EQ_UINT(OPEN_EXISTING, creation);
   UT_ASSERT_EQ_UINT(FILE_ATTRIBUTE_NORMAL, attributes);
   UT_ASSERT_NULL(template_file);
   return(ut_create_result);
}
BOOL WINAPI utm_SetupComm(HANDLE device, DWORD input, DWORD output)
{
   UT_ASSERT_EQ_PTR(ut_create_result, device);
   UT_ASSERT_EQ_UINT(16, input);
   UT_ASSERT_EQ_UINT(17, output);
   return(ut_setup_result);
}
BOOL WINAPI utm_GetCommState(HANDLE device, LPDCB dcb)
{
   UT_ASSERT_EQ_PTR(ut_create_result, device);
   UT_ASSERT_NOT_NULL(dcb);
   memset(dcb, 0, sizeof(*dcb));
   return(ut_get_state_result);
}
BOOL WINAPI utm_SetCommState(HANDLE device, LPDCB dcb)
{
   UT_ASSERT_EQ_PTR(ut_create_result, device);
   UT_ASSERT_NOT_NULL(dcb);
   ut_written_dcb = *dcb;
   return(ut_set_state_result);
}
BOOL WINAPI utm_CloseHandle(HANDLE device)
{
   UT_ASSERT_EQ_PTR(ut_create_result, device);
   ++ut_close_handle_calls;
   return(TRUE);
}
int utm_sprintf(char *buffer, const char *format, ...)
{
   va_list args;
   unsigned port;
   UT_ASSERT_EQ_INT(0, strcmp("COM%u", format));
   va_start(args, format);
   port = va_arg(args, unsigned);
   va_end(args);
   UT_ASSERT(port < 10);
   buffer[0] = 'C'; buffer[1] = 'O'; buffer[2] = 'M';
   buffer[3] = (char)('0' + port); buffer[4] = '\0';
   return(4);
}
#endif

#ifdef INCLUDE_STDIO_COM
#define UT_CUSTOM_MOCK_isatty
#define UT_CUSTOM_MOCK_tcgetattr
#define UT_CUSTOM_MOCK_tcsetattr
#define UT_CUSTOM_MOCK_setvbuf
static int ut_isatty_result;
static unsigned ut_tcgetattr_calls;
static unsigned ut_tcsetattr_calls;
static unsigned ut_setvbuf_calls;
int utm_isatty(int descriptor)
{
   UT_ASSERT_EQ_INT(STDIN_FILENO, descriptor);
   return(ut_isatty_result);
}
int utm_tcgetattr(int descriptor, struct termios *settings)
{
   UT_ASSERT_EQ_INT(STDIN_FILENO, descriptor);
   UT_ASSERT_EQ_PTR(&sio_tio_default, settings);
   settings->c_iflag = (tcflag_t)~0;
   settings->c_oflag = (tcflag_t)~0;
   settings->c_lflag = (tcflag_t)~0;
   settings->c_cflag = (tcflag_t)~0;
   settings->c_cc[VMIN] = 0;
   settings->c_cc[VTIME] = 1;
   ++ut_tcgetattr_calls;
   return(0);
}
int utm_tcsetattr(int descriptor, int action, const struct termios *settings)
{
   UT_ASSERT_EQ_INT(STDIN_FILENO, descriptor);
   UT_ASSERT_EQ_INT(TCSANOW, action);
   UT_ASSERT_NOT_NULL(settings);
   UT_ASSERT(settings->c_iflag == ((tcflag_t)~0 &
      ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL |
      IXON)));
   UT_ASSERT(settings->c_oflag == ((tcflag_t)~0 & ~OPOST));
   UT_ASSERT(settings->c_lflag == ((tcflag_t)~0 &
      ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN)));
   UT_ASSERT(settings->c_cflag == (((tcflag_t)~0 & ~(CSIZE | PARENB)) |
      CS8));
   UT_ASSERT_EQ_INT(1, settings->c_cc[VMIN]);
   UT_ASSERT_EQ_INT(0, settings->c_cc[VTIME]);
   ++ut_tcsetattr_calls;
   return(0);
}
int utm_setvbuf(FILE *stream, char *buffer, int mode, size_t size)
{
   UT_ASSERT_EQ_PTR(stdout, stream);
   UT_ASSERT_NULL(buffer);
   UT_ASSERT_EQ_INT(_IONBF, mode);
   UT_ASSERT_EQ_UINT(0, size);
   ++ut_setvbuf_calls;
   return(0);
}
#endif

static void reset_open(void)
{
   memset(&ut_port, 0, sizeof(ut_port));
   ut_port.btPort = 2;
   ut_port.btFlowControlSetting = FLOW_DEFAULT;
   ut_port.lSpeed = SPEED_UNSPECIFIED;
   ut_port.btWordFormat = ODPARITY_NONE | DATABITS_EIGHT | STOP_ONE;
   ut_port.nReceiveBufferSize = 16;
   ut_port.nTransmitBufferSize = 17;
   ut_port.btFIFOSetting = FIFO_ENABLE | FIFO_TRIGGER_8;
   ut_port.Method = kComMethodUnspecified;
   ut_malloc_calls = 0;
   ut_malloc_fail_call = 0;
#if defined(__WATCOMC__) && !defined(ODPLAT_DOS32)
   ut_fossil_detected = TRUE;
#endif
#ifdef ODPLAT_DOS32
   ut_fossil_detected = TRUE;
   ut_fossil_allocate_calls = 0;
   ut_fossil_flow = 0;
   ut_fossil_initialize_calls = 0;
   ut_fossil_settings = 0;
#endif
#ifdef INCLUDE_UART_COM
   ut_ier_reads = 0; ut_old_ier = 7; ut_uart_probe = 0;
   ut_modem_status = CTS; ut_fifo_probe = 0xc0; ut_old_mcr = DTR;
   ut_port_writes = ut_disable_calls = ut_enable_calls = 0;
   ut_reset_tx_calls = ut_reset_rx_calls = 0;
   ut_getvect_calls = ut_setvect_calls = 0;
   ut_divide_quotient = 12; ut_divide_remainder = 0; ut_divide_calls = 0;
   ut_port.nPortAddress = 0x3f8; ut_port.btIRQLevel = 4;
#endif
#ifdef ODPLAT_WIN32
   ut_load_failure = FALSE; ut_missing_export = NULL;
   ut_load_calls = ut_getproc_calls = ut_free_library_calls = 0;
   ut_create_result = (HANDLE)(DWORD_PTR)41;
   ut_setup_result = ut_get_state_result = ut_set_state_result = TRUE;
   ut_close_handle_calls = 0;
   memset(&ut_written_dcb, 0, sizeof(ut_written_dcb));
   SetEnvironmentVariableA("OPENDOORS_UNIT_DOOR32_INIT_FAIL", NULL);
#endif
#ifdef INCLUDE_STDIO_COM
   ut_isatty_result = 0;
   ut_tcgetattr_calls = ut_tcsetattr_calls = 0;
   ut_setvbuf_calls = 0;
#endif
}

#ifdef INCLUDE_FOSSIL_COM
static void opens_fossil_with_flow_and_every_speed_mapping(void)
{
   static const long speeds[] = {300L, 600L, 1200L, 2400L, 4800L,
      9600L, 19200L, 38400L, 12345L};
   unsigned index;
   reset_open();
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_INT(TRUE, ut_port.bIsOpen);
#ifdef ODPLAT_DOS32
   UT_ASSERT_EQ_INT(FLOW_RTSCTS | 0xf0, ut_fossil_flow);
#endif
   for(index = 0; index < sizeof(speeds) / sizeof(speeds[0]); ++index)
   {
      reset_open(); ut_port.Method = kComMethodFOSSIL;
      ut_port.btFlowControlSetting = FLOW_NONE;
      ut_port.lSpeed = speeds[index];
      UT_ASSERT_EQ_INT(kODRCSuccess,
         utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
      UT_ASSERT_EQ_INT(TRUE, ut_port.bIsOpen);
#ifdef ODPLAT_DOS32
      UT_ASSERT_EQ_INT(FLOW_NONE | 0xf0, ut_fossil_flow);
      if(speeds[index] == 12345L)
         UT_ASSERT_EQ_UINT(0, ut_fossil_initialize_calls);
      else
         UT_ASSERT_EQ_UINT(1, ut_fossil_initialize_calls);
#endif
   }
}

static void reports_a_missing_fossil(void)
{
   reset_open(); ut_port.Method = kComMethodFOSSIL;
#ifdef __TURBOC__
   ut_turbo_fossil_enabled(FALSE);
#endif
#if defined(__WATCOMC__) || defined(ODPLAT_DOS32)
   ut_fossil_detected = FALSE;
#endif
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
#ifdef __TURBOC__
   ut_turbo_fossil_enabled(TRUE);
#endif
}
#endif

#ifdef INCLUDE_UART_COM
static void opens_uart_with_each_major_hardware_option(void)
{
   reset_open();
#ifdef __TURBOC__
   ut_turbo_fossil_enabled(FALSE);
#endif
#if defined(__WATCOMC__) || defined(ODPLAT_DOS32)
   ut_fossil_detected = FALSE;
#endif
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_INT(kComMethodUART, ut_port.Method);
#ifdef __TURBOC__
   ut_turbo_fossil_enabled(TRUE);
#endif

   reset_open(); ut_port.Method = kComMethodUART;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_INT(kComMethodUART, ut_port.Method);
   UT_ASSERT_EQ_INT(TRUE, ut_port.bIsOpen);
   UT_ASSERT_EQ_UINT(1, ut_reset_tx_calls);
   UT_ASSERT_EQ_UINT(1, ut_reset_rx_calls);

   reset_open(); ut_port.Method = kComMethodUART;
   ut_port.btFlowControlSetting = FLOW_NONE;
   ut_port.btFIFOSetting = FIFO_DISABLE;
   ut_port.btIRQLevel = 9;
   ut_port.lSpeed = 9600L;
   ut_divide_remainder = 1;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_UINT(1, ut_divide_calls);

   reset_open(); ut_port.Method = kComMethodUART;
   ut_modem_status = 0; ut_port.lSpeed = 9600L;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_INT(TRUE, bStopTrans);
}

static void reports_uart_allocation_address_and_probe_failures(void)
{
   reset_open(); ut_port.Method = kComMethodUART; ut_malloc_fail_call = 1;
   UT_ASSERT_EQ_INT(kODRCNoMemory,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
   reset_open(); ut_port.Method = kComMethodUART; ut_malloc_fail_call = 2;
   UT_ASSERT_EQ_INT(kODRCNoMemory,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
   reset_open(); ut_port.Method = kComMethodUART; ut_port.nPortAddress = 0;
   UT_ASSERT_EQ_INT(kODRCNoPortAddress,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
   reset_open(); ut_port.Method = kComMethodUART; ut_uart_probe = 1;
   UT_ASSERT_EQ_INT(kODRCNoUART,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
}
#endif

#ifdef ODPLAT_WIN32
static void opens_the_real_door32_fixture_and_handles_loader_failure(void)
{
   reset_open(); ut_port.Method = kComMethodDoor32;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_INT(kComMethodDoor32, ut_port.Method);
   UT_ASSERT_EQ_UINT(6, ut_getproc_calls);
   utm_FreeLibrary(ut_port.hinstDoor32DLL);

   reset_open(); ut_port.Method = kComMethodDoor32; ut_load_failure = TRUE;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
}

static void rejects_each_missing_door32_export_and_initialization_failure(void)
{
   static const char *names[] = {"DoorInitialize", "DoorShutdown",
      "DoorWrite", "DoorRead", "DoorGetAvailableEventHandle",
      "DoorGetOfflineEventHandle"};
   unsigned index;
   for(index = 0; index < sizeof(names) / sizeof(names[0]); ++index)
   {
      reset_open(); ut_port.Method = kComMethodDoor32;
      ut_missing_export = names[index];
      UT_ASSERT_EQ_INT(kODRCGeneralFailure,
         utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
      UT_ASSERT_EQ_UINT(1, ut_free_library_calls);
   }
   reset_open(); ut_port.Method = kComMethodDoor32;
   SetEnvironmentVariableA("OPENDOORS_UNIT_DOOR32_INIT_FAIL", "1");
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_UINT(1, ut_free_library_calls);
   SetEnvironmentVariableA("OPENDOORS_UNIT_DOOR32_INIT_FAIL", NULL);
}

static void reports_win32_open_and_configuration_failures(void)
{
   reset_open(); ut_port.Method = kComMethodUnspecified;
   ut_load_failure = TRUE;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_INT(kComMethodWin32, ut_port.Method);
   reset_open(); ut_port.Method = kComMethodWin32;
   ut_create_result = INVALID_HANDLE_VALUE;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
   reset_open(); ut_port.Method = kComMethodWin32; ut_setup_result = FALSE;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_UINT(1, ut_close_handle_calls);
   reset_open(); ut_port.Method = kComMethodWin32; ut_get_state_result = FALSE;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
   reset_open(); ut_port.Method = kComMethodWin32; ut_set_state_result = FALSE;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
}

static void maps_every_win32_dcb_setting(void)
{
   static const BYTE words[] = {
      DATABITS_FIVE | ODPARITY_NONE | STOP_ONE,
      DATABITS_SIX | ODPARITY_ODD | STOP_ONE_POINT_FIVE,
      DATABITS_SEVEN | ODPARITY_EVEN | STOP_TWO,
      DATABITS_EIGHT | ODPARITY_NONE | STOP_ONE,
      DATABITS_EIGHT | 0x10 | STOP_ONE,
      0xff
   };
   unsigned index;
   for(index = 0; index < sizeof(words) / sizeof(words[0]); ++index)
   {
      reset_open(); ut_port.Method = kComMethodWin32;
      ut_port.btWordFormat = words[index];
      ut_port.btFlowControlSetting = index == 0 ? FLOW_DEFAULT :
         (index == 1 ? FLOW_RTSCTS : FLOW_NONE);
      ut_port.lSpeed = index == 0 ? SPEED_UNSPECIFIED : 19200L;
      UT_ASSERT_EQ_INT(kODRCSuccess,
         utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
      UT_ASSERT_EQ_INT(TRUE, ut_port.bIsOpen);
   }
}
#endif

#ifdef INCLUDE_STDIO_COM
static void opens_stdio_with_and_without_a_terminal(void)
{
   reset_open(); ut_port.Method = kComMethodStdIO; ut_isatty_result = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_UINT(0, ut_tcgetattr_calls);
   reset_open(); ut_port.Method = kComMethodUnspecified; ut_isatty_result = 1;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_UINT(1, ut_tcgetattr_calls);
   UT_ASSERT_EQ_UINT(1, ut_tcsetattr_calls);
   UT_ASSERT_EQ_UINT(1, ut_setvbuf_calls);
}
#endif

static void rejects_an_unsupported_explicit_method(void)
{
   reset_open(); ut_port.Method = (tComMethod)99;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComOpen(ODPTR2HANDLE(&ut_port, tPortInfo)));
}

static const UTTestCase ut_cases[] = {
#ifdef INCLUDE_FOSSIL_COM
   {"FOSSIL", opens_fossil_with_flow_and_every_speed_mapping},
   {"missing FOSSIL", reports_a_missing_fossil},
#endif
#ifdef INCLUDE_UART_COM
   {"UART", opens_uart_with_each_major_hardware_option},
   {"UART failures", reports_uart_allocation_address_and_probe_failures},
#endif
#ifdef ODPLAT_WIN32
   {"Door32 loader", opens_the_real_door32_fixture_and_handles_loader_failure},
   {"Door32 failures", rejects_each_missing_door32_export_and_initialization_failure},
   {"Win32 failures", reports_win32_open_and_configuration_failures},
   {"Win32 DCB", maps_every_win32_dcb_setting},
#endif
#ifdef INCLUDE_STDIO_COM
   {"stdio", opens_stdio_with_and_without_a_terminal},
#endif
   {"unsupported", rejects_an_unsupported_explicit_method}
};
