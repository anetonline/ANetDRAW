#ifdef ODPLAT_WIN32
#include "winptr.h"
#define UT_CUSTOM_MOCK_TranslateAcceleratorA
static tODFrameWindowInfo ut_info;static int ut_result;
UT_WINDOW_LONG_PTR WINAPI UT_GET_WINDOW_LONG_PTR(HWND window,int index)
{UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,window);UT_ASSERT_EQ_INT(GWLP_USERDATA,index);return((LONG_PTR)&ut_info);}
int WINAPI utm_TranslateAcceleratorA(HWND window,HACCEL accelerator,LPMSG message)
{(void)message;UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,window);UT_ASSERT_EQ_PTR(ut_info.hacclFrameCommands,accelerator);return(ut_result);}
static void converts_the_windows_result_to_boolean(void)
{MSG message;ut_info.hacclFrameCommands=(HACCEL)(UINT_PTR)2;ut_result=0;UT_ASSERT(!utt_ODFrameTranslateAccelerator((HWND)(UINT_PTR)1,&message));
 ut_result=2;UT_ASSERT(utt_ODFrameTranslateAccelerator((HWND)(UINT_PTR)1,&message));}
static const UTTestCase ut_cases[]={{"translation result",converts_the_windows_result_to_boolean}};
#endif
