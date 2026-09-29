#define UT_CUSTOM_MOCK_ODComSendBuffer
#define UT_CUSTOM_MOCK_ODComCallIdleFunction
#ifdef ODPLAT_NIX
static int ut_errno;
#define UT_ERRNO_STORAGE ut_errno
#include "../unix_errno_mock.h"
#endif
static tPortInfo ut_port;
static BYTE ut_tx_queue[4];
static unsigned ut_buffer_calls;
static tODResult ut_buffer_result;
tODResult utm_ODComSendBuffer(tPortHandle handle, BYTE *buffer, int size)
{
   UT_ASSERT_EQ_PTR(&ut_port, ODHANDLE2PTR(handle, tPortInfo));
   UT_ASSERT_NOT_NULL(buffer);
   UT_ASSERT_EQ_INT(1, size);
   UT_ASSERT_EQ_INT(0xa5, buffer[0]);
   ++ut_buffer_calls;
   return(ut_buffer_result);
}

static unsigned ut_idle_calls;
static void ODCALL ut_idle(void) { ++ut_idle_calls; }

void utm_ODComCallIdleFunction(tPortInfo *port)
{
   UT_ASSERT_NOT_NULL(port);
   if(port->pfIdleCallback != NULL) (*port->pfIdleCallback)();
}

#ifdef __TURBOC__
static void ut_turbo_fossil_send_failures(BYTE count)
{
   ASM mov ah, 0xfd
   ASM mov al, count
   ASM int 0x14
}
#endif

#if defined(__WATCOMC__) && !defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK_int86
static unsigned ut_fossil_attempts;
int utm_int86(int interrupt_number, const union REGS *input,
   union REGS *output)
{
   UT_ASSERT_EQ_INT(0x14, interrupt_number);
   UT_ASSERT_EQ_INT(0x0b, input->h.ah);
   UT_ASSERT_EQ_INT(0xa5, input->h.al);
   UT_ASSERT_EQ_INT(2, input->x.dx);
   *output = *input;
   output->x.ax = ++ut_fossil_attempts >= 2;
   return(0);
}
#endif

#ifdef ODPLAT_DOS32
#define UT_CUSTOM_MOCK_OD32FossilSendByte
static unsigned ut_fossil_attempts;
BOOL utm_OD32FossilSendByte(BYTE port, BYTE value)
{
   UT_ASSERT_EQ_INT(2, port);
   UT_ASSERT_EQ_INT(0xa5, value);
   return(++ut_fossil_attempts >= 2);
}
#endif

#ifdef INCLUDE_UART_COM
#define UT_CUSTOM_MOCK_ODComInternalTXReady
#define UT_CUSTOM_MOCK_UTInp
#define UT_CUSTOM_MOCK_UTOutp
#define UT_CUSTOM_MOCK_UTDisable
#define UT_CUSTOM_MOCK_UTEnable
static unsigned ut_ready_calls;
static unsigned ut_port_writes;
static unsigned ut_disable_calls;
static unsigned ut_enable_calls;
BOOL utm_ODComInternalTXReady(void) { return(++ut_ready_calls >= 2); }
#ifdef __TURBOC__
int
#else
unsigned
#endif
utm_UTInp(unsigned port)
{
   UT_ASSERT_EQ_UINT((unsigned)nIntEnableRegAddr, port);
   return(1);
}
#ifdef __TURBOC__
int
#else
unsigned
#endif
utm_UTOutp(unsigned port, unsigned value)
{
   UT_ASSERT_EQ_UINT((unsigned)nIntEnableRegAddr, port);
   UT_ASSERT_EQ_UINT(1 | THRE, value);
   ++ut_port_writes;
   return(value);
}
void utm_UTDisable(void) { ++ut_disable_calls; }
void utm_UTEnable(void) { ++ut_enable_calls; }
#endif

#ifdef INCLUDE_WIN32_COM
#define UT_CUSTOM_MOCK_WriteFile
#define UT_CUSTOM_MOCK_ClearCommError
static BOOL ut_write_result;
static DWORD ut_bytes_written;
static unsigned ut_clear_calls;
BOOL WINAPI utm_WriteFile(HANDLE device, LPCVOID buffer, DWORD size,
   LPDWORD written, LPOVERLAPPED overlapped)
{
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, device);
   UT_ASSERT_EQ_INT(0xa5, *(const BYTE *)buffer);
   UT_ASSERT_EQ_UINT(1, size);
   UT_ASSERT_NULL(overlapped);
   *written = ut_bytes_written;
   return(ut_write_result);
}
BOOL WINAPI utm_ClearCommError(HANDLE device, LPDWORD errors,
   LPCOMSTAT status)
{
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, device);
   UT_ASSERT_NOT_NULL(errors);
   UT_ASSERT_NULL(status);
   ++ut_clear_calls;
   return(TRUE);
}
#endif

#ifdef INCLUDE_DOOR32_COM
static BOOL ut_door_result;
static unsigned ut_door_calls;
static BOOL WINAPI ut_door_write(const BYTE *buffer, DWORD size)
{
   UT_ASSERT_EQ_INT(0xa5, buffer[0]);
   UT_ASSERT_EQ_UINT(1, size);
   ++ut_door_calls;
   return(ut_door_result);
}
#endif

#ifdef INCLUDE_SOCKET_COM
#define UT_CUSTOM_MOCK_send
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_select
#define UT_CUSTOM_MOCK_WSAGetLastError
#else
#define UT_CUSTOM_MOCK_poll
#endif
#ifdef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_ODThreadSleep
#else
#define UT_CUSTOM_MOCK_od_sleep
#endif
static int ut_ready_results[4];
static short ut_ready_events[4];
static unsigned ut_ready_calls;
static int ut_send_results[4];
static unsigned ut_send_calls;
static int ut_socket_error;
#ifdef ODPLAT_WIN32
int PASCAL utm_select(int count, fd_set FAR *read_set,
   fd_set FAR *write_set, fd_set FAR *error_set,
   const struct timeval FAR *timeout)
{
   UT_ASSERT_EQ_INT(46, count); UT_ASSERT_NULL(read_set);
   UT_ASSERT_NOT_NULL(write_set); UT_ASSERT_NULL(error_set);
   UT_ASSERT_NOT_NULL(timeout); UT_ASSERT(ut_ready_calls < 4);
   return(ut_ready_results[ut_ready_calls++]);
}
int PASCAL utm_send(SOCKET socket_handle, const char FAR *buffer,
   int size, int flags)
#else
int utm_poll(struct pollfd *descriptors, nfds_t count, int timeout)
{
   UT_ASSERT_NOT_NULL(descriptors); UT_ASSERT_EQ_UINT(1, count);
   UT_ASSERT_EQ_INT(1000, timeout); UT_ASSERT(ut_ready_calls < 4);
   descriptors[0].revents = ut_ready_events[ut_ready_calls];
   return(ut_ready_results[ut_ready_calls++]);
}
ssize_t utm_send(int socket_handle, const void *buffer, size_t size, int flags)
#endif
{
   UT_ASSERT_EQ_INT(45, socket_handle);
   UT_ASSERT_EQ_INT(0xa5, *(const BYTE *)buffer);
   UT_ASSERT_EQ_UINT(1, size); UT_ASSERT_EQ_INT(0, flags);
   UT_ASSERT(ut_send_calls < 3);
   return(ut_send_results[ut_send_calls++]);
}
#ifdef ODPLAT_WIN32
int PASCAL utm_WSAGetLastError(void) { return(ut_socket_error); }
#endif
#ifdef OD_THREAD_SUPPORT
void utm_ODThreadSleep(tODMilliSec delay) { UT_ASSERT(delay == 50); }
#else
void ODCALL utm_od_sleep(tODMilliSec delay) { UT_ASSERT(delay == 50); }
#endif
#endif

#ifdef INCLUDE_STDIO_COM
#define UT_CUSTOM_MOCK_select
#define UT_CUSTOM_MOCK_fwrite
static int ut_stdio_select_results[12];
static unsigned ut_stdio_select_calls;
static unsigned ut_fwrite_calls;
static size_t ut_fwrite_result;
int utm_select(int count, fd_set *read_set, fd_set *write_set,
   fd_set *error_set, struct timeval *timeout)
{
   UT_ASSERT_EQ_INT(STDOUT_FILENO + 1, count); UT_ASSERT_NULL(read_set);
   UT_ASSERT_NOT_NULL(write_set); UT_ASSERT_NULL(error_set);
   UT_ASSERT_NOT_NULL(timeout);
   return(ut_stdio_select_results[ut_stdio_select_calls++]);
}
size_t utm_fwrite(const void *buffer, size_t size, size_t count, FILE *stream)
{
   UT_ASSERT_EQ_INT(0xa5, *(const BYTE *)buffer);
   UT_ASSERT_EQ_UINT(1, size); UT_ASSERT_EQ_UINT(1, count);
   UT_ASSERT_EQ_PTR(stdout, stream);
   ++ut_fwrite_calls;
   return(ut_fwrite_result);
}
#endif

static void reset_send(void)
{
   unsigned index;
   memset(&ut_port, 0, sizeof(ut_port));
   ut_port.bIsOpen = TRUE; ut_port.btPort = 2; ut_port.pfIdleCallback = ut_idle;
   ut_buffer_calls = 0; ut_buffer_result = kODRCSafeFailure;
   ut_idle_calls = 0; od_control.od_cp437_to_utf8_out = FALSE;
#if defined(__WATCOMC__) || defined(ODPLAT_DOS32)
   ut_fossil_attempts = 0;
#endif
#ifdef INCLUDE_UART_COM
   pbtTXQueue = ut_tx_queue; nTXQueueSize = 4; nTXInIndex = 3; nTXChars = 0;
   nIntEnableRegAddr = 0x3f9; ut_ready_calls = ut_port_writes = 0;
   ut_disable_calls = ut_enable_calls = 0;
#endif
#ifdef INCLUDE_WIN32_COM
   ut_port.hCommDev = (HANDLE)(DWORD_PTR)41;
   ut_write_result = TRUE; ut_bytes_written = 1; ut_clear_calls = 0;
#endif
#ifdef INCLUDE_DOOR32_COM
   ut_port.pfDoorWrite = ut_door_write; ut_door_result = TRUE; ut_door_calls = 0;
#endif
#ifdef INCLUDE_SOCKET_COM
   ut_port.socket = 45; ut_ready_calls = 0;
   ut_send_calls = 0; ut_socket_error = 0;
   errno = 0;
   for(index = 0; index < 4; ++index) {
      ut_ready_results[index] = 1; ut_ready_events[index] = POLLOUT;
      ut_send_results[index] = 1;
   }
#endif
#ifdef INCLUDE_STDIO_COM
   ut_stdio_select_calls = 0; ut_fwrite_calls = 0; ut_fwrite_result = 1;
   for(index = 0; index < 12; ++index) ut_stdio_select_results[index] = 1;
#endif
}

static void delegates_cp437_output(void)
{
   reset_send(); od_control.od_cp437_to_utf8_out = TRUE;
   UT_ASSERT_EQ_INT(kODRCSafeFailure,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
   UT_ASSERT_EQ_UINT(1, ut_buffer_calls);
}

#ifdef INCLUDE_FOSSIL_COM
static void waits_for_fossil_output_with_optional_idle(void)
{
   reset_send(); ut_port.Method = kComMethodFOSSIL;
#ifdef __TURBOC__
   ut_turbo_fossil_send_failures(1);
#endif
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
#if defined(__WATCOMC__) || defined(ODPLAT_DOS32)
   UT_ASSERT_EQ_UINT(2, ut_fossil_attempts); UT_ASSERT_EQ_UINT(1, ut_idle_calls);
#endif
   reset_send(); ut_port.Method = kComMethodFOSSIL; ut_port.pfIdleCallback = NULL;
#ifdef __TURBOC__
   ut_turbo_fossil_send_failures(1);
#endif
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
}
#endif

#ifdef INCLUDE_UART_COM
static void queues_uart_output_and_wraps_the_index(void)
{
   reset_send(); ut_port.Method = kComMethodUART;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
   UT_ASSERT_EQ_INT(0xa5, ut_tx_queue[3]); UT_ASSERT_EQ_UINT(0, nTXInIndex);
   UT_ASSERT_EQ_INT(1, nTXChars); UT_ASSERT_EQ_UINT(1, ut_idle_calls);
   reset_send(); ut_port.Method = kComMethodUART; nTXInIndex = 1;
   ut_port.pfIdleCallback = NULL;
   utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5);
   UT_ASSERT_EQ_UINT(2, nTXInIndex);
}
#endif

#ifdef INCLUDE_WIN32_COM
static void reports_each_win32_write_outcome(void)
{
   reset_send(); ut_port.Method = kComMethodWin32;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
   reset_send(); ut_port.Method = kComMethodWin32; ut_write_result = FALSE;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
   reset_send(); ut_port.Method = kComMethodWin32; ut_bytes_written = 0;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
}
#endif

#ifdef INCLUDE_DOOR32_COM
static void reports_door32_write_outcomes(void)
{
   reset_send(); ut_port.Method = kComMethodDoor32; ut_door_result = TRUE;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
   reset_send(); ut_port.Method = kComMethodDoor32; ut_door_result = FALSE;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
}
#endif

#ifdef INCLUDE_SOCKET_COM
static void reports_socket_readiness_send_retry_and_failure(void)
{
   reset_send(); ut_port.Method = kComMethodSocket;
   ut_port.bTelnetSocket = TRUE;
   UT_ASSERT_EQ_INT(kODRCSafeFailure,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
   UT_ASSERT_EQ_UINT(1, ut_buffer_calls);
   reset_send(); ut_port.Method = kComMethodSocket; ut_ready_results[0] = 0;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
   reset_send(); ut_port.Method = kComMethodSocket; ut_ready_results[0] = -1;
   ut_socket_error = 1; errno = 1;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
#ifndef ODPLAT_WIN32
   reset_send(); ut_port.Method = kComMethodSocket;
   ut_ready_events[0] = POLLHUP;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
#endif
   reset_send(); ut_port.Method = kComMethodSocket;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
   reset_send(); ut_port.Method = kComMethodSocket;
   ut_send_results[0] = SOCKET_ERROR; ut_socket_error = WSAEWOULDBLOCK;
#ifndef ODPLAT_WIN32
   errno = WSAEWOULDBLOCK;
#endif
   ut_send_results[1] = 1;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
   reset_send(); ut_port.Method = kComMethodSocket;
   ut_send_results[0] = SOCKET_ERROR; ut_socket_error = 1;
#ifndef ODPLAT_WIN32
   errno = 1;
#endif
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
   reset_send(); ut_port.Method = kComMethodSocket;
   ut_send_results[0] = 0;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
   reset_send(); ut_port.Method = kComMethodSocket;
   ut_ready_results[0] = -1; ut_ready_results[1] = 1;
#ifdef ODPLAT_WIN32
   ut_socket_error = WSAEINTR;
#else
   errno = EINTR;
#endif
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
   UT_ASSERT_EQ_UINT(2, ut_ready_calls); UT_ASSERT_EQ_UINT(1, ut_send_calls);
   reset_send(); ut_port.Method = kComMethodSocket;
   ut_send_results[0] = SOCKET_ERROR; ut_send_results[1] = 1;
#ifdef ODPLAT_WIN32
   ut_socket_error = WSAEINTR;
#else
   errno = EINTR;
#endif
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
   UT_ASSERT_EQ_UINT(2, ut_ready_calls); UT_ASSERT_EQ_UINT(2, ut_send_calls);
}
#endif

#ifdef INCLUDE_STDIO_COM
static void reports_stdio_select_and_write_outcomes(void)
{
   reset_send(); ut_port.Method = kComMethodStdIO;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
   reset_send(); ut_port.Method = kComMethodStdIO; ut_fwrite_result = 0;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
   reset_send(); ut_port.Method = kComMethodStdIO;
   for(ut_stdio_select_calls = 0; ut_stdio_select_calls < 11;
      ++ut_stdio_select_calls) ut_stdio_select_results[ut_stdio_select_calls] = 0;
   ut_stdio_select_calls = 0;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
   UT_ASSERT_EQ_UINT(10, ut_stdio_select_calls);
   UT_ASSERT_EQ_UINT(0, ut_fwrite_calls);
   reset_send(); ut_port.Method = kComMethodStdIO;
   ut_stdio_select_results[0] = 2;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
   reset_send(); ut_port.Method = kComMethodStdIO;
   ut_stdio_select_results[0] = -1; errno = 1;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
   reset_send(); ut_port.Method = kComMethodStdIO;
   ut_stdio_select_results[0] = -1; ut_stdio_select_results[1] = 1;
   errno = EINTR;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
}
#endif

static void accepts_an_unknown_method(void)
{
   reset_send(); ut_port.Method = (tComMethod)99;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSendByte(ODPTR2HANDLE(&ut_port, tPortInfo), 0xa5));
}

static const UTTestCase ut_cases[] = {
   {"CP437", delegates_cp437_output},
#ifdef INCLUDE_FOSSIL_COM
   {"FOSSIL", waits_for_fossil_output_with_optional_idle},
#endif
#ifdef INCLUDE_UART_COM
   {"UART", queues_uart_output_and_wraps_the_index},
#endif
#ifdef INCLUDE_WIN32_COM
   {"Win32", reports_each_win32_write_outcome},
#endif
#ifdef INCLUDE_DOOR32_COM
   {"Door32", reports_door32_write_outcomes},
#endif
#ifdef INCLUDE_SOCKET_COM
   {"socket", reports_socket_readiness_send_retry_and_failure},
#endif
#ifdef INCLUDE_STDIO_COM
   {"stdio", reports_stdio_select_and_write_outcomes},
#endif
   {"unknown", accepts_an_unknown_method}
};
