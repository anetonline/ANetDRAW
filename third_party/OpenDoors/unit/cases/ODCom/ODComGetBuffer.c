static tPortInfo ut_port;

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
#define UT_CUSTOM_MOCK_OD32FossilReceiveBlock
#define UT_CUSTOM_MOCK_OD32FossilStatus
#define UT_CUSTOM_MOCK_OD32FossilGetByte
static int ut_block_result;
static WORD ut_status_results[5];
static unsigned ut_status_calls;
static unsigned ut_get_calls;
INT utm_OD32FossilReceiveBlock(BYTE port, tOD32FossilBuffer *fossil,
   BYTE *buffer, INT size)
{
   UT_ASSERT_EQ_INT(2, port); UT_ASSERT_EQ_PTR(&ut_port.FossilBuffer, fossil);
   UT_ASSERT_NOT_NULL(buffer); UT_ASSERT_EQ_INT(3, size);
   if(ut_block_result > 0) buffer[0] = 0x51;
   return(ut_block_result);
}
WORD utm_OD32FossilStatus(BYTE port)
{
   UT_ASSERT_EQ_INT(2, port); UT_ASSERT(ut_status_calls < 5);
   return(ut_status_results[ut_status_calls++]);
}
BYTE utm_OD32FossilGetByte(BYTE port)
{
   UT_ASSERT_EQ_INT(2, port);
   return((BYTE)(0x52 + ut_get_calls++));
}
#endif

#ifdef INCLUDE_UART_COM
#define UT_CUSTOM_MOCK_UTDisable
#define UT_CUSTOM_MOCK_UTEnable
static BYTE ut_queue[4];
static unsigned ut_disable_calls;
static unsigned ut_enable_calls;
void utm_UTDisable(void) { ++ut_disable_calls; }
void utm_UTEnable(void) { ++ut_enable_calls; }
#endif

#ifdef INCLUDE_WIN32_COM
#define UT_CUSTOM_MOCK_ODComWin32SetReadTimeouts
#define UT_CUSTOM_MOCK_ReadFile
#define UT_CUSTOM_MOCK_ClearCommError
static BOOL ut_read_result;
static DWORD ut_read_count;
static unsigned ut_clear_calls;
tODResult utm_ODComWin32SetReadTimeouts(tPortInfo *port,
   tReadTimeoutState state)
{
   UT_ASSERT_EQ_PTR(&ut_port, port); UT_ASSERT_EQ_INT(kNonBlocking, state);
   return(kODRCSuccess);
}
BOOL WINAPI utm_ReadFile(HANDLE device, LPVOID buffer, DWORD size,
   LPDWORD count, LPOVERLAPPED overlapped)
{
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, device);
   UT_ASSERT_EQ_UINT(3, size); UT_ASSERT_NULL(overlapped);
   ((BYTE *)buffer)[0] = 0x61; *count = ut_read_count;
   return(ut_read_result);
}
BOOL WINAPI utm_ClearCommError(HANDLE device, LPDWORD errors,
   LPCOMSTAT status)
{
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, device);
   UT_ASSERT_NOT_NULL(errors); UT_ASSERT_NULL(status); ++ut_clear_calls;
   return(TRUE);
}
#endif

#ifdef INCLUDE_DOOR32_COM
static DWORD WINAPI ut_door_read(BYTE *buffer, DWORD size)
{
   UT_ASSERT_EQ_UINT(3, size); buffer[0] = 0x62; return(2);
}
#endif

#ifdef INCLUDE_SOCKET_COM
#define UT_CUSTOM_MOCK_recv
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_select
#else
#define UT_CUSTOM_MOCK_poll
#endif
static int ut_ready_result;
static short ut_ready_events;
static int ut_recv_result;
#ifdef ODPLAT_WIN32
int PASCAL utm_select(int count, fd_set FAR *read_set,
   fd_set FAR *write_set, fd_set FAR *error_set,
   const struct timeval FAR *timeout)
{
   UT_ASSERT_EQ_INT(46, count); UT_ASSERT_NOT_NULL(read_set);
   UT_ASSERT_NULL(write_set); UT_ASSERT_NULL(error_set);
   UT_ASSERT_NOT_NULL(timeout); return(ut_ready_result);
}
int PASCAL utm_recv(SOCKET socket_handle, char FAR *buffer,
   int size, int flags)
#else
int utm_poll(struct pollfd *descriptors, nfds_t count, int timeout)
{
   UT_ASSERT_NOT_NULL(descriptors); UT_ASSERT_EQ_UINT(1, count);
   UT_ASSERT_EQ_INT(1, timeout); descriptors[0].revents = ut_ready_events;
   return(ut_ready_result);
}
ssize_t utm_recv(int socket_handle, void *buffer, size_t size, int flags)
#endif
{
   UT_ASSERT_EQ_INT(45, socket_handle); UT_ASSERT_EQ_UINT(3, size);
   UT_ASSERT_EQ_INT(0, flags); ((BYTE *)buffer)[0] = 0x63;
   return(ut_recv_result);
}
#endif

#if defined(INCLUDE_STDIO_COM) || defined(INCLUDE_SOCKET_COM)
#define UT_CUSTOM_MOCK_ODComGetByte
static tODResult ut_byte_results[4];
static unsigned ut_byte_calls;
tODResult utm_ODComGetByte(tPortHandle port, char *value, BOOL wait)
{
   tODResult result;
   UT_ASSERT_EQ_PTR(&ut_port, ODHANDLE2PTR(port, tPortInfo));
   UT_ASSERT(!wait); UT_ASSERT(ut_byte_calls < 4);
   result = ut_byte_results[ut_byte_calls];
   if(result == kODRCSuccess) *value = (char)(0x70 + ut_byte_calls);
   ++ut_byte_calls; return(result);
}
#endif

static void reset_buffer(void)
{
   unsigned index;
   memset(&ut_port, 0, sizeof(ut_port));
   ut_port.bIsOpen = TRUE; ut_port.btPort = 2;
#ifdef ODPLAT_DOS32
   ut_block_result = 1; ut_status_calls = ut_get_calls = 0;
   for(index = 0; index < 5; ++index) ut_status_results[index] = 0;
#endif
#ifdef INCLUDE_UART_COM
   pbtRXQueue = ut_queue; nRXQueueSize = 4; nRXOutIndex = 1; nRXChars = 3;
   ut_queue[0] = 0x40; ut_queue[1] = 0x41;
   ut_queue[2] = 0x42; ut_queue[3] = 0x43;
   ut_disable_calls = ut_enable_calls = 0;
#endif
#ifdef INCLUDE_WIN32_COM
   ut_port.hCommDev = (HANDLE)(DWORD_PTR)41;
   ut_read_result = TRUE; ut_read_count = 2; ut_clear_calls = 0;
#endif
#ifdef INCLUDE_DOOR32_COM
   ut_port.pfDoorRead = ut_door_read;
#endif
#ifdef INCLUDE_SOCKET_COM
   ut_port.socket = 45; ut_ready_result = 1; ut_ready_events = POLLIN;
   ut_recv_result = 2;
#endif
#if defined(INCLUDE_STDIO_COM) || defined(INCLUDE_SOCKET_COM)
   ut_byte_calls = 0;
   for(index = 0; index < 4; ++index) ut_byte_results[index] = kODRCSuccess;
#endif
}

#ifdef INCLUDE_FOSSIL_COM
static void receives_fossil_blocks(void)
{
   BYTE buffer[3] = {0}; int count = -1;
   reset_buffer(); ut_port.Method = kComMethodFOSSIL;
#if defined(__TURBOC__) || (defined(__WATCOMC__) && !defined(ODPLAT_DOS32))
   ut_fossil_clear(); ut_fossil_queue(0x51); ut_fossil_queue(0x52);
#endif
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetBuffer(
      ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count));
#ifdef ODPLAT_DOS32
   UT_ASSERT_EQ_INT(1, ut_block_result);
   UT_ASSERT_EQ_INT(ut_block_result, count);
   UT_ASSERT_EQ_INT(0x51, buffer[0]);
   reset_buffer(); ut_port.Method = kComMethodFOSSIL; ut_block_result = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetBuffer(
      ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count));
   UT_ASSERT_EQ_INT(0, count);
   reset_buffer(); ut_port.Method = kComMethodFOSSIL; ut_block_result = -1;
   ut_status_results[0] = 0x100; ut_status_results[1] = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetBuffer(
      ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count));
   UT_ASSERT_EQ_INT(1, count); UT_ASSERT_EQ_INT(0x52, buffer[0]);
   reset_buffer(); ut_port.Method = kComMethodFOSSIL; ut_block_result = 0;
   ut_status_results[0] = ut_status_results[1] = ut_status_results[2] =
      ut_status_results[3] = 0x100;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetBuffer(
      ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count));
   UT_ASSERT_EQ_INT(3, count);
#else
   UT_ASSERT_EQ_INT(2, count); UT_ASSERT_EQ_INT(0x51, buffer[0]);
#endif
}
#endif

#ifdef INCLUDE_UART_COM
static void receives_uart_queue_halves_and_empty_ranges(void)
{
   BYTE buffer[3] = {0}; int count;
   reset_buffer(); ut_port.Method = kComMethodUART; nRXOutIndex = 3;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetBuffer(
      ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count));
   UT_ASSERT_EQ_INT(3, count); UT_ASSERT_EQ_INT(0x43, buffer[0]);
   UT_ASSERT_EQ_INT(0x40, buffer[1]); UT_ASSERT_EQ_INT(2, nRXOutIndex);
   reset_buffer(); ut_port.Method = kComMethodUART; nRXOutIndex = 1;
   nRXChars = 3;
   utt_ODComGetBuffer(ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count);
   UT_ASSERT_EQ_INT(0, nRXOutIndex);
   reset_buffer(); ut_port.Method = kComMethodUART; nRXChars = 1;
   utt_ODComGetBuffer(ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count);
   UT_ASSERT_EQ_INT(2, nRXOutIndex);
   reset_buffer(); ut_port.Method = kComMethodUART; nRXChars = 0;
   utt_ODComGetBuffer(ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count);
   UT_ASSERT_EQ_INT(0, count);
}
#endif

#ifdef INCLUDE_WIN32_COM
static void reports_win32_read_results(void)
{
   BYTE buffer[3]; int count = -1;
   reset_buffer(); ut_port.Method = kComMethodWin32;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetBuffer(
      ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count));
   UT_ASSERT_EQ_INT(2, count);
   reset_buffer(); ut_port.Method = kComMethodWin32; ut_read_result = FALSE;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure, utt_ODComGetBuffer(
      ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count));
   UT_ASSERT_EQ_UINT(1, ut_clear_calls);
}
#endif

#ifdef INCLUDE_DOOR32_COM
static void reports_door32_read_count(void)
{
   BYTE buffer[3]; int count = -1;
   reset_buffer(); ut_port.Method = kComMethodDoor32;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetBuffer(
      ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count));
   UT_ASSERT_EQ_INT(2, count); UT_ASSERT_EQ_INT(0x62, buffer[0]);
}
#endif

#ifdef INCLUDE_SOCKET_COM
static void reports_socket_readiness_and_receive_count(void)
{
   BYTE buffer[3]; int count = -1;
   reset_buffer(); ut_port.Method = kComMethodSocket; ut_ready_result = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetBuffer(
      ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count));
   UT_ASSERT_EQ_INT(0, count);
#ifndef ODPLAT_WIN32
   reset_buffer(); ut_port.Method = kComMethodSocket; ut_ready_result = 1;
   ut_ready_events = POLLHUP;
   utt_ODComGetBuffer(ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count);
   UT_ASSERT_EQ_INT(0, count);
   reset_buffer(); ut_port.Method = kComMethodSocket; ut_ready_result = -1;
   utt_ODComGetBuffer(ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count);
   UT_ASSERT_EQ_INT(0, count);
#endif
   reset_buffer(); ut_port.Method = kComMethodSocket;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetBuffer(
      ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count));
   UT_ASSERT_EQ_INT(2, count); UT_ASSERT_EQ_INT(0x63, buffer[0]);
}

static void receives_telnet_data_through_the_byte_decoder(void)
{
   BYTE buffer[3] = {0}; int count = -1;
   reset_buffer(); ut_port.Method = kComMethodSocket;
   ut_port.bTelnetSocket = TRUE; ut_byte_results[2] = kODRCNothingWaiting;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetBuffer(
      ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count));
   UT_ASSERT_EQ_INT(2, count); UT_ASSERT_EQ_UINT(3, ut_byte_calls);
   UT_ASSERT_EQ_INT(0x70, buffer[0]); UT_ASSERT_EQ_INT(0x71, buffer[1]);

   reset_buffer(); ut_port.Method = kComMethodSocket;
   ut_port.bTelnetSocket = TRUE;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetBuffer(
      ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count));
   UT_ASSERT_EQ_INT(3, count); UT_ASSERT_EQ_UINT(3, ut_byte_calls);
}
#endif

#ifdef INCLUDE_STDIO_COM
static void receives_stdio_bytes_until_full_or_unavailable(void)
{
   BYTE buffer[3]; int count = -1;
   reset_buffer(); ut_port.Method = kComMethodStdIO;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetBuffer(
      ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count));
   UT_ASSERT_EQ_INT(3, count);
   reset_buffer(); ut_port.Method = kComMethodStdIO;
   ut_byte_results[1] = kODRCNothingWaiting;
   utt_ODComGetBuffer(ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count);
   UT_ASSERT_EQ_INT(1, count);
}
#endif

static void accepts_an_unknown_method(void)
{
   BYTE buffer[3]; int count = 9;
   reset_buffer(); ut_port.Method = (tComMethod)99;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetBuffer(
      ODPTR2HANDLE(&ut_port, tPortInfo), buffer, 3, &count));
   UT_ASSERT_EQ_INT(9, count);
}

static const UTTestCase ut_cases[] = {
#ifdef INCLUDE_FOSSIL_COM
   {"FOSSIL", receives_fossil_blocks},
#endif
#ifdef INCLUDE_UART_COM
   {"UART", receives_uart_queue_halves_and_empty_ranges},
#endif
#ifdef INCLUDE_WIN32_COM
   {"Win32", reports_win32_read_results},
#endif
#ifdef INCLUDE_DOOR32_COM
   {"Door32", reports_door32_read_count},
#endif
#ifdef INCLUDE_SOCKET_COM
   {"socket", reports_socket_readiness_and_receive_count},
   {"Telnet socket", receives_telnet_data_through_the_byte_decoder},
#endif
#ifdef INCLUDE_STDIO_COM
   {"stdio", receives_stdio_bytes_until_full_or_unavailable},
#endif
   {"unknown", accepts_an_unknown_method}
};
