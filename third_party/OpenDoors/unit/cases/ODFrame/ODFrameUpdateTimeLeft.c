#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODKrnlGetUIState
#define UT_CUSTOM_MOCK_sprintf
#define UT_CUSTOM_MOCK_SetWindowTextA
#define UT_CUSTOM_MOCK_SendMessageA
static unsigned ut_state_calls,ut_text_calls;static tODUIState ut_state;
void utm_ODKrnlGetUIState(tODUIState *state){++ut_state_calls;*state=ut_state;}
int utm_sprintf(char *destination,const char *format,...)
{ UT_ASSERT(strcmp(format,"%d min.")==0); strcpy(destination,"42 min."); return(7); }
BOOL WINAPI utm_SetWindowTextA(HWND window,LPCSTR text)
{UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,window);UT_ASSERT(strcmp(text,"42 min.")==0);++ut_text_calls;return(TRUE);}
LRESULT WINAPI utm_SendMessageA(HWND window,UINT message,WPARAM first,LPARAM second)
{UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)2,window);UT_ASSERT_EQ_UINT(UDM_SETPOS,message);UT_ASSERT_EQ_UINT(0,first);
 UT_ASSERT_EQ_INT(42,LOWORD(second));return(0);}
static void ignores_an_absent_edit_control(void)
{tODFrameWindowInfo info;memset(&info,0,sizeof(info));ut_state_calls=ut_text_calls=0;utt_ODFrameUpdateTimeLeft(&info);UT_ASSERT_EQ_UINT(0,ut_text_calls);UT_ASSERT_EQ_UINT(0,ut_state_calls);}
static void updates_text_redraw_and_spinner(void)
{tODFrameWindowInfo info;memset(&info,0,sizeof(info));memset(&ut_state,0,sizeof(ut_state));info.hwndTimeEdit=(HWND)(UINT_PTR)1;info.hwndTimeUpDown=(HWND)(UINT_PTR)2;
 ut_state.nTimeLimit=42;ut_state_calls=ut_text_calls=0;utt_ODFrameUpdateTimeLeft(&info);UT_ASSERT_EQ_UINT(1,ut_text_calls);UT_ASSERT_EQ_UINT(1,ut_state_calls);}
static const UTTestCase ut_cases[]={{"no edit",ignores_an_absent_edit_control},{"time update",updates_text_redraw_and_spinner}};
#endif
