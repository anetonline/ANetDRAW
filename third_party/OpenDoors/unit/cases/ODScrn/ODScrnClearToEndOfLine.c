#define UT_CUSTOM_MOCK_ODSessionScreenIsEmulating
#define UT_CUSTOM_MOCK_ODSessionScreenClearToEndOfLine
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODScrnInvalidate
#endif
static BOOL ut_emulating;
static BYTE ut_screen[40];
static unsigned ut_session_calls;
static unsigned ut_invalidate_calls;
BOOL utm_ODSessionScreenIsEmulating(void) { return ut_emulating; }
void utm_ODSessionScreenClearToEndOfLine(void) { ++ut_session_calls; }
#ifdef ODPLAT_WIN32
void utm_ODScrnInvalidate(BYTE left, BYTE top, BYTE right, BYTE bottom)
{
   ++ut_invalidate_calls;
   UT_ASSERT_EQ_UINT(2, left); UT_ASSERT_EQ_UINT(0, top);
   UT_ASSERT_EQ_UINT(4, right); UT_ASSERT_EQ_UINT(0, bottom);
}
#endif
static void forwards_or_clears_the_local_line_tail(void)
{
   unsigned index;
   for(index = 0; index < sizeof(ut_screen); ++index) ut_screen[index] = 0x55;
   pScrnBuffer = ut_screen;
   btLeftBoundary = 1; btRightBoundary = 4; btTopBoundary = 0;
   btCursorColumn = 1; btCursorRow = 0; btCurrentAttribute = 0x2e;
   ut_session_calls = ut_invalidate_calls = 0; ut_emulating = TRUE;
   utt_ODScrnClearToEndOfLine();
   UT_ASSERT_EQ_UINT(1, ut_session_calls); UT_ASSERT_EQ_UINT(0x55, ut_screen[12]);
   ut_emulating = FALSE; utt_ODScrnClearToEndOfLine();
   UT_ASSERT_EQ_UINT(' ', ut_screen[4]); UT_ASSERT_EQ_UINT(0x2e, ut_screen[5]);
   UT_ASSERT_EQ_UINT(' ', ut_screen[6]); UT_ASSERT_EQ_UINT(0x2e, ut_screen[7]);
   UT_ASSERT_EQ_UINT(0x55, ut_screen[8]);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(1, ut_invalidate_calls);
#endif
}
static const UTTestCase ut_cases[] = {{"line tail", forwards_or_clears_the_local_line_tail}};
