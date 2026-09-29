#define UT_CUSTOM_MOCK_ODComInbound
#define UT_CUSTOM_MOCK_ODComCallIdleFunction
static tPortInfo ut_port;
static int ut_inbound_size;
tODResult utm_ODComInbound(tPortHandle port, int *waiting)
{
   UT_ASSERT_NOT_NULL(port); UT_ASSERT_NOT_NULL(waiting);
   *waiting = ut_inbound_size;
   return(kODRCSuccess);
}

void utm_ODComCallIdleFunction(tPortInfo *port)
{
   UT_ASSERT_NOT_NULL(port);
   if(port->pfIdleCallback != NULL) (*port->pfIdleCallback)();
}

#ifdef ODPLAT_NIX
static int ut_errno;
#define UT_ERRNO_STORAGE ut_errno
#include "../unix_errno_mock.h"
#endif

#if defined(__TURBOC__) || (defined(__WATCOMC__) && !defined(ODPLAT_DOS32))
static void ut_fossil_clear(void)
{
   ASM mov ah, 0x0a
   ASM int 0x14
}
static void ut_fossil_queue(BYTE value)
{
   ASM mov ah, 0x0b
   ASM mov al, value
   ASM int 0x14
}
#endif

#ifdef ODPLAT_DOS32
#define UT_CUSTOM_MOCK_OD32FossilGetByte
static BYTE ut_fossil_byte;
BYTE utm_OD32FossilGetByte(BYTE port)
{
   UT_ASSERT_EQ_INT(2, port);
   return(ut_fossil_byte);
}
#endif

#ifdef INCLUDE_UART_COM
#define UT_CUSTOM_MOCK_UTInp
#define UT_CUSTOM_MOCK_UTOutp
#define UT_CUSTOM_MOCK_UTDisable
#define UT_CUSTOM_MOCK_UTEnable
static BYTE ut_rx_queue[4];
static unsigned ut_disable_calls;
static unsigned ut_enable_calls;
static unsigned ut_port_writes;
static unsigned ut_idle_calls;
#ifdef __TURBOC__
int
#else
unsigned
#endif
utm_UTInp(unsigned port)
{
   UT_ASSERT_EQ_UINT((unsigned)nModemCtrlRegAddr, port);
   return(1);
}
#ifdef __TURBOC__
int
#else
unsigned
#endif
utm_UTOutp(unsigned port, unsigned value)
{
   UT_ASSERT_EQ_UINT((unsigned)nModemCtrlRegAddr, port);
   UT_ASSERT_EQ_UINT(1 | RTS, value);
   ++ut_port_writes;
   return(value);
}
void utm_UTDisable(void) { ++ut_disable_calls; }
void utm_UTEnable(void) { ++ut_enable_calls; }
static void ODCALL ut_supply_uart_byte(void)
{
   ++ut_idle_calls;
   nRXChars = 1;
}
#endif

#ifdef INCLUDE_WIN32_COM
#define UT_CUSTOM_MOCK_ODComWin32SetReadTimeouts
#define UT_CUSTOM_MOCK_ReadFile
#define UT_CUSTOM_MOCK_ClearCommError
static tReadTimeoutState ut_timeout_state;
static BOOL ut_read_succeeds;
static DWORD ut_bytes_read;
static unsigned ut_clear_calls;
tODResult utm_ODComWin32SetReadTimeouts(tPortInfo *port,
   tReadTimeoutState state)
{
   UT_ASSERT_EQ_PTR(&ut_port, port);
   ut_timeout_state = state;
   return(kODRCSuccess);
}
BOOL WINAPI utm_ReadFile(HANDLE device, LPVOID buffer, DWORD size,
   LPDWORD read_count, LPOVERLAPPED overlapped)
{
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, device);
   UT_ASSERT_EQ_UINT(1, size); UT_ASSERT_NULL(overlapped);
   *(BYTE *)buffer = 0x5a; *read_count = ut_bytes_read;
   return(ut_read_succeeds);
}
BOOL WINAPI utm_ClearCommError(HANDLE device, LPDWORD errors,
   LPCOMSTAT status)
{
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, device);
   UT_ASSERT_NOT_NULL(errors); UT_ASSERT_NULL(status);
   ++ut_clear_calls;
   return(TRUE);
}
#endif

#ifdef INCLUDE_DOOR32_COM
#define UT_CUSTOM_MOCK_WaitForSingleObject
static DWORD ut_wait_result;
static HANDLE WINAPI ut_available_handle(void)
{
   return((HANDLE)(DWORD_PTR)43);
}
static DWORD WINAPI ut_door_read(BYTE *buffer, DWORD size)
{
   UT_ASSERT_EQ_UINT(1, size); buffer[0] = 0x5b; return(1);
}
DWORD WINAPI utm_WaitForSingleObject(HANDLE object, DWORD timeout)
{
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)43, object);
   UT_ASSERT(timeout == 0 || timeout == INFINITE);
   return(ut_wait_result);
}
#endif

#ifdef INCLUDE_SOCKET_COM
#define UT_CUSTOM_MOCK_recv
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_select
#define UT_CUSTOM_MOCK_WSAGetLastError
#else
#define UT_CUSTOM_MOCK_poll
#endif
#ifdef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_ODSemaphoreUp
#define UT_CUSTOM_MOCK_ODThreadSleep
#else
#define UT_CUSTOM_MOCK_od_sleep
#endif
static int ut_ready_result;
static short ut_ready_events;
static int ut_recv_results[16];
static BYTE ut_recv_bytes[16];
static unsigned ut_recv_calls;
static int ut_socket_error;
static unsigned ut_semaphore_calls, ut_sleep_calls;
#ifdef ODPLAT_WIN32
int PASCAL utm_select(int count, fd_set FAR *read_set,
   fd_set FAR *write_set, fd_set FAR *error_set,
   const struct timeval FAR *timeout)
{
   UT_ASSERT_EQ_INT(46, count); UT_ASSERT_NOT_NULL(read_set);
   UT_ASSERT_NULL(write_set); UT_ASSERT_NULL(error_set);
   UT_ASSERT(timeout == NULL || timeout->tv_usec == 100);
   return(ut_ready_result);
}
int PASCAL utm_WSAGetLastError(void) { return(ut_socket_error); }
int PASCAL utm_recv(SOCKET socket_handle, char FAR *buffer,
   int size, int flags)
#else
int utm_poll(struct pollfd *descriptors, nfds_t count, int timeout)
{
   UT_ASSERT_NOT_NULL(descriptors); UT_ASSERT_EQ_UINT(1, count);
   UT_ASSERT(timeout == -1 || timeout <= 200);
   descriptors[0].revents = ut_ready_events;
   return(ut_ready_result);
}
ssize_t utm_recv(int socket_handle, void *buffer, size_t size, int flags)
#endif
{
   int result;
   UT_ASSERT_EQ_INT(45, socket_handle); UT_ASSERT_EQ_UINT(1, size);
   UT_ASSERT_EQ_INT(0, flags); UT_ASSERT(ut_recv_calls < 16);
   result = ut_recv_results[ut_recv_calls++];
   if(result == 1) *(BYTE *)buffer = ut_recv_bytes[ut_recv_calls - 1];
   return(result);
}
#ifdef OD_THREAD_SUPPORT
void utm_ODSemaphoreUp(tODSemaphoreHandle semaphore, INT count)
{
   UT_ASSERT_EQ_PTR((tODSemaphoreHandle)(DWORD_PTR)47, semaphore);
   UT_ASSERT_EQ_UINT(1, count); ++ut_semaphore_calls;
}
void utm_ODThreadSleep(tODMilliSec delay)
{ UT_ASSERT(delay == 50); ++ut_sleep_calls; }
#else
void ODCALL utm_od_sleep(tODMilliSec delay)
{ UT_ASSERT(delay == 50); ++ut_sleep_calls; }
#endif
#endif

#ifdef INCLUDE_STDIO_COM
#define UT_CUSTOM_MOCK_select
#define UT_CUSTOM_MOCK_read
static int ut_stdio_select[3];
static unsigned ut_stdio_select_calls;
static int ut_stdio_read_result;
int utm_select(int count, fd_set *read_set, fd_set *write_set,
   fd_set *error_set, struct timeval *timeout)
{
   UT_ASSERT_EQ_INT(STDIN_FILENO + 1, count); UT_ASSERT_NOT_NULL(read_set);
   UT_ASSERT_NULL(write_set); UT_ASSERT_NULL(error_set);
   UT_ASSERT(timeout == NULL || timeout->tv_usec <= 200000);
   return(ut_stdio_select[ut_stdio_select_calls++]);
}
ssize_t utm_read(int descriptor, void *buffer, size_t size)
{
   UT_ASSERT_EQ_INT(STDIN_FILENO, descriptor); UT_ASSERT_EQ_UINT(1, size);
   if(ut_stdio_read_result == 1) *(BYTE *)buffer = 0x5d;
   return(ut_stdio_read_result);
}
#endif

static void reset_get(void)
{
   unsigned index;
   memset(&ut_port, 0, sizeof(ut_port));
   ut_port.bIsOpen = TRUE; ut_port.btPort = 2; ut_inbound_size = 1;
#ifdef ODPLAT_DOS32
   ut_fossil_byte = 0x59;
#endif
#ifdef INCLUDE_UART_COM
   pbtRXQueue = ut_rx_queue; nRXQueueSize = 4; nRXOutIndex = 3;
   nRXChars = 1; nRXLowWaterMark = 0; btFlowControl = 0;
   nModemCtrlRegAddr = 0x3fc; ut_rx_queue[3] = 0x58;
   ut_disable_calls = ut_enable_calls = ut_port_writes = ut_idle_calls = 0;
   ut_port.pfIdleCallback = ut_supply_uart_byte;
#endif
#ifdef INCLUDE_WIN32_COM
   ut_port.hCommDev = (HANDLE)(DWORD_PTR)41; ut_timeout_state = kNotSet;
   ut_read_succeeds = TRUE; ut_bytes_read = 1; ut_clear_calls = 0;
#endif
#ifdef INCLUDE_DOOR32_COM
   ut_port.pfDoorGetAvailableEventHandle = ut_available_handle;
   ut_port.pfDoorRead = ut_door_read; ut_wait_result = WAIT_OBJECT_0;
#endif
#ifdef INCLUDE_SOCKET_COM
   ut_port.socket = 45; ut_ready_result = 1; ut_ready_events = POLLIN;
   ut_recv_calls = 0; ut_socket_error = 0; ut_semaphore_calls = 0;
   ut_sleep_calls = 0;
   errno = 0;
   for(index = 0; index < 16; ++index) {
      ut_recv_results[index] = 1; ut_recv_bytes[index] = 0x5c;
   }
#endif
#ifdef INCLUDE_STDIO_COM
   ut_stdio_select_calls = 0; ut_stdio_read_result = 1;
   for(index = 0; index < 3; ++index) ut_stdio_select[index] = 1;
   ODMaxMSToWait = OD_NO_TIMEOUT;
#endif
}

#ifdef INCLUDE_FOSSIL_COM
static void gets_fossil_bytes_and_reports_an_empty_nonblocking_queue(void)
{
   char value = 0;
   reset_get(); ut_port.Method = kComMethodFOSSIL; ut_inbound_size = 0;
   UT_ASSERT_EQ_INT(kODRCNothingWaiting,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
#if defined(__TURBOC__) || (defined(__WATCOMC__) && !defined(ODPLAT_DOS32))
   ut_fossil_clear(); ut_fossil_queue(0x59);
#endif
   reset_get(); ut_port.Method = kComMethodFOSSIL;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
#if defined(__TURBOC__) || (defined(__WATCOMC__) && !defined(ODPLAT_DOS32))
   ut_fossil_queue(0x59);
#endif
   reset_get(); ut_port.Method = kComMethodFOSSIL;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   UT_ASSERT_EQ_INT(0x59, (BYTE)value);
}
#endif

#ifdef INCLUDE_UART_COM
static void gets_uart_bytes_with_queue_flow_and_wait_variants(void)
{
   char value = 0;
   reset_get(); ut_port.Method = kComMethodUART; nRXChars = 0;
   UT_ASSERT_EQ_INT(kODRCNothingWaiting,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
   reset_get(); ut_port.Method = kComMethodUART;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
   reset_get(); ut_port.Method = kComMethodUART; nRXChars = 0;
   nRXLowWaterMark = 1; btFlowControl = FLOW_RTSCTS;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   UT_ASSERT_EQ_INT(0x58, (BYTE)value); UT_ASSERT_EQ_UINT(0, nRXOutIndex);
   UT_ASSERT_EQ_UINT(1, ut_idle_calls); UT_ASSERT_EQ_UINT(1, ut_port_writes);
   reset_get(); ut_port.Method = kComMethodUART; nRXOutIndex = 1;
   nRXLowWaterMark = 0; nRXChars = 2;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   UT_ASSERT_EQ_UINT(2, nRXOutIndex); UT_ASSERT_EQ_UINT(0, ut_port_writes);
   reset_get(); ut_port.Method = kComMethodUART; nRXLowWaterMark = 1;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   UT_ASSERT_EQ_UINT(0, ut_port_writes);
}
#endif

#ifdef INCLUDE_WIN32_COM
static void reports_win32_read_outcomes(void)
{
   char value = 0;
   reset_get(); ut_port.Method = kComMethodWin32;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   UT_ASSERT_EQ_INT(kBlocking, ut_timeout_state);
   reset_get(); ut_port.Method = kComMethodWin32; ut_read_succeeds = FALSE;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
   UT_ASSERT_EQ_UINT(1, ut_clear_calls);
   reset_get(); ut_port.Method = kComMethodWin32; ut_bytes_read = 0;
   UT_ASSERT_EQ_INT(kODRCNothingWaiting,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
   UT_ASSERT_EQ_INT(kNonBlocking, ut_timeout_state);
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
}
#endif

#ifdef INCLUDE_DOOR32_COM
static void reports_door32_event_outcomes(void)
{
   char value = 0;
   reset_get(); ut_port.Method = kComMethodDoor32;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   UT_ASSERT_EQ_INT(0x5b, (BYTE)value);
   reset_get(); ut_port.Method = kComMethodDoor32; ut_wait_result = WAIT_TIMEOUT;
   UT_ASSERT_EQ_INT(kODRCNothingWaiting,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
}
#endif

#ifdef INCLUDE_SOCKET_COM
static void reports_socket_readiness_receive_and_retry_outcomes(void)
{
   char value = 0;
   reset_get(); ut_port.Method = kComMethodSocket; ut_ready_result = 0;
#ifndef ODPLAT_WIN32
   ODMaxMSToWait = 250;
#endif
   UT_ASSERT_EQ_INT(kODRCNothingWaiting,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
   reset_get(); ut_port.Method = kComMethodSocket; ut_ready_result = -1;
#ifndef ODPLAT_WIN32
   ODMaxMSToWait = 100;
#endif
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
#ifdef ODPLAT_WIN32
   reset_get(); ut_port.Method = kComMethodSocket; ut_ready_result = -1;
   ut_port.bTelnetSocket = TRUE;
   ut_port.TelnetInputState = kTelnetInputIAC;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
   UT_ASSERT_EQ_INT(kTelnetInputData, ut_port.TelnetInputState);
   UT_ASSERT_EQ_INT(FALSE, ut_port.bTelnetInputReplay);
#endif
#ifndef ODPLAT_WIN32
   reset_get(); ut_port.Method = kComMethodSocket;
   ut_ready_events = POLLHUP;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
   reset_get(); ut_port.Method = kComMethodSocket; ut_ready_events = 0;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
#endif
   reset_get(); ut_port.Method = kComMethodSocket;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   UT_ASSERT_EQ_INT(0x5c, (BYTE)value);
   reset_get(); ut_port.Method = kComMethodSocket;
   ut_recv_results[0] = SOCKET_ERROR; ut_recv_results[1] = 1;
   ut_socket_error = WSAEWOULDBLOCK; errno = WSAEWOULDBLOCK;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   UT_ASSERT_EQ_UINT(1, ut_sleep_calls);
   reset_get(); ut_port.Method = kComMethodSocket;
   ut_recv_results[0] = SOCKET_ERROR;
   ut_socket_error = WSAEWOULDBLOCK; errno = WSAEWOULDBLOCK;
   UT_ASSERT_EQ_INT(kODRCNothingWaiting,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
   UT_ASSERT_EQ_UINT(0, ut_sleep_calls);
   reset_get(); ut_port.Method = kComMethodSocket; ut_recv_results[0] = 0;
   UT_ASSERT_EQ_INT(kODRCNothingWaiting,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   reset_get(); ut_port.Method = kComMethodSocket;
   ut_recv_results[0] = SOCKET_ERROR; ut_socket_error = 1; errno = 1;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
}

static void filters_telnet_negotiation_before_returning_data(void)
{
   char value = 0;
   reset_get(); ut_port.Method = kComMethodSocket;
   ut_port.bTelnetSocket = TRUE;
   ut_recv_bytes[0] = 0xff;
   ut_recv_bytes[1] = 0xfb;
   ut_recv_bytes[2] = 1;
   ut_recv_bytes[3] = 'A';
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   UT_ASSERT_EQ_INT('A', (BYTE)value);
   UT_ASSERT_EQ_UINT(4, ut_recv_calls);
}

static void decodes_telnet_data_and_preserves_stream_state(void)
{
   char value = 0;
   unsigned index;
   static const BYTE negotiation_commands[4] = {251, 252, 253, 254};

   reset_get(); ut_port.Method = kComMethodSocket;
   ut_port.bTelnetSocket = TRUE; ut_recv_bytes[0] = 'A';
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetByte(
      ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   UT_ASSERT_EQ_INT('A', (BYTE)value);

   reset_get(); ut_port.Method = kComMethodSocket;
   ut_port.bTelnetSocket = TRUE;
   ut_recv_bytes[0] = 0xff; ut_recv_bytes[1] = 0xff;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetByte(
      ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   UT_ASSERT_EQ_INT(0xff, (BYTE)value);

   reset_get(); ut_port.Method = kComMethodSocket;
   ut_port.bTelnetSocket = TRUE;
   ut_recv_bytes[0] = '\r'; ut_recv_bytes[1] = '\n';
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetByte(
      ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   UT_ASSERT_EQ_INT('\r', (BYTE)value);

   reset_get(); ut_port.Method = kComMethodSocket;
   ut_port.bTelnetSocket = TRUE;
   ut_recv_bytes[0] = '\r'; ut_recv_bytes[1] = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetByte(
      ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   UT_ASSERT_EQ_INT('\r', (BYTE)value);

   reset_get(); ut_port.Method = kComMethodSocket;
   ut_port.bTelnetSocket = TRUE;
   ut_recv_bytes[0] = '\r'; ut_recv_bytes[1] = 'B';
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetByte(
      ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   UT_ASSERT_EQ_INT('\r', (BYTE)value);
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetByte(
      ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
   UT_ASSERT_EQ_INT('B', (BYTE)value);
   UT_ASSERT_EQ_UINT(2, ut_recv_calls);

   reset_get(); ut_port.Method = kComMethodSocket;
   ut_port.bTelnetSocket = TRUE;
   ut_recv_bytes[0] = 0xff; ut_recv_bytes[1] = 241;
   ut_recv_bytes[2] = 'C';
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetByte(
      ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   UT_ASSERT_EQ_INT('C', (BYTE)value);

   reset_get(); ut_port.Method = kComMethodSocket;
   ut_port.bTelnetSocket = TRUE;
   ut_recv_bytes[0] = 0xff; ut_recv_bytes[1] = 250;
   ut_recv_bytes[2] = 1; ut_recv_bytes[3] = 'x';
   ut_recv_bytes[4] = 0xff; ut_recv_bytes[5] = 0xff;
   ut_recv_bytes[6] = 'y'; ut_recv_bytes[7] = 0xff;
   ut_recv_bytes[8] = 240; ut_recv_bytes[9] = 'D';
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetByte(
      ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   UT_ASSERT_EQ_INT('D', (BYTE)value);
   UT_ASSERT_EQ_UINT(10, ut_recv_calls);

   for(index = 0; index < 4; ++index)
   {
      reset_get(); ut_port.Method = kComMethodSocket;
      ut_port.bTelnetSocket = TRUE;
      ut_recv_bytes[0] = 0xff;
      ut_recv_bytes[1] = negotiation_commands[index];
      ut_recv_bytes[2] = 1; ut_recv_bytes[3] = 'E';
      UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetByte(
         ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
      UT_ASSERT_EQ_INT('E', (BYTE)value);
   }

   reset_get(); ut_port.Method = kComMethodSocket;
   ut_port.bTelnetSocket = TRUE;
   ut_port.TelnetInputState = (tTelnetInputState)99;
   ut_recv_bytes[0] = 'x'; ut_recv_bytes[1] = 'F';
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetByte(
      ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   UT_ASSERT_EQ_INT('F', (BYTE)value);
}

static void retains_or_discards_incomplete_telnet_sequences(void)
{
   char value = 0;

   reset_get(); ut_port.Method = kComMethodSocket;
   ut_port.bTelnetSocket = TRUE;
   ut_recv_bytes[0] = 0xff;
   ut_recv_results[1] = SOCKET_ERROR;
   ut_socket_error = WSAEWOULDBLOCK; errno = WSAEWOULDBLOCK;
   UT_ASSERT_EQ_INT(kODRCNothingWaiting, utt_ODComGetByte(
      ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
   UT_ASSERT_EQ_INT(kTelnetInputIAC, ut_port.TelnetInputState);
   ut_recv_results[2] = 1; ut_recv_bytes[2] = 0xff;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetByte(
      ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
   UT_ASSERT_EQ_INT(0xff, (BYTE)value);

   reset_get(); ut_port.Method = kComMethodSocket;
   ut_port.bTelnetSocket = TRUE;
   ut_recv_bytes[0] = 0xff; ut_recv_results[1] = 0;
   UT_ASSERT_EQ_INT(kODRCNothingWaiting, utt_ODComGetByte(
      ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   UT_ASSERT_EQ_INT(kTelnetInputData, ut_port.TelnetInputState);
   UT_ASSERT_EQ_INT(FALSE, ut_port.bTelnetInputReplay);

   reset_get(); ut_port.Method = kComMethodSocket;
   ut_port.bTelnetSocket = TRUE;
   ut_recv_bytes[0] = '\r'; ut_recv_results[1] = SOCKET_ERROR;
   ut_socket_error = 1; errno = 1;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure, utt_ODComGetByte(
      ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   UT_ASSERT_EQ_INT(kTelnetInputData, ut_port.TelnetInputState);

#ifndef ODPLAT_WIN32
   reset_get(); ut_port.Method = kComMethodSocket;
   ut_port.bTelnetSocket = TRUE; ut_ready_result = -1;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure, utt_ODComGetByte(
      ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
   UT_ASSERT_EQ_INT(kTelnetInputData, ut_port.TelnetInputState);
#endif
}
#endif

#ifdef INCLUDE_STDIO_COM
static void reports_stdio_select_and_read_outcomes(void)
{
   char value = 0;
   reset_get(); ut_port.Method = kComMethodStdIO;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
   reset_get(); ut_port.Method = kComMethodStdIO; ODMaxMSToWait = 250;
   ut_stdio_select[0] = 0;
   UT_ASSERT_EQ_INT(kODRCNothingWaiting,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
   reset_get(); ut_port.Method = kComMethodStdIO; ODMaxMSToWait = 100;
   ut_stdio_select[0] = -1; errno = 1;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
   reset_get(); ut_port.Method = kComMethodStdIO;
   ut_stdio_select[0] = -1; ut_stdio_select[1] = 1; errno = EINTR;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
   reset_get(); ut_port.Method = kComMethodStdIO; ut_stdio_read_result = 0;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, TRUE));
}
#endif

static void accepts_an_unknown_method(void)
{
   char value = 0;
   reset_get(); ut_port.Method = (tComMethod)99;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComGetByte(ODPTR2HANDLE(&ut_port, tPortInfo), &value, FALSE));
}

static const UTTestCase ut_cases[] = {
#ifdef INCLUDE_FOSSIL_COM
   {"FOSSIL", gets_fossil_bytes_and_reports_an_empty_nonblocking_queue},
#endif
#ifdef INCLUDE_UART_COM
   {"UART", gets_uart_bytes_with_queue_flow_and_wait_variants},
#endif
#ifdef INCLUDE_WIN32_COM
   {"Win32", reports_win32_read_outcomes},
#endif
#ifdef INCLUDE_DOOR32_COM
   {"Door32", reports_door32_event_outcomes},
#endif
#ifdef INCLUDE_SOCKET_COM
   {"socket", reports_socket_readiness_receive_and_retry_outcomes},
   {"Telnet negotiation", filters_telnet_negotiation_before_returning_data},
   {"Telnet data", decodes_telnet_data_and_preserves_stream_state},
   {"Telnet incomplete", retains_or_discards_incomplete_telnet_sequences},
#endif
#ifdef INCLUDE_STDIO_COM
   {"stdio", reports_stdio_select_and_read_outcomes},
#endif
   {"unknown", accepts_an_unknown_method}
};
