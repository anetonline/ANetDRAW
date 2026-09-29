#define UT_CUSTOM_MOCK_SetCaretPos
#define UT_CUSTOM_MOCK_ShowCaret
#define UT_CUSTOM_MOCK_HideCaret

static HWND ut_screen = (HWND)1;
static unsigned ut_set_calls;
static unsigned ut_show_calls;
static unsigned ut_hide_calls;

BOOL WINAPI utm_SetCaretPos(int x, int y)
{
   ++ut_set_calls; UT_ASSERT_EQ_INT(40, x); UT_ASSERT_EQ_INT(109, y);
   return TRUE;
}

BOOL WINAPI utm_ShowCaret(HWND window)
{
   ++ut_show_calls; UT_ASSERT(window == ut_screen); return TRUE;
}

BOOL WINAPI utm_HideCaret(HWND window)
{
   ++ut_hide_calls; UT_ASSERT(window == ut_screen); return TRUE;
}

static void reset_caret(void)
{
   nFontCellWidth = 8; nFontCellHeight = 16;
   btDisplayCursorColumn = 5; btDisplayCursorRow = 6;
   hwndScreenWindow = ut_screen; bScreenHasFocus = FALSE;
   bDisplayCaretOn = FALSE; bWinCaretShown = FALSE;
   ut_set_calls = ut_show_calls = ut_hide_calls = 0;
}

static void ignores_the_caret_without_focus(void)
{
   reset_caret(); utt_ODScrnSetWinCaretPos();
   UT_ASSERT_EQ_UINT(0, ut_set_calls); UT_ASSERT_EQ_UINT(0, ut_show_calls);
}

static void positions_and_shows_the_published_caret(void)
{
   reset_caret(); bScreenHasFocus = TRUE; bDisplayCaretOn = TRUE;
   utt_ODScrnSetWinCaretPos();
   UT_ASSERT_EQ_UINT(1, ut_set_calls); UT_ASSERT_EQ_UINT(1, ut_show_calls);
   UT_ASSERT_EQ_INT(TRUE, bWinCaretShown);
   utt_ODScrnSetWinCaretPos();
   UT_ASSERT_EQ_UINT(2, ut_set_calls); UT_ASSERT_EQ_UINT(1, ut_show_calls);
}

static void hides_a_caret_disabled_in_the_published_state(void)
{
   reset_caret(); bScreenHasFocus = TRUE;
   utt_ODScrnSetWinCaretPos();
   UT_ASSERT_EQ_UINT(0, ut_hide_calls);

   bWinCaretShown = TRUE;
   utt_ODScrnSetWinCaretPos();
   UT_ASSERT_EQ_UINT(2, ut_set_calls); UT_ASSERT_EQ_UINT(1, ut_hide_calls);
   UT_ASSERT_EQ_INT(FALSE, bWinCaretShown);
}

static const UTTestCase ut_cases[] = {
   {"unfocused", ignores_the_caret_without_focus},
   {"show", positions_and_shows_the_published_caret},
   {"hide", hides_a_caret_disabled_in_the_published_state}
};
