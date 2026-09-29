#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_GetWindowRect
#define UT_CUSTOM_MOCK_SendMessageA
static HWND ut_status;
BOOL WINAPI utm_GetWindowRect(HWND window, LPRECT rectangle)
{
   UT_ASSERT_EQ_PTR(ut_status, window);
   rectangle->left = 11; rectangle->right = 311;
   rectangle->top = 20; rectangle->bottom = 40;
   return(TRUE);
}
LRESULT WINAPI utm_SendMessageA(HWND window, UINT message,
   WPARAM parameter, LPARAM data)
{
   int *widths = (int *)data;
   UT_ASSERT_EQ_PTR(ut_status, window); UT_ASSERT_EQ_UINT(SB_SETPARTS, message);
   UT_ASSERT_EQ_UINT(NUM_STATUS_PARTS, parameter);
   UT_ASSERT_EQ_INT(300 - NODE_PART_WIDTH, widths[0]);
   UT_ASSERT_EQ_INT(-1, widths[1]);
   return(0);
}
static void sizes_the_main_and_node_parts(void)
{
   ut_status = (HWND)(UINT_PTR)0x1234;
   utt_ODFrameSizeStatusBar(ut_status);
}
static const UTTestCase ut_cases[] = {
   {"status parts", sizes_the_main_and_node_parts}
};
#endif
