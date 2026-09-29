#define UT_CUSTOM_MOCK_LoadCursorA
#define UT_CUSTOM_MOCK_RegisterClassA
#define UT_CUSTOM_MOCK_CreateWindowExA
#define UT_CUSTOM_MOCK_ODScrnWindowProc
#define UT_CUSTOM_MOCK_memset
#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock

static HWND ut_frame = (HWND)1;
static HANDLE ut_instance = (HANDLE)2;
static HCURSOR ut_cursor = (HCURSOR)3;
static HWND ut_created = (HWND)4;
static BOOL ut_create_fails;
static unsigned ut_register_calls;
static unsigned ut_create_calls;
static unsigned ut_lock_calls;
static unsigned ut_unlock_calls;

void utm_ODMutexLock(tODMutex *mutex)
{
   ++ut_lock_calls; UT_ASSERT(mutex == &ScreenPresentationMutex);
}

void utm_ODMutexUnlock(tODMutex *mutex)
{
   ++ut_unlock_calls; UT_ASSERT(mutex == &ScreenPresentationMutex);
}

void *utm_memset(void *destination, int value, size_t size)
{
   unsigned char *bytes = (unsigned char *)destination;
   size_t index;
   for(index = 0; index < size; ++index) bytes[index] = (unsigned char)value;
   return destination;
}

LRESULT CALLBACK utm_ODScrnWindowProc(HWND window, UINT message,
   WPARAM wparam, LPARAM lparam)
{
   (void)window; (void)message; (void)wparam; (void)lparam;
   ut_unexpected_mock(3, "ODScrnWindowProc");
   return 0;
}

HCURSOR WINAPI utm_LoadCursorA(HINSTANCE instance, LPCSTR cursor_name)
{
   UT_ASSERT(instance == NULL); UT_ASSERT(cursor_name == IDC_ARROW);
   return ut_cursor;
}

ATOM WINAPI utm_RegisterClassA(const WNDCLASSA *window_class)
{
   ++ut_register_calls; UT_ASSERT_NOT_NULL(window_class);
   UT_ASSERT_EQ_UINT(CS_HREDRAW | CS_VREDRAW, window_class->style);
   UT_ASSERT(window_class->lpfnWndProc == utm_ODScrnWindowProc);
   UT_ASSERT_EQ_INT(0, window_class->cbClsExtra);
   UT_ASSERT_EQ_INT(0, window_class->cbWndExtra);
   UT_ASSERT(window_class->hInstance == ut_instance);
   UT_ASSERT(window_class->hIcon == NULL); UT_ASSERT(window_class->hCursor == ut_cursor);
   UT_ASSERT(window_class->hbrBackground == NULL);
   UT_ASSERT(window_class->lpszMenuName == NULL);
   UT_ASSERT(strcmp("ODScreen", window_class->lpszClassName) == 0);
   return 1;
}

HWND WINAPI utm_CreateWindowExA(DWORD extended_style, LPCSTR class_name,
   LPCSTR window_name, DWORD style, int x, int y, int width, int height,
   HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter)
{
   ++ut_create_calls; UT_ASSERT_EQ_UINT(WS_EX_CLIENTEDGE, extended_style);
   UT_ASSERT(strcmp("ODScreen", class_name) == 0);
   UT_ASSERT(strcmp("", window_name) == 0);
   UT_ASSERT_EQ_UINT(WS_CHILD | WS_BORDER, style);
   UT_ASSERT_EQ_INT(0, x); UT_ASSERT_EQ_INT(0, y);
   UT_ASSERT_EQ_INT(500, width); UT_ASSERT_EQ_INT(300, height);
   UT_ASSERT(parent == ut_frame); UT_ASSERT(menu == NULL);
   UT_ASSERT(instance == ut_instance); UT_ASSERT(parameter == ut_instance);
   return ut_create_fails ? NULL : ut_created;
}

static void reset_create(void)
{
   ut_create_fails = FALSE; ut_register_calls = ut_create_calls = 0;
   ut_lock_calls = ut_unlock_calls = 0;
   hwndScreenWindow = NULL;
}

static void returns_null_when_window_creation_fails(void)
{
   reset_create(); ut_create_fails = TRUE;
   UT_ASSERT_NULL(utt_ODScrnCreateWin(ut_frame, ut_instance));
   UT_ASSERT(hwndScreenWindow == NULL); UT_ASSERT_EQ_UINT(1, ut_register_calls);
   UT_ASSERT_EQ_UINT(1, ut_create_calls);
   UT_ASSERT_EQ_UINT(0, ut_lock_calls);
}

static void registers_creates_and_publishes_the_screen_window(void)
{
   reset_create();
   UT_ASSERT(utt_ODScrnCreateWin(ut_frame, ut_instance) == ut_created);
   UT_ASSERT(hwndScreenWindow == ut_created);
   UT_ASSERT_EQ_UINT(1, ut_register_calls); UT_ASSERT_EQ_UINT(1, ut_create_calls);
   UT_ASSERT_EQ_UINT(1, ut_lock_calls); UT_ASSERT_EQ_UINT(1, ut_unlock_calls);
}

static const UTTestCase ut_cases[] = {
   {"failure", returns_null_when_window_creation_fails},
   {"success", registers_creates_and_publishes_the_screen_window}
};
