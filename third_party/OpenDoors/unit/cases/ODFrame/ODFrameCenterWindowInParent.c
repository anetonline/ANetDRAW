#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_GetParent
#define UT_CUSTOM_MOCK_GetWindowRect
#define UT_CUSTOM_MOCK_SetWindowPos
HWND WINAPI utm_GetParent(HWND child){UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,child);return((HWND)(UINT_PTR)2);}
BOOL WINAPI utm_GetWindowRect(HWND window,LPRECT rectangle)
{if(window==(HWND)(UINT_PTR)1){rectangle->left=0;rectangle->top=0;rectangle->right=40;rectangle->bottom=20;}
 else {UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)2,window);rectangle->left=100;rectangle->top=200;rectangle->right=300;rectangle->bottom=300;}return(TRUE);}
BOOL WINAPI utm_SetWindowPos(HWND window,HWND after,int x,int y,int width,int height,UINT flags)
{UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,window);UT_ASSERT_NULL(after);UT_ASSERT_EQ_INT(180,x);UT_ASSERT_EQ_INT(240,y);
 UT_ASSERT_EQ_INT(0,width);UT_ASSERT_EQ_INT(0,height);UT_ASSERT_EQ_UINT(SWP_NOSIZE|SWP_NOZORDER,flags);return(TRUE);}
static void centers_in_the_parent_rectangle(void){utt_ODFrameCenterWindowInParent((HWND)(UINT_PTR)1);}
static const UTTestCase ut_cases[]={{"center",centers_in_the_parent_rectangle}};
#endif
