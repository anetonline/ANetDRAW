#define UT_CUSTOM_MOCK_OD32FossilBufferFree

static tPortInfo ut_port;
static unsigned ut_free_calls;

void utm_OD32FossilBufferFree(tOD32FossilBuffer *buffer)
{
   ++ut_free_calls;
   UT_ASSERT_EQ_PTR(&ut_port.FossilBuffer, buffer);
}

static void rejects_each_invalid_state_and_frees_an_open_fossil_buffer(void)
{
   memset(&ut_port, 0, sizeof(ut_port));
   ut_free_calls = 0;
   UT_ASSERT_EQ_INT(kODRCInvalidCall,
      utt_ODComDOS32DisableFossilBlockIO(ODPTR2HANDLE(NULL, tPortInfo)));

   ut_port.Method = kComMethodFOSSIL;
   UT_ASSERT_EQ_INT(kODRCInvalidCall, utt_ODComDOS32DisableFossilBlockIO(
      ODPTR2HANDLE(&ut_port, tPortInfo)));

   ut_port.bIsOpen = TRUE;
   ut_port.Method = kComMethodSocket;
   UT_ASSERT_EQ_INT(kODRCInvalidCall, utt_ODComDOS32DisableFossilBlockIO(
      ODPTR2HANDLE(&ut_port, tPortInfo)));

   ut_port.Method = kComMethodFOSSIL;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComDOS32DisableFossilBlockIO(
      ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
}

static const UTTestCase ut_cases[] = {
   {"states", rejects_each_invalid_state_and_frees_an_open_fossil_buffer}
};
