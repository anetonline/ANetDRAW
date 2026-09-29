#ifdef ODPLAT_WIN32
#include "winptr.h"
#define UT_CUSTOM_MOCK_DestroyAcceleratorTable
#define UT_CUSTOM_MOCK_DestroyWindow
static tODFrameWindowInfo ut_info;static unsigned ut_accel_calls,ut_window_calls;
UT_WINDOW_LONG_PTR WINAPI UT_GET_WINDOW_LONG_PTR(HWND window,int index){(void)window;UT_ASSERT_EQ_INT(GWLP_USERDATA,index);return((UT_WINDOW_LONG_PTR)&ut_info);}
BOOL WINAPI utm_DestroyAcceleratorTable(HACCEL accelerator){UT_ASSERT_EQ_PTR(ut_info.hacclFrameCommands,accelerator);++ut_accel_calls;return(TRUE);}
BOOL WINAPI utm_DestroyWindow(HWND window){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,window);++ut_window_calls;return(TRUE);}
static void destroys_optional_accelerators_before_the_frame(void)
{memset(&ut_info,0,sizeof(ut_info));ut_accel_calls=ut_window_calls=0;utt_ODFrameDestroyWindow((HWND)(UINT_PTR)1);UT_ASSERT_EQ_UINT(0,ut_accel_calls);UT_ASSERT_EQ_UINT(1,ut_window_calls);
 ut_info.hacclFrameCommands=(HACCEL)(UINT_PTR)2;utt_ODFrameDestroyWindow((HWND)(UINT_PTR)1);UT_ASSERT_EQ_UINT(1,ut_accel_calls);UT_ASSERT_EQ_UINT(2,ut_window_calls);}
static const UTTestCase ut_cases[]={{"destroy frame",destroys_optional_accelerators_before_the_frame}};
#endif
