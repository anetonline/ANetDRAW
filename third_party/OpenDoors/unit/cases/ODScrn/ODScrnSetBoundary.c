#define UT_CUSTOM_MOCK_ODSessionScreenIsEmulating
#define UT_CUSTOM_MOCK_ODSessionScreenSetBoundary
#define UT_CUSTOM_MOCK_ODScrnUpdateCaretPos

static BOOL ut_emulating;
static unsigned ut_session_calls;
static unsigned ut_caret_calls;

BOOL utm_ODSessionScreenIsEmulating(void) { return ut_emulating; }
void utm_ODSessionScreenSetBoundary(INT left, INT top, INT right, INT bottom)
{
   ++ut_session_calls;
   UT_ASSERT_EQ_INT(2, left); UT_ASSERT_EQ_INT(3, top);
   UT_ASSERT_EQ_INT(8, right); UT_ASSERT_EQ_INT(9, bottom);
}
void utm_ODScrnUpdateCaretPos(void) { ++ut_caret_calls; }

static void forwards_during_session_emulation(void)
{
   ut_emulating = TRUE; ut_session_calls = ut_caret_calls = 0;
   utt_ODScrnSetBoundary(2, 3, 8, 9);
   UT_ASSERT_EQ_UINT(1, ut_session_calls);
   UT_ASSERT_EQ_UINT(0, ut_caret_calls);
}

static void stores_boundaries_and_clamps_each_cursor_edge(void)
{
   ut_emulating = FALSE; ut_session_calls = ut_caret_calls = 0;
   btCursorColumn = 20; btCursorRow = 20;
   utt_ODScrnSetBoundary(2, 3, 8, 9);
   UT_ASSERT_EQ_UINT(1, btLeftBoundary); UT_ASSERT_EQ_UINT(7, btRightBoundary);
   UT_ASSERT_EQ_UINT(2, btTopBoundary); UT_ASSERT_EQ_UINT(8, btBottomBoundary);
   UT_ASSERT_EQ_UINT(6, btCursorColumn); UT_ASSERT_EQ_UINT(6, btCursorRow);

   btCursorColumn = 0; btCursorRow = 0;
   utt_ODScrnSetBoundary(2, 3, 8, 9);
   UT_ASSERT_EQ_UINT(1, btCursorColumn); UT_ASSERT_EQ_UINT(2, btCursorRow);

   btCursorColumn = 3; btCursorRow = 4;
   utt_ODScrnSetBoundary(2, 3, 8, 9);
   UT_ASSERT_EQ_UINT(3, btCursorColumn); UT_ASSERT_EQ_UINT(4, btCursorRow);
   UT_ASSERT_EQ_UINT(3, ut_caret_calls);
}

static const UTTestCase ut_cases[] = {
   {"session forwarding", forwards_during_session_emulation},
   {"local boundary", stores_boundaries_and_clamps_each_cursor_edge}
};
