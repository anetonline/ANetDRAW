#define UT_CUSTOM_MOCK_OD32DPMIRealModeInterrupt

static tOD32RealModeRegisters ut_registers;
static BOOL ut_result;
static unsigned ut_calls;

BOOL utm_OD32DPMIRealModeInterrupt(BYTE interrupt_number,
   tOD32RealModeRegisters *registers)
{
   UT_ASSERT_EQ_UINT(0x14, interrupt_number);
   UT_ASSERT_EQ_PTR(&ut_registers, registers);
   ++ut_calls;
   return ut_result;
}

static void delegates_fossil_interrupt_and_result(void)
{
   ut_calls = 0;
   ut_result = FALSE;
   UT_ASSERT(!utt_OD32FossilCall(&ut_registers));
   ut_result = TRUE;
   UT_ASSERT(utt_OD32FossilCall(&ut_registers));
   UT_ASSERT_EQ_UINT(2, ut_calls);
}

static const UTTestCase ut_cases[] = {
   {"delegates interrupt 14h", delegates_fossil_interrupt_and_result}
};
