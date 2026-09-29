#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_GetCurrentThreadId
#define UT_CUSTOM_MOCK_PostMessageA
#include "winptr.h"
#define UT_CUSTOM_MOCK_ODFrameUpdateTimeLeft
static DWORD ut_current_thread;static unsigned ut_post_calls,ut_update_calls;static tODFrameWindowInfo ut_info;
DWORD WINAPI utm_GetCurrentThreadId(void){return(ut_current_thread);}
BOOL WINAPI utm_PostMessageA(HWND window,UINT message,WPARAM first,LPARAM second)
{UT_ASSERT_EQ_PTR(hwndCurrentFrame,window);UT_ASSERT_EQ_UINT(WM_OD_UPDATE_TIME,message);UT_ASSERT_EQ_UINT(0,first);UT_ASSERT_EQ_INT(0,second);++ut_post_calls;return(TRUE);}
UT_WINDOW_LONG_PTR WINAPI UT_GET_WINDOW_LONG_PTR(HWND window,int index){UT_ASSERT_EQ_PTR(hwndCurrentFrame,window);UT_ASSERT_EQ_INT(GWLP_USERDATA,index);return((UT_WINDOW_LONG_PTR)&ut_info);}
static void utm_ODFrameUpdateTimeLeft(tODFrameWindowInfo *info){UT_ASSERT_EQ_PTR(&ut_info,info);++ut_update_calls;}
static void routes_updates_to_the_frame_thread(void)
{ut_post_calls=ut_update_calls=0;hwndCurrentFrame=NULL;utt_ODFrameUpdateTimeDisplay();UT_ASSERT_EQ_UINT(0,ut_post_calls);
 hwndCurrentFrame=(HWND)(UINT_PTR)1;dwFrameThreadID=7;ut_current_thread=8;utt_ODFrameUpdateTimeDisplay();UT_ASSERT_EQ_UINT(1,ut_post_calls);UT_ASSERT_EQ_UINT(0,ut_update_calls);
 ut_current_thread=7;utt_ODFrameUpdateTimeDisplay();UT_ASSERT_EQ_UINT(1,ut_update_calls);}
static const UTTestCase ut_cases[]={{"thread routing",routes_updates_to_the_frame_thread}};
#endif
