static void reads_the_bios_tick_counter(void)
{
   DWORD ticks = utt_OD32BIOSClock();
   UT_ASSERT(ticks < OD_DOS32_TICKS_PER_DAY);
}

static const UTTestCase ut_cases[] = {
   {"BIOS ticks", reads_the_bios_tick_counter}
};
