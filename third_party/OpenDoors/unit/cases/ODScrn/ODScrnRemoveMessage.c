#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_GetParent
#define UT_CUSTOM_MOCK_PostMessageA
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#define UT_CUSTOM_MOCK_ODStoreTextInfo
#define UT_CUSTOM_MOCK_ODScrnDestroyWindow
#define UT_CUSTOM_MOCK_ODRestoreTextInfo
#define UT_CUSTOM_MOCK_ODScrnEnableCaret
static BYTE ut_message;
static tODWindowsSubsystem ut_subsystem;
static unsigned ut_parent_calls;
static unsigned ut_post_calls;
static unsigned ut_store_calls;
static unsigned ut_destroy_calls;
static unsigned ut_restore_calls;
static unsigned ut_caret_calls;
HWND WINAPI utm_GetParent(HWND window)
{
   ++ut_parent_calls;
   UT_ASSERT(window == hwndScreenWindow);
   return (HWND)2;
}
BOOL WINAPI utm_PostMessageA(HWND window, UINT message, WPARAM wparam,
   LPARAM lparam)
{
   ++ut_post_calls;
   UT_ASSERT(window == (HWND)2); UT_ASSERT_EQ_UINT(WM_REMOVE_MESSAGE, message);
   UT_ASSERT_EQ_UINT(0, wparam); UT_ASSERT_EQ_INT(0, lparam);
   return TRUE;
}
tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{ return(ut_subsystem); }
void utm_ODStoreTextInfo(void) { ++ut_store_calls; }
void utm_ODScrnDestroyWindow(void *message)
{
   ++ut_destroy_calls;
   UT_ASSERT(message == &ut_message);
}
void utm_ODRestoreTextInfo(void) { ++ut_restore_calls; }
void utm_ODScrnEnableCaret(BOOL enable)
{
   ++ut_caret_calls;
   UT_ASSERT_EQ_INT(TRUE, enable);
}
#else
#define UT_CUSTOM_MOCK_ODStoreTextInfo
#define UT_CUSTOM_MOCK_ODScrnDestroyWindow
#define UT_CUSTOM_MOCK_ODRestoreTextInfo
#define UT_CUSTOM_MOCK_ODScrnEnableCaret

static unsigned ut_store_calls;
static unsigned ut_destroy_calls;
static unsigned ut_restore_calls;
static unsigned ut_caret_calls;
static BYTE ut_message;

void utm_ODStoreTextInfo(void) { ++ut_store_calls; }
void utm_ODScrnDestroyWindow(void *message)
{
   ++ut_destroy_calls;
   UT_ASSERT(message == &ut_message);
}
void utm_ODRestoreTextInfo(void) { ++ut_restore_calls; }
void utm_ODScrnEnableCaret(BOOL enable)
{
   ++ut_caret_calls;
   UT_ASSERT_EQ_INT(TRUE, enable);
}
#endif

static void does_nothing_in_silent_mode(void)
{
   od_control.od_silent_mode = TRUE;
#ifndef ODPLAT_WIN32
   ut_store_calls = ut_destroy_calls = ut_restore_calls = ut_caret_calls = 0;
#endif
   utt_ODScrnRemoveMessage(NULL);
#ifndef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(0, ut_store_calls); UT_ASSERT_EQ_UINT(0, ut_destroy_calls);
#endif
}

#ifndef ODPLAT_WIN32
static void ignores_a_null_message(void)
{
   od_control.od_silent_mode = FALSE;
   ut_store_calls = ut_destroy_calls = ut_restore_calls = ut_caret_calls = 0;
   utt_ODScrnRemoveMessage(NULL);
   UT_ASSERT_EQ_UINT(0, ut_store_calls); UT_ASSERT_EQ_UINT(0, ut_destroy_calls);
}

static void restores_a_text_message_and_the_caret(void)
{
   od_control.od_silent_mode = FALSE;
   ut_store_calls = ut_destroy_calls = ut_restore_calls = ut_caret_calls = 0;
   utt_ODScrnRemoveMessage(&ut_message);
   UT_ASSERT_EQ_UINT(1, ut_store_calls); UT_ASSERT_EQ_UINT(1, ut_destroy_calls);
   UT_ASSERT_EQ_UINT(1, ut_restore_calls); UT_ASSERT_EQ_UINT(1, ut_caret_calls);
}
#else
static void posts_the_remove_request(void)
{
   od_control.od_silent_mode = FALSE;
   hwndScreenWindow = (HWND)1;
   ut_subsystem = kODWindowsSubsystemGUI;
   ut_parent_calls = ut_post_calls = 0;
   utt_ODScrnRemoveMessage((void *)3);
   UT_ASSERT_EQ_UINT(1, ut_parent_calls); UT_ASSERT_EQ_UINT(1, ut_post_calls);
}
static void handles_console_message_state(void)
{
   od_control.od_silent_mode = FALSE;
   ut_subsystem = kODWindowsSubsystemConsole;
   ut_store_calls = ut_destroy_calls = ut_restore_calls = ut_caret_calls = 0;
   utt_ODScrnRemoveMessage(NULL);
   UT_ASSERT_EQ_UINT(0, ut_store_calls); UT_ASSERT_EQ_UINT(0, ut_destroy_calls);

   utt_ODScrnRemoveMessage(&ut_message);
   UT_ASSERT_EQ_UINT(1, ut_store_calls); UT_ASSERT_EQ_UINT(1, ut_destroy_calls);
   UT_ASSERT_EQ_UINT(1, ut_restore_calls); UT_ASSERT_EQ_UINT(1, ut_caret_calls);
}
#endif

static const UTTestCase ut_cases[] = {
   {"silent mode", does_nothing_in_silent_mode},
#ifndef ODPLAT_WIN32
   {"null message", ignores_a_null_message},
   {"restore text message", restores_a_text_message_and_the_caret}
#else
   {"post remove request", posts_the_remove_request},
   {"console message", handles_console_message_state}
#endif
};
