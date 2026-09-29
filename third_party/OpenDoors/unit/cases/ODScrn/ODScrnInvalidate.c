#define UT_CUSTOM_MOCK_InvalidateRect

static unsigned ut_invalidate_calls;

BOOL WINAPI utm_InvalidateRect(HWND window, const RECT *rectangle,
   BOOL erase)
{
   ++ut_invalidate_calls; UT_ASSERT(window == hwndScreenWindow);
   UT_ASSERT(rectangle != NULL);
   UT_ASSERT_EQ_INT(16, rectangle->left); UT_ASSERT_EQ_INT(48, rectangle->top);
   UT_ASSERT_EQ_INT(40, rectangle->right); UT_ASSERT_EQ_INT(96, rectangle->bottom);
   UT_ASSERT_EQ_INT(FALSE, erase);
   return TRUE;
}

static void only_marks_the_owner_screen_dirty(void)
{
   nFontCellWidth = 8; nFontCellHeight = 16; ut_invalidate_calls = 0;
   hwndScreenWindow = NULL;
   utt_ODScrnInvalidate(2, 3, 4, 5);
   UT_ASSERT_EQ_UINT(0, ut_invalidate_calls);
   hwndScreenWindow = (HWND)1;
   utt_ODScrnInvalidate(2, 3, 4, 5);
   UT_ASSERT_EQ_UINT(0, ut_invalidate_calls);
}

static const UTTestCase ut_cases[] = {
   {"deferred invalidation", only_marks_the_owner_screen_dirty}
};
