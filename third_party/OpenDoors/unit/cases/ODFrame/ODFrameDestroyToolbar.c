#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_DestroyWindow
static HWND ut_destroyed[3]; static unsigned ut_destroy_calls;
BOOL WINAPI utm_DestroyWindow(HWND window)
{ ut_destroyed[ut_destroy_calls++] = window; return(TRUE); }
static void destroys_controls_in_dependency_order(void)
{
   tODFrameWindowInfo info; memset(&info, 0, sizeof(info));
   info.hwndTimeUpDown = (HWND)(UINT_PTR)1;
   info.hwndTimeEdit = (HWND)(UINT_PTR)2;
   ut_destroy_calls = 0;
   utt_ODFrameDestroyToolbar((HWND)(UINT_PTR)3, &info);
   UT_ASSERT_EQ_UINT(3, ut_destroy_calls);
   UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1, ut_destroyed[0]);
   UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)2, ut_destroyed[1]);
   UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)3, ut_destroyed[2]);
   UT_ASSERT_NULL(info.hwndTimeUpDown); UT_ASSERT_NULL(info.hwndTimeEdit);
}
static const UTTestCase ut_cases[] = {{"destroy children", destroys_controls_in_dependency_order}};
#endif
