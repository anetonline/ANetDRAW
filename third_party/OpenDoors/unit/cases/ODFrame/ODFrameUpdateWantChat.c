#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_GetCurrentThreadId
#define UT_CUSTOM_MOCK_PostMessageA
#include "winptr.h"
#define UT_CUSTOM_MOCK_ODKrnlGetUIState
#define UT_CUSTOM_MOCK_ODStringCopy
#define UT_CUSTOM_MOCK_ODFrameSetMainStatusText
#define UT_CUSTOM_MOCK_SetWindowTextA
#define UT_CUSTOM_MOCK_FlashWindow
#define UT_CUSTOM_MOCK_KillTimer
#define UT_CUSTOM_MOCK_SetTimer
#define UT_CUSTOM_MOCK_GetCaretBlinkTime
#define UT_CUSTOM_MOCK_sprintf
static tODFrameWindowInfo ut_info;static tODUIState ut_state;static DWORD ut_thread;static unsigned ut_post_calls,ut_state_calls,ut_status_calls;
static unsigned ut_title_calls,ut_flash_calls,ut_kill_calls,ut_timer_calls;static char ut_title[100];
DWORD WINAPI utm_GetCurrentThreadId(void){return(ut_thread);}
BOOL WINAPI utm_PostMessageA(HWND frame,UINT message,WPARAM first,LPARAM second){UT_ASSERT_EQ_PTR(hwndCurrentFrame,frame);UT_ASSERT_EQ_UINT(WM_OD_UPDATE_CHAT,message);UT_ASSERT_EQ_UINT(0,first);UT_ASSERT_EQ_INT(0,second);++ut_post_calls;return(TRUE);}
UT_WINDOW_LONG_PTR WINAPI UT_GET_WINDOW_LONG_PTR(HWND frame,int index){UT_ASSERT_EQ_PTR(hwndCurrentFrame,frame);UT_ASSERT_EQ_INT(GWLP_USERDATA,index);return((UT_WINDOW_LONG_PTR)&ut_info);}
void utm_ODKrnlGetUIState(tODUIState *state){++ut_state_calls;*state=ut_state;}
void utm_ODStringCopy(char *destination,const char *source,INT size){size_t n=0;while(source[n])++n;if(n>=(size_t)size)n=(size_t)size-1;memcpy(destination,source,n);destination[n]='\0';}
static void utm_ODFrameSetMainStatusText(HWND status){UT_ASSERT_EQ_PTR(ut_info.hwndStatusBar,status);++ut_status_calls;}
BOOL WINAPI utm_SetWindowTextA(HWND frame,LPCSTR text){UT_ASSERT_EQ_PTR(hwndCurrentFrame,frame);strcpy(ut_title,text);++ut_title_calls;return(TRUE);}
BOOL WINAPI utm_FlashWindow(HWND frame,BOOL invert){UT_ASSERT_EQ_PTR(hwndCurrentFrame,frame);UT_ASSERT(!invert);++ut_flash_calls;return(TRUE);}
BOOL WINAPI utm_KillTimer(HWND frame,UINT_PTR id){UT_ASSERT_EQ_PTR(hwndCurrentFrame,frame);UT_ASSERT_EQ_UINT(1,id);++ut_kill_calls;return(TRUE);}
UINT_PTR WINAPI utm_SetTimer(HWND frame,UINT_PTR id,UINT interval,TIMERPROC proc){UT_ASSERT_EQ_PTR(hwndCurrentFrame,frame);UT_ASSERT_EQ_UINT(1,id);UT_ASSERT_EQ_UINT(500,interval);UT_ASSERT_NULL(proc);++ut_timer_calls;return(1);}
UINT WINAPI utm_GetCaretBlinkTime(void){return(500);}
int utm_sprintf(char *destination,const char *format,...){UT_ASSERT(strcmp(format,"%s - User Wants Chat")==0);strcpy(destination,"Door - User Wants Chat");return(22);}
static void reset_chat(void){memset(&ut_info,0,sizeof(ut_info));memset(&ut_state,0,sizeof(ut_state));ut_thread=7;ut_post_calls=ut_state_calls=ut_status_calls=0;ut_title_calls=ut_flash_calls=ut_kill_calls=ut_timer_calls=0;
 strcpy(ut_state.szProgramName,"Door");ut_state.bUserWantsChat=FALSE;hwndCurrentFrame=(HWND)(UINT_PTR)1;dwFrameThreadID=7;ut_info.hwndStatusBar=(HWND)(UINT_PTR)2;ut_title[0]='\0';}
static void routes_cross_thread_updates(void){reset_chat();hwndCurrentFrame=NULL;utt_ODFrameUpdateWantChat();UT_ASSERT_EQ_UINT(0,ut_post_calls);
 reset_chat();ut_thread=8;utt_ODFrameUpdateWantChat();UT_ASSERT_EQ_UINT(1,ut_post_calls);UT_ASSERT_EQ_UINT(0,ut_state_calls);}
static void toggles_the_indicator_and_optional_status_text(void)
{reset_chat();utt_ODFrameUpdateWantChat();UT_ASSERT_EQ_UINT(0,ut_status_calls);UT_ASSERT_EQ_UINT(0,ut_title_calls);
 reset_chat();ut_info.bStatusBarOn=TRUE;ut_info.bWantsChatIndicator=TRUE;ut_state.bUserWantsChat=TRUE;utt_ODFrameUpdateWantChat();UT_ASSERT_EQ_UINT(1,ut_status_calls);UT_ASSERT_EQ_UINT(0,ut_title_calls);
 reset_chat();ut_info.bWantsChatIndicator=TRUE;ut_state.bUserWantsChat=FALSE;utt_ODFrameUpdateWantChat();UT_ASSERT(strcmp(ut_title,"Door")==0);UT_ASSERT_EQ_UINT(1,ut_flash_calls);UT_ASSERT_EQ_UINT(1,ut_kill_calls);UT_ASSERT(!ut_info.bWantsChatIndicator);
 reset_chat();ut_state.bUserWantsChat=TRUE;utt_ODFrameUpdateWantChat();UT_ASSERT(strcmp(ut_title,"Door - User Wants Chat")==0);UT_ASSERT_EQ_UINT(1,ut_timer_calls);UT_ASSERT(ut_info.bWantsChatIndicator);}
static const UTTestCase ut_cases[]={{"thread routing",routes_cross_thread_updates},{"chat indicator",toggles_the_indicator_and_optional_status_text}};
#endif
