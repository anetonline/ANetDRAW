#ifdef UT_CLEAR_OUTBOUND
#define UT_CALL utt_ODComClearOutbound
#define UT_UART_RESET ODComInternalResetTX
#define UT_CUSTOM_MOCK_ODComInternalResetTX
#ifdef ODPLAT_DOS32
#define UT_CUSTOM_MOCK_OD32FossilClearOutbound
#endif
#ifdef INCLUDE_WIN32_COM
#define UT_PURGE_FLAG PURGE_TXCLEAR
#endif
#else
#define UT_CALL utt_ODComClearInbound
#define UT_UART_RESET ODComInternalResetRX
#define UT_CUSTOM_MOCK_ODComInternalResetRX
#ifdef ODPLAT_DOS32
#define UT_CUSTOM_MOCK_OD32FossilClearInbound
#endif
#ifdef INCLUDE_WIN32_COM
#define UT_PURGE_FLAG PURGE_RXCLEAR
#endif
#endif

static unsigned ut_fossil_calls;
static unsigned ut_uart_calls;

#ifdef ODPLAT_DOS32
#ifdef UT_CLEAR_OUTBOUND
void utm_OD32FossilClearOutbound(BYTE port)
#else
void utm_OD32FossilClearInbound(BYTE port)
#endif
{
   ++ut_fossil_calls;
   UT_ASSERT_EQ_INT(2, port);
}
#endif

#ifdef INCLUDE_UART_COM
#ifdef UT_CLEAR_OUTBOUND
void utm_ODComInternalResetTX(void)
#else
void utm_ODComInternalResetRX(void)
#endif
{
   ++ut_uart_calls;
}
#endif

#ifdef INCLUDE_WIN32_COM
#define UT_CUSTOM_MOCK_PurgeComm
static BOOL ut_purge_succeeds;
static unsigned ut_purge_calls;

BOOL WINAPI utm_PurgeComm(HANDLE device, DWORD flags)
{
   ++ut_purge_calls;
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, device);
   UT_ASSERT_EQ_UINT(UT_PURGE_FLAG, flags);
   return(ut_purge_succeeds);
}
#endif

static tPortInfo ut_port;

static void reset_clear(void)
{
   memset(&ut_port, 0, sizeof(ut_port));
   ut_port.bIsOpen = TRUE;
   ut_port.btPort = 2;
   ut_fossil_calls = 0;
   ut_uart_calls = 0;
#ifdef INCLUDE_WIN32_COM
   ut_port.hCommDev = (HANDLE)(DWORD_PTR)41;
   ut_purge_succeeds = TRUE;
   ut_purge_calls = 0;
#endif
}

#ifdef INCLUDE_FOSSIL_COM
static void clears_the_fossil_queue(void)
{
   reset_clear();
   ut_port.Method = kComMethodFOSSIL;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      UT_CALL(ODPTR2HANDLE(&ut_port, tPortInfo)));
#ifdef ODPLAT_DOS32
   UT_ASSERT_EQ_UINT(1, ut_fossil_calls);
#endif
}
#endif

#ifdef INCLUDE_UART_COM
static void clears_the_uart_queue(void)
{
   reset_clear();
   ut_port.Method = kComMethodUART;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      UT_CALL(ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_UINT(1, ut_uart_calls);
}
#endif

#ifdef INCLUDE_WIN32_COM
static void purges_win32_and_reports_failure(void)
{
   reset_clear();
   ut_port.Method = kComMethodWin32;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      UT_CALL(ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_UINT(1, ut_purge_calls);
   ut_purge_succeeds = FALSE;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      UT_CALL(ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_UINT(2, ut_purge_calls);
}
#endif

#ifdef INCLUDE_DOOR32_COM
static void rejects_door32(void)
{
   reset_clear();
   ut_port.Method = kComMethodDoor32;
   UT_ASSERT_EQ_INT(kODRCUnsupported,
      UT_CALL(ODPTR2HANDLE(&ut_port, tPortInfo)));
}
#endif

#ifdef INCLUDE_SOCKET_COM
static void rejects_sockets(void)
{
   reset_clear();
   ut_port.Method = kComMethodSocket;
   UT_ASSERT_EQ_INT(kODRCUnsupported,
      UT_CALL(ODPTR2HANDLE(&ut_port, tPortInfo)));
}
#endif

#ifdef INCLUDE_STDIO_COM
static void rejects_stdio(void)
{
   reset_clear();
   ut_port.Method = kComMethodStdIO;
   UT_ASSERT_EQ_INT(kODRCUnsupported,
      UT_CALL(ODPTR2HANDLE(&ut_port, tPortInfo)));
}
#endif

static void accepts_an_unknown_method_without_an_operation(void)
{
   reset_clear();
   ut_port.Method = (tComMethod)99;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      UT_CALL(ODPTR2HANDLE(&ut_port, tPortInfo)));
}

static const UTTestCase ut_cases[] = {
#ifdef INCLUDE_FOSSIL_COM
   {"FOSSIL", clears_the_fossil_queue},
#endif
#ifdef INCLUDE_UART_COM
   {"UART", clears_the_uart_queue},
#endif
#ifdef INCLUDE_WIN32_COM
   {"Win32", purges_win32_and_reports_failure},
#endif
#ifdef INCLUDE_DOOR32_COM
   {"Door32", rejects_door32},
#endif
#ifdef INCLUDE_SOCKET_COM
   {"socket", rejects_sockets},
#endif
#ifdef INCLUDE_STDIO_COM
   {"stdio", rejects_stdio},
#endif
   {"unknown", accepts_an_unknown_method_without_an_operation}
};
