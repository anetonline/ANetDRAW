#if defined(__WATCOMC__) && !defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK_int86
static BOOL ut_fossil_empty;

int utm_int86(int interrupt_number, const union REGS *input,
   union REGS *output)
{
   UT_ASSERT_EQ_INT(0x14, interrupt_number);
   UT_ASSERT_NOT_NULL(input);
   UT_ASSERT_NOT_NULL(output);
   UT_ASSERT_EQ_INT(3, input->h.ah);
   UT_ASSERT_EQ_INT(2, input->x.dx);
   *output = *input;
   output->h.ah = ut_fossil_empty ? 0x40 : 0;
   return(0);
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
#define UT_CUSTOM_MOCK_ClearCommError
static BOOL ut_status_succeeds;
static DWORD ut_outbound_count;

BOOL WINAPI utm_ClearCommError(HANDLE device, LPDWORD errors,
   LPCOMSTAT status)
{
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, device);
   UT_ASSERT_NOT_NULL(errors);
   UT_ASSERT_NOT_NULL(status);
   *errors = 0x1234;
   status->cbOutQue = ut_outbound_count;
   return(ut_status_succeeds);
}
#endif

#ifdef INCLUDE_UART_COM
#define UT_CUSTOM_MOCK_UTInp
static unsigned ut_uart_status;
static unsigned ut_uart_status_calls;

#ifdef __TURBOC__
int
#else
unsigned
#endif
utm_UTInp(unsigned port)
{
   UT_ASSERT_EQ_UINT((unsigned)nLineStatusRegAddr, port);
   ++ut_uart_status_calls;
   return(ut_uart_status);
}
#endif

static tPortInfo ut_port;

static void reset_outbound(void)
{
   memset(&ut_port, 0, sizeof(ut_port));
   ut_port.bIsOpen = TRUE;
   ut_port.btPort = 2;
#if defined(__WATCOMC__) && !defined(ODPLAT_DOS32)
   ut_fossil_empty = FALSE;
#endif
#ifdef ODPLAT_DOS32
   ut_fossil_status = 0;
#endif
#ifdef INCLUDE_WIN32_COM
   ut_port.hCommDev = (HANDLE)(DWORD_PTR)41;
   ut_status_succeeds = TRUE;
   ut_outbound_count = 0;
#endif
#ifdef INCLUDE_UART_COM
   nLineStatusRegAddr = 0x3fd;
   ut_uart_status = 0;
   ut_uart_status_calls = 0;
#endif
}

#ifdef INCLUDE_FOSSIL_COM
static void reports_fossil_empty_or_nonempty_state(void)
{
   int waiting = 99;
   reset_outbound();
   ut_port.Method = kComMethodFOSSIL;
#ifdef ODPLAT_DOS32
   ut_fossil_status = 0x4000;
#endif
#if defined(__WATCOMC__) && !defined(ODPLAT_DOS32)
   ut_fossil_empty = TRUE;
#endif
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComOutbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
#ifdef __TURBOC__
   UT_ASSERT(waiting == 0 || waiting == SIZE_NON_ZERO);
#else
   UT_ASSERT_EQ_INT(0, waiting);
#endif

#ifdef ODPLAT_DOS32
   ut_fossil_status = 0;
#endif
#if defined(__WATCOMC__) && !defined(ODPLAT_DOS32)
   ut_fossil_empty = FALSE;
#endif
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComOutbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
#ifdef __TURBOC__
   UT_ASSERT(waiting == 0 || waiting == SIZE_NON_ZERO);
#else
   UT_ASSERT_EQ_INT(SIZE_NON_ZERO, waiting);
#endif
}
#endif

#ifdef INCLUDE_UART_COM
static void reports_the_uart_software_and_hardware_state(void)
{
   int waiting;
   reset_outbound();
   ut_port.Method = kComMethodUART;
   nTXChars = 17;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComOutbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(17, waiting);
   UT_ASSERT_EQ_UINT(0, ut_uart_status_calls);

   nTXChars = 0;
   ut_uart_status = 0x40;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComOutbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(0, waiting);
   UT_ASSERT_EQ_UINT(1, ut_uart_status_calls);

   ut_uart_status = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComOutbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(SIZE_NON_ZERO, waiting);
   UT_ASSERT_EQ_UINT(2, ut_uart_status_calls);
}
#endif

#ifdef INCLUDE_WIN32_COM
static void reports_the_win32_queue_count_and_failure(void)
{
   int waiting = 99;
   reset_outbound();
   ut_port.Method = kComMethodWin32;
   ut_status_succeeds = FALSE;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComOutbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(99, waiting);
   ut_status_succeeds = TRUE;
   ut_outbound_count = 123;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComOutbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(123, waiting);
}
#endif

#ifdef INCLUDE_DOOR32_COM
static void reports_door32_as_unsupported_and_empty(void)
{
   int waiting = 99;
   reset_outbound();
   ut_port.Method = kComMethodDoor32;
   UT_ASSERT_EQ_INT(kODRCUnsupported,
      utt_ODComOutbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(0, waiting);
}
#endif

#ifdef INCLUDE_SOCKET_COM
static void reports_sockets_as_unsupported_and_empty(void)
{
   int waiting = 99;
   reset_outbound();
   ut_port.Method = kComMethodSocket;
   UT_ASSERT_EQ_INT(kODRCUnsupported,
      utt_ODComOutbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(0, waiting);
}
#endif

#ifdef INCLUDE_STDIO_COM
static void reports_stdio_as_unsupported_and_empty(void)
{
   int waiting = 99;
   reset_outbound();
   ut_port.Method = kComMethodStdIO;
   UT_ASSERT_EQ_INT(kODRCUnsupported,
      utt_ODComOutbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(0, waiting);
}
#endif

static void leaves_output_untouched_for_an_unknown_method(void)
{
   int waiting = 99;
   reset_outbound();
   ut_port.Method = (tComMethod)99;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComOutbound(ODPTR2HANDLE(&ut_port, tPortInfo), &waiting));
   UT_ASSERT_EQ_INT(99, waiting);
}

static const UTTestCase ut_cases[] = {
#ifdef INCLUDE_FOSSIL_COM
   {"FOSSIL", reports_fossil_empty_or_nonempty_state},
#endif
#ifdef INCLUDE_UART_COM
   {"UART", reports_the_uart_software_and_hardware_state},
#endif
#ifdef INCLUDE_WIN32_COM
   {"Win32", reports_the_win32_queue_count_and_failure},
#endif
#ifdef INCLUDE_DOOR32_COM
   {"Door32", reports_door32_as_unsupported_and_empty},
#endif
#ifdef INCLUDE_SOCKET_COM
   {"socket", reports_sockets_as_unsupported_and_empty},
#endif
#ifdef INCLUDE_STDIO_COM
   {"stdio", reports_stdio_as_unsupported_and_empty},
#endif
   {"unknown", leaves_output_untouched_for_an_unknown_method}
};
