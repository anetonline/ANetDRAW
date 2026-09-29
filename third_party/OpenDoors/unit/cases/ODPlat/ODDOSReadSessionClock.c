#define UT_CUSTOM_MOCK_intdos
#define UT_CUSTOM_MOCK_memset

static unsigned ut_interrupt_call;
static unsigned ut_date_read;
static WORD ut_year;
static BYTE ut_month;
static BYTE ut_day;
static int ut_mismatch;

void *utm_memset(void *destination, int value, size_t size)
{
   unsigned char *bytes = (unsigned char *)destination;
   size_t index;
   for(index = 0; index < size; ++index)
      bytes[index] = (unsigned char)value;
   return(destination);
}

#ifdef __TURBOC__
int utm_intdos(union REGS *input, union REGS *output)
#else
int utm_intdos(const union REGS *input, union REGS *output)
#endif
{
   BYTE function = input->h.ah;
   ++ut_interrupt_call;

   if(function == 0x2a)
   {
      ++ut_date_read;
#ifdef ODPLAT_DOS32
      output->w.cx = ut_year;
#else
      output->x.cx = ut_year;
#endif
      output->h.dh = ut_month;
      output->h.dl = ut_day;
      if(ut_date_read == 2)
      {
         if(ut_mismatch == 1)
#ifdef ODPLAT_DOS32
            ++output->w.cx;
#else
            ++output->x.cx;
#endif
         else if(ut_mismatch == 2)
            output->h.dh = (BYTE)(ut_month == 12 ? 11 : ut_month + 1);
         else if(ut_mismatch == 3)
            output->h.dl = (BYTE)(ut_day == 31 ? 30 : ut_day + 1);
      }
   }
   else
   {
      UT_ASSERT_EQ_UINT(0x2c, function);
      output->h.ch = 1;
      output->h.cl = 2;
      output->h.dh = 3;
      output->h.dl = 45;
   }
   return(0);
}

static void check_read(WORD year, BYTE month, BYTE day, int mismatch,
   DWORD expected_days)
{
   DWORD days = 0;
   DWORD seconds = 0;
   WORD milliseconds = 0;

   ut_interrupt_call = 0;
   ut_date_read = 0;
   ut_year = year;
   ut_month = month;
   ut_day = day;
   ut_mismatch = mismatch;
   utt_ODDOSReadSessionClock(&days, &seconds, &milliseconds);
   UT_ASSERT_EQ_UINT(mismatch == 0 ? 3 : 6, ut_interrupt_call);
   UT_ASSERT_EQ_UINT(expected_days, days);
   UT_ASSERT_EQ_UINT(3723, seconds);
   UT_ASSERT_EQ_UINT(450, milliseconds);
}

static void retries_incoherent_dates_and_converts_gregorian_dates(void)
{
   check_read(2024, 2, 29, 1, 16130UL);
   check_read(2024, 2, 29, 2, 16130UL);
   check_read(2024, 2, 29, 3, 16130UL);
   check_read(2024, 3, 1, 0, 16131UL);
   check_read(2000, 3, 1, 0, 7365UL);
   check_read(2100, 3, 1, 0, 43889UL);
   check_read(2101, 3, 1, 0, 44254UL);
   check_read(1980, 1, 0, 0, 0UL);
}

static const UTTestCase ut_cases[] = {
   {"DOS clock", retries_incoherent_dates_and_converts_gregorian_dates}
};
