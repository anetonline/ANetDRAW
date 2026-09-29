#ifdef ODPLAT_WIN32
#include <stdarg.h>
#define UT_CUSTOM_MOCK_ODKrnlGetUIState
#define UT_CUSTOM_MOCK_ODStringCopy
#define UT_CUSTOM_MOCK_sprintf
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_strcat
#define UT_CUSTOM_MOCK_SendMessageA
static unsigned ut_state_calls;static tODUIState ut_state;static char ut_status_text[512];
size_t utm_strlen(const char *text);
char *utm_strcat(char *destination,const char *source);
void utm_ODKrnlGetUIState(tODUIState *state){++ut_state_calls;*state=ut_state;}
void utm_ODStringCopy(char *destination,const char *source,INT size)
{size_t n=0;while(source[n])++n;if(n>=(size_t)size)n=(size_t)size-1;memcpy(destination,source,n);destination[n]='\0';}
int utm_sprintf(char *destination,const char *format,...)
{va_list args;const char *name,*location;va_start(args,format);name=va_arg(args,const char *);location=va_arg(args,const char *);
 strcpy(ut_status_text,name);utm_strcat(ut_status_text," of ");utm_strcat(ut_status_text,location);
 if(strcmp(format,"%s of %s in local mode")==0)utm_strcat(ut_status_text," in local mode");
 else {DWORD speed=va_arg(args,DWORD);UT_ASSERT_EQ_UINT(57600,speed);utm_strcat(ut_status_text," at 57600bps");}va_end(args);
 strcpy(destination,ut_status_text);return((int)utm_strlen(destination));}
size_t utm_strlen(const char *text){size_t n=0;while(text[n])++n;return(n);}
char *utm_strcat(char *destination,const char *source){char *p=destination+utm_strlen(destination);while((*p++=*source++)!='\0'){}return(destination);}
LRESULT WINAPI utm_SendMessageA(HWND window,UINT message,WPARAM part,LPARAM text)
{UT_ASSERT_EQ_PTR((HWND)(UINT_PTR)1,window);UT_ASSERT_EQ_UINT(SB_SETTEXT,message);UT_ASSERT_EQ_UINT(0,part);strcpy(ut_status_text,(const char *)text);return(0);}
static void reset_status(void){memset(&ut_state,0,sizeof(ut_state));strcpy(ut_state.szUserName,"Alice");strcpy(ut_state.szUserLocation,"Detroit");strcpy(ut_state.szUserReasonForChat,"Help");
 ut_state.dwConnectSpeed=57600;ut_state.bUserWantsChat=FALSE;ut_state_calls=0;ut_status_text[0]='\0';}
static void formats_local_and_remote_connections(void)
{reset_status();ut_state.dwBaud=0;utt_ODFrameSetMainStatusText((HWND)(UINT_PTR)1);UT_ASSERT(strcmp(ut_status_text,"Alice of Detroit in local mode")==0);
 reset_status();ut_state.dwBaud=38400;utt_ODFrameSetMainStatusText((HWND)(UINT_PTR)1);UT_ASSERT(strcmp(ut_status_text,"Alice of Detroit at 57600bps")==0);}
static void appends_only_a_nonempty_requested_chat_reason(void)
{reset_status();ut_state.dwBaud=0;ut_state.bUserWantsChat=TRUE;utt_ODFrameSetMainStatusText((HWND)(UINT_PTR)1);
 UT_ASSERT(strcmp(ut_status_text,"Alice of Detroit in local mode (Reason for chat: \"Help\")")==0);
 reset_status();ut_state.bUserWantsChat=TRUE;ut_state.szUserReasonForChat[0]='\0';utt_ODFrameSetMainStatusText((HWND)(UINT_PTR)1);
 UT_ASSERT(strcmp(ut_status_text,"Alice of Detroit in local mode")==0);
 reset_status();ut_state.bUserWantsChat=FALSE;utt_ODFrameSetMainStatusText((HWND)(UINT_PTR)1);}
static const UTTestCase ut_cases[]={{"connection text",formats_local_and_remote_connections},{"chat reason",appends_only_a_nonempty_requested_chat_reason}};
#endif
