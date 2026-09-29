#define UT_CUSTOM_MOCK_ODPlatRingBell
static unsigned ut_bell_calls;
void utm_ODPlatRingBell(void) { ++ut_bell_calls; }
static void suppresses_or_emits_the_local_bell(void)
{
   ut_bell_calls = 0;
   od_control.od_silent_mode = TRUE;
   utt_ODScrnRingBell();
   UT_ASSERT_EQ_UINT(0, ut_bell_calls);
   od_control.od_silent_mode = FALSE;
   utt_ODScrnRingBell();
   UT_ASSERT_EQ_UINT(1, ut_bell_calls);
}
static const UTTestCase ut_cases[] = {{"silent and audible", suppresses_or_emits_the_local_bell}};
