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
   UT_ASSERT_EQ_UINT(0x02, input->h.ah);
   UT_ASSERT_EQ_UINT(0, input->h.bh);
   UT_ASSERT_EQ_UINT(7, input->h.dh); UT_ASSERT_EQ_UINT(9, input->h.dl);
   return 0;
}
#endif

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
static void does_not_move_a_disabled_caret(void)
{
   bCaretOn = FALSE;
#ifdef ODPLAT_DOS32
   ut_int_calls = 0;
#endif
   utt_ODScrnUpdateCaretPos();
#ifdef ODPLAT_DOS32
   UT_ASSERT_EQ_UINT(0, ut_int_calls);
#endif
}

static void moves_the_caret_to_the_boundary_relative_position(void)
{
   btDisplayPage = 0; btTopBoundary = 2; btLeftBoundary = 3;
   btCursorRow = 5; btCursorColumn = 6; bCaretOn = TRUE;
#ifdef ODPLAT_DOS32
   ut_int_calls = 0;
#endif
   utt_ODScrnUpdateCaretPos();
#ifdef ODPLAT_DOS32
   UT_ASSERT_EQ_UINT(1, ut_int_calls);
#else
   {
      union REGS registers;
      registers.h.ah = 0x03; registers.h.bh = 0;
      int86(0x10, &registers, &registers);
      UT_ASSERT_EQ_UINT(7, registers.h.dh); UT_ASSERT_EQ_UINT(9, registers.h.dl);
   }
#endif
}
#elif defined(ODPLAT_WIN32)
static void marks_the_owner_screen_generation_dirty(void)
{
   bScreenDirty = FALSE;
   utt_ODScrnUpdateCaretPos();
   UT_ASSERT_EQ_INT(TRUE, bScreenDirty);
}
#else
static void is_a_noop_without_a_hardware_caret(void)
{
   utt_ODScrnUpdateCaretPos();
   UT_ASSERT(TRUE);
}
#endif

static const UTTestCase ut_cases[] = {
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   {"caret off", does_not_move_a_disabled_caret},
   {"caret position", moves_the_caret_to_the_boundary_relative_position}
#elif defined(ODPLAT_WIN32)
   {"dirty generation", marks_the_owner_screen_generation_dirty}
#else
   {"no hardware caret", is_a_noop_without_a_hardware_caret}
#endif
};
