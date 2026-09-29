#ifdef ODPLAT_WIN32
#include "winptr.h"
#define UT_CUSTOM_MOCK_ODKrnlGetUIState
#ifdef _MSC_VER
#define UT_CUSTOM_MOCK__InterlockedExchange
#else
#define UT_CUSTOM_MOCK_InterlockedExchange
#endif
#define UT_CUSTOM_MOCK_ODFrameUpdateCmdUI
#define UT_CUSTOM_MOCK_GetMenu
#define UT_CUSTOM_MOCK_RemoveMenu
#define UT_CUSTOM_MOCK_ODFrameCopyProgramName
#define UT_CUSTOM_MOCK_MessageBoxA
#define UT_CUSTOM_MOCK_ODFrameDestroyWindow
#define UT_CUSTOM_MOCK_PostQuitMessage
#define UT_CUSTOM_MOCK_GetDlgItem
#define UT_CUSTOM_MOCK_ODFrameDestroyToolbar
#define UT_CUSTOM_MOCK_ODFrameDestroyStatusBar
#define UT_CUSTOM_MOCK_ODKrnlRequestShutdown
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_ODScrnSetFocusToWindow
#define UT_CUSTOM_MOCK_FlashWindow
#define UT_CUSTOM_MOCK_ODFrameUpdateTimeDisplay
#define UT_CUSTOM_MOCK_ODFrameUpdateWantChat
#define UT_CUSTOM_MOCK_DialogBoxParamA
#define UT_CUSTOM_MOCK_PostMessageA
#define UT_CUSTOM_MOCK_ODKrnlRequestChatToggle
#define UT_CUSTOM_MOCK_ODKrnlRequestKeyboardToggle
#define UT_CUSTOM_MOCK_ODKrnlRequestSysopNextToggle
#define UT_CUSTOM_MOCK_ODKrnlRequestLockout
#define UT_CUSTOM_MOCK_ODFrameCreateToolbar
#define UT_CUSTOM_MOCK_CheckMenuItem
#define UT_CUSTOM_MOCK_ODScrnAdjustWindows
#define UT_CUSTOM_MOCK_RegCreateKeyA
#define UT_CUSTOM_MOCK_RegSetValueExA
#define UT_CUSTOM_MOCK_RegCloseKey
#define UT_CUSTOM_MOCK_ODFrameCreateStatusBar
#define UT_CUSTOM_MOCK_ODKrnlRequestTimeAdjustment
#define UT_CUSTOM_MOCK_ODKrnlRequestInactivityToggle
#define UT_CUSTOM_MOCK_SendMessageA
#define UT_CUSTOM_MOCK_ODFrameSizeStatusBar
#define UT_CUSTOM_MOCK_DefWindowProcA
#define UT_CUSTOM_MOCK_strcpy
#define UT_CUSTOM_MOCK_LoadStringA
static tODFrameWindowInfo ut_info;static tODUIState ut_state;static int ut_message_result;
static unsigned ut_update_calls,ut_remove_calls,ut_destroy_frame_calls,ut_quit_calls,ut_free_calls,ut_shutdown_calls;
static unsigned ut_focus_calls,ut_flash_calls,ut_time_calls,ut_chat_update_calls,ut_dialog_calls,ut_post_calls;
static unsigned ut_chat_calls,ut_keyboard_calls,ut_sysop_calls,ut_lockout_calls,ut_adjust_calls,ut_inactivity_calls;
static unsigned ut_destroy_toolbar_calls,ut_destroy_status_calls,ut_create_toolbar_calls,ut_create_status_calls,ut_adjust_windows_calls;
static unsigned ut_send_calls,ut_size_status_calls,ut_def_calls;static INT ut_adjustment;static BYTE ut_shutdown_reason;
static unsigned ut_help_callback_calls,ut_config_callback_calls;static char *ut_last_freed;
char *utm_strcpy(char *destination,const char *source){char *result=destination;while((*destination++=*source++)!='\0'){}return(result);}
void utm_ODKrnlGetUIState(tODUIState *state){*state=ut_state;}
#ifdef _MSC_VER
LONG utm__InterlockedExchange(LONG volatile *target,LONG value){LONG old=*target;*target=value;return(old);}
#else
LONG WINAPI utm_InterlockedExchange(LONG volatile *target,LONG value){LONG old=*target;*target=value;return(old);}
#endif
UT_WINDOW_LONG_PTR WINAPI UT_GET_WINDOW_LONG_PTR(HWND window,int index){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,window);UT_ASSERT_EQ_INT(GWLP_USERDATA,index);return((UT_WINDOW_LONG_PTR)&ut_info);}
UT_WINDOW_LONG_PTR WINAPI UT_SET_WINDOW_LONG_PTR(HWND window,int index,UT_WINDOW_LONG_PTR value){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,window);UT_ASSERT_EQ_INT(GWLP_USERDATA,index);(void)value;return(0);}
void utm_ODFrameUpdateCmdUI(void){++ut_update_calls;}
HMENU WINAPI utm_GetMenu(HWND window){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,window);return((HMENU)(UINT_PTR)2);}
BOOL WINAPI utm_RemoveMenu(HMENU menu,UINT item,UINT flags){UT_ASSERT_EQ_PTR((HMENU)(UINT_PTR)2,menu);UT_ASSERT(item==ID_HELP_CONTENTS||item==ID_DOOR_CONFIG);UT_ASSERT_EQ_UINT(MF_BYCOMMAND,flags);++ut_remove_calls;return(TRUE);}
static void utm_ODFrameCopyProgramName(char *destination,size_t size){UT_ASSERT(size>4);utm_strcpy(destination,"Door");}
int WINAPI utm_MessageBoxA(HWND window,LPCSTR text,LPCSTR caption,UINT type){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,window);UT_ASSERT_NOT_NULL(text);UT_ASSERT(strcmp(caption,"Door")==0);UT_ASSERT_EQ_UINT(MB_ICONQUESTION|MB_YESNO,type);return(ut_message_result);}
static void utm_ODFrameDestroyWindow(HWND window){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,window);++ut_destroy_frame_calls;}
void WINAPI utm_PostQuitMessage(int code){UT_ASSERT_EQ_INT(0,code);++ut_quit_calls;}
HWND WINAPI utm_GetDlgItem(HWND window,int id){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,window);return((HWND)(UINT_PTR)id);}
static void utm_ODFrameDestroyToolbar(HWND toolbar,tODFrameWindowInfo *info){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)ID_TOOLBAR,toolbar);UT_ASSERT_EQ_PTR(&ut_info,info);++ut_destroy_toolbar_calls;}
static void utm_ODFrameDestroyStatusBar(HWND status){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)ID_STATUSBAR,status);++ut_destroy_status_calls;}
void utm_ODKrnlRequestShutdown(BYTE reason){ut_shutdown_reason=reason;++ut_shutdown_calls;}
void utm_free(void *memory){ut_last_freed=(char *)memory;++ut_free_calls;}
void utm_ODScrnSetFocusToWindow(void){++ut_focus_calls;}
BOOL WINAPI utm_FlashWindow(HWND window,BOOL invert){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,window);UT_ASSERT(invert);++ut_flash_calls;return(TRUE);}
void utm_ODFrameUpdateTimeDisplay(void){++ut_time_calls;}
void utm_ODFrameUpdateWantChat(void){++ut_chat_update_calls;}
INT_PTR WINAPI utm_DialogBoxParamA(HINSTANCE instance,LPCSTR resource,HWND parent,DLGPROC procedure,LPARAM parameter)
{UT_ASSERT_EQ_PTR(ut_info.hInstance,instance);UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,parent);UT_ASSERT_NOT_NULL(procedure);(void)resource;(void)parameter;++ut_dialog_calls;return(1);}
BOOL WINAPI utm_PostMessageA(HWND window,UINT message,WPARAM first,LPARAM second){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,window);UT_ASSERT_EQ_UINT(WM_CLOSE,message);UT_ASSERT_EQ_UINT(0,first);UT_ASSERT_EQ_INT(0,second);++ut_post_calls;return(TRUE);}
void utm_ODKrnlRequestChatToggle(void){++ut_chat_calls;}void utm_ODKrnlRequestKeyboardToggle(void){++ut_keyboard_calls;}void utm_ODKrnlRequestSysopNextToggle(void){++ut_sysop_calls;}void utm_ODKrnlRequestLockout(void){++ut_lockout_calls;}
static HWND utm_ODFrameCreateToolbar(HWND parent,HANDLE instance,tODFrameWindowInfo *info){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,parent);UT_ASSERT_EQ_PTR(ut_info.hInstance,instance);UT_ASSERT_EQ_PTR(&ut_info,info);++ut_create_toolbar_calls;return((HWND)(UINT_PTR)3);}
DWORD WINAPI utm_CheckMenuItem(HMENU menu,UINT item,UINT flags){UT_ASSERT_EQ_PTR((HMENU)(UINT_PTR)2,menu);UT_ASSERT(item==ID_VIEW_TOOL_BAR||item==ID_VIEW_STAT_BAR);UT_ASSERT_EQ_UINT(0,flags&MF_BYPOSITION);return(0);}
void utm_ODScrnAdjustWindows(void){++ut_adjust_windows_calls;}
LSTATUS WINAPI utm_RegCreateKeyA(HKEY key,LPCSTR name,PHKEY result){UT_ASSERT_EQ_PTR(HKEY_CURRENT_USER,key);UT_ASSERT_NOT_NULL(name);*result=(HKEY)(UINT_PTR)4;return(ERROR_SUCCESS);}
LSTATUS WINAPI utm_RegSetValueExA(HKEY key,LPCSTR name,DWORD reserved,DWORD type,const BYTE *data,DWORD size){UT_ASSERT_EQ_PTR((HKEY)(UINT_PTR)4,key);UT_ASSERT_NOT_NULL(name);UT_ASSERT_EQ_UINT(0,reserved);UT_ASSERT_EQ_UINT(REG_DWORD,type);UT_ASSERT_EQ_UINT(sizeof(BOOL),size);UT_ASSERT_NOT_NULL(data);return(ERROR_SUCCESS);}
LSTATUS WINAPI utm_RegCloseKey(HKEY key){UT_ASSERT_EQ_PTR((HKEY)(UINT_PTR)4,key);return(ERROR_SUCCESS);}
static HWND utm_ODFrameCreateStatusBar(HWND parent,HANDLE instance){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,parent);UT_ASSERT_EQ_PTR(ut_info.hInstance,instance);++ut_create_status_calls;return((HWND)(UINT_PTR)5);}
void utm_ODKrnlRequestTimeAdjustment(INT minutes){ut_adjustment=minutes;++ut_adjust_calls;}void utm_ODKrnlRequestInactivityToggle(void){++ut_inactivity_calls;}
LRESULT WINAPI utm_SendMessageA(HWND window,UINT message,WPARAM first,LPARAM second){(void)window;(void)message;(void)first;(void)second;++ut_send_calls;return(0);}
static void utm_ODFrameSizeStatusBar(HWND status){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)ID_STATUSBAR,status);++ut_size_status_calls;}
LRESULT WINAPI utm_DefWindowProcA(HWND window,UINT message,WPARAM first,LPARAM second){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,window);(void)message;(void)first;(void)second;++ut_def_calls;return(123);}
int WINAPI utm_LoadStringA(HINSTANCE instance,UINT id,LPSTR buffer,int size){UT_ASSERT_EQ_PTR(ut_info.hInstance,instance);UT_ASSERT(id!=0);UT_ASSERT(size>7);utm_strcpy(buffer,"command");return(7);}
static void help_callback(void){++ut_help_callback_calls;}static void config_callback(void){++ut_config_callback_calls;}
static void reset_proc(void){memset(&ut_info,0,sizeof(ut_info));memset(&ut_state,0,sizeof(ut_state));ut_info.hInstance=(HINSTANCE)(UINT_PTR)9;hwndCurrentFrame=(HWND)(UINT_PTR)1;lControlStateDirty=1;ut_message_result=IDNO;
 ut_update_calls=ut_remove_calls=ut_destroy_frame_calls=ut_quit_calls=ut_free_calls=ut_shutdown_calls=0;ut_focus_calls=ut_flash_calls=ut_time_calls=ut_chat_update_calls=ut_dialog_calls=ut_post_calls=0;
 ut_chat_calls=ut_keyboard_calls=ut_sysop_calls=ut_lockout_calls=ut_adjust_calls=ut_inactivity_calls=0;ut_destroy_toolbar_calls=ut_destroy_status_calls=ut_create_toolbar_calls=ut_create_status_calls=ut_adjust_windows_calls=0;
 ut_send_calls=ut_size_status_calls=ut_def_calls=0;ut_adjustment=0;ut_shutdown_reason=0;ut_help_callback_calls=ut_config_callback_calls=0;ut_last_freed=NULL;}
static LRESULT call_proc(UINT message,WPARAM first,LPARAM second){return(utt_ODFrameWindowProc((HWND)(UINT_PTR)1,message,first,second));}
static void covers_creation_close_shutdown_and_destroy(void)
{CREATESTRUCT create;reset_proc();memset(&create,0,sizeof(create));create.lpCreateParams=&ut_info;call_proc(WM_CREATE,0,(LPARAM)&create);UT_ASSERT_EQ_UINT(2,ut_remove_calls);UT_ASSERT_EQ_UINT(1,ut_update_calls);
 reset_proc();ut_state.pfHelpCallback=help_callback;ut_state.pfConfigCallback=config_callback;create.lpCreateParams=&ut_info;call_proc(WM_CREATE,0,(LPARAM)&create);UT_ASSERT_EQ_UINT(0,ut_remove_calls);
 reset_proc();ut_message_result=IDNO;call_proc(WM_CLOSE,0,0);UT_ASSERT_EQ_UINT(0,ut_destroy_frame_calls);ut_message_result=IDYES;call_proc(WM_CLOSE,0,0);UT_ASSERT_EQ_UINT(1,ut_destroy_frame_calls);
 reset_proc();call_proc(WM_OD_SHUTDOWN,0,0);UT_ASSERT(ut_info.bProgrammaticShutdown);UT_ASSERT_EQ_UINT(1,ut_quit_calls);
 reset_proc();ut_info.bToolbarOn=TRUE;ut_info.bStatusBarOn=TRUE;call_proc(WM_DESTROY,0,0);UT_ASSERT_EQ_UINT(1,ut_destroy_toolbar_calls);UT_ASSERT_EQ_UINT(1,ut_destroy_status_calls);UT_ASSERT_EQ_UINT(1,ut_shutdown_calls);UT_ASSERT_EQ_INT(ERRORLEVEL_DROPTOBBS,ut_shutdown_reason);UT_ASSERT_EQ_PTR((char *)&ut_info,ut_last_freed);UT_ASSERT_NULL(hwndCurrentFrame);
 reset_proc();ut_info.bProgrammaticShutdown=TRUE;call_proc(WM_DESTROY,0,0);UT_ASSERT_EQ_UINT(0,ut_shutdown_calls);}
static void covers_simple_frame_messages(void)
{reset_proc();call_proc(WM_SETFOCUS,0,0);call_proc(WM_TIMER,0,0);call_proc(WM_OD_UPDATE_COMMANDS,0,0);call_proc(WM_OD_UPDATE_TIME,0,0);call_proc(WM_OD_UPDATE_CHAT,0,0);call_proc(WM_OD_CONTROL_STATE,0,0);
 UT_ASSERT_EQ_UINT(1,ut_focus_calls);UT_ASSERT_EQ_UINT(1,ut_flash_calls);UT_ASSERT_EQ_UINT(2,ut_update_calls);UT_ASSERT_EQ_UINT(2,ut_time_calls);UT_ASSERT_EQ_UINT(2,ut_chat_update_calls);UT_ASSERT_EQ_INT(0,lControlStateDirty);}
static LRESULT command(UINT id,UINT notification,LPARAM source){return(call_proc(WM_COMMAND,MAKEWPARAM(id,notification),source));}
static void covers_callback_and_session_commands(void)
{reset_proc();UT_ASSERT_EQ_INT(FALSE,command(ID_HELP_ABOUT,0,0));UT_ASSERT_EQ_UINT(1,ut_dialog_calls);
 command(ID_HELP_CONTENTS,0,0);ut_state.pfHelpCallback=help_callback;command(ID_HELP_CONTENTS,0,0);UT_ASSERT_EQ_UINT(1,ut_help_callback_calls);
 command(ID_DOOR_CONFIG,0,0);ut_state.pfConfigCallback=config_callback;command(ID_DOOR_CONFIG,0,0);UT_ASSERT_EQ_UINT(1,ut_config_callback_calls);
 command(ID_DOOR_EXIT,0,0);command(ID_DOOR_CHATMODE,0,0);command(ID_DOOR_USERKEYBOARDOFF,0,0);command(ID_DOOR_SYSOPNEXT,0,0);
 UT_ASSERT_EQ_UINT(1,ut_post_calls);UT_ASSERT_EQ_UINT(1,ut_chat_calls);UT_ASSERT_EQ_UINT(1,ut_keyboard_calls);UT_ASSERT_EQ_UINT(1,ut_sysop_calls);
 ut_message_result=IDNO;command(ID_DOOR_HANGUP,0,0);ut_message_result=IDYES;command(ID_DOOR_HANGUP,0,0);UT_ASSERT_EQ_UINT(1,ut_shutdown_calls);UT_ASSERT_EQ_INT(ERRORLEVEL_HANGUP,ut_shutdown_reason);
 ut_message_result=IDNO;command(ID_DOOR_LOCKOUT,0,0);ut_message_result=IDYES;command(ID_DOOR_LOCKOUT,0,0);UT_ASSERT_EQ_UINT(1,ut_lockout_calls);}
static void covers_view_and_time_commands(void)
{reset_proc();ut_info.bToolbarOn=TRUE;command(ID_VIEW_TOOL_BAR,0,0);UT_ASSERT(!ut_info.bToolbarOn);UT_ASSERT_EQ_UINT(1,ut_destroy_toolbar_calls);
 command(ID_VIEW_TOOL_BAR,0,0);UT_ASSERT(ut_info.bToolbarOn);UT_ASSERT_EQ_UINT(1,ut_create_toolbar_calls);UT_ASSERT_EQ_UINT(1,ut_update_calls);UT_ASSERT_EQ_UINT(2,ut_adjust_windows_calls);
 reset_proc();ut_info.bStatusBarOn=TRUE;command(ID_VIEW_STAT_BAR,0,0);UT_ASSERT(!ut_info.bStatusBarOn);UT_ASSERT_EQ_UINT(1,ut_destroy_status_calls);
 command(ID_VIEW_STAT_BAR,0,0);UT_ASSERT(ut_info.bStatusBarOn);UT_ASSERT_EQ_UINT(1,ut_create_status_calls);UT_ASSERT_EQ_UINT(2,ut_adjust_windows_calls);
 reset_proc();command(ID_USER_ADDONEMINUTE,0,0);UT_ASSERT_EQ_INT(1,ut_adjustment);command(ID_USER_ADDFIVEMINUTES,0,0);UT_ASSERT_EQ_INT(5,ut_adjustment);command(ID_USER_SUBTRACTONEMINUTE,0,0);UT_ASSERT_EQ_INT(-1,ut_adjustment);command(ID_USER_SUBTRACTFIVEMINUTES,0,0);UT_ASSERT_EQ_INT(-5,ut_adjustment);UT_ASSERT_EQ_UINT(4,ut_adjust_calls);
 command(ID_USER_INACTIVITYTIMER,0,0);UT_ASSERT_EQ_UINT(1,ut_inactivity_calls);UT_ASSERT(command(ID_TIME_EDIT,0,(LPARAM)(HWND)(UINT_PTR)6));UT_ASSERT(command(0xffff,0,0));}
static void covers_notifications_scroll_and_resize(void)
{TOOLTIPTEXTA tooltip;NMUPDOWN delta;static const UINT ids[]={ID_DOOR_EXIT,ID_DOOR_CHATMODE,ID_DOOR_USERKEYBOARDOFF,ID_DOOR_SYSOPNEXT,ID_DOOR_HANGUP,ID_DOOR_LOCKOUT,999};unsigned i;
 reset_proc();memset(&tooltip,0,sizeof(tooltip));tooltip.hdr.code=0;call_proc(WM_NOTIFY,0,(LPARAM)&tooltip);
 for(i=0;i<sizeof(ids)/sizeof(ids[0]);++i){memset(&tooltip,0,sizeof(tooltip));tooltip.hdr.code=TTN_NEEDTEXT;tooltip.hdr.idFrom=ids[i];call_proc(WM_NOTIFY,0,(LPARAM)&tooltip);if(i<6)UT_ASSERT_NOT_NULL(tooltip.lpszText);}
 reset_proc();memset(&delta,0,sizeof(delta));delta.hdr.code=UDN_DELTAPOS;delta.iDelta=-44;call_proc(WM_NOTIFY,0,(LPARAM)&delta);UT_ASSERT_EQ_UINT(1,ut_adjust_calls);UT_ASSERT_EQ_INT(44,ut_adjustment);
 reset_proc();call_proc(WM_SIZE,3,4);UT_ASSERT_EQ_UINT(2,ut_send_calls);UT_ASSERT_EQ_UINT(1,ut_size_status_calls);}
static void covers_menu_status_text_and_default_dispatch(void)
{static const UINT system_ids[]={SC_SIZE,SC_MOVE,SC_MINIMIZE,SC_MAXIMIZE,SC_CLOSE,SC_RESTORE,SC_TASKLIST,0x1234};unsigned i;
 reset_proc();UT_ASSERT_EQ_INT(123,call_proc(WM_MENUSELECT,MAKEWPARAM(0,0xffff),0));
 UT_ASSERT_EQ_INT(123,call_proc(WM_MENUSELECT,MAKEWPARAM(ID_HELP_ABOUT,0),0));
 for(i=0;i<sizeof(system_ids)/sizeof(system_ids[0]);++i)UT_ASSERT_EQ_INT(123,call_proc(WM_MENUSELECT,MAKEWPARAM(system_ids[i],MF_SYSMENU),0));
 UT_ASSERT_EQ_UINT(10,ut_def_calls);UT_ASSERT_EQ_INT(123,call_proc(WM_PAINT,0,0));}
static void covers_message_dialog_ownership(void)
{char first[8],second[8],third[8];reset_proc();call_proc(WM_SHOW_MESSAGE,5,(LPARAM)first);UT_ASSERT_EQ_UINT(1,ut_dialog_calls);UT_ASSERT_EQ_PTR(first,ut_last_freed);UT_ASSERT_NULL(ut_info.pszCurrentMessage);
 reset_proc();ut_info.hwndMessageWindow=(HWND)(UINT_PTR)8;call_proc(WM_SHOW_MESSAGE,0,(LPARAM)second);UT_ASSERT_EQ_PTR(second,ut_last_freed);
 reset_proc();call_proc(WM_REMOVE_MESSAGE,0,0);UT_ASSERT_EQ_UINT(0,ut_send_calls);ut_info.hwndMessageWindow=(HWND)(UINT_PTR)8;call_proc(WM_REMOVE_MESSAGE,0,0);UT_ASSERT_EQ_UINT(1,ut_send_calls);UT_ASSERT_NULL(ut_info.hwndMessageWindow);(void)third;}
static const UTTestCase ut_cases[]={{"lifetime messages",covers_creation_close_shutdown_and_destroy},{"simple messages",covers_simple_frame_messages},{"session commands",covers_callback_and_session_commands},{"view/time commands",covers_view_and_time_commands},{"notifications",covers_notifications_scroll_and_resize},{"menu/default",covers_menu_status_text_and_default_dispatch},{"message ownership",covers_message_dialog_ownership}};
#endif
