#define UT_CUSTOM_MOCK_ODSessionScreenPresent

static unsigned ut_present_calls;

void utm_ODSessionScreenPresent(void)
{
   ++ut_present_calls;
}

static void ends_and_presents_only_when_available(void)
{
   bSessionScreenEmulating = TRUE;
   bSessionScreenAvailable = FALSE;
   ut_present_calls = 0;
   utt_ODSessionScreenEndEmulation();
   UT_ASSERT_EQ_INT(TRUE, bSessionScreenEmulating);
   UT_ASSERT_EQ_UINT(0, ut_present_calls);
   bSessionScreenAvailable = TRUE;
   utt_ODSessionScreenEndEmulation();
   UT_ASSERT_EQ_INT(FALSE, bSessionScreenEmulating);
   UT_ASSERT_EQ_UINT(1, ut_present_calls);
}

static const UTTestCase ut_cases[] = {
   {"end emulation", ends_and_presents_only_when_available}
};
