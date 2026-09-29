#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_memset
#define UT_CUSTOM_MOCK_ODComSetPort
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_WSAStartup
#endif

static tPortInfo ut_storage;
static BOOL ut_malloc_fails;
static unsigned ut_set_port_calls;
#ifdef ODPLAT_WIN32
static unsigned ut_wsa_startup_calls;
#endif

void *memset(void *memory, int value, size_t size);

void *utm_memset(void *memory, int value, size_t size)
{
   BYTE *bytes = (BYTE *)memory;
   size_t index;
   for(index = 0; index < size; ++index)
      bytes[index] = (BYTE)value;
   return memory;
}

void *utm_malloc(size_t size)
{
   UT_ASSERT_EQ_UINT(sizeof(tPortInfo), size);
   return ut_malloc_fails ? NULL : &ut_storage;
}

tODResult utm_ODComSetPort(tPortHandle handle, BYTE port)
{
   ++ut_set_port_calls;
   UT_ASSERT_EQ_PTR(&ut_storage, ODHANDLE2PTR(handle, tPortInfo));
   UT_ASSERT_EQ_INT(0, port);
   ut_storage.btPort = port;
   return kODRCSuccess;
}

#ifdef ODPLAT_WIN32
int WSAAPI utm_WSAStartup(WORD version, LPWSADATA data)
{
   ++ut_wsa_startup_calls;
   UT_ASSERT_EQ_UINT(MAKEWORD(1, 1), version);
   UT_ASSERT_EQ_PTR(&WSAData, data);
   return 0;
}
#endif

static void reset_alloc(void)
{
   utm_memset(&ut_storage, 0xa5, sizeof(ut_storage));
   ut_malloc_fails = FALSE;
   ut_set_port_calls = 0;
#ifdef ODPLAT_WIN32
   ut_wsa_startup_calls = 0;
#endif
}

static void reports_allocation_failure_and_clears_the_handle(void)
{
   tPortHandle handle = ODPTR2HANDLE(&ut_storage, tPortInfo);
   reset_alloc();
   ut_malloc_fails = TRUE;
   UT_ASSERT_EQ_INT(kODRCNoMemory, utt_ODComAlloc(&handle));
   UT_ASSERT_NULL(ODHANDLE2PTR(handle, tPortInfo));
   UT_ASSERT_EQ_UINT(0, ut_set_port_calls);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(0, ut_wsa_startup_calls);
#endif
}

static void initializes_every_generic_port_setting(void)
{
   tPortHandle handle = ODPTR2HANDLE(NULL, tPortInfo);
   tPortInfo *port;
   reset_alloc();
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComAlloc(&handle));
   port = ODHANDLE2PTR(handle, tPortInfo);
   UT_ASSERT_EQ_PTR(&ut_storage, port);
   UT_ASSERT_EQ_INT(FALSE, port->bIsOpen);
   UT_ASSERT_EQ_INT(FALSE, port->bUsingClientsHandle);
   UT_ASSERT_EQ_INT(FLOW_DEFAULT, port->btFlowControlSetting);
   UT_ASSERT((long)SPEED_UNSPECIFIED == port->lSpeed);
   UT_ASSERT_EQ_INT(ODPARITY_NONE | DATABITS_EIGHT | STOP_ONE,
      port->btWordFormat);
   UT_ASSERT_EQ_INT(1024, port->nReceiveBufferSize);
   UT_ASSERT_EQ_INT(1024, port->nTransmitBufferSize);
   UT_ASSERT_EQ_INT(FIFO_ENABLE | FIFO_TRIGGER_8, port->btFIFOSetting);
   UT_ASSERT_EQ_INT(kComMethodUnspecified, port->Method);
   UT_ASSERT_NULL(port->pfIdleCallback);
#ifdef INCLUDE_SOCKET_COM
   UT_ASSERT_EQ_INT(FALSE, port->bTelnetSocket);
   UT_ASSERT_EQ_INT(kTelnetInputData, port->TelnetInputState);
   UT_ASSERT_EQ_INT(FALSE, port->bTelnetInputReplay);
   UT_ASSERT_EQ_INT(0, port->btTelnetInputReplay);
#endif
   UT_ASSERT_EQ_UINT(1, ut_set_port_calls);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(1, ut_wsa_startup_calls);
#endif
#ifdef ODPLAT_DOS32
   {
      unsigned index;
      const BYTE *bytes = (const BYTE *)&port->FossilBuffer;
      for(index = 0; index < sizeof(port->FossilBuffer); ++index)
         UT_ASSERT_EQ_INT(0, bytes[index]);
   }
#endif
}

static const UTTestCase ut_cases[] = {
   {"allocation failure", reports_allocation_failure_and_clears_the_handle},
   {"defaults", initializes_every_generic_port_setting}
};
