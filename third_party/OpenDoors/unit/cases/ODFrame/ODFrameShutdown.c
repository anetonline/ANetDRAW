#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_GetCurrentThreadId
#define UT_CUSTOM_MOCK_ODFrameRequestShutdown
#define UT_CUSTOM_MOCK_ODThreadWaitForExit
#define UT_CUSTOM_MOCK_CloseHandle
static DWORD ut_thread;
static BOOL ut_request_result;
static unsigned ut_request_calls;
static unsigned ut_wait_calls;
static unsigned ut_close_frame_calls;
static unsigned ut_close_started_calls;

DWORD WINAPI utm_GetCurrentThreadId(void)
{
   return(ut_thread);
}

BOOL utm_ODFrameRequestShutdown(tODThreadHandle thread)
{
   UT_ASSERT_EQ_PTR((HANDLE)(UINT_PTR)1, thread);
   ++ut_request_calls;
   return(ut_request_result);
}

void utm_ODThreadWaitForExit(tODThreadHandle thread)
{
   UT_ASSERT_EQ_PTR((HANDLE)(UINT_PTR)1, thread);
   ++ut_wait_calls;
}

BOOL WINAPI utm_CloseHandle(HANDLE handle)
{
   if(handle == (HANDLE)(UINT_PTR)1)
      ++ut_close_frame_calls;
   else if(handle == (HANDLE)(UINT_PTR)4)
      ++ut_close_started_calls;
   else
      UT_ASSERT(FALSE);
   return(TRUE);
}

static void reset_shutdown(tODThreadHandle *thread)
{
   *thread = (HANDLE)(UINT_PTR)1;
   hwndCurrentFrame = NULL;
   dwFrameThreadID = 7;
   hFrameStartedEvent = (HANDLE)(UINT_PTR)4;
   ut_thread = 8;
   ut_request_result = TRUE;
   ut_request_calls = 0;
   ut_wait_calls = 0;
   ut_close_frame_calls = 0;
   ut_close_started_calls = 0;
}

static void ignores_an_absent_frame_thread(void)
{
   tODThreadHandle thread = NULL;
   reset_shutdown(&thread);
   thread = NULL;
   utt_ODFrameShutdown(&thread);
   UT_ASSERT_EQ_UINT(0, ut_request_calls);
   UT_ASSERT_EQ_UINT(0, ut_close_frame_calls);
}

static void posts_shutdown_and_joins(void)
{
   tODThreadHandle thread;
   reset_shutdown(&thread);
   hwndCurrentFrame = (HWND)(UINT_PTR)2;
   utt_ODFrameShutdown(&thread);
   UT_ASSERT_EQ_UINT(1, ut_request_calls);
   UT_ASSERT_EQ_UINT(1, ut_wait_calls);
   UT_ASSERT_EQ_UINT(1, ut_close_frame_calls);
   UT_ASSERT_EQ_UINT(1, ut_close_started_calls);
   UT_ASSERT_NULL(thread);
   UT_ASSERT_NULL(hFrameStartedEvent);
}

static void does_not_wait_or_release_state_after_delivery_failure(void)
{
   tODThreadHandle thread;
   reset_shutdown(&thread);
   ut_request_result = FALSE;
   utt_ODFrameShutdown(&thread);
   UT_ASSERT_EQ_UINT(1, ut_request_calls);
   UT_ASSERT_EQ_UINT(0, ut_wait_calls);
   UT_ASSERT_EQ_UINT(0, ut_close_frame_calls);
   UT_ASSERT_EQ_PTR((HANDLE)(UINT_PTR)1, thread);
   UT_ASSERT_EQ_PTR((HANDLE)(UINT_PTR)4, hFrameStartedEvent);
}

static void quits_a_windowless_thread_without_self_joining(void)
{
   tODThreadHandle thread;
   reset_shutdown(&thread);
   ut_thread = 7;
   utt_ODFrameShutdown(&thread);
   UT_ASSERT_EQ_UINT(1, ut_request_calls);
   UT_ASSERT_EQ_UINT(0, ut_wait_calls);
   UT_ASSERT_EQ_UINT(1, ut_close_frame_calls);
   UT_ASSERT_EQ_UINT(1, ut_close_started_calls);
}

static void cleans_up_without_a_startup_event(void)
{
   tODThreadHandle thread;
   reset_shutdown(&thread);
   hFrameStartedEvent = NULL;
   utt_ODFrameShutdown(&thread);
   UT_ASSERT_EQ_UINT(1, ut_request_calls);
   UT_ASSERT_EQ_UINT(1, ut_wait_calls);
   UT_ASSERT_EQ_UINT(1, ut_close_frame_calls);
   UT_ASSERT_EQ_UINT(0, ut_close_started_calls);
   UT_ASSERT_NULL(thread);
}

static const UTTestCase ut_cases[] = {
   {"absent thread", ignores_an_absent_frame_thread},
   {"joined shutdown", posts_shutdown_and_joins},
   {"delivery failure", does_not_wait_or_release_state_after_delivery_failure},
   {"self shutdown", quits_a_windowless_thread_without_self_joining},
   {"no startup event", cleans_up_without_a_startup_event}
};
#endif
