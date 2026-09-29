#define UT_CUSTOM_MOCK_ODSessionScreenIsEmulating
#define UT_CUSTOM_MOCK_ODSessionScreenSetAttribute
static BOOL ut_emulating;
static unsigned ut_session_calls;
BOOL utm_ODSessionScreenIsEmulating(void) { return ut_emulating; }
void utm_ODSessionScreenSetAttribute(BYTE attribute)
{ ++ut_session_calls; UT_ASSERT_EQ_UINT(0x2e, attribute); }
static void forwards_or_stores_the_attribute(void)
{
   btCurrentAttribute = 7; ut_session_calls = 0; ut_emulating = TRUE;
   utt_ODScrnSetAttribute(0x2e);
   UT_ASSERT_EQ_UINT(1, ut_session_calls); UT_ASSERT_EQ_UINT(7, btCurrentAttribute);
   ut_emulating = FALSE; utt_ODScrnSetAttribute(0x2e);
   UT_ASSERT_EQ_UINT(0x2e, btCurrentAttribute);
}
static const UTTestCase ut_cases[] = {{"display attribute", forwards_or_stores_the_attribute}};
