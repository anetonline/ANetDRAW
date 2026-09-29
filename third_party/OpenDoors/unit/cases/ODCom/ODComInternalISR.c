#define UT_CUSTOM_MOCK_UTInp
#define UT_CUSTOM_MOCK_UTOutp

#define UT_IO_MAX 32

static unsigned ut_read_values[UT_IO_MAX];
static unsigned ut_read_ports[UT_IO_MAX];
static unsigned ut_read_count;
static unsigned ut_read_index;
static unsigned ut_write_ports[UT_IO_MAX];
static unsigned ut_write_values[UT_IO_MAX];
static unsigned ut_write_count;

#ifdef __TURBOC__
int
#else
unsigned
#endif
utm_UTInp(unsigned port)
{
   UT_ASSERT(ut_read_index < ut_read_count);
   if(ut_read_index >= ut_read_count)
      return 1;
   UT_ASSERT_EQ_UINT(ut_read_ports[ut_read_index], port);
   return ut_read_values[ut_read_index++];
}

#ifdef __TURBOC__
int
#else
unsigned
#endif
utm_UTOutp(unsigned port, unsigned value)
{
   UT_ASSERT(ut_write_count < UT_IO_MAX);
   if(ut_write_count < UT_IO_MAX)
   {
      ut_write_ports[ut_write_count] = port;
      ut_write_values[ut_write_count++] = value;
   }
   return value;
}

static void ut_io_reset(void)
{
   ut_read_count = 0;
   ut_read_index = 0;
   ut_write_count = 0;
   nIntIDRegAddr = 0x3fa;
   nI8259EndOfIntRegAddr = 0x20;
   nI8259MasterEndOfIntRegAddr = 0;
}

static void ut_read(unsigned port, unsigned value)
{
   UT_ASSERT(ut_read_count < UT_IO_MAX);
   if(ut_read_count < UT_IO_MAX)
   {
      ut_read_ports[ut_read_count] = port;
      ut_read_values[ut_read_count++] = value;
   }
}

static void ut_run_isr(void)
{
   utt_ODComInternalISR();
   UT_ASSERT_EQ_UINT(ut_read_count, ut_read_index);
}

static void finishes_and_signals_interrupt_controllers(void)
{
   ut_io_reset();
   ut_read(nIntIDRegAddr, 1);
   ut_run_isr();
   UT_ASSERT_EQ_UINT(1, ut_write_count);
   UT_ASSERT_EQ_UINT(nI8259EndOfIntRegAddr, ut_write_ports[0]);

   ut_io_reset();
   nI8259MasterEndOfIntRegAddr = 0xa0;
   ut_read(nIntIDRegAddr, 1);
   ut_run_isr();
   UT_ASSERT_EQ_UINT(2, ut_write_count);
   UT_ASSERT_EQ_UINT(0xa0, ut_write_ports[1]);
   UT_ASSERT_EQ_UINT(0x20, ut_write_values[1]);
}

static void handles_modem_status_changes(void)
{
   ut_io_reset();
   nModemStatusRegAddr = 0x3fe;
   btFlowControl = 0;
   ut_read(nIntIDRegAddr, 0);
   ut_read(nModemStatusRegAddr, DCTS | CTS);
   ut_read(nIntIDRegAddr, 1);
   ut_run_isr();

   ut_io_reset();
   btFlowControl = FLOW_RTSCTS;
   ut_read(nIntIDRegAddr, 0);
   ut_read(nModemStatusRegAddr, CTS);
   ut_read(nIntIDRegAddr, 1);
   ut_run_isr();

   ut_io_reset();
   btFlowControl = FLOW_RTSCTS;
   bStopTrans = FALSE;
   ut_read(nIntIDRegAddr, 0);
   ut_read(nModemStatusRegAddr, DCTS);
   ut_read(nIntIDRegAddr, 1);
   ut_run_isr();
   UT_ASSERT_EQ_INT(TRUE, bStopTrans);

   ut_io_reset();
   nIntEnableRegAddr = 0x3f9;
   btFlowControl = FLOW_RTSCTS;
   bStopTrans = TRUE;
   nTXChars = 0;
   ut_read(nIntIDRegAddr, 0);
   ut_read(nModemStatusRegAddr, DCTS | CTS);
   ut_read(nIntIDRegAddr, 1);
   ut_run_isr();
   UT_ASSERT_EQ_INT(FALSE, bStopTrans);

   ut_io_reset();
   nIntEnableRegAddr = 0x3f9;
   btFlowControl = FLOW_RTSCTS;
   bStopTrans = TRUE;
   nTXChars = 1;
   ut_read(nIntIDRegAddr, 0);
   ut_read(nModemStatusRegAddr, DCTS | CTS);
   ut_read(nIntEnableRegAddr, 0x10);
   ut_read(nIntIDRegAddr, 1);
   ut_run_isr();
   UT_ASSERT_EQ_UINT(nIntEnableRegAddr, ut_write_ports[0]);
   UT_ASSERT_EQ_UINT(0x10 | THRE, ut_write_values[0]);
}

static void handles_transmit_interrupts(void)
{
   static char queue[2] = {'A', 'B'};

   ut_io_reset();
   nIntEnableRegAddr = 0x3f9;
   nTXChars = 0;
   bStopTrans = FALSE;
   ut_read(nIntIDRegAddr, 2);
   ut_read(nIntEnableRegAddr, 0xff);
   ut_read(nIntIDRegAddr, 1);
   ut_run_isr();
   UT_ASSERT_EQ_UINT(0xfd, ut_write_values[0]);

   ut_io_reset();
   nIntEnableRegAddr = 0x3f9;
   nTXChars = 1;
   bStopTrans = TRUE;
   ut_read(nIntIDRegAddr, 2);
   ut_read(nIntEnableRegAddr, 0xff);
   ut_read(nIntIDRegAddr, 1);
   ut_run_isr();

   ut_io_reset();
   nLineStatusRegAddr = 0x3fd;
   nTXChars = 1;
   bStopTrans = FALSE;
   ut_read(nIntIDRegAddr, 2);
   ut_read(nLineStatusRegAddr, 0);
   ut_read(nIntIDRegAddr, 1);
   ut_run_isr();
   UT_ASSERT_EQ_INT(1, nTXChars);

   ut_io_reset();
   nLineStatusRegAddr = 0x3fd;
   nDataRegAddr = 0x3f8;
   pbtTXQueue = queue;
   nTXQueueSize = 2;
   nTXOutIndex = 0;
   nTXChars = 2;
   bStopTrans = FALSE;
   ut_read(nIntIDRegAddr, 2);
   ut_read(nLineStatusRegAddr, TXR);
   ut_read(nIntIDRegAddr, 1);
   ut_run_isr();
   UT_ASSERT_EQ_INT(1, nTXOutIndex);
   UT_ASSERT_EQ_INT(1, nTXChars);
   UT_ASSERT_EQ_UINT('A', ut_write_values[0]);

   ut_io_reset();
   nTXOutIndex = 1;
   nTXChars = 1;
   ut_read(nIntIDRegAddr, 2);
   ut_read(nLineStatusRegAddr, TXR);
   ut_read(nIntIDRegAddr, 1);
   ut_run_isr();
   UT_ASSERT_EQ_INT(0, nTXOutIndex);
   UT_ASSERT_EQ_UINT('B', ut_write_values[0]);
}

static void handles_receive_interrupts(void)
{
   static char queue[2];

   ut_io_reset();
   nDataRegAddr = 0x3f8;
   nModemCtrlRegAddr = 0x3fc;
   pbtRXQueue = queue;
   nRXQueueSize = 2;
   nRXHighWaterMark = 1;
   nRXChars = 0;
   nRXInIndex = 0;
   btFlowControl = 0;
   ut_read(nIntIDRegAddr, 4);
   ut_read(nDataRegAddr, 'A');
   ut_read(nIntIDRegAddr, 1);
   ut_run_isr();
   UT_ASSERT_EQ_INT('A', queue[0]);
   UT_ASSERT_EQ_INT(1, nRXInIndex);

   ut_io_reset();
   nRXChars = 1;
   nRXInIndex = 1;
   btFlowControl = 0;
   ut_read(nIntIDRegAddr, 4);
   ut_read(nDataRegAddr, 'B');
   ut_read(nIntIDRegAddr, 1);
   ut_run_isr();
   UT_ASSERT_EQ_INT(0, nRXInIndex);

   ut_io_reset();
   nRXChars = 1;
   nRXInIndex = 0;
   btFlowControl = FLOW_RTSCTS;
   ut_read(nIntIDRegAddr, 4);
   ut_read(nDataRegAddr, 'C');
   ut_read(nModemCtrlRegAddr, RTS | 1);
   ut_read(nIntIDRegAddr, 1);
   ut_run_isr();
   UT_ASSERT_EQ_INT('C', queue[0]);
   UT_ASSERT_EQ_UINT((RTS | 1) & NOT_RTS, ut_write_values[0]);

   ut_io_reset();
   nRXChars = nRXQueueSize;
   nRXInIndex = 0;
   btFlowControl = 0;
   queue[0] = 'Z';
   ut_read(nIntIDRegAddr, 4);
   ut_read(nDataRegAddr, 'D');
   ut_read(nIntIDRegAddr, 1);
   ut_run_isr();
   UT_ASSERT_EQ_INT('Z', queue[0]);
}

static void acknowledges_line_status(void)
{
   ut_io_reset();
   nLineStatusRegAddr = 0x3fd;
   ut_read(nIntIDRegAddr, 6);
   ut_read(nLineStatusRegAddr, 0x55);
   ut_read(nIntIDRegAddr, 1);
   ut_run_isr();
}

static const UTTestCase ut_cases[] = {
   {"interrupt completion", finishes_and_signals_interrupt_controllers},
   {"modem status", handles_modem_status_changes},
   {"transmit", handles_transmit_interrupts},
   {"receive", handles_receive_interrupts},
   {"line status", acknowledges_line_status}
};
