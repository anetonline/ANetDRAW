#define UT_CUSTOM_MOCK_OD32FossilCall
#define UT_CUSTOM_MOCK_memset

static DWORD ut_expected_eax;
static BYTE ut_expected_port;
static DWORD ut_return_eax;
static BOOL ut_call_result;
static unsigned ut_call_count;
static unsigned ut_memset_count;

static void ut_reset_register_call(DWORD eax, BYTE port)
{
   ut_expected_eax = eax;
   ut_expected_port = port;
   ut_return_eax = 0;
   ut_call_result = TRUE;
   ut_call_count = 0;
   ut_memset_count = 0;
}

void *utm_memset(void *memory, int value, size_t size)
{
   unsigned char *bytes = (unsigned char *)memory;
   size_t index;
   UT_ASSERT_NOT_NULL(memory);
   UT_ASSERT_EQ_INT(0, value);
   UT_ASSERT_EQ_UINT(sizeof(tOD32RealModeRegisters), size);
   for(index = 0; index < size; ++index) bytes[index] = 0;
   ++ut_memset_count;
   return memory;
}

BOOL utm_OD32FossilCall(tOD32RealModeRegisters *registers)
{
   UT_ASSERT_NOT_NULL(registers);
   UT_ASSERT_EQ_UINT(ut_expected_eax, registers->eax);
   UT_ASSERT_EQ_UINT(ut_expected_port, registers->edx);
   UT_ASSERT_EQ_UINT(0, registers->ebx);
   UT_ASSERT_EQ_UINT(0, registers->ecx);
   UT_ASSERT_EQ_UINT(0, registers->esi);
   UT_ASSERT_EQ_UINT(0, registers->edi);
   UT_ASSERT_EQ_UINT(0, registers->ebp);
   UT_ASSERT_EQ_UINT(0, registers->ds);
   UT_ASSERT_EQ_UINT(0, registers->es);
   registers->eax = ut_return_eax;
   ++ut_call_count;
   return ut_call_result;
}

static void ut_assert_one_register_call(void)
{
   UT_ASSERT_EQ_UINT(1, ut_memset_count);
   UT_ASSERT_EQ_UINT(1, ut_call_count);
}
