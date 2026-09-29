#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_PostMessageA
#define UT_CUSTOM_MOCK_SendMessageTimeoutA
#define UT_CUSTOM_MOCK_ODFramePostThreadQuit
static BOOL ut_post_result;
static LRESULT ut_send_result;
static BOOL ut_quit_result;
static unsigned ut_post_calls;
static unsigned ut_send_calls;
static unsigned ut_quit_calls;

BOOL WINAPI utm_PostMessageA(HWND frame, UINT message, WPARAM first,
   LPARAM second)
{
   UT_ASSERT_EQ_PTR(hwndCurrentFrame, frame);
   UT_ASSERT_EQ_UINT(WM_OD_SHUTDOWN, message);
   UT_ASSERT_EQ_UINT(0, first);
   UT_ASSERT_EQ_INT(0, second);
   ++ut_post_calls;
   return(ut_post_result);
}

LRESULT WINAPI utm_SendMessageTimeoutA(HWND frame, UINT message,
   WPARAM first, LPARAM second, UINT flags, UINT timeout,
   PDWORD_PTR result)
{
   UT_ASSERT_EQ_PTR(hwndCurrentFrame, frame);
   UT_ASSERT_EQ_UINT(WM_OD_SHUTDOWN, message);
   UT_ASSERT_EQ_UINT(0, first);
   UT_ASSERT_EQ_INT(0, second);
   UT_ASSERT_EQ_UINT(SMTO_ABORTIFHUNG | SMTO_BLOCK, flags);
   UT_ASSERT_EQ_UINT(OD_UI_THREAD_TIMEOUT, timeout);
   UT_ASSERT(result != NULL);
   *result = 0;
   ++ut_send_calls;
   return(ut_send_result);
}

static BOOL utm_ODFramePostThreadQuit(tODThreadHandle thread, DWORD id)
{
   UT_ASSERT_EQ_PTR((HANDLE)(UINT_PTR)1, thread);
   UT_ASSERT_EQ_UINT(dwFrameThreadID, id);
   ++ut_quit_calls;
   return(ut_quit_result);
}

static void reset_request(void)
{
   hwndCurrentFrame = NULL;
   dwFrameThreadID = 7;
   ut_post_result = TRUE;
   ut_send_result = 1;
   ut_quit_result = TRUE;
   ut_post_calls = 0;
   ut_send_calls = 0;
   ut_quit_calls = 0;
}

static void ignores_an_absent_thread(void)
{
   reset_request();
   UT_ASSERT(utt_ODFrameRequestShutdown(NULL));
   UT_ASSERT_EQ_UINT(0, ut_post_calls);
   UT_ASSERT_EQ_UINT(0, ut_send_calls);
   UT_ASSERT_EQ_UINT(0, ut_quit_calls);
}

static void posts_to_an_existing_window(void)
{
   reset_request();
   hwndCurrentFrame = (HWND)(UINT_PTR)2;
   UT_ASSERT(utt_ODFrameRequestShutdown((HANDLE)(UINT_PTR)1));
   UT_ASSERT_EQ_UINT(1, ut_post_calls);
   UT_ASSERT_EQ_UINT(0, ut_send_calls);
   UT_ASSERT_EQ_UINT(0, ut_quit_calls);
}

static void synchronously_sends_after_a_failed_window_post(void)
{
   reset_request();
   hwndCurrentFrame = (HWND)(UINT_PTR)2;
   ut_post_result = FALSE;
   UT_ASSERT(utt_ODFrameRequestShutdown((HANDLE)(UINT_PTR)1));
   UT_ASSERT_EQ_UINT(1, ut_post_calls);
   UT_ASSERT_EQ_UINT(1, ut_send_calls);
   UT_ASSERT_EQ_UINT(0, ut_quit_calls);
}

static void falls_back_to_the_thread_queue(void)
{
   reset_request();
   hwndCurrentFrame = (HWND)(UINT_PTR)2;
   ut_post_result = FALSE;
   ut_send_result = 0;
   UT_ASSERT(utt_ODFrameRequestShutdown((HANDLE)(UINT_PTR)1));
   UT_ASSERT_EQ_UINT(1, ut_post_calls);
   UT_ASSERT_EQ_UINT(1, ut_send_calls);
   UT_ASSERT_EQ_UINT(1, ut_quit_calls);

   reset_request();
   UT_ASSERT(utt_ODFrameRequestShutdown((HANDLE)(UINT_PTR)1));
   UT_ASSERT_EQ_UINT(0, ut_post_calls);
   UT_ASSERT_EQ_UINT(0, ut_send_calls);
   UT_ASSERT_EQ_UINT(1, ut_quit_calls);
}

static void reports_complete_delivery_failure(void)
{
   reset_request();
   hwndCurrentFrame = (HWND)(UINT_PTR)2;
   ut_post_result = FALSE;
   ut_send_result = 0;
   ut_quit_result = FALSE;
   UT_ASSERT(!utt_ODFrameRequestShutdown((HANDLE)(UINT_PTR)1));
   UT_ASSERT_EQ_UINT(1, ut_post_calls);
   UT_ASSERT_EQ_UINT(1, ut_send_calls);
   UT_ASSERT_EQ_UINT(1, ut_quit_calls);
}

static const UTTestCase ut_cases[] = {
   {"absent thread", ignores_an_absent_thread},
   {"window shutdown", posts_to_an_existing_window},
   {"synchronous fallback", synchronously_sends_after_a_failed_window_post},
   {"thread fallback", falls_back_to_the_thread_queue},
   {"delivery failure", reports_complete_delivery_failure}
};
#endif
