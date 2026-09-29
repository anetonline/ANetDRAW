#define UT_CUSTOM_MOCK_ODSessionScreenIsEmulating
#define UT_CUSTOM_MOCK_ODSessionScreenDisplayString
#define UT_CUSTOM_MOCK_ODScrnDisplayBuffer
#define UT_CUSTOM_MOCK_strlen
static BOOL ut_emulating;
static unsigned ut_session_calls;
static unsigned ut_buffer_calls;
BOOL utm_ODSessionScreenIsEmulating(void) { return ut_emulating; }
void utm_ODSessionScreenDisplayString(const char *text)
{ ++ut_session_calls; UT_ASSERT_EQ_INT('A', text[0]); }
size_t utm_strlen(const char *text)
{ UT_ASSERT_EQ_INT('A', text[0]); return 3; }
void ODCALL utm_ODScrnDisplayBuffer(const char *buffer, INT count)
{ ++ut_buffer_calls; UT_ASSERT_EQ_INT('A', buffer[0]); UT_ASSERT_EQ_UINT(3, count); }
static void forwards_to_the_active_screen(void)
{
   ut_session_calls = ut_buffer_calls = 0; ut_emulating = TRUE;
   utt_ODScrnDisplayString("ABC");
   UT_ASSERT_EQ_UINT(1, ut_session_calls); UT_ASSERT_EQ_UINT(0, ut_buffer_calls);
   ut_emulating = FALSE; utt_ODScrnDisplayString("ABC");
   UT_ASSERT_EQ_UINT(1, ut_session_calls); UT_ASSERT_EQ_UINT(1, ut_buffer_calls);
}
static const UTTestCase ut_cases[] = {{"display string", forwards_to_the_active_screen}};
