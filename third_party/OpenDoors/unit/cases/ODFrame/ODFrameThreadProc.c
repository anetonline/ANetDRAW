#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_GetCurrentThreadId
#define UT_CUSTOM_MOCK_PeekMessageA
#define UT_CUSTOM_MOCK_ODFrameCreateWindow
#define UT_CUSTOM_MOCK_SetEvent
#define UT_CUSTOM_MOCK_ODFrameMessageLoop
#define UT_CUSTOM_MOCK_IsWindow
#define UT_CUSTOM_MOCK_ODFrameDestroyWindow
#define UT_CUSTOM_MOCK_ODScrnStopWindow
static HWND ut_frame;static BOOL ut_is_window;static unsigned ut_event_calls,ut_loop_calls,ut_destroy_calls,ut_screen_stop_calls;
DWORD WINAPI utm_GetCurrentThreadId(void){return(77);}
BOOL WINAPI utm_PeekMessageA(LPMSG message,HWND window,UINT first,UINT last,UINT remove){(void)message;UT_ASSERT_NULL(window);UT_ASSERT_EQ_UINT(WM_USER,first);UT_ASSERT_EQ_UINT(WM_USER,last);UT_ASSERT_EQ_UINT(PM_NOREMOVE,remove);return(FALSE);}
static HWND utm_ODFrameCreateWindow(HANDLE instance){UT_ASSERT_EQ_PTR((HANDLE)(UINT_PTR)1,instance);return(ut_frame);}
BOOL WINAPI utm_SetEvent(HANDLE event){UT_ASSERT_EQ_PTR(hFrameStartedEvent,event);++ut_event_calls;return(TRUE);}
static void utm_ODFrameMessageLoop(HANDLE instance,HWND frame){UT_ASSERT_EQ_PTR((HANDLE)(UINT_PTR)1,instance);UT_ASSERT_EQ_PTR(ut_frame,frame);++ut_loop_calls;}
BOOL WINAPI utm_IsWindow(HWND frame){UT_ASSERT_EQ_PTR(ut_frame,frame);return(ut_is_window);}
static void utm_ODFrameDestroyWindow(HWND frame){UT_ASSERT_EQ_PTR(ut_frame,frame);++ut_destroy_calls;}
void utm_ODScrnStopWindow(void){++ut_screen_stop_calls;}
static void reset_thread(void){ut_event_calls=ut_loop_calls=ut_destroy_calls=ut_screen_stop_calls=0;hFrameStartedEvent=(HANDLE)(UINT_PTR)2;hwndCurrentFrame=NULL;dwFrameThreadID=0;}
static void reports_window_creation_failure(void)
{reset_thread();ut_frame=NULL;UT_ASSERT_EQ_UINT(FALSE,utt_ODFrameThreadProc((void *)(UINT_PTR)1));UT_ASSERT_EQ_INT(kODRCGeneralFailure,FrameStartResult);UT_ASSERT_EQ_UINT(1,ut_event_calls);UT_ASSERT_EQ_UINT(0,dwFrameThreadID);}
static void runs_the_loop_and_conditionally_destroys_the_frame(void)
{reset_thread();ut_frame=(HWND)(UINT_PTR)3;ut_is_window=FALSE;UT_ASSERT_EQ_UINT(TRUE,utt_ODFrameThreadProc((void *)(UINT_PTR)1));UT_ASSERT_EQ_INT(kODRCSuccess,FrameStartResult);UT_ASSERT_EQ_PTR(ut_frame,hwndCurrentFrame);UT_ASSERT_EQ_UINT(1,ut_loop_calls);UT_ASSERT_EQ_UINT(1,ut_screen_stop_calls);UT_ASSERT_EQ_UINT(0,ut_destroy_calls);UT_ASSERT_EQ_UINT(0,dwFrameThreadID);
 reset_thread();ut_frame=(HWND)(UINT_PTR)3;ut_is_window=TRUE;UT_ASSERT_EQ_UINT(TRUE,utt_ODFrameThreadProc((void *)(UINT_PTR)1));UT_ASSERT_EQ_UINT(1,ut_screen_stop_calls);UT_ASSERT_EQ_UINT(1,ut_destroy_calls);}
static const UTTestCase ut_cases[]={{"creation failure",reports_window_creation_failure},{"frame lifetime",runs_the_loop_and_conditionally_destroys_the_frame}};
#endif
