#define UT_CUSTOM_MOCK_strlen

size_t utm_strlen(const char *string)
{
   size_t length;
   length = 0;
   while(string[length] != '\0') ++length;
   return length;
}

#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_GetParent
#define UT_CUSTOM_MOCK_PostMessageA
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#define UT_CUSTOM_MOCK_ODStringCopy
#define UT_CUSTOM_MOCK_ODStoreTextInfo
#define UT_CUSTOM_MOCK_ODScrnCreateWindow
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos
#define UT_CUSTOM_MOCK_ODScrnDisplayString
#define UT_CUSTOM_MOCK_ODRestoreTextInfo
#define UT_CUSTOM_MOCK_ODScrnEnableCaret
static char ut_copy[6];
static BYTE ut_window;
static BOOL ut_malloc_fails;
static BOOL ut_post_result;
static BOOL ut_create_fails;
static tODWindowsSubsystem ut_subsystem;
static unsigned ut_malloc_calls;
static unsigned ut_copy_calls;
static unsigned ut_free_calls;
static unsigned ut_parent_calls;
static unsigned ut_post_calls;
static unsigned ut_string_copy_calls;
static unsigned ut_store_calls;
static unsigned ut_create_calls;
static unsigned ut_cursor_calls;
static unsigned ut_display_calls;
static unsigned ut_restore_calls;
static unsigned ut_caret_calls;

void *utm_malloc(size_t count)
{
   ++ut_malloc_calls; UT_ASSERT_EQ_UINT(sizeof(ut_copy), count);
   return ut_malloc_fails ? NULL : ut_copy;
}
void *utm_memcpy(void *destination, const void *source, size_t count)
{
   size_t index;
   ++ut_copy_calls; UT_ASSERT(destination == ut_copy);
   UT_ASSERT_EQ_UINT(sizeof(ut_copy), count);
   for(index = 0; index < count; ++index)
      ((char *)destination)[index] = ((const char *)source)[index];
   return destination;
}
void utm_free(void *allocation)
{
   ++ut_free_calls; UT_ASSERT(allocation == ut_copy);
}
HWND WINAPI utm_GetParent(HWND window)
{
   ++ut_parent_calls; UT_ASSERT(window == hwndScreenWindow); return (HWND)2;
}
BOOL WINAPI utm_PostMessageA(HWND window, UINT message, WPARAM wparam,
   LPARAM lparam)
{
   ++ut_post_calls; UT_ASSERT(window == (HWND)2);
   UT_ASSERT_EQ_UINT(WM_SHOW_MESSAGE, message); UT_ASSERT_EQ_UINT(0, wparam);
   UT_ASSERT(lparam == (LPARAM)ut_copy);
   return ut_post_result;
}
tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{ return(ut_subsystem); }
void utm_ODStringCopy(char *destination, CONST char *source, INT size)
{
   ++ut_string_copy_calls; UT_ASSERT(strcmp(source, "source") == 0);
   UT_ASSERT_EQ_INT(74, size);
   destination[0] = 'H'; destination[1] = 'E'; destination[2] = 'L';
   destination[3] = 'L'; destination[4] = 'O'; destination[5] = '\0';
}
void utm_ODStoreTextInfo(void) { ++ut_store_calls; }
void *utm_ODScrnCreateWindow(BYTE left, BYTE top, BYTE right, BYTE bottom,
   BYTE attribute, char *title, BYTE title_attribute)
{
   ++ut_create_calls;
   UT_ASSERT_EQ_UINT(36, left); UT_ASSERT_EQ_UINT(10, top);
   UT_ASSERT_EQ_UINT(44, right); UT_ASSERT_EQ_UINT(14, bottom);
   UT_ASSERT_EQ_UINT(0x17, attribute); UT_ASSERT(title[0] == '\0');
   UT_ASSERT_EQ_UINT(0x17, title_attribute);
   return ut_create_fails ? NULL : &ut_window;
}
void ODCALL utm_ODScrnSetCursorPos(BYTE column, BYTE row)
{
   ++ut_cursor_calls; UT_ASSERT_EQ_UINT(38, column); UT_ASSERT_EQ_UINT(12, row);
}
void ODCALL utm_ODScrnDisplayString(const char *string)
{
   ++ut_display_calls; UT_ASSERT(strcmp(string, "HELLO") == 0);
}
void utm_ODRestoreTextInfo(void) { ++ut_restore_calls; }
void utm_ODScrnEnableCaret(BOOL enable)
{
   ++ut_caret_calls; UT_ASSERT_EQ_INT(FALSE, enable);
}

static void reset_message(void)
{
   od_control.od_silent_mode = FALSE; hwndScreenWindow = (HWND)1;
   od_control.od_local_win_col = 0x17;
   ut_malloc_fails = FALSE; ut_post_result = TRUE; ut_create_fails = FALSE;
   ut_subsystem = kODWindowsSubsystemGUI;
   ut_malloc_calls = ut_copy_calls = ut_free_calls = 0;
   ut_parent_calls = ut_post_calls = 0;
   ut_string_copy_calls = ut_store_calls = ut_create_calls = 0;
   ut_cursor_calls = ut_display_calls = ut_restore_calls = ut_caret_calls = 0;
}
#else
#define UT_CUSTOM_MOCK_ODStringCopy
#define UT_CUSTOM_MOCK_ODStoreTextInfo
#define UT_CUSTOM_MOCK_ODScrnCreateWindow
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos
#define UT_CUSTOM_MOCK_ODScrnDisplayString
#define UT_CUSTOM_MOCK_ODRestoreTextInfo
#define UT_CUSTOM_MOCK_ODScrnEnableCaret
static BYTE ut_window;
static BOOL ut_create_fails;
static unsigned ut_copy_calls;
static unsigned ut_store_calls;
static unsigned ut_create_calls;
static unsigned ut_cursor_calls;
static unsigned ut_display_calls;
static unsigned ut_restore_calls;
static unsigned ut_caret_calls;

void utm_ODStringCopy(char *destination, CONST char *source, INT size)
{
   ++ut_copy_calls; UT_ASSERT(strcmp(source, "source") == 0);
   UT_ASSERT_EQ_INT(74, size);
   destination[0] = 'H'; destination[1] = 'E'; destination[2] = 'L';
   destination[3] = 'L'; destination[4] = 'O'; destination[5] = '\0';
}
void utm_ODStoreTextInfo(void) { ++ut_store_calls; }
void *utm_ODScrnCreateWindow(BYTE left, BYTE top, BYTE right, BYTE bottom,
   BYTE attribute, char *title, BYTE title_attribute)
{
   ++ut_create_calls;
   UT_ASSERT_EQ_UINT(36, left); UT_ASSERT_EQ_UINT(10, top);
   UT_ASSERT_EQ_UINT(44, right); UT_ASSERT_EQ_UINT(14, bottom);
   UT_ASSERT_EQ_UINT(0x17, attribute); UT_ASSERT(title[0] == '\0');
   UT_ASSERT_EQ_UINT(0x17, title_attribute);
   return ut_create_fails ? NULL : &ut_window;
}
void ODCALL utm_ODScrnSetCursorPos(BYTE column, BYTE row)
{
   ++ut_cursor_calls; UT_ASSERT_EQ_UINT(38, column); UT_ASSERT_EQ_UINT(12, row);
}
void ODCALL utm_ODScrnDisplayString(const char *string)
{
   ++ut_display_calls; UT_ASSERT(strcmp(string, "HELLO") == 0);
}
void utm_ODRestoreTextInfo(void) { ++ut_restore_calls; }
void utm_ODScrnEnableCaret(BOOL enable)
{
   ++ut_caret_calls; UT_ASSERT_EQ_INT(FALSE, enable);
}

static void reset_message(void)
{
   od_control.od_silent_mode = FALSE; od_control.od_local_win_col = 0x17;
   ut_create_fails = FALSE;
   ut_copy_calls = ut_store_calls = ut_create_calls = ut_cursor_calls = 0;
   ut_display_calls = ut_restore_calls = ut_caret_calls = 0;
}
#endif

static void does_nothing_in_silent_mode(void)
{
   reset_message(); od_control.od_silent_mode = TRUE;
   UT_ASSERT(utt_ODScrnShowMessage("source", 0) == NULL);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(0, ut_malloc_calls);
#else
   UT_ASSERT_EQ_UINT(0, ut_copy_calls); UT_ASSERT_EQ_UINT(0, ut_store_calls);
#endif
}

#ifdef ODPLAT_WIN32
static void reports_copy_allocation_failure(void)
{
   reset_message(); ut_malloc_fails = TRUE;
   UT_ASSERT(utt_ODScrnShowMessage("HELLO", 0) == NULL);
   UT_ASSERT_EQ_UINT(1, ut_malloc_calls); UT_ASSERT_EQ_UINT(0, ut_copy_calls);
}
static void posts_an_owned_message_copy(void)
{
   reset_message();
   UT_ASSERT(utt_ODScrnShowMessage("HELLO", 0) == NULL);
   UT_ASSERT(strcmp(ut_copy, "HELLO") == 0);
   UT_ASSERT_EQ_UINT(1, ut_copy_calls); UT_ASSERT_EQ_UINT(1, ut_post_calls);
   UT_ASSERT_EQ_UINT(0, ut_free_calls);
}
static void releases_the_copy_when_posting_fails(void)
{
   reset_message(); ut_post_result = FALSE;
   UT_ASSERT(utt_ODScrnShowMessage("HELLO", 0) == NULL);
   UT_ASSERT_EQ_UINT(1, ut_post_calls); UT_ASSERT_EQ_UINT(1, ut_free_calls);
}
static void reports_console_window_creation_failure(void)
{
   reset_message(); ut_subsystem = kODWindowsSubsystemConsole;
   ut_create_fails = TRUE;
   UT_ASSERT(utt_ODScrnShowMessage("source", 0) == NULL);
   UT_ASSERT_EQ_UINT(1, ut_string_copy_calls);
   UT_ASSERT_EQ_UINT(1, ut_store_calls); UT_ASSERT_EQ_UINT(1, ut_create_calls);
   UT_ASSERT_EQ_UINT(0, ut_cursor_calls);
}
static void displays_and_returns_the_console_window(void)
{
   reset_message(); ut_subsystem = kODWindowsSubsystemConsole;
   UT_ASSERT(utt_ODScrnShowMessage("source", 0) == &ut_window);
   UT_ASSERT_EQ_UINT(1, ut_cursor_calls); UT_ASSERT_EQ_UINT(1, ut_display_calls);
   UT_ASSERT_EQ_UINT(1, ut_restore_calls); UT_ASSERT_EQ_UINT(1, ut_caret_calls);
}
#else
static void reports_text_window_creation_failure(void)
{
   reset_message(); ut_create_fails = TRUE;
   UT_ASSERT(utt_ODScrnShowMessage("source", 0) == NULL);
   UT_ASSERT_EQ_UINT(1, ut_copy_calls); UT_ASSERT_EQ_UINT(1, ut_store_calls);
   UT_ASSERT_EQ_UINT(1, ut_create_calls); UT_ASSERT_EQ_UINT(0, ut_cursor_calls);
}
static void displays_and_returns_the_text_window(void)
{
   reset_message();
   UT_ASSERT(utt_ODScrnShowMessage("source", 0) == &ut_window);
   UT_ASSERT_EQ_UINT(1, ut_cursor_calls); UT_ASSERT_EQ_UINT(1, ut_display_calls);
   UT_ASSERT_EQ_UINT(1, ut_restore_calls); UT_ASSERT_EQ_UINT(1, ut_caret_calls);
}
#endif

static const UTTestCase ut_cases[] = {
   {"silent mode", does_nothing_in_silent_mode},
#ifdef ODPLAT_WIN32
   {"allocation failure", reports_copy_allocation_failure},
   {"post message", posts_an_owned_message_copy},
   {"post failure", releases_the_copy_when_posting_fails},
   {"console window failure", reports_console_window_creation_failure},
   {"console message", displays_and_returns_the_console_window}
#else
   {"window failure", reports_text_window_creation_failure},
   {"text message", displays_and_returns_the_text_window}
#endif
};
