#define UT_CUSTOM_MOCK_OD32FossilBufferFree
#define UT_CUSTOM_MOCK_int386
#define UT_CUSTOM_MOCK_memset

static tOD32FossilBuffer ut_buffer;
static unsigned ut_interrupt_step;
static unsigned ut_fail_step;
static unsigned ut_free_calls;
static unsigned ut_buffer_clears;
static unsigned ut_register_clears;

void *utm_memset(void *memory, int value, size_t size)
{
   unsigned char *bytes = (unsigned char *)memory;
   size_t index;
   UT_ASSERT_EQ_INT(0, value);
   if(memory == &ut_buffer)
   {
      UT_ASSERT_EQ_UINT(sizeof(ut_buffer), size);
      ++ut_buffer_clears;
   }
   else
   {
      UT_ASSERT_EQ_UINT(sizeof(union REGS), size);
      ++ut_register_clears;
   }
   for(index = 0; index < size; ++index) bytes[index] = 0;
   return memory;
}

int utm_int386(int interrupt_number, const union REGS *input,
   union REGS *output)
{
   ++ut_interrupt_step;
   UT_ASSERT_EQ_INT(0x31, interrupt_number);
   UT_ASSERT_EQ_PTR(input, output);
   if(ut_interrupt_step == 1)
   {
      UT_ASSERT_EQ_UINT(0x0100, input->w.ax);
      UT_ASSERT_EQ_UINT(0x100, input->w.bx);
      output->w.ax = 0x2468;
      output->w.dx = 0x1357;
   }
   else
   {
      UT_ASSERT_EQ_UINT(2, ut_interrupt_step);
      UT_ASSERT_EQ_UINT(0x0006, input->w.ax);
      UT_ASSERT_EQ_UINT(0x1357, input->w.bx);
      output->w.cx = 0x1234;
      output->w.dx = 0x5678;
   }
   output->x.cflag = ut_interrupt_step == ut_fail_step;
   return 0;
}

void utm_OD32FossilBufferFree(tOD32FossilBuffer *buffer)
{
   UT_ASSERT_EQ_PTR(&ut_buffer, buffer);
   ++ut_free_calls;
}

static void reset_allocate(void)
{
   ut_buffer.wRealSegment = 1;
   ut_buffer.wSelector = 2;
   ut_buffer.pLinear = (BYTE *)3;
   ut_buffer.wSize = 4;
   ut_interrupt_step = 0;
   ut_fail_step = 0;
   ut_free_calls = 0;
   ut_buffer_clears = 0;
   ut_register_clears = 0;
}

static void rejects_each_invalid_argument(void)
{
   reset_allocate();
   UT_ASSERT(!utt_OD32FossilBufferAllocate(NULL, 0x1000));
   UT_ASSERT_EQ_UINT(0, ut_buffer_clears);
   reset_allocate();
   UT_ASSERT(!utt_OD32FossilBufferAllocate(&ut_buffer, 0));
   UT_ASSERT_EQ_UINT(0, ut_buffer_clears);
}

static void reports_allocation_failure(void)
{
   reset_allocate();
   ut_fail_step = 1;
   UT_ASSERT(!utt_OD32FossilBufferAllocate(&ut_buffer, 0x1000));
   UT_ASSERT_EQ_UINT(1, ut_interrupt_step);
   UT_ASSERT_EQ_UINT(1, ut_buffer_clears);
   UT_ASSERT_EQ_UINT(1, ut_register_clears);
}

static void frees_allocation_when_address_query_fails(void)
{
   reset_allocate();
   ut_fail_step = 2;
   UT_ASSERT(!utt_OD32FossilBufferAllocate(&ut_buffer, 0x1000));
   UT_ASSERT_EQ_UINT(2, ut_interrupt_step);
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
}

static void records_real_and_linear_addresses(void)
{
   reset_allocate();
   UT_ASSERT(utt_OD32FossilBufferAllocate(&ut_buffer, 0x1000));
   UT_ASSERT_EQ_UINT(2, ut_interrupt_step);
   UT_ASSERT_EQ_UINT(2, ut_register_clears);
   UT_ASSERT_EQ_UINT(0x2468, ut_buffer.wRealSegment);
   UT_ASSERT_EQ_UINT(0x1357, ut_buffer.wSelector);
   UT_ASSERT_EQ_UINT(0x1000, ut_buffer.wSize);
   UT_ASSERT_EQ_PTR((BYTE *)0x12345678UL, ut_buffer.pLinear);
}

static const UTTestCase ut_cases[] = {
   {"invalid arguments", rejects_each_invalid_argument},
   {"allocation failure", reports_allocation_failure},
   {"address failure", frees_allocation_when_address_query_fails},
   {"success", records_real_and_linear_addresses}
};
