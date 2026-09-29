#define UT_CUSTOM_MOCK_UTOutp
#ifdef ODPLAT_DOS32
#define UT_CUSTOM_MOCK__disable
#define UT_CUSTOM_MOCK__enable
#endif

static unsigned ut_outp_calls;
static unsigned ut_outp_port;
static unsigned ut_outp_value;
#ifdef ODPLAT_DOS32
static unsigned ut_disable_calls;
static unsigned ut_enable_calls;
#endif

#ifdef __TURBOC__
int
#else
unsigned
#endif
utm_UTOutp(unsigned port, unsigned value)
{
   ++ut_outp_calls;
   ut_outp_port = port;
   ut_outp_value = value;
   return value;
}

#ifdef ODPLAT_DOS32
void utm__disable(void)
{
   ++ut_disable_calls;
}

void utm__enable(void)
{
   ++ut_enable_calls;
}
#endif

static void resets_software_queue_without_fifo_write(void)
{
   bUsingFIFO = FALSE;
   UT_CHARS = 7;
   UT_IN_INDEX = 3;
   UT_OUT_INDEX = 5;
   ut_outp_calls = 0;
#ifdef ODPLAT_DOS32
   ut_disable_calls = 0;
   ut_enable_calls = 0;
#endif

   UT_RESET();

   UT_ASSERT_EQ_INT(0, UT_CHARS);
   UT_ASSERT_EQ_INT(0, UT_IN_INDEX);
   UT_ASSERT_EQ_INT(0, UT_OUT_INDEX);
   UT_ASSERT_EQ_UINT(0, ut_outp_calls);
#ifdef ODPLAT_DOS32
   UT_ASSERT_EQ_UINT(1, ut_disable_calls);
   UT_ASSERT_EQ_UINT(1, ut_enable_calls);
#endif
}

static void resets_hardware_fifo_and_software_queue(void)
{
   bUsingFIFO = TRUE;
   nIntIDRegAddr = 0x3fa;
   btBaseFIFOCtrl = 0x81;
   UT_CHARS = 7;
   UT_IN_INDEX = 3;
   UT_OUT_INDEX = 5;
   ut_outp_calls = 0;
#ifdef ODPLAT_DOS32
   ut_disable_calls = 0;
   ut_enable_calls = 0;
#endif

   UT_RESET();

   UT_ASSERT_EQ_INT(0, UT_CHARS);
   UT_ASSERT_EQ_INT(0, UT_IN_INDEX);
   UT_ASSERT_EQ_INT(0, UT_OUT_INDEX);
   UT_ASSERT_EQ_UINT(1, ut_outp_calls);
   UT_ASSERT_EQ_UINT(0x3fa, ut_outp_port);
   UT_ASSERT_EQ_UINT((unsigned)(0x81 | UT_FIFO_RESET), ut_outp_value);
#ifdef ODPLAT_DOS32
   UT_ASSERT_EQ_UINT(1, ut_disable_calls);
   UT_ASSERT_EQ_UINT(1, ut_enable_calls);
#endif
}

static const UTTestCase ut_cases[] = {
   {"software queue", resets_software_queue_without_fifo_write},
   {"hardware FIFO", resets_hardware_fifo_and_software_queue}
};
