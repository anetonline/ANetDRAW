#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_CreateToolbarEx
#define UT_CUSTOM_MOCK_CreateWindowExA
#include "winptr.h"
#define UT_CUSTOM_MOCK_SetWindowPos
#define UT_CUSTOM_MOCK_GetStockObject
#define UT_CUSTOM_MOCK_SendMessageA
#define UT_CUSTOM_MOCK_ODFrameUpdateTimeLeft
#define UT_CUSTOM_MOCK_DestroyWindow
static HWND ut_toolbar,ut_edit,ut_updown,ut_tooltip;static unsigned ut_create_calls,ut_destroy_calls,ut_update_calls;
static HWND ut_destroyed[3];
HWND WINAPI utm_CreateToolbarEx(HWND parent,DWORD style,UINT id,int bitmaps,HINSTANCE instance,UINT_PTR bitmap,LPCTBBUTTON buttons,int count,int bx,int by,int ix,int iy,UINT size)
{UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,parent);UT_ASSERT((style&WS_CHILD)!=0);UT_ASSERT_EQ_UINT(ID_TOOLBAR,id);UT_ASSERT_EQ_INT(NUM_TOOLBAR_BITMAPS,bitmaps);UT_ASSERT_EQ_PTR((HINSTANCE)(UINT_PTR)2,instance);UT_ASSERT_EQ_UINT(IDB_TOOLBAR,bitmap);UT_ASSERT_EQ_PTR(atbButtons,buttons);UT_ASSERT_EQ_INT(DIM(atbButtons),count);(void)bx;(void)by;(void)ix;(void)iy;UT_ASSERT_EQ_UINT(sizeof(TBBUTTON),size);return(ut_toolbar);}
HWND WINAPI utm_CreateWindowExA(DWORD ex,LPCSTR class_name,LPCSTR title,DWORD style,int x,int y,int width,int height,HWND parent,HMENU menu,HINSTANCE instance,LPVOID parameter)
{unsigned call=ut_create_calls++;UT_ASSERT_EQ_PTR(ut_toolbar,parent);UT_ASSERT_EQ_PTR((HINSTANCE)(UINT_PTR)2,instance);UT_ASSERT_NULL(parameter);UT_ASSERT(strcmp(title,"")==0);(void)style;(void)x;(void)y;(void)width;(void)height;
 if(call==0){UT_ASSERT_EQ_UINT(WS_EX_STATICEDGE,ex);UT_ASSERT(strcmp(class_name,"STATIC")==0);UT_ASSERT_EQ_PTR((HMENU)(UINT_PTR)ID_TIME_EDIT,menu);return(ut_edit);}
 UT_ASSERT_EQ_UINT(0,ex);UT_ASSERT(strcmp(class_name,UPDOWN_CLASS)==0);UT_ASSERT_EQ_PTR((HMENU)(UINT_PTR)ID_TIME_UPDOWN,menu);return(ut_updown);}
UT_WINDOW_LONG_PTR WINAPI UT_GET_WINDOW_LONG_PTR(HWND window,int index){UT_ASSERT_EQ_PTR(ut_toolbar,window);UT_ASSERT_EQ_INT(GWLP_WNDPROC,index);return((UT_WINDOW_LONG_PTR)(UINT_PTR)0x1234);}
UT_WINDOW_LONG_PTR WINAPI UT_SET_WINDOW_LONG_PTR(HWND window,int index,UT_WINDOW_LONG_PTR value){UT_ASSERT_EQ_PTR(ut_toolbar,window);UT_ASSERT_EQ_INT(GWLP_WNDPROC,index);UT_ASSERT(value!=0);return(0);}
BOOL WINAPI utm_SetWindowPos(HWND window,HWND after,int x,int y,int width,int height,UINT flags){UT_ASSERT_EQ_PTR(ut_edit,window);UT_ASSERT_NULL(after);UT_ASSERT_EQ_INT(2,x);UT_ASSERT_EQ_INT(2,y);(void)width;(void)height;UT_ASSERT_EQ_UINT(SWP_NOZORDER|SWP_NOSIZE,flags);return(TRUE);}
HGDIOBJ WINAPI utm_GetStockObject(int object){UT_ASSERT_EQ_INT(DEFAULT_GUI_FONT,object);return((HGDIOBJ)(UINT_PTR)9);}
LRESULT WINAPI utm_SendMessageA(HWND window,UINT message,WPARAM first,LPARAM second)
{if(window==ut_toolbar&&message==TB_GETTOOLTIPS)return((LRESULT)(UINT_PTR)ut_tooltip);
 if(window==ut_edit){UT_ASSERT_EQ_UINT(WM_SETFONT,message);UT_ASSERT_EQ_PTR((HGDIOBJ)(UINT_PTR)9,(HGDIOBJ)first);return(0);}
 if(window==ut_tooltip){UT_ASSERT_EQ_UINT(TTM_ADDTOOL,message);UT_ASSERT_EQ_UINT(0,first);UT_ASSERT_NOT_NULL((void *)second);return(0);}
 UT_ASSERT_EQ_PTR(ut_updown,window);UT_ASSERT(message==UDM_SETBUDDY||message==UDM_SETRANGE||message==UDM_SETACCEL);
 if(message==UDM_SETACCEL){const UDACCEL *acceleration=(const UDACCEL *)second;UT_ASSERT_EQ_UINT(3,first);UT_ASSERT_EQ_UINT(0,acceleration[0].nSec);UT_ASSERT_EQ_UINT(1,acceleration[0].nInc);UT_ASSERT_EQ_UINT(2,acceleration[1].nSec);UT_ASSERT_EQ_UINT(5,acceleration[1].nInc);UT_ASSERT_EQ_UINT(5,acceleration[2].nSec);UT_ASSERT_EQ_UINT(10,acceleration[2].nInc);}return(0);}
static void utm_ODFrameUpdateTimeLeft(tODFrameWindowInfo *info){UT_ASSERT_EQ_PTR(ut_edit,info->hwndTimeEdit);UT_ASSERT_EQ_PTR(ut_updown,info->hwndTimeUpDown);++ut_update_calls;}
BOOL WINAPI utm_DestroyWindow(HWND window){UT_ASSERT(ut_destroy_calls<3);ut_destroyed[ut_destroy_calls++]=window;return(TRUE);}
static void reset_toolbar(void){ut_toolbar=(HWND)(UINT_PTR)3;ut_edit=(HWND)(UINT_PTR)4;ut_updown=(HWND)(UINT_PTR)5;ut_tooltip=(HWND)(UINT_PTR)6;ut_create_calls=ut_destroy_calls=ut_update_calls=0;pfnDefToolbarProc=NULL;}
static HWND call_toolbar(tODFrameWindowInfo *info){memset(info,0,sizeof(*info));return(utt_ODFrameCreateToolbar((HWND)(UINT_PTR)1,(HANDLE)(UINT_PTR)2,info));}
static void unwinds_each_creation_failure_at_the_current_contract(void)
{tODFrameWindowInfo info;reset_toolbar();ut_toolbar=NULL;UT_ASSERT_NULL(call_toolbar(&info));UT_ASSERT_EQ_UINT(0,ut_destroy_calls);
 reset_toolbar();ut_edit=NULL;UT_ASSERT_NULL(call_toolbar(&info));UT_ASSERT_EQ_UINT(1,ut_destroy_calls);UT_ASSERT_EQ_PTR(ut_toolbar,ut_destroyed[0]);
 reset_toolbar();ut_updown=NULL;UT_ASSERT_NULL(call_toolbar(&info));UT_ASSERT_EQ_UINT(2,ut_destroy_calls);UT_ASSERT_EQ_PTR(ut_edit,ut_destroyed[0]);UT_ASSERT_EQ_PTR(ut_toolbar,ut_destroyed[1]);}
static void creates_and_subclasses_all_toolbar_controls(void)
{tODFrameWindowInfo info;reset_toolbar();ut_tooltip=NULL;UT_ASSERT_EQ_PTR(ut_toolbar,call_toolbar(&info));UT_ASSERT_EQ_PTR(ut_edit,info.hwndTimeEdit);UT_ASSERT_EQ_PTR(ut_updown,info.hwndTimeUpDown);UT_ASSERT_EQ_UINT(1,ut_update_calls);UT_ASSERT_EQ_UINT(0,ut_destroy_calls);
 reset_toolbar();UT_ASSERT_EQ_PTR(ut_toolbar,call_toolbar(&info));UT_ASSERT_EQ_UINT(1,ut_update_calls);}
static const UTTestCase ut_cases[]={{"creation failures",unwinds_each_creation_failure_at_the_current_contract},{"toolbar controls",creates_and_subclasses_all_toolbar_controls}};
#endif
