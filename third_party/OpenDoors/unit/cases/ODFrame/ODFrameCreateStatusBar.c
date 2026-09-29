#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_CreateWindowExA
#define UT_CUSTOM_MOCK_ODFrameSizeStatusBar
#define UT_CUSTOM_MOCK_ODFrameSetMainStatusText
#define UT_CUSTOM_MOCK_ODKrnlGetUIState
#define UT_CUSTOM_MOCK_sprintf
#define UT_CUSTOM_MOCK_SendMessageA
static HWND ut_create_result;static tODUIState ut_state;static unsigned ut_size_calls,ut_main_calls,ut_state_calls,ut_send_calls;
HWND WINAPI utm_CreateWindowExA(DWORD ex,LPCSTR class_name,LPCSTR title,DWORD style,int x,int y,int width,int height,HWND parent,HMENU menu,HINSTANCE instance,LPVOID parameter)
{UT_ASSERT_EQ_UINT(0,ex);UT_ASSERT(strcmp(class_name,STATUSCLASSNAME)==0);UT_ASSERT(strcmp(title,"")==0);UT_ASSERT_EQ_UINT(WS_CHILD|WS_VISIBLE,style);
 (void)x;(void)y;(void)width;(void)height;UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,parent);UT_ASSERT_EQ_PTR((HMENU)(UINT_PTR)ID_STATUSBAR,menu);UT_ASSERT_EQ_PTR((HINSTANCE)(UINT_PTR)2,instance);UT_ASSERT_NULL(parameter);return(ut_create_result);}
static void utm_ODFrameSizeStatusBar(HWND status){UT_ASSERT_EQ_PTR(ut_create_result,status);++ut_size_calls;}
static void utm_ODFrameSetMainStatusText(HWND status){UT_ASSERT_EQ_PTR(ut_create_result,status);++ut_main_calls;}
void utm_ODKrnlGetUIState(tODUIState *state){++ut_state_calls;*state=ut_state;}
int utm_sprintf(char *destination,const char *format,...){UT_ASSERT(strcmp(format,"Node %d")==0);strcpy(destination,"Node 12");return(7);}
LRESULT WINAPI utm_SendMessageA(HWND status,UINT message,WPARAM part,LPARAM text){UT_ASSERT_EQ_PTR(ut_create_result,status);UT_ASSERT_EQ_UINT(SB_SETTEXT,message);UT_ASSERT_EQ_UINT(1,part);UT_ASSERT(strcmp((char *)text,"Node 12")==0);++ut_send_calls;return(0);}
static void reset_statusbar(void){memset(&ut_state,0,sizeof(ut_state));ut_size_calls=ut_main_calls=ut_state_calls=ut_send_calls=0;ut_state.nNode=12;}
static void returns_null_when_the_control_cannot_be_created(void)
{reset_statusbar();ut_create_result=NULL;UT_ASSERT_NULL(utt_ODFrameCreateStatusBar((HWND)(UINT_PTR)1,(HANDLE)(UINT_PTR)2));UT_ASSERT_EQ_UINT(0,ut_size_calls);}
static void initializes_both_status_parts(void)
{reset_statusbar();ut_create_result=(HWND)(UINT_PTR)3;UT_ASSERT_EQ_PTR(ut_create_result,utt_ODFrameCreateStatusBar((HWND)(UINT_PTR)1,(HANDLE)(UINT_PTR)2));
 UT_ASSERT_EQ_UINT(1,ut_size_calls);UT_ASSERT_EQ_UINT(1,ut_main_calls);UT_ASSERT_EQ_UINT(1,ut_state_calls);UT_ASSERT_EQ_UINT(1,ut_send_calls);}
static const UTTestCase ut_cases[]={{"creation failure",returns_null_when_the_control_cannot_be_created},{"status initialization",initializes_both_status_parts}};
#endif
