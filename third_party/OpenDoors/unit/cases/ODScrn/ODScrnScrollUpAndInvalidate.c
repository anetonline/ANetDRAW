#define UT_CUSTOM_MOCK_ODScrnScrollUpOneLine
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODScrnInvalidate
#endif
static unsigned ut_scroll_calls;
static unsigned ut_invalidate_calls;
void utm_ODScrnScrollUpOneLine(void) { ++ut_scroll_calls; }
#ifdef ODPLAT_WIN32
void utm_ODScrnInvalidate(BYTE left, BYTE top, BYTE right, BYTE bottom)
{
   ++ut_invalidate_calls;
   UT_ASSERT_EQ_UINT(1, left); UT_ASSERT_EQ_UINT(2, top);
   UT_ASSERT_EQ_UINT(3, right); UT_ASSERT_EQ_UINT(4, bottom);
}
#endif
static void scrolls_and_invalidates_only_when_enabled(void)
{
   btLeftBoundary = 1; btTopBoundary = 2; btRightBoundary = 3; btBottomBoundary = 4;
   ut_scroll_calls = ut_invalidate_calls = 0;
   bScrollEnabled = FALSE; utt_ODScrnScrollUpAndInvalidate();
   UT_ASSERT_EQ_UINT(0, ut_scroll_calls); UT_ASSERT_EQ_UINT(0, ut_invalidate_calls);
   bScrollEnabled = TRUE; utt_ODScrnScrollUpAndInvalidate();
   UT_ASSERT_EQ_UINT(1, ut_scroll_calls);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(1, ut_invalidate_calls);
#else
   UT_ASSERT_EQ_UINT(0, ut_invalidate_calls);
#endif
}
static const UTTestCase ut_cases[] = {{"scroll gate", scrolls_and_invalidates_only_when_enabled}};
