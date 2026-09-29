#define UT_CUSTOM_MOCK_GetWindowLongPtrA
#define UT_CUSTOM_MOCK_SetWindowLongPtrA
#define UT_CUSTOM_MOCK_GetWindowLongA
#define UT_CUSTOM_MOCK_SetWindowLongA
#define UT_CUSTOM_MOCK_GetParent
#define UT_CUSTOM_MOCK_PostMessageA
#define UT_CUSTOM_MOCK_DefWindowProcA
#define UT_CUSTOM_MOCK_BeginPaint
#define UT_CUSTOM_MOCK_EndPaint
#define UT_CUSTOM_MOCK_ODScrnPaint
#define UT_CUSTOM_MOCK_ODScrnSetWinCaretPos
#define UT_CUSTOM_MOCK_SetFocus
#define UT_CUSTOM_MOCK_CreateCaret
#define UT_CUSTOM_MOCK_DestroyCaret
#define UT_CUSTOM_MOCK_ODKrnlHandleLocalKey
#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock

static HWND ut_window = (HWND)1;
static HWND ut_frame = (HWND)2;
static HINSTANCE ut_instance = (HINSTANCE)3;
static HDC ut_dc = (HDC)4;
static BOOL ut_begin_fails;
static unsigned ut_post_calls;
static unsigned ut_default_calls;
static unsigned ut_set_long_calls;
static unsigned ut_begin_calls;
static unsigned ut_end_calls;
static unsigned ut_paint_calls;
static unsigned ut_caret_position_calls;
static unsigned ut_focus_calls;
static unsigned ut_create_caret_calls;
static unsigned ut_destroy_caret_calls;
static unsigned ut_lock_calls;
static unsigned ut_unlock_calls;
static WORD ut_keys[4];
static unsigned ut_key_calls;

LONG_PTR WINAPI utm_GetWindowLongPtrA(HWND window, int index)
{
   UT_ASSERT(window == ut_window); UT_ASSERT_EQ_INT(GWLP_USERDATA, index);
   return (LONG_PTR)ut_instance;
}

LONG WINAPI utm_GetWindowLongA(HWND window, int index)
{
   return (LONG)utm_GetWindowLongPtrA(window, index);
}

LONG_PTR WINAPI utm_SetWindowLongPtrA(HWND window, int index, LONG_PTR value)
{
   ++ut_set_long_calls; UT_ASSERT(window == ut_window);
   UT_ASSERT_EQ_INT(GWLP_USERDATA, index);
   UT_ASSERT((HINSTANCE)value == ut_instance); return 0;
}

LONG WINAPI utm_SetWindowLongA(HWND window, int index, LONG value)
{
   return (LONG)utm_SetWindowLongPtrA(window, index, (LONG_PTR)value);
}

HWND WINAPI utm_GetParent(HWND window)
{
   UT_ASSERT(window == ut_window); return ut_frame;
}

BOOL WINAPI utm_PostMessageA(HWND window, UINT message, WPARAM wparam,
   LPARAM lparam)
{
   ++ut_post_calls; UT_ASSERT(window == ut_frame);
   UT_ASSERT_EQ_UINT(WM_SYSCOMMAND, message);
   UT_ASSERT_EQ_UINT(SC_KEYMENU, wparam); UT_ASSERT_EQ_INT(29, lparam);
   return TRUE;
}

LRESULT WINAPI utm_DefWindowProcA(HWND window, UINT message, WPARAM wparam,
   LPARAM lparam)
{
   ++ut_default_calls; UT_ASSERT(window == ut_window);
   (void)message; (void)wparam; (void)lparam; return 77;
}

HDC WINAPI utm_BeginPaint(HWND window, LPPAINTSTRUCT paint)
{
   ++ut_begin_calls; UT_ASSERT(window == ut_window); UT_ASSERT_NOT_NULL(paint);
   memset(paint, 0, sizeof(*paint));
   paint->rcPaint.left = 16; paint->rcPaint.top = 32;
   paint->rcPaint.right = 40; paint->rcPaint.bottom = 64;
   return ut_begin_fails ? NULL : ut_dc;
}

BOOL WINAPI utm_EndPaint(HWND window, const PAINTSTRUCT *paint)
{
   ++ut_end_calls; UT_ASSERT(window == ut_window); UT_ASSERT_NOT_NULL(paint);
   return TRUE;
}

void utm_ODScrnPaint(HDC dc, INT left, INT top, INT right, INT bottom)
{
   ++ut_paint_calls; UT_ASSERT(dc == ut_dc);
   UT_ASSERT_EQ_INT(2, left); UT_ASSERT_EQ_INT(2, top);
   UT_ASSERT_EQ_INT(5, right); UT_ASSERT_EQ_INT(4, bottom);
}

void utm_ODScrnSetWinCaretPos(void) { ++ut_caret_position_calls; }

HWND WINAPI utm_SetFocus(HWND window)
{
   ++ut_focus_calls; UT_ASSERT(window == ut_window); return window;
}

BOOL WINAPI utm_CreateCaret(HWND window, HBITMAP bitmap, int width,
   int height)
{
   ++ut_create_caret_calls; UT_ASSERT(window == ut_window);
   UT_ASSERT(bitmap == NULL); UT_ASSERT_EQ_INT(8, width);
   UT_ASSERT_EQ_INT(CARET_HEIGHT, height); return TRUE;
}

void utm_ODMutexLock(tODMutex *mutex)
{
   ++ut_lock_calls; UT_ASSERT(mutex == &ScreenPresentationMutex);
}

void utm_ODMutexUnlock(tODMutex *mutex)
{
   ++ut_unlock_calls; UT_ASSERT(mutex == &ScreenPresentationMutex);
}

BOOL WINAPI utm_DestroyCaret(void)
{
   ++ut_destroy_caret_calls; return TRUE;
}

void utm_ODKrnlHandleLocalKey(WORD key)
{
   UT_ASSERT(ut_key_calls < 4); ut_keys[ut_key_calls++] = key;
}

static void reset_window_proc(void)
{
   memset(ut_keys, 0, sizeof(ut_keys));
   ut_begin_fails = FALSE; ut_post_calls = ut_default_calls = 0;
   ut_set_long_calls = ut_begin_calls = ut_end_calls = ut_paint_calls = 0;
   ut_caret_position_calls = ut_focus_calls = ut_create_caret_calls = 0;
   ut_destroy_caret_calls = ut_key_calls = 0;
   ut_lock_calls = ut_unlock_calls = 0;
   bScreenHasFocus = FALSE; bWinCaretShown = TRUE;
   hwndScreenWindow = ut_window;
   nFontCellWidth = 8; nFontCellHeight = 16;
}

static void relays_only_the_keyboard_menu_system_command(void)
{
   reset_window_proc();
   UT_ASSERT_EQ_INT(0, utt_ODScrnWindowProc(ut_window, WM_SYSCOMMAND,
      SC_KEYMENU, 29));
   UT_ASSERT_EQ_UINT(1, ut_post_calls); UT_ASSERT_EQ_UINT(0, ut_default_calls);
   reset_window_proc();
   UT_ASSERT_EQ_INT(77, utt_ODScrnWindowProc(ut_window, WM_SYSCOMMAND,
      SC_CLOSE, 30));
   UT_ASSERT_EQ_UINT(0, ut_post_calls); UT_ASSERT_EQ_UINT(1, ut_default_calls);
}

static void stores_the_instance_from_the_create_structure(void)
{
   CREATESTRUCT create_structure;
   reset_window_proc(); memset(&create_structure, 0, sizeof(create_structure));
   create_structure.lpCreateParams = ut_instance;
   UT_ASSERT_EQ_INT(0, utt_ODScrnWindowProc(ut_window, WM_CREATE, 0,
      (LPARAM)&create_structure));
   UT_ASSERT_EQ_UINT(1, ut_set_long_calls);
}

static void paints_only_when_begin_paint_returns_a_context(void)
{
   reset_window_proc(); ut_begin_fails = TRUE;
   UT_ASSERT_EQ_INT(0, utt_ODScrnWindowProc(ut_window, WM_PAINT, 0, 0));
   UT_ASSERT_EQ_UINT(1, ut_begin_calls); UT_ASSERT_EQ_UINT(0, ut_paint_calls);
   UT_ASSERT_EQ_UINT(0, ut_end_calls);
   reset_window_proc();
   UT_ASSERT_EQ_INT(0, utt_ODScrnWindowProc(ut_window, WM_PAINT, 0, 0));
   UT_ASSERT_EQ_UINT(1, ut_paint_calls); UT_ASSERT_EQ_UINT(1, ut_end_calls);
}

static void handles_mouse_focus_and_focus_transitions(void)
{
   reset_window_proc();
   utt_ODScrnWindowProc(ut_window, WM_LBUTTONDOWN, 0, 0);
   UT_ASSERT_EQ_UINT(1, ut_focus_calls);
   utt_ODScrnWindowProc(ut_window, WM_SETFOCUS, 0, 0);
   UT_ASSERT(bScreenHasFocus); UT_ASSERT_EQ_UINT(1, ut_create_caret_calls);
   UT_ASSERT_EQ_INT(FALSE, bWinCaretShown);
   UT_ASSERT_EQ_UINT(1, ut_caret_position_calls);
   UT_ASSERT_EQ_UINT(1, ut_lock_calls); UT_ASSERT_EQ_UINT(1, ut_unlock_calls);
   utt_ODScrnWindowProc(ut_window, WM_KILLFOCUS, 0, 0);
   UT_ASSERT(!bScreenHasFocus); UT_ASSERT_EQ_UINT(1, ut_destroy_caret_calls);
}

static void ignores_unmapped_and_zero_repeat_relayed_keys(void)
{
   reset_window_proc();
   utt_ODScrnWindowProc(ut_window, WM_KEYDOWN, 0xffff, 2);
   UT_ASSERT_EQ_UINT(0, ut_key_calls);
   utt_ODScrnWindowProc(ut_window, WM_KEYDOWN, VK_LEFT, 0);
   UT_ASSERT_EQ_UINT(0, ut_key_calls);
}

static void queues_each_repeat_of_a_mapped_relayed_key(void)
{
   reset_window_proc();
   utt_ODScrnWindowProc(ut_window, WM_KEYDOWN, VK_LEFT, 2);
   UT_ASSERT_EQ_UINT(2, ut_key_calls);
   UT_ASSERT_EQ_UINT(MAKEWORD(0, OD_KEY_LEFT), ut_keys[0]);
   UT_ASSERT_EQ_UINT(MAKEWORD(0, OD_KEY_LEFT), ut_keys[1]);
}

static void queues_each_character_with_its_scan_code(void)
{
   LPARAM parameters;
   reset_window_proc(); parameters = (LPARAM)((0x2aUL << 16) | 2UL);
   utt_ODScrnWindowProc(ut_window, WM_CHAR, 'Q', parameters);
   UT_ASSERT_EQ_UINT(2, ut_key_calls);
   UT_ASSERT_EQ_UINT(MAKEWORD('Q', 0x2a), ut_keys[0]);
   UT_ASSERT_EQ_UINT(MAKEWORD('Q', 0x2a), ut_keys[1]);
   reset_window_proc(); parameters = (LPARAM)(0x2aUL << 16);
   utt_ODScrnWindowProc(ut_window, WM_CHAR, 'Q', parameters);
   UT_ASSERT_EQ_UINT(0, ut_key_calls);
}

static void delegates_unrecognized_messages(void)
{
   reset_window_proc();
   UT_ASSERT_EQ_INT(77, utt_ODScrnWindowProc(ut_window, WM_USER + 20, 5, 6));
   UT_ASSERT_EQ_UINT(1, ut_default_calls);
}

static void clears_only_the_published_screen_handle_when_destroyed(void)
{
   reset_window_proc();
   utt_ODScrnWindowProc(ut_window, WM_DESTROY, 0, 0);
   UT_ASSERT_EQ_PTR(NULL, hwndScreenWindow);
   UT_ASSERT_EQ_UINT(1, ut_lock_calls); UT_ASSERT_EQ_UINT(1, ut_unlock_calls);

   reset_window_proc(); hwndScreenWindow = (HWND)9;
   utt_ODScrnWindowProc(ut_window, WM_DESTROY, 0, 0);
   UT_ASSERT(hwndScreenWindow == (HWND)9);
}

static const UTTestCase ut_cases[] = {
   {"system command", relays_only_the_keyboard_menu_system_command},
   {"create", stores_the_instance_from_the_create_structure},
   {"paint", paints_only_when_begin_paint_returns_a_context},
   {"focus", handles_mouse_focus_and_focus_transitions},
   {"ignored key", ignores_unmapped_and_zero_repeat_relayed_keys},
   {"mapped key", queues_each_repeat_of_a_mapped_relayed_key},
   {"character", queues_each_character_with_its_scan_code},
   {"destroy", clears_only_the_published_screen_handle_when_destroyed},
   {"default", delegates_unrecognized_messages}
};
