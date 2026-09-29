#ifdef ODPLAT_DOS32
#define UT_CUSTOM_MOCK_OD32FossilStatus
static WORD ut_fossil_status;

WORD utm_OD32FossilStatus(BYTE port)
{
   UT_ASSERT_EQ_INT(2, port);
   return(ut_fossil_status);
}
#endif

#ifdef INCLUDE_WIN32_COM
#define UT_CUSTOM_MOCK_ClearCommError
static BOOL ut_status_succeeds;
static DWORD ut_inbound_count;

BOOL WINAPI utm_ClearCommError(HANDLE device, LPDWORD errors,
   LPCOMSTAT status)
{
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, device);
   UT_ASSERT_NOT_NULL(errors);
   UT_ASSERT_NOT_NULL(status);
   *errors = 0x1234;
   status->cbInQue = ut_inbound_count;
   return(ut_status_succeeds);
}
#endif

#ifdef INCLUDE_DOOR32_COM
#define UT_CUSTOM_MOCK_WaitForSingleObject
static DWORD ut_wait_result;

static HANDLE WINAPI ut_available_handle(void)
{
   return((HANDLE)(DWORD_PTR)43);
}

DWORD WINAPI utm_WaitForSingleObject(HANDLE object, DWORD timeout)
{
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)43, object);
   UT_ASSERT_EQ_UINT(0, timeout);
   return(ut_wait_result);
}
#endif

#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ioctlsocket
static int ut_socket_result;
static u_long ut_socket_count;

int PASCAL utm_ioctlsocket(SOCKET socket_handle, long command,
   u_long FAR *value)
{
   UT_ASSERT_EQ_INT(45, socket_handle);
   UT_ASSERT((long)FIONREAD == command);
   UT_ASSERT_NOT_NULL(value);
   UT_ASSERT_EQ_UINT(99, *value);
   *value = ut_socket_count;
   return(ut_socket_result);
}
#endif

#ifdef ODPLAT_NIX
#define UT_CUSTOM_MOCK_ioctl
static int ut_socket_result;
static int ut_socket_count;
static int ut_stdio_result;
static int ut_stdio_count;

int utm_ioctl(int descriptor, unsigned long request, ...)
{
   va_list arguments;
   int *value;
   va_start(arguments, request);
   value = va_arg(arguments, int *);
   va_end(arguments);
   UT_ASSERT((unsigned long)FIONREAD == request);
   UT_ASSERT_NOT_NULL(value);
   if(descriptor == 45)
   {
      *value = ut_socket_count;
      return(ut_socket_result);
   }
   UT_ASSERT_EQ_INT(0, descriptor);
   *value = ut_stdio_count;
   return(ut_stdio_result);
}
#endif

static tPortInfo ut_port;

static void reset_inbound(void)
{
   memset(&ut_port, 0, sizeof(ut_port));
   ut_port.bIsOpen = TRUE;
   ut_port.btPort = 2;
#ifdef ODPLAT_DOS32
   ut_fossil_status = 0;
#endif
#ifdef INCLUDE_WIN32_COM
   ut_port.hCommDev = (HANDLE)(DWORD_PTR)41;
   ut_status_succeeds = TRUE;
   ut_inbound_count = 0;
#endif
#ifdef INCLUDE_DOOR32_COM
   ut_port.pfDoorGetAvailableEventHandle = ut_available_handle;
   ut_wait_result = WAIT_TIMEOUT;
#endif
#ifdef ODPLAT_WIN32
   ut_port.socket = 45;
   ut_socket_result = 0;
   ut_socket_count = 0;
#endif
#ifdef ODPLAT_NIX
   ut_port.socket = 45;
   ut_socket_result = 0;
   ut_socket_count = 0;
   ut_stdio_result = 0;
   ut_stdio_count = 0;
#endif
}

#ifdef INCLUDE_FOSSIL_COM
#ifndef ODPLAT_DOS32
static void ut_fossil_clear_input(void)
{
   ASM mov ah, 0x0a
   ASM mov dx, 2
   ASM int 0x14
}

static void ut_fossil_send_byte(void)
{
   ASM mov ah, 0x0b
   ASM mov al, 0xa5
   ASM mov dx, 2
   ASM int 0x14
}
#endif

static void reports_fossil_empty_or_nonempty_state(void)
{
   int waiting = 99;
   reset_inbound();
   ut_port.Method = kComMethodFOSSIL;
#ifdef ODPLAT_DOS32
   ut_fossil_status = 0x0100;
#else
   ut_fossil_clear_input();
#endif
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComInbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
#ifdef ODPLAT_DOS32
   UT_ASSERT_EQ_INT(SIZE_NON_ZERO, waiting);
   ut_fossil_status = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComInbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(0, waiting);
#else
   UT_ASSERT_EQ_INT(0, waiting);
   ut_fossil_send_byte();
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComInbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(SIZE_NON_ZERO, waiting);
   ut_fossil_clear_input();
#endif
}
#endif

#ifdef INCLUDE_UART_COM
static void reports_the_uart_queue_count(void)
{
   int waiting;
   reset_inbound();
   ut_port.Method = kComMethodUART;
   nRXChars = 17;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComInbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(17, waiting);
}
#endif

#ifdef INCLUDE_WIN32_COM
static void reports_the_win32_queue_count_and_failure(void)
{
   int waiting = 99;
   reset_inbound();
   ut_port.Method = kComMethodWin32;
   ut_status_succeeds = FALSE;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComInbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(99, waiting);
   ut_status_succeeds = TRUE;
   ut_inbound_count = 123;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComInbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(123, waiting);
}
#endif

#ifdef INCLUDE_DOOR32_COM
static void reports_door32_available_event_state(void)
{
   int waiting;
   reset_inbound();
   ut_port.Method = kComMethodDoor32;
   ut_wait_result = WAIT_OBJECT_0;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComInbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(SIZE_NON_ZERO, waiting);
   ut_wait_result = WAIT_TIMEOUT;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComInbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(0, waiting);
}
#endif

#ifdef INCLUDE_SOCKET_COM
static void reports_socket_count_and_maps_failure_to_empty(void)
{
   int waiting = 99;
   reset_inbound();
   ut_port.Method = kComMethodSocket;
   ut_socket_count = 23;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComInbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(23, waiting);
   waiting = 99;
   ut_socket_result = -1;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComInbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(0, waiting);
}
#endif

#ifdef INCLUDE_STDIO_COM
static void reports_stdio_count_and_maps_failure_to_empty(void)
{
   int waiting = 99;
   reset_inbound();
   ut_port.Method = kComMethodStdIO;
   ut_stdio_count = 24;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComInbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(24, waiting);
   waiting = 99;
   ut_stdio_result = -1;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComInbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(0, waiting);
}
#endif

static void leaves_output_untouched_for_an_unknown_method(void)
{
   int waiting = 99;
   reset_inbound();
   ut_port.Method = (tComMethod)99;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComInbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(99, waiting);
}

static const UTTestCase ut_cases[] = {
#ifdef INCLUDE_FOSSIL_COM
   {"FOSSIL", reports_fossil_empty_or_nonempty_state},
#endif
#ifdef INCLUDE_UART_COM
   {"UART", reports_the_uart_queue_count},
#endif
#ifdef INCLUDE_WIN32_COM
   {"Win32", reports_the_win32_queue_count_and_failure},
#endif
#ifdef INCLUDE_DOOR32_COM
   {"Door32", reports_door32_available_event_state},
#endif
#ifdef INCLUDE_SOCKET_COM
   {"socket", reports_socket_count_and_maps_failure_to_empty},
#endif
#ifdef INCLUDE_STDIO_COM
   {"stdio", reports_stdio_count_and_maps_failure_to_empty},
#endif
   {"unknown", leaves_output_untouched_for_an_unknown_method}
};
