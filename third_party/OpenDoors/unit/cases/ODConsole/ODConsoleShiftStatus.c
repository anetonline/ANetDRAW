static void maps_each_console_modifier(void)
{
   UT_ASSERT_EQ_UINT(0, utt_ODConsoleShiftStatus(0));
   UT_ASSERT_EQ_UINT(0x04, utt_ODConsoleShiftStatus(LEFT_CTRL_PRESSED));
   UT_ASSERT_EQ_UINT(0x04, utt_ODConsoleShiftStatus(RIGHT_CTRL_PRESSED));
   UT_ASSERT_EQ_UINT(0x08, utt_ODConsoleShiftStatus(LEFT_ALT_PRESSED));
   UT_ASSERT_EQ_UINT(0x08, utt_ODConsoleShiftStatus(RIGHT_ALT_PRESSED));
   UT_ASSERT_EQ_UINT(0x02, utt_ODConsoleShiftStatus(SHIFT_PRESSED));
   UT_ASSERT_EQ_UINT(0x10, utt_ODConsoleShiftStatus(SCROLLLOCK_ON));
   UT_ASSERT_EQ_UINT(0x20, utt_ODConsoleShiftStatus(NUMLOCK_ON));
   UT_ASSERT_EQ_UINT(0x40, utt_ODConsoleShiftStatus(CAPSLOCK_ON));
   UT_ASSERT_EQ_UINT(0x7e, utt_ODConsoleShiftStatus(
      LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED | SHIFT_PRESSED |
      SCROLLLOCK_ON | NUMLOCK_ON | CAPSLOCK_ON));
}
static const UTTestCase ut_cases[] = {
   {"modifier mapping", maps_each_console_modifier}
};
