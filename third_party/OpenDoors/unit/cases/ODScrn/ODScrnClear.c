#define UT_CUSTOM_MOCK_ODSessionScreenIsEmulating
#define UT_CUSTOM_MOCK_ODSessionScreenClear
#define UT_CUSTOM_MOCK_ODScrnUpdateCaretPos
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODScrnInvalidate
#endif
static BOOL ut_emulating;
static WORD ut_screen[2000];
static unsigned ut_session_calls;
static unsigned ut_caret_calls;
static unsigned ut_invalidate_calls;
BOOL utm_ODSessionScreenIsEmulating(void) { return ut_emulating; }
void utm_ODSessionScreenClear(void) { ++ut_session_calls; }
void utm_ODScrnUpdateCaretPos(void) { ++ut_caret_calls; }
#ifdef ODPLAT_WIN32
void utm_ODScrnInvalidate(BYTE left, BYTE top, BYTE right, BYTE bottom)
{
   ++ut_invalidate_calls;
   UT_ASSERT_EQ_UINT(1, left); UT_ASSERT_EQ_UINT(1, top);
   UT_ASSERT_EQ_UINT(2, right); UT_ASSERT_EQ_UINT(2, bottom);
}
#endif
static void forwards_or_clears_the_fixed_window(void)
{
   unsigned index;
   for(index = 0; index < DIM(ut_screen); ++index) ut_screen[index] = 0x5555;
   pScrnBuffer = ut_screen; btLeftBoundary = 1; btTopBoundary = 1;
   btRightBoundary = 2; btBottomBoundary = 2; btCurrentAttribute = 0x2e;
   btCursorColumn = 1; btCursorRow = 1;
   ut_session_calls = ut_caret_calls = ut_invalidate_calls = 0;
   ut_emulating = TRUE; utt_ODScrnClear();
   UT_ASSERT_EQ_UINT(1, ut_session_calls); UT_ASSERT_EQ_UINT(0x5555, ut_screen[81]);
   ut_emulating = FALSE; utt_ODScrnClear();
   UT_ASSERT_EQ_UINT(0x2e20, ut_screen[81]); UT_ASSERT_EQ_UINT(0x2e20, ut_screen[82]);
   UT_ASSERT_EQ_UINT(0x2e20, ut_screen[161]); UT_ASSERT_EQ_UINT(0x2e20, ut_screen[162]);
   UT_ASSERT_EQ_UINT(0x5555, ut_screen[83]);
   UT_ASSERT_EQ_UINT(0, btCursorColumn); UT_ASSERT_EQ_UINT(0, btCursorRow);
   UT_ASSERT_EQ_UINT(1, ut_caret_calls);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(1, ut_invalidate_calls);
#endif
}
static const UTTestCase ut_cases[] = {{"clear window", forwards_or_clears_the_fixed_window}};
