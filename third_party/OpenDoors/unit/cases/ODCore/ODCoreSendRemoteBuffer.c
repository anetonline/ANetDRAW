#define UT_CUSTOM_MOCK_ODComSendBuffer
#ifdef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_ODSyncAPIRelease
#define UT_CUSTOM_MOCK_ODSyncAPIReacquire
#endif

static unsigned ut_send_calls;
static tPortHandle ut_send_port;
static BYTE *ut_send_buffer;
static int ut_send_size;
#ifdef OD_THREAD_SUPPORT
static unsigned ut_release_calls;
static unsigned ut_reacquire_calls;
static unsigned ut_reacquire_level;
#endif

tODResult utm_ODComSendBuffer(tPortHandle port, BYTE *buffer, int size)
{
   ++ut_send_calls;
   ut_send_port = port;
   ut_send_buffer = buffer;
   ut_send_size = size;
   return kODRCGeneralFailure;
}

#ifdef OD_THREAD_SUPPORT
unsigned utm_ODSyncAPIRelease(void)
{
   ++ut_release_calls;
   return 9;
}

void utm_ODSyncAPIReacquire(unsigned level)
{
   ++ut_reacquire_calls;
   ut_reacquire_level = level;
}
#endif

static void sends_the_complete_buffer_outside_the_api_lock(void)
{
   BYTE buffer[3] = {1, 2, 3};
   hSerialPort = (tPortHandle)1;
   ut_send_calls = 0;
   ut_send_port = NULL;
   ut_send_buffer = NULL;
   ut_send_size = 0;
#ifdef OD_THREAD_SUPPORT
   ut_release_calls = 0;
   ut_reacquire_calls = 0;
   ut_reacquire_level = 0;
#endif

   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODCoreSendRemoteBuffer(buffer, sizeof(buffer)));
   UT_ASSERT_EQ_UINT(1, ut_send_calls);
   UT_ASSERT_EQ_PTR(hSerialPort, ut_send_port);
   UT_ASSERT_EQ_PTR(buffer, ut_send_buffer);
   UT_ASSERT_EQ_INT((int)sizeof(buffer), ut_send_size);
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT_EQ_UINT(1, ut_release_calls);
   UT_ASSERT_EQ_UINT(1, ut_reacquire_calls);
   UT_ASSERT_EQ_UINT(9, ut_reacquire_level);
#endif
}

static const UTTestCase ut_cases[] = {
   {"remote buffer", sends_the_complete_buffer_outside_the_api_lock}
};
