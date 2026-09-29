#define UT_CUSTOM_MOCK_SetFocus
static unsigned ut_focus_calls;
HWND WINAPI utm_SetFocus(HWND window)
{
   ++ut_focus_calls; UT_ASSERT(window == hwndScreenWindow); return NULL;
}
static void focuses_only_an_existing_screen_window(void)
{
   ut_focus_calls = 0; hwndScreenWindow = NULL;
   utt_ODScrnSetFocusToWindow(); UT_ASSERT_EQ_UINT(0, ut_focus_calls);
   hwndScreenWindow = (HWND)1;
   utt_ODScrnSetFocusToWindow(); UT_ASSERT_EQ_UINT(1, ut_focus_calls);
}
static const UTTestCase ut_cases[] = {
   {"screen focus", focuses_only_an_existing_screen_window}
};
