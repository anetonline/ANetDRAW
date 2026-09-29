#define UT_CUSTOM_MOCK__FP_SEG
#define UT_CUSTOM_MOCK_int386x
#define UT_CUSTOM_MOCK_memset
#define UT_CUSTOM_MOCK_segread

static tOD32RealModeRegisters ut_real_registers;
static unsigned ut_interrupt_calls;
static unsigned ut_segment_calls;
static unsigned ut_memset_calls;
static unsigned ut_carry;

void *utm_memset(void *memory, int value, size_t size)
{
   unsigned char *bytes = (unsigned char *)memory;
   size_t index;
   UT_ASSERT_EQ_INT(0, value);
   UT_ASSERT_EQ_UINT(sizeof(union REGS), size);
   for(index = 0; index < size; ++index) bytes[index] = 0;
   ++ut_memset_calls;
   return memory;
}

void utm_segread(struct SREGS *segments)
{
   UT_ASSERT_NOT_NULL(segments);
   segments->es = 0xaaaa;
   segments->cs = 0xbbbb;
   segments->ss = 0xcccc;
   segments->ds = 0xdddd;
   segments->fs = 0xeeee;
   segments->gs = 0xffff;
   ++ut_segment_calls;
}

unsigned short utm__FP_SEG(const volatile void __far *pointer)
{
   (void)pointer;
   return 0;
}

int utm_int386x(int interrupt_number, const union REGS *input,
   union REGS *output, struct SREGS *segments)
{
   UT_ASSERT_EQ_INT(0x31, interrupt_number);
   UT_ASSERT_EQ_PTR(input, output);
   UT_ASSERT_EQ_INT(0x0300, input->w.ax);
   UT_ASSERT_EQ_INT(0x7f, input->h.bl);
   UT_ASSERT_EQ_INT(0, input->h.bh);
   UT_ASSERT_EQ_INT(0, input->w.cx);
   UT_ASSERT_EQ_UINT(FP_OFF(&ut_real_registers), input->x.edi);
   UT_ASSERT_EQ_INT(utm__FP_SEG(&ut_real_registers), segments->es);
   UT_ASSERT_EQ_INT(0xbbbb, segments->cs);
   UT_ASSERT_EQ_INT(0xcccc, segments->ss);
   UT_ASSERT_EQ_INT(0xdddd, segments->ds);
   UT_ASSERT_EQ_INT(0xeeee, segments->fs);
   UT_ASSERT_EQ_INT(0xffff, segments->gs);
   output->x.cflag = ut_carry;
   ++ut_interrupt_calls;
   return 0;
}

static void reset_dpmi(void)
{
   ut_interrupt_calls = 0;
   ut_segment_calls = 0;
   ut_memset_calls = 0;
   ut_carry = 0;
}

static void returns_success_when_dpmi_clears_carry(void)
{
   reset_dpmi();
   UT_ASSERT(utt_OD32DPMIRealModeInterrupt(0x7f, &ut_real_registers));
   UT_ASSERT_EQ_UINT(1, ut_memset_calls);
   UT_ASSERT_EQ_UINT(1, ut_segment_calls);
   UT_ASSERT_EQ_UINT(1, ut_interrupt_calls);
}

static void returns_failure_when_dpmi_sets_carry(void)
{
   reset_dpmi();
   ut_carry = 1;
   UT_ASSERT(!utt_OD32DPMIRealModeInterrupt(0x7f, &ut_real_registers));
   UT_ASSERT_EQ_UINT(1, ut_interrupt_calls);
}

static const UTTestCase ut_cases[] = {
   {"carry clear", returns_success_when_dpmi_clears_carry},
   {"carry set", returns_failure_when_dpmi_sets_carry}
};
