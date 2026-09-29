#define UT_CUSTOM_MOCK_ODSessionScreenIsEmulating
#define UT_CUSTOM_MOCK_ODSessionScreenEnableScrolling
static BOOL ut_emulating;
static unsigned ut_session_calls;
BOOL utm_ODSessionScreenIsEmulating(void) { return ut_emulating; }
void utm_ODSessionScreenEnableScrolling(BOOL enable)
{ ++ut_session_calls; UT_ASSERT_EQ_INT(FALSE, enable); }
static void forwards_or_stores_the_scroll_setting(void)
{
   bScrollEnabled = TRUE; ut_session_calls = 0; ut_emulating = TRUE;
   utt_ODScrnEnableScrolling(FALSE);
   UT_ASSERT_EQ_UINT(1, ut_session_calls); UT_ASSERT_EQ_INT(TRUE, bScrollEnabled);
   ut_emulating = FALSE; utt_ODScrnEnableScrolling(FALSE);
   UT_ASSERT_EQ_INT(FALSE, bScrollEnabled);
}
static const UTTestCase ut_cases[] = {{"scroll setting", forwards_or_stores_the_scroll_setting}};
