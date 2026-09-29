static void reports_active_state(void)
{
   bConsoleActive = FALSE;
   UT_ASSERT(!utt_ODConsoleAvailable());
   bConsoleActive = TRUE;
   UT_ASSERT(utt_ODConsoleAvailable());
}
static const UTTestCase ut_cases[] = {{"state", reports_active_state}};
