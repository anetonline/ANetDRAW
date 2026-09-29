#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_GetParent
#define UT_CUSTOM_MOCK_SendMessageA
#define UT_CUSTOM_MOCK_CallWindowProcA
static UINT ut_message; static unsigned ut_relay_calls;
HWND WINAPI utm_GetParent(HWND window)
{ UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1, window); return((HWND)(UINT_PTR)2); }
LRESULT WINAPI utm_SendMessageA(HWND window, UINT message, WPARAM first, LPARAM second)
{
   if(message==TB_GETTOOLTIPS) { UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)2,window); return((LRESULT)(UINT_PTR)3); }
   UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)3,window); UT_ASSERT_EQ_UINT(TTM_RELAYEVENT,message);
   UT_ASSERT_EQ_UINT(0,first); { MSG *msg=(MSG *)second; UT_ASSERT_EQ_UINT(ut_message,msg->message);
      UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,msg->hwnd); UT_ASSERT_EQ_UINT(4,msg->wParam); UT_ASSERT_EQ_INT(5,msg->lParam); }
   ++ut_relay_calls; return(0);
}
LRESULT WINAPI utm_CallWindowProcA(WNDPROC procedure, HWND window, UINT message,
   WPARAM first, LPARAM second)
{ UT_ASSERT_EQ_PTR(pfnDefEditProc,procedure); UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,window);
  UT_ASSERT_EQ_UINT(ut_message,message); UT_ASSERT_EQ_UINT(4,first); UT_ASSERT_EQ_INT(5,second); return(88); }
static void relays_mouse_messages_to_the_tooltip(void)
{
   static const UINT messages[]={WM_MOUSEMOVE,WM_LBUTTONDOWN,WM_LBUTTONUP,WM_KEYDOWN}; unsigned i;
   pfnDefEditProc=(WNDPROC)(UINT_PTR)0x1234;
   for(i=0;i<4;++i){ut_message=messages[i];ut_relay_calls=0;
      UT_ASSERT_EQ_INT(88,utt_ODFrameTimeEditProc((HWND)(UINT_PTR)1,messages[i],4,5));
      UT_ASSERT_EQ_UINT(i<3?1:0,ut_relay_calls);}
}
static const UTTestCase ut_cases[]={{"tooltip relay",relays_mouse_messages_to_the_tooltip}};
#endif
