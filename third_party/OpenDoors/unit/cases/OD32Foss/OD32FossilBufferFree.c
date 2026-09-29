#define UT_CUSTOM_MOCK_int386
#define UT_CUSTOM_MOCK_memset

static tOD32FossilBuffer ut_buffer;
static unsigned ut_interrupt_calls;
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
   UT_ASSERT_EQ_INT(0x31, interrupt_number);
   UT_ASSERT_EQ_PTR(input, output);
   UT_ASSERT_EQ_UINT(0x0101, input->w.ax);
   UT_ASSERT_EQ_UINT(0x1357, input->w.dx);
   ++ut_interrupt_calls;
   return 0;
}

static void reset_free(WORD selector)
{
   ut_buffer.wRealSegment = 0x2468;
   ut_buffer.wSelector = selector;
   ut_buffer.pLinear = (BYTE *)0x12345678UL;
   ut_buffer.wSize = 4096;
   ut_interrupt_calls = 0;
   ut_buffer_clears = 0;
   ut_register_clears = 0;
}

static void ignores_null(void)
{
   reset_free(0x1357);
   utt_OD32FossilBufferFree(NULL);
   UT_ASSERT_EQ_UINT(0, ut_interrupt_calls);
   UT_ASSERT_EQ_UINT(0, ut_buffer_clears);
}

static void clears_an_empty_descriptor(void)
{
   reset_free(0);
   utt_OD32FossilBufferFree(&ut_buffer);
   UT_ASSERT_EQ_UINT(0, ut_interrupt_calls);
   UT_ASSERT_EQ_UINT(0, ut_register_clears);
   UT_ASSERT_EQ_UINT(1, ut_buffer_clears);
   UT_ASSERT_EQ_UINT(0, ut_buffer.wSize);
}

static void releases_selector_then_clears_descriptor(void)
{
   reset_free(0x1357);
   utt_OD32FossilBufferFree(&ut_buffer);
   UT_ASSERT_EQ_UINT(1, ut_interrupt_calls);
   UT_ASSERT_EQ_UINT(1, ut_register_clears);
   UT_ASSERT_EQ_UINT(1, ut_buffer_clears);
   UT_ASSERT_EQ_UINT(0, ut_buffer.wSelector);
}

static const UTTestCase ut_cases[] = {
   {"null", ignores_null},
   {"no selector", clears_an_empty_descriptor},
   {"selector", releases_selector_then_clears_descriptor}
};
