#define UT_CUSTOM_MOCK_GetParent
#define UT_CUSTOM_MOCK_ODFrameGetUsedClientAtTop
#define UT_CUSTOM_MOCK_ODFrameGetUsedClientAtBottom
#define UT_CUSTOM_MOCK_GetClientRect
#define UT_CUSTOM_MOCK_GetWindowRect
#define UT_CUSTOM_MOCK_SetWindowPos

static HWND ut_screen = (HWND)1;
static HWND ut_frame = (HWND)2;
static unsigned ut_client_calls;
static unsigned ut_window_calls;
static unsigned ut_position_calls;

HWND WINAPI utm_GetParent(HWND window)
{
   UT_ASSERT(window == ut_screen); return ut_frame;
}

INT utm_ODFrameGetUsedClientAtTop(HWND frame)
{
   UT_ASSERT(frame == ut_frame); return 11;
}

INT utm_ODFrameGetUsedClientAtBottom(HWND frame)
{
   UT_ASSERT(frame == ut_frame); return 13;
}

BOOL WINAPI utm_GetClientRect(HWND window, LPRECT rectangle)
{
   ++ut_client_calls; UT_ASSERT_NOT_NULL(rectangle);
   memset(rectangle, 0, sizeof(*rectangle));
   if(window == ut_screen)
   {
      rectangle->right = 800; rectangle->bottom = 400;
   }
   else
   {
      UT_ASSERT(window == ut_frame);
      rectangle->right = 650; rectangle->bottom = 450;
   }
   return TRUE;
}

BOOL WINAPI utm_GetWindowRect(HWND window, LPRECT rectangle)
{
   ++ut_window_calls; UT_ASSERT_NOT_NULL(rectangle);
   memset(rectangle, 0, sizeof(*rectangle));
   if(window == ut_screen)
   {
      rectangle->right = 810; rectangle->bottom = 430;
   }
   else
   {
      UT_ASSERT(window == ut_frame);
      rectangle->right = 670; rectangle->bottom = 490;
   }
   return TRUE;
}

BOOL WINAPI utm_SetWindowPos(HWND window, HWND insert_after, int x,
   int y, int width, int height, UINT flags)
{
   UT_ASSERT(insert_after == NULL); ++ut_position_calls;
   if(window == ut_screen)
   {
      UT_ASSERT_EQ_INT(0, x); UT_ASSERT_EQ_INT(11, y);
      UT_ASSERT_EQ_INT(650, width); UT_ASSERT_EQ_INT(430, height);
      UT_ASSERT_EQ_UINT(SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER, flags);
   }
   else
   {
      UT_ASSERT(window == ut_frame); UT_ASSERT_EQ_INT(0, x);
      UT_ASSERT_EQ_INT(0, y); UT_ASSERT_EQ_INT(670, width);
      UT_ASSERT_EQ_INT(494, height);
      UT_ASSERT_EQ_UINT(SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE |
         SWP_NOZORDER, flags);
   }
   return TRUE;
}

static void sizes_the_screen_and_frame_from_font_and_nonclient_dimensions(void)
{
   ut_client_calls = ut_window_calls = ut_position_calls = 0;
   hwndScreenWindow = ut_screen; nFontCellWidth = 8; nFontCellHeight = 16;
   utt_ODScrnAdjustWindows();
   UT_ASSERT_EQ_UINT(2, ut_client_calls); UT_ASSERT_EQ_UINT(2, ut_window_calls);
   UT_ASSERT_EQ_UINT(2, ut_position_calls);
}

static const UTTestCase ut_cases[] = {
   {"sizes", sizes_the_screen_and_frame_from_font_and_nonclient_dimensions}
};
