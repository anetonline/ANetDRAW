#define UT_CUSTOM_MOCK_EscapeCommFunction

#if defined(__WATCOMC__) && !defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK_int86
static unsigned ut_int86_calls;
static BOOL ut_int86_high;

int utm_int86(int interrupt_number, const union REGS *input,
   union REGS *output)
{
   ++ut_int86_calls;
   UT_ASSERT_EQ_INT(0x14, interrupt_number);
   UT_ASSERT_NOT_NULL(input);
   UT_ASSERT_EQ_INT(6, input->h.ah);
   UT_ASSERT_EQ_INT(2, input->x.dx);
   ut_int86_high = input->h.al;
   if(output != NULL)
      *output = *input;
   return(0);
}
#endif

#ifdef ODPLAT_DOS32
#define UT_CUSTOM_MOCK_OD32FossilSetDTR
static unsigned ut_fossil_calls;
static BOOL ut_fossil_high;

void utm_OD32FossilSetDTR(BYTE port, BOOL high)
{
   ++ut_fossil_calls;
   UT_ASSERT_EQ_INT(2, port);
   ut_fossil_high = high;
}
#endif

#ifdef INCLUDE_UART_COM
#define UT_CUSTOM_MOCK_UTInp
#define UT_CUSTOM_MOCK_UTOutp
#define UT_CUSTOM_MOCK_UTDisable
#define UT_CUSTOM_MOCK_UTEnable
static unsigned ut_port_value;
static unsigned ut_write_port;
static unsigned ut_write_value;
static unsigned ut_write_calls;
static unsigned ut_disable_calls;
static unsigned ut_enable_calls;

#ifdef __TURBOC__
int
#else
unsigned
#endif
utm_UTInp(unsigned port)
{
   UT_ASSERT_EQ_UINT((unsigned)nModemCtrlRegAddr, port);
   return(ut_port_value);
}

#ifdef __TURBOC__
int
#else
unsigned
#endif
utm_UTOutp(unsigned port, unsigned value)
{
   ++ut_write_calls;
   ut_write_port = port;
   ut_write_value = value;
   return(value);
}

void utm_UTDisable(void) { ++ut_disable_calls; }
void utm_UTEnable(void) { ++ut_enable_calls; }
#endif

#ifdef INCLUDE_WIN32_COM
static BOOL ut_escape_succeeds;
static DWORD ut_escape_operation;

BOOL WINAPI utm_EscapeCommFunction(HANDLE device, DWORD operation)
{
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, device);
   ut_escape_operation = operation;
   return(ut_escape_succeeds);
}
#endif

#ifdef INCLUDE_SOCKET_COM
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_closesocket
#else
#define UT_CUSTOM_MOCK_UTCloseSocket
#endif
static unsigned ut_close_calls;

#ifdef ODPLAT_WIN32
int PASCAL utm_closesocket(SOCKET socket_handle)
#else
int utm_UTCloseSocket(int socket_handle)
#endif
{
   ++ut_close_calls;
   UT_ASSERT_EQ_INT(45, socket_handle);
   return(0);
}
#endif

static tPortInfo ut_port;

static void reset_dtr(void)
{
   memset(&ut_port, 0, sizeof(ut_port));
   ut_port.bIsOpen = TRUE;
   ut_port.btPort = 2;
#if defined(__WATCOMC__) && !defined(ODPLAT_DOS32)
   ut_int86_calls = 0;
   ut_int86_high = FALSE;
#endif
#ifdef ODPLAT_DOS32
   ut_fossil_calls = 0;
   ut_fossil_high = FALSE;
#endif
#ifdef INCLUDE_UART_COM
   nModemCtrlRegAddr = 0x3fc;
   ut_port_value = 0xa0;
   ut_write_port = ut_write_value = ut_write_calls = 0;
   ut_disable_calls = ut_enable_calls = 0;
#endif
#ifdef INCLUDE_WIN32_COM
   ut_port.hCommDev = (HANDLE)(DWORD_PTR)41;
   ut_escape_succeeds = TRUE;
   ut_escape_operation = 0;
#endif
#ifdef INCLUDE_SOCKET_COM
   ut_port.socket = 45;
   ut_close_calls = 0;
#endif
}

#ifdef INCLUDE_FOSSIL_COM
static void raises_and_lowers_fossil_dtr(void)
{
   reset_dtr();
   ut_port.Method = kComMethodFOSSIL;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSetDTR(ODPTR2HANDLE(&ut_port, tPortInfo), TRUE));
#ifdef ODPLAT_DOS32
   UT_ASSERT_EQ_UINT(1, ut_fossil_calls);
   UT_ASSERT(ut_fossil_high != FALSE);
#else
#ifdef __WATCOMC__
   UT_ASSERT_EQ_UINT(1, ut_int86_calls);
   UT_ASSERT(ut_int86_high != FALSE);
#endif
#endif
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSetDTR(ODPTR2HANDLE(&ut_port, tPortInfo), FALSE));
#ifdef ODPLAT_DOS32
   UT_ASSERT_EQ_UINT(2, ut_fossil_calls);
   UT_ASSERT_EQ_INT(FALSE, ut_fossil_high);
#else
#ifdef __WATCOMC__
   UT_ASSERT_EQ_UINT(2, ut_int86_calls);
   UT_ASSERT_EQ_INT(FALSE, ut_int86_high);
#endif
#endif
}
#endif

#ifdef INCLUDE_UART_COM
static void raises_and_lowers_uart_dtr_atomically(void)
{
   reset_dtr();
   ut_port.Method = kComMethodUART;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSetDTR(ODPTR2HANDLE(&ut_port, tPortInfo), TRUE));
   UT_ASSERT_EQ_UINT(1, ut_write_calls);
   UT_ASSERT_EQ_UINT(0x3fc, ut_write_port);
   UT_ASSERT_EQ_UINT(0xa0 | DTR, ut_write_value);
#ifdef ODPLAT_DOS32
   UT_ASSERT_EQ_UINT(1, ut_disable_calls);
   UT_ASSERT_EQ_UINT(1, ut_enable_calls);
#endif

   ut_port_value = 0xa1;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSetDTR(ODPTR2HANDLE(&ut_port, tPortInfo), FALSE));
   UT_ASSERT_EQ_UINT(2, ut_write_calls);
   UT_ASSERT_EQ_UINT(0xa1 & NOT_DTR, ut_write_value);
#ifdef ODPLAT_DOS32
   UT_ASSERT_EQ_UINT(2, ut_disable_calls);
   UT_ASSERT_EQ_UINT(2, ut_enable_calls);
#endif
}
#endif

#ifdef INCLUDE_WIN32_COM
static void controls_win32_dtr_and_reports_failure(void)
{
   reset_dtr();
   ut_port.Method = kComMethodWin32;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSetDTR(ODPTR2HANDLE(&ut_port, tPortInfo), TRUE));
   UT_ASSERT_EQ_UINT(SETDTR, ut_escape_operation);
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSetDTR(ODPTR2HANDLE(&ut_port, tPortInfo), FALSE));
   UT_ASSERT_EQ_UINT(CLRDTR, ut_escape_operation);
   ut_escape_succeeds = FALSE;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComSetDTR(ODPTR2HANDLE(&ut_port, tPortInfo), TRUE));
}
#endif

#ifdef INCLUDE_DOOR32_COM
static void reports_door32_as_unsupported(void)
{
   reset_dtr();
   ut_port.Method = kComMethodDoor32;
   UT_ASSERT_EQ_INT(kODRCUnsupported,
      utt_ODComSetDTR(ODPTR2HANDLE(&ut_port, tPortInfo), TRUE));
}
#endif

#ifdef INCLUDE_SOCKET_COM
static void closes_a_lowered_socket_and_rejects_raise(void)
{
   reset_dtr();
   ut_port.Method = kComMethodSocket;
   UT_ASSERT_EQ_INT(kODRCUnsupported,
      utt_ODComSetDTR(ODPTR2HANDLE(&ut_port, tPortInfo), TRUE));
   UT_ASSERT_EQ_UINT(0, ut_close_calls);
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSetDTR(ODPTR2HANDLE(&ut_port, tPortInfo), FALSE));
   UT_ASSERT_EQ_UINT(1, ut_close_calls);
}
#endif

#ifdef INCLUDE_STDIO_COM
static void reports_stdio_as_unsupported(void)
{
   reset_dtr();
   ut_port.Method = kComMethodStdIO;
   UT_ASSERT_EQ_INT(kODRCUnsupported,
      utt_ODComSetDTR(ODPTR2HANDLE(&ut_port, tPortInfo), TRUE));
}
#endif

static void accepts_an_unknown_method_without_an_operation(void)
{
   reset_dtr();
   ut_port.Method = (tComMethod)99;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSetDTR(ODPTR2HANDLE(&ut_port, tPortInfo), TRUE));
}

static const UTTestCase ut_cases[] = {
#ifdef INCLUDE_FOSSIL_COM
   {"FOSSIL", raises_and_lowers_fossil_dtr},
#endif
#ifdef INCLUDE_UART_COM
   {"UART", raises_and_lowers_uart_dtr_atomically},
#endif
#ifdef INCLUDE_WIN32_COM
   {"Win32", controls_win32_dtr_and_reports_failure},
#endif
#ifdef INCLUDE_DOOR32_COM
   {"Door32", reports_door32_as_unsupported},
#endif
#ifdef INCLUDE_SOCKET_COM
   {"socket", closes_a_lowered_socket_and_rejects_raise},
#endif
#ifdef INCLUDE_STDIO_COM
   {"stdio", reports_stdio_as_unsupported},
#endif
   {"unknown", accepts_an_unknown_method_without_an_operation}
};
