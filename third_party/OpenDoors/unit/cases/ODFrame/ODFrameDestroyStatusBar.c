#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_DestroyWindow
static HWND ut_destroyed;
BOOL WINAPI utm_DestroyWindow(HWND window)
{ ut_destroyed = window; return(TRUE); }
static void destroys_the_supplied_window(void)
{
   HWND window = (HWND)(UINT_PTR)0x1234;
   ut_destroyed = NULL; utt_ODFrameDestroyStatusBar(window);
   UT_ASSERT_EQ_PTR(window, ut_destroyed);
}
static const UTTestCase ut_cases[] = {
   {"destroy", destroys_the_supplied_window}
};
#endif
