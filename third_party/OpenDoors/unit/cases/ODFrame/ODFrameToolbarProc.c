#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_GetParent
#define UT_CUSTOM_MOCK_SendMessageA
#define UT_CUSTOM_MOCK_CallWindowProcA
static unsigned ut_forward_calls; static UINT ut_expected_message;
HWND WINAPI utm_GetParent(HWND window)
{ UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1, window); return((HWND)(UINT_PTR)2); }
LRESULT WINAPI utm_SendMessageA(HWND window, UINT message, WPARAM first, LPARAM second)
{
   UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)2, window); UT_ASSERT_EQ_UINT(ut_expected_message, message);
   UT_ASSERT_EQ_UINT(3, first); UT_ASSERT_EQ_INT(4, second); ++ut_forward_calls; return(0);
}
LRESULT WINAPI utm_CallWindowProcA(WNDPROC procedure, HWND window, UINT message,
   WPARAM first, LPARAM second)
{
   UT_ASSERT_EQ_PTR(pfnDefToolbarProc, procedure); UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1, window);
   UT_ASSERT_EQ_UINT(ut_expected_message, message); UT_ASSERT_EQ_UINT(3, first);
   UT_ASSERT_EQ_INT(4, second); return(99);
}
static void forwards_commands_and_scrolls_only(void)
{
   static const UINT messages[] = {WM_COMMAND, WM_VSCROLL, WM_PAINT}; unsigned i;
   pfnDefToolbarProc = (WNDPROC)(UINT_PTR)0x1234;
   for(i=0;i<3;++i) { ut_expected_message=messages[i]; ut_forward_calls=0;
      UT_ASSERT_EQ_INT(99, utt_ODFrameToolbarProc((HWND)(UINT_PTR)1,messages[i],3,4));
      UT_ASSERT_EQ_UINT(i<2?1:0,ut_forward_calls); }
}
static const UTTestCase ut_cases[] = {{"message forwarding", forwards_commands_and_scrolls_only}};
#endif
