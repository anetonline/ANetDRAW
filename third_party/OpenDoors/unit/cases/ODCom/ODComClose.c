#ifdef ODPLAT_DOS32
#define UT_CUSTOM_MOCK_OD32FossilClose
#define UT_CUSTOM_MOCK_OD32FossilBufferFree
static unsigned ut_fossil_close_calls;
static unsigned ut_fossil_free_calls;
void utm_OD32FossilClose(BYTE port)
{
   UT_ASSERT_EQ_INT(2, port);
   ++ut_fossil_close_calls;
}
void utm_OD32FossilBufferFree(tOD32FossilBuffer *buffer)
{
   UT_ASSERT_NOT_NULL(buffer);
   ++ut_fossil_free_calls;
}
#endif

#ifdef INCLUDE_UART_COM
#define UT_CUSTOM_MOCK_UTInp
#define UT_CUSTOM_MOCK_UTOutp
#define UT_CUSTOM_MOCK_UTDisable
#define UT_CUSTOM_MOCK_UTEnable
#define UT_CUSTOM_MOCK_ODComSetVect
static unsigned ut_port_values[4];
static unsigned ut_port_reads;
static unsigned ut_port_writes;
static unsigned ut_disable_calls;
static unsigned ut_enable_calls;
static unsigned ut_setvect_calls;
#ifdef __TURBOC__
int
#else
unsigned
#endif
utm_UTInp(unsigned port)
{
   UT_ASSERT_EQ_UINT((unsigned)nI8259MaskRegAddr, port);
   ++ut_port_reads;
   return(ut_port_values[0]);
}
#ifdef __TURBOC__
int
#else
unsigned
#endif
utm_UTOutp(unsigned port, unsigned value)
{
   (void)port;
   (void)value;
   ++ut_port_writes;
   return(value);
}
void utm_UTDisable(void) { ++ut_disable_calls; }
void utm_UTEnable(void) { ++ut_enable_calls; }
void utm_ODComSetVect(BYTE vector, void (INTERRUPT far *handler)(void))
{
   UT_ASSERT_EQ_INT(btIntVector, vector);
   UT_ASSERT_EQ_PTR(pfOldISR, handler);
   ++ut_setvect_calls;
}
#endif

#ifdef INCLUDE_WIN32_COM
#define UT_CUSTOM_MOCK_CloseHandle
static unsigned ut_close_handle_calls;
BOOL WINAPI utm_CloseHandle(HANDLE handle)
{
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, handle);
   ++ut_close_handle_calls;
   return(TRUE);
}
#endif

#ifdef INCLUDE_DOOR32_COM
#define UT_CUSTOM_MOCK_FreeLibrary
static unsigned ut_door_shutdown_calls;
static unsigned ut_free_library_calls;
static BOOL WINAPI ut_door_shutdown(void)
{
   ++ut_door_shutdown_calls;
   return(TRUE);
}
BOOL WINAPI utm_FreeLibrary(HMODULE module)
{
   UT_ASSERT_EQ_PTR((HMODULE)(DWORD_PTR)43, module);
   ++ut_free_library_calls;
   return(TRUE);
}
#endif

#ifdef INCLUDE_SOCKET_COM
#define UT_CUSTOM_MOCK_setsockopt
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_closesocket
int PASCAL utm_setsockopt(SOCKET socket_handle, int level,
   int option, const char FAR *value, int size)
#else
#define UT_CUSTOM_MOCK_UTCloseSocket
int utm_setsockopt(int socket_handle, int level, int option,
   const void *value, socklen_t size)
#endif
{
   UT_ASSERT_EQ_INT(45, socket_handle);
   UT_ASSERT_EQ_INT(IPPROTO_TCP, level);
   UT_ASSERT_EQ_INT(TCP_NODELAY, option);
   UT_ASSERT_NOT_NULL(value);
   UT_ASSERT(size == sizeof(int));
   return(0);
}
#ifdef ODPLAT_WIN32
int PASCAL utm_closesocket(SOCKET socket_handle)
#else
int utm_UTCloseSocket(int socket_handle)
#endif
{
   UT_ASSERT_EQ_INT(45, socket_handle);
   return(0);
}
#endif

#ifdef INCLUDE_STDIO_COM
#define UT_CUSTOM_MOCK_isatty
#define UT_CUSTOM_MOCK_tcsetattr
static int ut_isatty_result;
static unsigned ut_tcsetattr_calls;
int utm_isatty(int descriptor)
{
   UT_ASSERT_EQ_INT(STDIN_FILENO, descriptor);
   return(ut_isatty_result);
}
int utm_tcsetattr(int descriptor, int action, const struct termios *settings)
{
   UT_ASSERT_EQ_INT(STDIN_FILENO, descriptor);
   UT_ASSERT_EQ_INT(TCSANOW, action);
   UT_ASSERT_EQ_PTR(&sio_tio_default, settings);
   ++ut_tcsetattr_calls;
   return(0);
}
#endif

static tPortInfo ut_port;

static void reset_close(void)
{
   memset(&ut_port, 0, sizeof(ut_port));
   ut_port.bIsOpen = TRUE;
#ifdef ODPLAT_DOS32
   ut_fossil_close_calls = 0;
   ut_fossil_free_calls = 0;
#endif
#ifdef INCLUDE_UART_COM
   nModemCtrlRegAddr = 0x3fc;
   nIntEnableRegAddr = 0x3f9;
   nI8259MaskRegAddr = 0x21;
   btOldModemCtrlReg = 1;
   btOldIntEnableReg = 2;
   btI8259Bit = 4;
   btI8259Mask = 4;
   btIntVector = 12;
   pfOldISR = NULL;
   ut_port_values[0] = 0xa0;
   ut_port_reads = ut_port_writes = 0;
   ut_disable_calls = ut_enable_calls = ut_setvect_calls = 0;
#endif
#ifdef INCLUDE_WIN32_COM
   ut_port.hCommDev = (HANDLE)(DWORD_PTR)41;
   ut_close_handle_calls = 0;
#endif
#ifdef INCLUDE_DOOR32_COM
   ut_port.pfDoorShutdown = ut_door_shutdown;
   ut_port.hinstDoor32DLL = (HINSTANCE)(DWORD_PTR)43;
   ut_door_shutdown_calls = ut_free_library_calls = 0;
#endif
#ifdef INCLUDE_SOCKET_COM
   ut_port.socket = 45;
   ut_port.old_delay = 1;
   ut_port.TelnetInputState = kTelnetInputSubnegotiation;
   ut_port.bTelnetInputReplay = TRUE;
   ut_port.btTelnetInputReplay = 0xa5;
#endif
#ifdef INCLUDE_STDIO_COM
   ut_isatty_result = 0;
   ut_tcsetattr_calls = 0;
#endif
}

static void leaves_a_clients_handle_open(void)
{
   reset_close();
   ut_port.bUsingClientsHandle = TRUE;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComClose(ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_INT(FALSE, ut_port.bIsOpen);
#ifdef INCLUDE_SOCKET_COM
   UT_ASSERT_EQ_INT(kTelnetInputData, ut_port.TelnetInputState);
   UT_ASSERT_EQ_INT(FALSE, ut_port.bTelnetInputReplay);
   UT_ASSERT_EQ_INT(0, ut_port.btTelnetInputReplay);
#endif
}

#ifdef INCLUDE_FOSSIL_COM
static void closes_a_fossil_port(void)
{
   reset_close(); ut_port.Method = kComMethodFOSSIL; ut_port.btPort = 2;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComClose(ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_INT(FALSE, ut_port.bIsOpen);
#ifdef ODPLAT_DOS32
   UT_ASSERT_EQ_UINT(1, ut_fossil_close_calls);
   UT_ASSERT_EQ_UINT(1, ut_fossil_free_calls);
#endif
}
#endif

#ifdef INCLUDE_UART_COM
static void restores_uart_state(void)
{
   reset_close(); ut_port.Method = kComMethodUART;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComClose(ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_UINT(1, ut_port_reads);
   UT_ASSERT_EQ_UINT(3, ut_port_writes);
#ifdef ODPLAT_DOS32
   UT_ASSERT_EQ_UINT(1, ut_disable_calls);
   UT_ASSERT_EQ_UINT(1, ut_enable_calls);
#endif
   UT_ASSERT_EQ_UINT(1, ut_setvect_calls);
}
#endif

#ifdef INCLUDE_WIN32_COM
static void closes_a_win32_handle(void)
{
   reset_close(); ut_port.Method = kComMethodWin32;
   utt_ODComClose(ODPTR2HANDLE(&ut_port, tPortInfo));
   UT_ASSERT_EQ_UINT(1, ut_close_handle_calls);
}
#endif

#ifdef INCLUDE_DOOR32_COM
static void shuts_down_door32(void)
{
   reset_close(); ut_port.Method = kComMethodDoor32;
   utt_ODComClose(ODPTR2HANDLE(&ut_port, tPortInfo));
   UT_ASSERT_EQ_UINT(1, ut_door_shutdown_calls);
   UT_ASSERT_EQ_UINT(1, ut_free_library_calls);
}
#endif

#ifdef INCLUDE_SOCKET_COM
static void restores_and_closes_a_socket(void)
{
   reset_close(); ut_port.Method = kComMethodSocket;
   utt_ODComClose(ODPTR2HANDLE(&ut_port, tPortInfo));
   UT_ASSERT_EQ_INT(FALSE, ut_port.bIsOpen);
   UT_ASSERT_EQ_INT(kTelnetInputData, ut_port.TelnetInputState);
   UT_ASSERT_EQ_INT(FALSE, ut_port.bTelnetInputReplay);
   UT_ASSERT_EQ_INT(0, ut_port.btTelnetInputReplay);
}
#endif

#ifdef INCLUDE_STDIO_COM
static void restores_stdio_only_when_it_is_a_terminal(void)
{
   reset_close(); ut_port.Method = kComMethodStdIO; ut_isatty_result = 0;
   utt_ODComClose(ODPTR2HANDLE(&ut_port, tPortInfo));
   UT_ASSERT_EQ_UINT(0, ut_tcsetattr_calls);
   reset_close(); ut_port.Method = kComMethodStdIO; ut_isatty_result = 1;
   utt_ODComClose(ODPTR2HANDLE(&ut_port, tPortInfo));
   UT_ASSERT_EQ_UINT(1, ut_tcsetattr_calls);
}
#endif

static void accepts_an_unknown_method(void)
{
   reset_close(); ut_port.Method = (tComMethod)99;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComClose(ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_INT(FALSE, ut_port.bIsOpen);
}

static const UTTestCase ut_cases[] = {
   {"client handle", leaves_a_clients_handle_open},
#ifdef INCLUDE_FOSSIL_COM
   {"FOSSIL", closes_a_fossil_port},
#endif
#ifdef INCLUDE_UART_COM
   {"UART", restores_uart_state},
#endif
#ifdef INCLUDE_WIN32_COM
   {"Win32", closes_a_win32_handle},
#endif
#ifdef INCLUDE_DOOR32_COM
   {"Door32", shuts_down_door32},
#endif
#ifdef INCLUDE_SOCKET_COM
   {"socket", restores_and_closes_a_socket},
#endif
#ifdef INCLUDE_STDIO_COM
   {"stdio", restores_stdio_only_when_it_is_a_terminal},
#endif
   {"unknown", accepts_an_unknown_method}
};
