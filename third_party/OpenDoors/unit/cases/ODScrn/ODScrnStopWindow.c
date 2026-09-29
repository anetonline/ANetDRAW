#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock
#define UT_CUSTOM_MOCK_IsWindow
#define UT_CUSTOM_MOCK_DestroyWindow

static HWND ut_screen = (HWND)1;
static BOOL ut_is_window;
static unsigned ut_locks;
static unsigned ut_unlocks;
static unsigned ut_is_window_calls;
static unsigned ut_destroy_calls;

void utm_ODMutexLock(tODMutex *mutex)
{
   ++ut_locks; UT_ASSERT(mutex == &ScreenPresentationMutex);
}

void utm_ODMutexUnlock(tODMutex *mutex)
{
   ++ut_unlocks; UT_ASSERT(mutex == &ScreenPresentationMutex);
}

BOOL WINAPI utm_IsWindow(HWND window)
{
   ++ut_is_window_calls; UT_ASSERT(window == ut_screen); return ut_is_window;
}

BOOL WINAPI utm_DestroyWindow(HWND window)
{
   ++ut_destroy_calls; UT_ASSERT(window == ut_screen); return TRUE;
}

static void reset_stop(void)
{
   hwndScreenWindow = NULL; ut_is_window = FALSE;
   ut_locks = ut_unlocks = ut_is_window_calls = ut_destroy_calls = 0;
}

static void ignores_an_absent_screen_child(void)
{
   reset_stop(); utt_ODScrnStopWindow();
   UT_ASSERT_EQ_UINT(1, ut_locks); UT_ASSERT_EQ_UINT(1, ut_unlocks);
   UT_ASSERT_EQ_UINT(0, ut_is_window_calls); UT_ASSERT_EQ_UINT(0, ut_destroy_calls);
}

static void ignores_a_child_that_was_already_destroyed(void)
{
   reset_stop(); hwndScreenWindow = ut_screen;
   utt_ODScrnStopWindow();
   UT_ASSERT_EQ_UINT(1, ut_is_window_calls); UT_ASSERT_EQ_UINT(0, ut_destroy_calls);
}

static void destroys_the_child_synchronously_on_the_frame_thread(void)
{
   reset_stop(); hwndScreenWindow = ut_screen; ut_is_window = TRUE;
   utt_ODScrnStopWindow();
   UT_ASSERT_EQ_UINT(1, ut_is_window_calls); UT_ASSERT_EQ_UINT(1, ut_destroy_calls);
}

static const UTTestCase ut_cases[] = {
   {"absent child", ignores_an_absent_screen_child},
   {"destroyed child", ignores_a_child_that_was_already_destroyed},
   {"destroy child", destroys_the_child_synchronously_on_the_frame_thread}
};
