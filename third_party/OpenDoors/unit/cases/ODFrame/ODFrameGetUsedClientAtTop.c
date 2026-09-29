#ifdef ODPLAT_WIN32
#include "winptr.h"
#define UT_CUSTOM_MOCK_GetWindowRect
static tODFrameWindowInfo ut_info;
UT_WINDOW_LONG_PTR WINAPI UT_GET_WINDOW_LONG_PTR(HWND window, int index)
{ (void)window; UT_ASSERT_EQ_INT(GWLP_USERDATA, index); return((LONG_PTR)&ut_info); }
BOOL WINAPI utm_GetWindowRect(HWND window, LPRECT rectangle)
{
   UT_ASSERT_EQ_PTR(ut_info.hwndToolbar, window);
   rectangle->top = 10; rectangle->bottom = 42; return(TRUE);
}
static void returns_zero_without_a_toolbar(void)
{
   memset(&ut_info, 0, sizeof(ut_info));
   UT_ASSERT_EQ_INT(0, utt_ODFrameGetUsedClientAtTop((HWND)(UINT_PTR)1));
}
static void returns_the_toolbar_height_less_the_border(void)
{
   memset(&ut_info, 0, sizeof(ut_info)); ut_info.bToolbarOn = TRUE;
   ut_info.hwndToolbar = (HWND)(UINT_PTR)2;
   UT_ASSERT_EQ_INT(30, utt_ODFrameGetUsedClientAtTop((HWND)(UINT_PTR)1));
}
static const UTTestCase ut_cases[] = {
   {"toolbar off", returns_zero_without_a_toolbar},
   {"toolbar on", returns_the_toolbar_height_less_the_border}
};
#endif
