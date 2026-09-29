#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_GetMessageA
#define UT_CUSTOM_MOCK_ODFrameTranslateAccelerator
#define UT_CUSTOM_MOCK_TranslateMessage
#define UT_CUSTOM_MOCK_DispatchMessageA
static unsigned ut_get_calls,ut_translate_calls,ut_dispatch_calls;
BOOL WINAPI utm_GetMessageA(LPMSG message,HWND window,UINT first,UINT last)
{(void)message;UT_ASSERT_NULL(window);UT_ASSERT_EQ_UINT(0,first);UT_ASSERT_EQ_UINT(0,last);return(ut_get_calls++<2);}
BOOL utm_ODFrameTranslateAccelerator(HWND frame,LPMSG message)
{(void)message;UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,frame);return(ut_translate_calls++==0);}
BOOL WINAPI utm_TranslateMessage(const MSG *message){(void)message;return(TRUE);}
LRESULT WINAPI utm_DispatchMessageA(const MSG *message){(void)message;++ut_dispatch_calls;return(0);}
static void dispatches_only_untranslated_messages(void)
{ut_get_calls=ut_translate_calls=ut_dispatch_calls=0;utt_ODFrameMessageLoop((HANDLE)(UINT_PTR)2,(HWND)(UINT_PTR)1);
 UT_ASSERT_EQ_UINT(3,ut_get_calls);UT_ASSERT_EQ_UINT(2,ut_translate_calls);UT_ASSERT_EQ_UINT(1,ut_dispatch_calls);}
static const UTTestCase ut_cases[]={{"message loop",dispatches_only_untranslated_messages}};
#endif
