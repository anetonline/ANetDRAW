#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_memset
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_LoadIconA
#define UT_CUSTOM_MOCK_LoadCursorA
#define UT_CUSTOM_MOCK_RegisterClassA
#define UT_CUSTOM_MOCK_RegCreateKeyA
#define UT_CUSTOM_MOCK_RegQueryValueExA
#define UT_CUSTOM_MOCK_RegSetValueExA
#define UT_CUSTOM_MOCK_RegCloseKey
#define UT_CUSTOM_MOCK_CreateWindowExA
#define UT_CUSTOM_MOCK_LoadAcceleratorsA
#define UT_CUSTOM_MOCK_ODFrameCreateToolbar
#define UT_CUSTOM_MOCK_ODFrameCreateStatusBar
#define UT_CUSTOM_MOCK_ODFrameUpdateWantChat
#define UT_CUSTOM_MOCK_ODScrnStartWindow
#define UT_CUSTOM_MOCK_ODFrameDestroyWindow
#define UT_CUSTOM_MOCK_ODKrnlGetUIState
static tODFrameWindowInfo ut_storage;static BOOL ut_malloc_fails;static HWND ut_frame;
static tODUIState ut_state;
static LSTATUS ut_query_results[2];static DWORD ut_query_values[2];static unsigned ut_query_calls,ut_set_calls,ut_free_calls;
static unsigned ut_toolbar_calls,ut_status_calls,ut_update_calls,ut_destroy_calls;static tODResult ut_screen_result;static BOOL ut_loaded_icon;
void *utm_memset(void *memory,int value,size_t size){unsigned char *p=memory;size_t i;for(i=0;i<size;++i)p[i]=(unsigned char)value;return(memory);}
void *utm_malloc(size_t size){UT_ASSERT_EQ_UINT(sizeof(tODFrameWindowInfo),size);return(ut_malloc_fails?NULL:&ut_storage);}
void utm_free(void *memory){UT_ASSERT_EQ_PTR(&ut_storage,memory);++ut_free_calls;}
HICON WINAPI utm_LoadIconA(HINSTANCE instance,LPCSTR name){UT_ASSERT_EQ_PTR((HINSTANCE)(UINT_PTR)1,instance);UT_ASSERT_EQ_PTR(MAKEINTRESOURCE(IDI_OPENDOORS),name);ut_loaded_icon=TRUE;return((HICON)(UINT_PTR)8);}
HCURSOR WINAPI utm_LoadCursorA(HINSTANCE instance,LPCSTR name){UT_ASSERT_NULL(instance);UT_ASSERT_EQ_PTR(IDC_ARROW,name);return((HCURSOR)(UINT_PTR)9);}
ATOM WINAPI utm_RegisterClassA(const WNDCLASSA *window_class){UT_ASSERT(strcmp(window_class->lpszClassName,"ODFrame")==0);UT_ASSERT_EQ_PTR((HINSTANCE)(UINT_PTR)1,window_class->hInstance);return(1);}
LSTATUS WINAPI utm_RegCreateKeyA(HKEY key,LPCSTR subkey,PHKEY result){UT_ASSERT_EQ_PTR(HKEY_CURRENT_USER,key);UT_ASSERT(strcmp(subkey,"Software\\Pirie\\OpenDoors")==0);*result=(HKEY)(UINT_PTR)10;return(ERROR_SUCCESS);}
LSTATUS WINAPI utm_RegQueryValueExA(HKEY key,LPCSTR name,LPDWORD reserved,LPDWORD type,LPBYTE data,LPDWORD size)
{unsigned call=ut_query_calls++;UT_ASSERT_EQ_PTR((HKEY)(UINT_PTR)10,key);UT_ASSERT_NULL(reserved);UT_ASSERT_NULL(type);UT_ASSERT_EQ_UINT(sizeof(BOOL),*size);
 UT_ASSERT(strcmp(name,call==0?"ToolBarOn":"StatusBarOn")==0);*(BOOL *)data=(BOOL)ut_query_values[call];return(ut_query_results[call]);}
LSTATUS WINAPI utm_RegSetValueExA(HKEY key,LPCSTR name,DWORD reserved,DWORD type,const BYTE *data,DWORD size)
{UT_ASSERT_EQ_PTR((HKEY)(UINT_PTR)10,key);UT_ASSERT(name!=NULL);UT_ASSERT_EQ_UINT(0,reserved);UT_ASSERT_EQ_UINT(REG_DWORD,type);UT_ASSERT_EQ_UINT(sizeof(BOOL),size);UT_ASSERT(*(const BOOL *)data);++ut_set_calls;return(ERROR_SUCCESS);}
LSTATUS WINAPI utm_RegCloseKey(HKEY key){UT_ASSERT_EQ_PTR((HKEY)(UINT_PTR)10,key);return(ERROR_SUCCESS);}
HWND WINAPI utm_CreateWindowExA(DWORD ex,LPCSTR class_name,LPCSTR title,DWORD style,int x,int y,int width,int height,HWND parent,HMENU menu,HINSTANCE instance,LPVOID parameter)
{UT_ASSERT_EQ_UINT(0,ex);UT_ASSERT(strcmp(class_name,"ODFrame")==0);UT_ASSERT(strcmp(title,ut_state.szProgramName)==0);UT_ASSERT((style&WS_CAPTION)!=0);(void)x;(void)y;(void)width;(void)height;UT_ASSERT_NULL(parent);UT_ASSERT_NULL(menu);UT_ASSERT_EQ_PTR((HINSTANCE)(UINT_PTR)1,instance);UT_ASSERT_EQ_PTR(&ut_storage,parameter);return(ut_frame);}
HACCEL WINAPI utm_LoadAcceleratorsA(HINSTANCE instance,LPCSTR name){UT_ASSERT_EQ_PTR((HINSTANCE)(UINT_PTR)1,instance);UT_ASSERT_EQ_PTR(MAKEINTRESOURCE(IDR_FRAME),name);return((HACCEL)(UINT_PTR)11);}
static HWND utm_ODFrameCreateToolbar(HWND parent,HANDLE instance,tODFrameWindowInfo *info){UT_ASSERT_EQ_PTR(ut_frame,parent);UT_ASSERT_EQ_PTR((HANDLE)(UINT_PTR)1,instance);UT_ASSERT_EQ_PTR(&ut_storage,info);++ut_toolbar_calls;return((HWND)(UINT_PTR)12);}
static HWND utm_ODFrameCreateStatusBar(HWND parent,HANDLE instance){UT_ASSERT_EQ_PTR(ut_frame,parent);UT_ASSERT_EQ_PTR((HANDLE)(UINT_PTR)1,instance);++ut_status_calls;return((HWND)(UINT_PTR)13);}
void utm_ODFrameUpdateWantChat(void){++ut_update_calls;}
tODResult utm_ODScrnStartWindow(HANDLE instance,HWND parent){UT_ASSERT_EQ_PTR((HANDLE)(UINT_PTR)1,instance);UT_ASSERT_EQ_PTR(ut_frame,parent);return(ut_screen_result);}
static void utm_ODFrameDestroyWindow(HWND frame){UT_ASSERT_EQ_PTR(ut_frame,frame);UT_ASSERT(ut_storage.bProgrammaticShutdown);++ut_destroy_calls;}
void utm_ODKrnlGetUIState(tODUIState *state){*state=ut_state;}
static void reset_window(void){utm_memset(&ut_storage,0,sizeof(ut_storage));utm_memset(&ut_state,0,sizeof(ut_state));ut_malloc_fails=FALSE;ut_frame=(HWND)(UINT_PTR)2;ut_query_results[0]=ut_query_results[1]=ERROR_SUCCESS;
 ut_query_values[0]=ut_query_values[1]=FALSE;ut_query_calls=ut_set_calls=ut_free_calls=0;ut_toolbar_calls=ut_status_calls=ut_update_calls=ut_destroy_calls=0;ut_screen_result=kODRCSuccess;ut_loaded_icon=FALSE;
 strcpy(ut_state.szProgramName,"Door");ut_state.hAppIcon=NULL;}
static void reports_allocation_and_frame_creation_failures(void)
{reset_window();ut_malloc_fails=TRUE;UT_ASSERT_NULL(utt_ODFrameCreateWindow((HANDLE)(UINT_PTR)1));UT_ASSERT_EQ_UINT(0,ut_query_calls);
 reset_window();ut_frame=NULL;UT_ASSERT_NULL(utt_ODFrameCreateWindow((HANDLE)(UINT_PTR)1));UT_ASSERT_EQ_UINT(1,ut_free_calls);}
static void uses_registry_defaults_and_the_default_icon(void)
{reset_window();ut_query_results[0]=ut_query_results[1]=ERROR_FILE_NOT_FOUND;UT_ASSERT_EQ_PTR(ut_frame,utt_ODFrameCreateWindow((HANDLE)(UINT_PTR)1));
 UT_ASSERT(ut_loaded_icon);UT_ASSERT_EQ_UINT(2,ut_set_calls);UT_ASSERT(ut_storage.bToolbarOn);UT_ASSERT(ut_storage.bStatusBarOn);UT_ASSERT_EQ_UINT(1,ut_toolbar_calls);UT_ASSERT_EQ_UINT(1,ut_status_calls);UT_ASSERT_EQ_UINT(1,ut_update_calls);UT_ASSERT_EQ_PTR((HACCEL)(UINT_PTR)11,ut_storage.hacclFrameCommands);}
static void honors_disabled_children_and_a_custom_icon(void)
{reset_window();ut_state.hAppIcon=(HICON)(UINT_PTR)15;UT_ASSERT_EQ_PTR(ut_frame,utt_ODFrameCreateWindow((HANDLE)(UINT_PTR)1));
 UT_ASSERT(!ut_loaded_icon);UT_ASSERT_EQ_UINT(0,ut_toolbar_calls);UT_ASSERT_EQ_UINT(0,ut_status_calls);}
static void destroys_the_frame_when_the_screen_cannot_start(void)
{reset_window();ut_screen_result=kODRCGeneralFailure;UT_ASSERT_NULL(utt_ODFrameCreateWindow((HANDLE)(UINT_PTR)1));UT_ASSERT_EQ_UINT(1,ut_destroy_calls);}
static const UTTestCase ut_cases[]={{"creation failures",reports_allocation_and_frame_creation_failures},{"registry defaults",uses_registry_defaults_and_the_default_icon},{"custom settings",honors_disabled_children_and_a_custom_icon},{"screen failure",destroys_the_frame_when_the_screen_cannot_start}};
#endif
