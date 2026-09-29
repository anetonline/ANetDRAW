#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODFrameCopyProgramName
#define UT_CUSTOM_MOCK_SetWindowTextA
#define UT_CUSTOM_MOCK_GetDlgItem
#define UT_CUSTOM_MOCK_ODFrameCenterWindowInParent
#define UT_CUSTOM_MOCK_EndDialog
static unsigned ut_text_calls,ut_center_calls,ut_end_calls;static tODFrameWindowInfo ut_info;
static BOOL ut_equal(const char *a,const char *b){while(*a&&*a==*b){++a;++b;}return(*a==*b);}
static void utm_ODFrameCopyProgramName(char *destination,size_t size){UT_ASSERT(size>=8);strcpy(destination,"My Door");}
HWND WINAPI utm_GetDlgItem(HWND dialog,int id){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,dialog);UT_ASSERT_EQ_INT(IDC_MESSAGE_TEXT1,id);return((HWND)(UINT_PTR)2);}
BOOL WINAPI utm_SetWindowTextA(HWND window,LPCSTR text){if(ut_text_calls==0){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,window);UT_ASSERT(ut_equal("My Door",text));}
 else {UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)2,window);UT_ASSERT(ut_equal("Message",text));}++ut_text_calls;return(TRUE);}
void utm_ODFrameCenterWindowInParent(HWND window){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,window);++ut_center_calls;}
BOOL WINAPI utm_EndDialog(HWND dialog,INT_PTR result){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,dialog);UT_ASSERT_EQ_INT(IDOK,result);++ut_end_calls;return(TRUE);}
static void initializes_the_message_dialog(void)
{memset(&ut_info,0,sizeof(ut_info));ut_info.pszCurrentMessage="Message";ut_text_calls=ut_center_calls=0;
 UT_ASSERT(!utt_ODFrameMessageDlgProc((HWND)(UINT_PTR)1,WM_INITDIALOG,0,(LPARAM)&ut_info));
 UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,ut_info.hwndMessageWindow);UT_ASSERT_EQ_UINT(2,ut_text_calls);UT_ASSERT_EQ_UINT(1,ut_center_calls);}
static void handles_only_the_ok_command(void)
{ut_end_calls=0;UT_ASSERT(utt_ODFrameMessageDlgProc((HWND)(UINT_PTR)1,WM_COMMAND,99,0));UT_ASSERT_EQ_UINT(0,ut_end_calls);
 UT_ASSERT(utt_ODFrameMessageDlgProc((HWND)(UINT_PTR)1,WM_COMMAND,IDOK,0));UT_ASSERT_EQ_UINT(1,ut_end_calls);
 UT_ASSERT(!utt_ODFrameMessageDlgProc((HWND)(UINT_PTR)1,WM_PAINT,0,0));}
static const UTTestCase ut_cases[]={{"dialog initialization",initializes_the_message_dialog},{"dialog commands",handles_only_the_ok_command}};
#endif
