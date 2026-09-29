#ifdef ODPLAT_DOS32
#define UT_CUSTOM_MOCK_memset
#define UT_CUSTOM_MOCK_int386
static unsigned ut_int_calls;
void *utm_memset(void *memory, int value, size_t count)
{
   BYTE *bytes;
   size_t index;
   bytes = (BYTE *)memory;
   for(index = 0; index < count; ++index) bytes[index] = (BYTE)value;
   return memory;
}
int utm_int386(int interrupt_number, const union REGS *input,
   union REGS *output)
{
   ++ut_int_calls; UT_ASSERT_EQ_INT(0x10, interrupt_number);
   UT_ASSERT(input == (const union REGS *)output);
   UT_ASSERT_EQ_UINT(0x03, input->h.ah);
   UT_ASSERT_EQ_UINT(0, input->h.bh);
   output->h.dh = 7; output->h.dl = 9;
   return 0;
}
#endif

#ifdef ODPLAT_DOS
static void position_bios_cursor(BYTE row, BYTE column)
{
   union REGS registers;
   registers.h.ah = 0x02; registers.h.bh = 0;
   registers.h.dh = row; registers.h.dl = column;
   int86(0x10, &registers, &registers);
}
#endif

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
static void leaves_the_stored_position_when_the_caret_is_off(void)
{
   bCaretOn = FALSE; btCursorRow = 11; btCursorColumn = 12;
#ifdef ODPLAT_DOS32
   ut_int_calls = 0;
#endif
   utt_ODScrnGetCursorPos();
   UT_ASSERT_EQ_UINT(11, btCursorRow); UT_ASSERT_EQ_UINT(12, btCursorColumn);
#ifdef ODPLAT_DOS32
   UT_ASSERT_EQ_UINT(0, ut_int_calls);
#endif
}

static void reads_the_bios_position_relative_to_the_boundary(void)
{
   btDisplayPage = 0; btTopBoundary = 2; btLeftBoundary = 3;
   btCursorRow = btCursorColumn = 0; bCaretOn = TRUE;
#ifdef ODPLAT_DOS32
   ut_int_calls = 0;
#endif
#ifdef ODPLAT_DOS
   position_bios_cursor(7, 9);
#endif
   utt_ODScrnGetCursorPos();
   UT_ASSERT_EQ_UINT(5, btCursorRow); UT_ASSERT_EQ_UINT(6, btCursorColumn);
#ifdef ODPLAT_DOS32
   UT_ASSERT_EQ_UINT(1, ut_int_calls);
#endif
}
#endif

#if !defined(ODPLAT_DOS) && !defined(ODPLAT_DOS32)
static void is_a_noop_without_a_hardware_text_cursor(void)
{
   btCursorRow = 11; btCursorColumn = 12;
   utt_ODScrnGetCursorPos();
   UT_ASSERT_EQ_UINT(11, btCursorRow); UT_ASSERT_EQ_UINT(12, btCursorColumn);
}
#endif

static const UTTestCase ut_cases[] = {
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   {"caret off", leaves_the_stored_position_when_the_caret_is_off},
   {"bios cursor", reads_the_bios_position_relative_to_the_boundary}
#else
   {"no hardware cursor", is_a_noop_without_a_hardware_text_cursor}
#endif
};
