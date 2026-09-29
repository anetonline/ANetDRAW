#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK_ODScrnUpdateCaretPos
static unsigned ut_update_calls;
void utm_ODScrnUpdateCaretPos(void) { ++ut_update_calls; }
#endif

#if defined(ODPLAT_DOS) && defined(__WATCOMC__)
#define UT_CUSTOM_MOCK_int86
static unsigned ut_int_calls;
int utm_int86(int interrupt_number, const union REGS *input,
   union REGS *output)
{
   ++ut_int_calls; UT_ASSERT_EQ_INT(0x10, interrupt_number);
   UT_ASSERT(input == (const union REGS *)output);
   UT_ASSERT_EQ_UINT(0, input->h.bh);
   if(ut_int_calls & 1)
   {
      UT_ASSERT_EQ_UINT(0x03, input->h.ah); output->h.ch = 0x66;
   }
   else
   {
      UT_ASSERT_EQ_UINT(0x01, input->h.ah);
      UT_ASSERT_EQ_UINT(bCaretOn ? 0x06 : 0x26, input->h.ch);
   }
   return 0;
}
#endif

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
   UT_ASSERT_EQ_UINT(0, input->h.bh);
   if(ut_int_calls & 1)
   {
      UT_ASSERT_EQ_UINT(0x03, input->h.ah); output->h.ch = 0x66;
   }
   else
   {
      UT_ASSERT_EQ_UINT(0x01, input->h.ah);
      UT_ASSERT_EQ_UINT(bCaretOn ? 0x06 : 0x26, input->h.ch);
   }
   return 0;
}
#endif

static void updates_the_requested_state_only_outside_presentation_changes(void)
{
   bCaretPresentationChange = FALSE; bRequestedCaretOn = FALSE;
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) || defined(ODPLAT_WIN32)
   bCaretOn = TRUE;
#endif
   utt_ODScrnEnableCaret(TRUE);
   UT_ASSERT_EQ_INT(TRUE, bRequestedCaretOn);

   bCaretPresentationChange = TRUE; bRequestedCaretOn = FALSE;
   utt_ODScrnEnableCaret(TRUE);
   UT_ASSERT_EQ_INT(FALSE, bRequestedCaretOn);
}

#ifdef ODPLAT_WIN32
static void dirties_only_a_changed_windows_caret_state(void)
{
   bCaretPresentationChange = FALSE; bRequestedCaretOn = FALSE;
   bCaretOn = FALSE; bScreenDirty = FALSE;
   utt_ODScrnEnableCaret(TRUE);
   UT_ASSERT_EQ_INT(TRUE, bCaretOn); UT_ASSERT_EQ_INT(TRUE, bScreenDirty);

   bScreenDirty = FALSE;
   utt_ODScrnEnableCaret(TRUE);
   UT_ASSERT_EQ_INT(FALSE, bScreenDirty);

   utt_ODScrnEnableCaret(FALSE);
   UT_ASSERT_EQ_INT(FALSE, bCaretOn); UT_ASSERT_EQ_INT(TRUE, bScreenDirty);
}
#endif

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
static void returns_without_bios_work_when_the_state_already_matches(void)
{
   bCaretPresentationChange = FALSE; bRequestedCaretOn = FALSE;
   bCaretOn = FALSE; ut_update_calls = 0;
#if defined(ODPLAT_DOS32) || defined(__WATCOMC__)
   ut_int_calls = 0;
#endif
   utt_ODScrnEnableCaret(FALSE);
   UT_ASSERT_EQ_INT(FALSE, bCaretOn); UT_ASSERT_EQ_UINT(0, ut_update_calls);
#if defined(ODPLAT_DOS32) || defined(__WATCOMC__)
   UT_ASSERT_EQ_UINT(0, ut_int_calls);
#endif
}

static void enables_and_disables_the_bios_caret(void)
{
   btDisplayPage = 0; bCaretPresentationChange = FALSE;
   bCaretOn = FALSE; ut_update_calls = 0;
#if defined(ODPLAT_DOS32) || defined(__WATCOMC__)
   ut_int_calls = 0;
#endif
   utt_ODScrnEnableCaret(TRUE);
   UT_ASSERT_EQ_INT(TRUE, bCaretOn); UT_ASSERT_EQ_UINT(1, ut_update_calls);
#if defined(ODPLAT_DOS32) || defined(__WATCOMC__)
   UT_ASSERT_EQ_UINT(2, ut_int_calls);
   ut_int_calls = 0;
#endif
   utt_ODScrnEnableCaret(FALSE);
   UT_ASSERT_EQ_INT(FALSE, bCaretOn); UT_ASSERT_EQ_UINT(1, ut_update_calls);
#if defined(ODPLAT_DOS32) || defined(__WATCOMC__)
   UT_ASSERT_EQ_UINT(2, ut_int_calls);
#endif
}
#endif

static const UTTestCase ut_cases[] = {
   {"requested state", updates_the_requested_state_only_outside_presentation_changes},
#ifdef ODPLAT_WIN32
   {"windows state", dirties_only_a_changed_windows_caret_state},
#endif
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   {"unchanged state", returns_without_bios_work_when_the_state_already_matches},
   {"bios caret", enables_and_disables_the_bios_caret}
#endif
};
