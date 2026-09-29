#if defined(ODPLAT_DOS) && defined(__WATCOMC__)
#define UT_CUSTOM_MOCK_intdos
#define UT_CUSTOM_MOCK_int86
static BYTE ut_dos_version;
static BYTE ut_desqview_result;
static BYTE ut_windows_result;
static unsigned ut_dos_version_calls;
static unsigned ut_desqview_calls;
static unsigned ut_windows_calls;

int utm_intdos(CONST union REGS *input, union REGS *output)
{
   UT_ASSERT_EQ_PTR(input, output);
   if(input->h.ah == 0x30)
   {
      ++ut_dos_version_calls;
      output->h.al = ut_dos_version;
   }
   else
   {
      ++ut_desqview_calls;
      UT_ASSERT_EQ_UINT(0x2b01, input->x.ax);
      UT_ASSERT_EQ_UINT(0x4445, input->x.cx);
      UT_ASSERT_EQ_UINT(0x5351, input->x.dx);
      output->h.al = ut_desqview_result;
   }
   return(0);
}

int utm_int86(int interrupt_number, CONST union REGS *input,
   union REGS *output)
{
   ++ut_windows_calls;
   UT_ASSERT_EQ_INT(0x2f, interrupt_number);
   UT_ASSERT_EQ_PTR(input, output);
   UT_ASSERT_EQ_UINT(0x1600, input->x.ax);
   output->h.al = ut_windows_result;
   return(0);
}

static void reset_detection(void)
{
   ODMultitasker = (tODMultitasker)99;
   ut_dos_version_calls = 0;
   ut_desqview_calls = 0;
   ut_windows_calls = 0;
}

static void detects_os2(void)
{
   reset_detection();
   ut_dos_version = 0x0a;
   utt_ODPlatInit();
   UT_ASSERT_EQ_INT(kMultitaskerOS2, ODMultitasker);
   UT_ASSERT_EQ_UINT(1, ut_dos_version_calls);
   UT_ASSERT_EQ_UINT(0, ut_desqview_calls);
   UT_ASSERT_EQ_UINT(0, ut_windows_calls);
}

static void detects_desqview(void)
{
   reset_detection();
   ut_dos_version = 9;
   ut_desqview_result = 0;
   utt_ODPlatInit();
   UT_ASSERT_EQ_INT(kMultitaskerDV, ODMultitasker);
   UT_ASSERT_EQ_UINT(1, ut_desqview_calls);
   UT_ASSERT_EQ_UINT(0, ut_windows_calls);
}

static void detects_windows(void)
{
   reset_detection();
   ut_dos_version = 9;
   ut_desqview_result = 0xff;
   ut_windows_result = 1;
   utt_ODPlatInit();
   UT_ASSERT_EQ_INT(kMultitaskerWin, ODMultitasker);
   UT_ASSERT_EQ_UINT(1, ut_windows_calls);
}

static void recognizes_non_windows_results(void)
{
   reset_detection();
   ut_dos_version = 9;
   ut_desqview_result = 0xff;
   ut_windows_result = 0;
   utt_ODPlatInit();
   UT_ASSERT_EQ_INT(kMultitaskerNone, ODMultitasker);
   reset_detection();
   ut_dos_version = 9;
   ut_desqview_result = 0xff;
   ut_windows_result = 0x80;
   utt_ODPlatInit();
   UT_ASSERT_EQ_INT(kMultitaskerNone, ODMultitasker);
}
#elif defined(ODPLAT_DOS) && defined(__TURBOC__)
static void set_fixture_mode(BYTE mode)
{
   ASM mov al, mode
   ASM int 0x60
}

static void detects_each_multitasker(void)
{
   set_fixture_mode(1);
   ODMultitasker = (tODMultitasker)99;
   utt_ODPlatInit();
   UT_ASSERT_EQ_INT(kMultitaskerOS2, ODMultitasker);

   set_fixture_mode(2);
   ODMultitasker = (tODMultitasker)99;
   utt_ODPlatInit();
   UT_ASSERT_EQ_INT(kMultitaskerDV, ODMultitasker);

   set_fixture_mode(3);
   ODMultitasker = (tODMultitasker)99;
   utt_ODPlatInit();
   UT_ASSERT_EQ_INT(kMultitaskerWin, ODMultitasker);

   set_fixture_mode(0);
   ODMultitasker = (tODMultitasker)99;
   utt_ODPlatInit();
   UT_ASSERT_EQ_INT(kMultitaskerNone, ODMultitasker);
}
#else
static void is_a_no_op_on_this_platform(void)
{
   utt_ODPlatInit();
}
#endif

static const UTTestCase ut_cases[] = {
#if defined(ODPLAT_DOS) && defined(__WATCOMC__)
   {"OS/2", detects_os2},
   {"DESQview", detects_desqview},
   {"Windows", detects_windows},
   {"no Windows", recognizes_non_windows_results}
#elif defined(ODPLAT_DOS) && defined(__TURBOC__)
   {"multitasker detection", detects_each_multitasker}
#else
   {"no operation", is_a_no_op_on_this_platform}
#endif
};
