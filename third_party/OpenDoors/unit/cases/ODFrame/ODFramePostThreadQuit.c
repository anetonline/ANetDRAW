#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_WaitForSingleObject
#define UT_CUSTOM_MOCK_PostThreadMessageA
static DWORD ut_wait_result;
static BOOL ut_post_result;
static unsigned ut_post_calls;

DWORD WINAPI utm_WaitForSingleObject(HANDLE handle, DWORD timeout)
{
   UT_ASSERT_EQ_PTR((HANDLE)(UINT_PTR)1, handle);
   UT_ASSERT_EQ_UINT(0, timeout);
   return(ut_wait_result);
}

BOOL WINAPI utm_PostThreadMessageA(DWORD thread, UINT message,
   WPARAM first, LPARAM second)
{
   UT_ASSERT_EQ_UINT(7, thread);
   UT_ASSERT_EQ_UINT(WM_QUIT, message);
   UT_ASSERT_EQ_UINT(0, first);
   UT_ASSERT_EQ_INT(0, second);
   ++ut_post_calls;
   return(ut_post_result);
}

static void reports_whether_shutdown_was_delivered(void)
{
   ut_post_calls = 0;
   ut_post_result = TRUE;
   ut_wait_result = WAIT_OBJECT_0;
   UT_ASSERT(utt_ODFramePostThreadQuit((HANDLE)(UINT_PTR)1, 7));
   UT_ASSERT_EQ_UINT(0, ut_post_calls);

   ut_wait_result = WAIT_TIMEOUT;
   UT_ASSERT(!utt_ODFramePostThreadQuit((HANDLE)(UINT_PTR)1, 0));
   UT_ASSERT_EQ_UINT(0, ut_post_calls);

   UT_ASSERT(utt_ODFramePostThreadQuit((HANDLE)(UINT_PTR)1, 7));
   UT_ASSERT_EQ_UINT(1, ut_post_calls);

   ut_post_result = FALSE;
   UT_ASSERT(!utt_ODFramePostThreadQuit((HANDLE)(UINT_PTR)1, 7));
   UT_ASSERT_EQ_UINT(2, ut_post_calls);

   ut_wait_result = WAIT_FAILED;
   UT_ASSERT(!utt_ODFramePostThreadQuit((HANDLE)(UINT_PTR)1, 7));
   UT_ASSERT_EQ_UINT(2, ut_post_calls);
}

static const UTTestCase ut_cases[] = {
   {"shutdown delivery", reports_whether_shutdown_was_delivered}
};
#endif
