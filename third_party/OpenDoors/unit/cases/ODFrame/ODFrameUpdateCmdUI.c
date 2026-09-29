#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_GetMenu
#define UT_CUSTOM_MOCK_GetDlgItem
#define UT_CUSTOM_MOCK_GetCurrentThreadId
#define UT_CUSTOM_MOCK_PostMessageA
#include "winptr.h"
#define UT_CUSTOM_MOCK_ODKrnlGetUIState
#define UT_CUSTOM_MOCK_CheckMenuItem
#define UT_CUSTOM_MOCK_SendMessageA
static tODFrameWindowInfo ut_info;static DWORD ut_thread;static BOOL ut_null_info;
static tODUIState ut_state;static unsigned ut_post_calls,ut_state_calls,ut_menu_calls,ut_send_calls;
static UINT ut_menu_ids[6],ut_menu_flags[6];static WPARAM ut_send_ids[3];static LPARAM ut_send_values[3];
HMENU WINAPI utm_GetMenu(HWND frame){(void)frame;return((HMENU)(UINT_PTR)2);}
HWND WINAPI utm_GetDlgItem(HWND frame,int id){(void)frame;UT_ASSERT_EQ_INT(ID_TOOLBAR,id);return((HWND)(UINT_PTR)3);}
DWORD WINAPI utm_GetCurrentThreadId(void){return(ut_thread);}
BOOL WINAPI utm_PostMessageA(HWND frame,UINT message,WPARAM first,LPARAM second)
{UT_ASSERT_EQ_PTR(hwndCurrentFrame,frame);UT_ASSERT_EQ_UINT(WM_OD_UPDATE_COMMANDS,message);UT_ASSERT_EQ_UINT(0,first);UT_ASSERT_EQ_INT(0,second);++ut_post_calls;return(TRUE);}
UT_WINDOW_LONG_PTR WINAPI UT_GET_WINDOW_LONG_PTR(HWND frame,int index){UT_ASSERT_EQ_PTR(hwndCurrentFrame,frame);UT_ASSERT_EQ_INT(GWLP_USERDATA,index);return(ut_null_info?0:(UT_WINDOW_LONG_PTR)&ut_info);}
void utm_ODKrnlGetUIState(tODUIState *state){++ut_state_calls;*state=ut_state;}
DWORD WINAPI utm_CheckMenuItem(HMENU menu,UINT id,UINT flags){UT_ASSERT_EQ_PTR((HMENU)(UINT_PTR)2,menu);UT_ASSERT(ut_menu_calls<6);
 ut_menu_ids[ut_menu_calls]=id;ut_menu_flags[ut_menu_calls]=flags;++ut_menu_calls;return(0);}
LRESULT WINAPI utm_SendMessageA(HWND window,UINT message,WPARAM id,LPARAM value){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)3,window);UT_ASSERT_EQ_UINT(TB_CHECKBUTTON,message);UT_ASSERT(ut_send_calls<3);
 ut_send_ids[ut_send_calls]=id;ut_send_values[ut_send_calls]=value;++ut_send_calls;return(0);}
static void reset_commands(void){memset(&ut_info,0,sizeof(ut_info));memset(&ut_state,0,sizeof(ut_state));ut_thread=7;ut_null_info=FALSE;ut_post_calls=ut_state_calls=ut_menu_calls=ut_send_calls=0;
 hwndCurrentFrame=(HWND)(UINT_PTR)1;dwFrameThreadID=7;}
static void handles_missing_and_cross_thread_frames(void)
{reset_commands();hwndCurrentFrame=NULL;utt_ODFrameUpdateCmdUI();UT_ASSERT_EQ_UINT(0,ut_post_calls);
 reset_commands();ut_thread=8;utt_ODFrameUpdateCmdUI();UT_ASSERT_EQ_UINT(1,ut_post_calls);UT_ASSERT_EQ_UINT(0,ut_menu_calls);
 reset_commands();ut_null_info=TRUE;utt_ODFrameUpdateCmdUI();UT_ASSERT_EQ_UINT(0,ut_menu_calls);}
static void assert_command_state(BOOL enabled)
{unsigned i;UT_ASSERT_EQ_UINT(1,ut_state_calls);UT_ASSERT_EQ_UINT(6,ut_menu_calls);UT_ASSERT_EQ_UINT(3,ut_send_calls);
 UT_ASSERT_EQ_UINT(ID_VIEW_TOOL_BAR,ut_menu_ids[0]);UT_ASSERT_EQ_UINT(ID_VIEW_STAT_BAR,ut_menu_ids[1]);
 UT_ASSERT_EQ_UINT(ID_USER_INACTIVITYTIMER,ut_menu_ids[2]);UT_ASSERT_EQ_UINT(ID_DOOR_SYSOPNEXT,ut_menu_ids[3]);
 UT_ASSERT_EQ_UINT(ID_DOOR_USERKEYBOARDOFF,ut_menu_ids[4]);UT_ASSERT_EQ_UINT(ID_DOOR_CHATMODE,ut_menu_ids[5]);
 for(i=0;i<6;++i){BOOL checked=(ut_menu_flags[i]&MF_CHECKED)!=0;
    UT_ASSERT_EQ_INT((i==2||i==4)?!enabled:enabled,checked);}
 UT_ASSERT_EQ_UINT(ID_DOOR_SYSOPNEXT,ut_send_ids[0]);UT_ASSERT_EQ_UINT(ID_DOOR_USERKEYBOARDOFF,ut_send_ids[1]);UT_ASSERT_EQ_UINT(ID_DOOR_CHATMODE,ut_send_ids[2]);
 UT_ASSERT_EQ_INT(enabled,LOWORD(ut_send_values[0]));UT_ASSERT_EQ_INT(!enabled,LOWORD(ut_send_values[1]));UT_ASSERT_EQ_INT(enabled,LOWORD(ut_send_values[2]));}
static void reflects_each_control_flag(void)
{reset_commands();ut_state.bInactivityDisabled=FALSE;ut_state.bSysopNext=FALSE;ut_state.bUserKeyboardOn=FALSE;ut_state.bChatActive=FALSE;
 utt_ODFrameUpdateCmdUI();assert_command_state(FALSE);
 reset_commands();ut_info.bToolbarOn=TRUE;ut_info.bStatusBarOn=TRUE;ut_state.bInactivityDisabled=TRUE;ut_state.bSysopNext=TRUE;ut_state.bUserKeyboardOn=TRUE;ut_state.bChatActive=TRUE;
 utt_ODFrameUpdateCmdUI();assert_command_state(TRUE);}
static const UTTestCase ut_cases[]={{"early routing",handles_missing_and_cross_thread_frames},{"command state",reflects_each_control_flag}};
#endif
