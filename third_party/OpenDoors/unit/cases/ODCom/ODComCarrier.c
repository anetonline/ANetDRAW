#define UT_CUSTOM_MOCK_GetCommModemStatus
#define UT_CUSTOM_MOCK_WaitForSingleObject
#define UT_CUSTOM_MOCK_select

#ifdef INCLUDE_UART_COM
#define UT_CUSTOM_MOCK_UTInp
static unsigned ut_uart_value;

#ifdef __TURBOC__
int
#else
unsigned
#endif
utm_UTInp(unsigned port)
{
   UT_ASSERT_EQ_UINT((unsigned)nModemStatusRegAddr, port);
   return(ut_uart_value);
}
#endif

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
static BOOL ut_modem_status_succeeds;
static DWORD ut_modem_status;

BOOL WINAPI utm_GetCommModemStatus(HANDLE device, LPDWORD status)
{
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, device);
   UT_ASSERT_NOT_NULL(status);
   *status = ut_modem_status;
   return(ut_modem_status_succeeds);
}
#endif

#ifdef INCLUDE_DOOR32_COM
static DWORD ut_wait_result;
static HANDLE WINAPI ut_offline_handle(void)
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

#ifdef INCLUDE_SOCKET_COM
#define UT_CUSTOM_MOCK_recv
static int ut_recv_result;

#ifdef ODPLAT_WIN32
static int ut_select_result;

int PASCAL utm_select(int count, fd_set FAR *read_set,
   fd_set FAR *write_set, fd_set FAR *error_set,
   const struct timeval FAR *timeout)
{
   UT_ASSERT_EQ_INT(46, count);
   UT_ASSERT_NOT_NULL(read_set);
   UT_ASSERT_EQ_UINT(1, read_set->fd_count);
   UT_ASSERT_EQ_INT(45, read_set->fd_array[0]);
   UT_ASSERT_NULL(write_set);
   UT_ASSERT_NULL(error_set);
   UT_ASSERT_NOT_NULL(timeout);
   UT_ASSERT_EQ_INT(0, timeout->tv_sec);
   UT_ASSERT_EQ_INT(0, timeout->tv_usec);
   return(ut_select_result);
}

int PASCAL utm_recv(SOCKET socket_handle, char FAR *buffer, int size,
   int flags)
#else
#define UT_CUSTOM_MOCK_poll
static int ut_poll_result;
static short ut_poll_events;

int utm_poll(struct pollfd *descriptors, nfds_t count, int timeout)
{
   UT_ASSERT_NOT_NULL(descriptors);
   UT_ASSERT_EQ_UINT(1, count);
   UT_ASSERT_EQ_INT(0, timeout);
   UT_ASSERT_EQ_INT(45, descriptors[0].fd);
   UT_ASSERT_EQ_INT(POLLIN | POLLHUP, descriptors[0].events);
   descriptors[0].revents = ut_poll_events;
   return(ut_poll_result);
}

ssize_t utm_recv(int socket_handle, void *buffer, size_t size, int flags)
#endif
{
   UT_ASSERT_EQ_INT(45, socket_handle);
   UT_ASSERT_NOT_NULL(buffer);
   UT_ASSERT_EQ_UINT(1, size);
   UT_ASSERT_EQ_INT(MSG_PEEK, flags);
   return(ut_recv_result);
}
#endif

#ifdef INCLUDE_STDIO_COM
#define UT_CUSTOM_MOCK_sigpending
#define UT_CUSTOM_MOCK_sigismember
static int ut_hangup_pending;

int utm_sigpending(sigset_t *set)
{
   UT_ASSERT_NOT_NULL(set);
   return(0);
}

int utm_sigismember(const sigset_t *set, int signal_number)
{
   UT_ASSERT_NOT_NULL(set);
   UT_ASSERT_EQ_INT(SIGHUP, signal_number);
   return(ut_hangup_pending);
}
#endif

static tPortInfo ut_port;

static void reset_carrier(void)
{
   memset(&ut_port, 0, sizeof(ut_port));
   ut_port.bIsOpen = TRUE;
#ifdef INCLUDE_UART_COM
   nModemStatusRegAddr = 0x3fe;
   ut_uart_value = 0;
#endif
#ifdef ODPLAT_DOS32
   ut_fossil_status = 0;
#endif
#ifdef INCLUDE_WIN32_COM
   ut_port.hCommDev = (HANDLE)(DWORD_PTR)41;
   ut_modem_status_succeeds = TRUE;
   ut_modem_status = 0;
#endif
#ifdef INCLUDE_DOOR32_COM
   ut_port.pfDoorGetOfflineEventHandle = ut_offline_handle;
   ut_wait_result = WAIT_TIMEOUT;
#endif
#ifdef INCLUDE_SOCKET_COM
   ut_port.socket = 45;
   ut_recv_result = 0;
# ifdef ODPLAT_WIN32
   ut_select_result = 0;
# else
   ut_poll_result = 0;
   ut_poll_events = 0;
# endif
#endif
#ifdef INCLUDE_STDIO_COM
   ut_hangup_pending = 0;
#endif
}

#ifdef INCLUDE_FOSSIL_COM
static void reports_fossil_carrier_state(void)
{
   BOOL carrier = FALSE;
   reset_carrier();
   ut_port.Method = kComMethodFOSSIL;
   ut_port.btPort = 2;
#ifdef ODPLAT_DOS32
   ut_fossil_status = 0x80;
#endif
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
#ifdef ODPLAT_DOS32
   UT_ASSERT(carrier != FALSE);
#else
   (void)carrier;
#endif
}
#endif

#ifdef INCLUDE_UART_COM
static void reports_uart_carrier_state(void)
{
   BOOL carrier;
   reset_carrier();
   ut_port.Method = kComMethodUART;
   ut_uart_value = RLSD;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
   UT_ASSERT(carrier != FALSE);
   ut_uart_value = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
   UT_ASSERT_EQ_INT(FALSE, carrier);
}
#endif

#ifdef INCLUDE_WIN32_COM
static void reports_win32_modem_status_and_failure(void)
{
   BOOL carrier;
   reset_carrier();
   ut_port.Method = kComMethodWin32;
   ut_modem_status_succeeds = FALSE;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
   ut_modem_status_succeeds = TRUE;
   ut_modem_status = MS_RLSD_ON;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
   UT_ASSERT_EQ_INT(TRUE, carrier);
   ut_modem_status = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
   UT_ASSERT_EQ_INT(FALSE, carrier);
}
#endif

#ifdef INCLUDE_DOOR32_COM
static void reports_door32_offline_event_state(void)
{
   BOOL carrier;
   reset_carrier();
   ut_port.Method = kComMethodDoor32;
   ut_wait_result = WAIT_TIMEOUT;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
   UT_ASSERT_EQ_INT(TRUE, carrier);
   ut_wait_result = WAIT_OBJECT_0;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
   UT_ASSERT_EQ_INT(FALSE, carrier);
}
#endif

#ifdef INCLUDE_SOCKET_COM
static void reports_socket_carrier_state(void)
{
   BOOL carrier;
   reset_carrier();
   ut_port.Method = kComMethodSocket;
#ifdef ODPLAT_WIN32
   ut_select_result = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
   UT_ASSERT_EQ_INT(TRUE, carrier);
   ut_select_result = 1;
   ut_recv_result = 1;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
   UT_ASSERT_EQ_INT(TRUE, carrier);
   ut_recv_result = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
   UT_ASSERT_EQ_INT(FALSE, carrier);
   ut_select_result = 2;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
   UT_ASSERT_EQ_INT(FALSE, carrier);
#else
   ut_poll_result = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
   UT_ASSERT_EQ_INT(TRUE, carrier);
   ut_poll_result = -1;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
   UT_ASSERT_EQ_INT(FALSE, carrier);
   ut_poll_result = 1;
   ut_poll_events = POLLHUP;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
   UT_ASSERT_EQ_INT(FALSE, carrier);
   ut_poll_events = POLLIN;
   ut_recv_result = 1;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
   UT_ASSERT_EQ_INT(TRUE, carrier);
   ut_recv_result = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
   UT_ASSERT_EQ_INT(FALSE, carrier);
#endif
}
#endif

#ifdef INCLUDE_STDIO_COM
static void reports_stdio_hangup_state(void)
{
   BOOL carrier;
   reset_carrier();
   ut_port.Method = kComMethodStdIO;
   ut_hangup_pending = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
   UT_ASSERT_EQ_INT(TRUE, carrier);
   ut_hangup_pending = 1;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
   UT_ASSERT_EQ_INT(FALSE, carrier);
}
#endif

static void leaves_output_untouched_for_an_unknown_method(void)
{
   BOOL carrier = 123;
   reset_carrier();
   ut_port.Method = (tComMethod)99;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComCarrier(ODPTR2HANDLE(&ut_port, tPortInfo), &carrier));
   UT_ASSERT_EQ_INT(123, carrier);
}

static const UTTestCase ut_cases[] = {
#ifdef INCLUDE_FOSSIL_COM
   {"FOSSIL", reports_fossil_carrier_state},
#endif
#ifdef INCLUDE_UART_COM
   {"UART", reports_uart_carrier_state},
#endif
#ifdef INCLUDE_WIN32_COM
   {"Win32", reports_win32_modem_status_and_failure},
#endif
#ifdef INCLUDE_DOOR32_COM
   {"Door32", reports_door32_offline_event_state},
#endif
#ifdef INCLUDE_SOCKET_COM
   {"socket", reports_socket_carrier_state},
#endif
#ifdef INCLUDE_STDIO_COM
   {"stdio", reports_stdio_hangup_state},
#endif
   {"unknown", leaves_output_untouched_for_an_unknown_method}
};
