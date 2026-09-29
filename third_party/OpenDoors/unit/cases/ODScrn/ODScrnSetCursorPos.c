#define UT_CUSTOM_MOCK_ODSessionScreenIsEmulating
#define UT_CUSTOM_MOCK_ODSessionScreenSetCursorPos
#define UT_CUSTOM_MOCK_ODScrnUpdateCaretPos
static BOOL ut_emulating;
static unsigned ut_session_calls;
static unsigned ut_caret_calls;
BOOL utm_ODSessionScreenIsEmulating(void) { return ut_emulating; }
void utm_ODSessionScreenSetCursorPos(INT column, INT row)
{ ++ut_session_calls; UT_ASSERT_EQ_INT(9, column); UT_ASSERT_EQ_INT(8, row); }
void utm_ODScrnUpdateCaretPos(void) { ++ut_caret_calls; }
static void forwards_or_clamps_the_cursor(void)
{
   ut_emulating = TRUE; ut_session_calls = ut_caret_calls = 0;
   utt_ODScrnSetCursorPos(9, 8);
   UT_ASSERT_EQ_UINT(1, ut_session_calls); UT_ASSERT_EQ_UINT(0, ut_caret_calls);
   ut_emulating = FALSE;
   btLeftBoundary = 2; btRightBoundary = 6; btTopBoundary = 3; btBottomBoundary = 5;
   utt_ODScrnSetCursorPos(9, 8);
   UT_ASSERT_EQ_UINT(4, btCursorColumn); UT_ASSERT_EQ_UINT(2, btCursorRow);
   utt_ODScrnSetCursorPos(2, 2);
   UT_ASSERT_EQ_UINT(1, btCursorColumn); UT_ASSERT_EQ_UINT(1, btCursorRow);
   UT_ASSERT_EQ_UINT(2, ut_caret_calls);
}
static const UTTestCase ut_cases[] = {{"cursor position", forwards_or_clamps_the_cursor}};
