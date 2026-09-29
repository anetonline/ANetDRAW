#define UT_CUSTOM_MOCK_OD32FossilCall
#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_memset

static tOD32FossilBuffer ut_buffer;
static BYTE ut_linear[8];
static BYTE ut_source[8];
static BOOL ut_call_result;
static WORD ut_return_count;
static INT ut_expected_transfer;
static unsigned ut_call_count;
static unsigned ut_copy_count;

void *utm_memset(void *memory, int value, size_t size)
{
   unsigned char *bytes = (unsigned char *)memory;
   size_t index;
   UT_ASSERT_EQ_INT(0, value);
   UT_ASSERT_EQ_UINT(sizeof(tOD32RealModeRegisters), size);
   for(index = 0; index < size; ++index) bytes[index] = 0;
   return memory;
}

void *utm_memcpy(void *destination, const void *source, size_t size)
{
   size_t index;
   UT_ASSERT_EQ_PTR(ut_linear, destination);
   UT_ASSERT_EQ_PTR(ut_source, source);
   UT_ASSERT_EQ_UINT(ut_expected_transfer, size);
   for(index = 0; index < size; ++index)
      ((BYTE *)destination)[index] = ((const BYTE *)source)[index];
   ++ut_copy_count;
   return destination;
}

BOOL utm_OD32FossilCall(tOD32RealModeRegisters *registers)
{
   UT_ASSERT_EQ_UINT(0x1900, registers->eax);
   UT_ASSERT_EQ_UINT(ut_expected_transfer, registers->ecx);
   UT_ASSERT_EQ_UINT(3, registers->edx);
   UT_ASSERT_EQ_UINT(0x2468, registers->es);
   UT_ASSERT_EQ_UINT(0, registers->edi);
   registers->eax = ut_return_count;
   ++ut_call_count;
   return ut_call_result;
}

static void reset_send(void)
{
   unsigned index;
   ut_buffer.wRealSegment = 0x2468;
   ut_buffer.wSelector = 1;
   ut_buffer.pLinear = ut_linear;
   ut_buffer.wSize = sizeof(ut_linear);
   for(index = 0; index < sizeof(ut_linear); ++index)
   {
      ut_linear[index] = 0;
      ut_source[index] = (BYTE)(index + 10);
   }
   ut_call_result = TRUE;
   ut_return_count = 1;
   ut_expected_transfer = 0;
   ut_call_count = 0;
   ut_copy_count = 0;
}

static void rejects_each_invalid_argument(void)
{
   reset_send();
   UT_ASSERT_EQ_INT(-1, utt_OD32FossilSendBlock(3, NULL, ut_source, 1));
   ut_buffer.pLinear = NULL;
   UT_ASSERT_EQ_INT(-1, utt_OD32FossilSendBlock(3, &ut_buffer, ut_source, 1));
   ut_buffer.pLinear = ut_linear;
   UT_ASSERT_EQ_INT(-1, utt_OD32FossilSendBlock(3, &ut_buffer, NULL, 1));
   UT_ASSERT_EQ_INT(-1, utt_OD32FossilSendBlock(3, &ut_buffer, ut_source, 0));
   UT_ASSERT_EQ_UINT(0, ut_copy_count);
}

static void reports_interrupt_and_bad_count_failures(void)
{
   reset_send();
   ut_expected_transfer = 3;
   ut_call_result = FALSE;
   UT_ASSERT_EQ_INT(-1, utt_OD32FossilSendBlock(3, &ut_buffer, ut_source, 3));
   reset_send();
   ut_expected_transfer = 3;
   ut_return_count = 0;
   UT_ASSERT_EQ_INT(-1, utt_OD32FossilSendBlock(3, &ut_buffer, ut_source, 3));
   reset_send();
   ut_expected_transfer = 3;
   ut_return_count = 4;
   UT_ASSERT_EQ_INT(-1, utt_OD32FossilSendBlock(3, &ut_buffer, ut_source, 3));
}

static void copies_and_limits_successful_send(void)
{
   reset_send();
   ut_expected_transfer = sizeof(ut_linear);
   ut_return_count = 7;
   UT_ASSERT_EQ_INT(7, utt_OD32FossilSendBlock(3, &ut_buffer,
      ut_source, 20));
   UT_ASSERT_EQ_UINT(1, ut_call_count);
   UT_ASSERT_EQ_UINT(1, ut_copy_count);
   UT_ASSERT_EQ_UINT(10, ut_linear[0]);
   UT_ASSERT_EQ_UINT(17, ut_linear[7]);
}

static const UTTestCase ut_cases[] = {
   {"invalid arguments", rejects_each_invalid_argument},
   {"FOSSIL failures", reports_interrupt_and_bad_count_failures},
   {"copy and limit", copies_and_limits_successful_send}
};
