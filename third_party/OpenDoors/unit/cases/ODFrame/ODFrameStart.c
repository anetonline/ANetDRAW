#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_CreateEventA
#define UT_CUSTOM_MOCK_ODThreadCreate
#define UT_CUSTOM_MOCK_WaitForSingleObject
#define UT_CUSTOM_MOCK_CloseHandle
#define UT_CUSTOM_MOCK_ODFrameShutdown
#define UT_CUSTOM_MOCK_ODThreadWaitForExit
static HANDLE ut_event;static tODResult ut_create_result;static DWORD ut_wait_results[2];
static tODResult ut_frame_result;
static unsigned ut_wait_calls,ut_close_event_calls,ut_close_thread_calls,ut_shutdown_calls,ut_join_calls;
DWORD OD_THREAD_FUNC utm_ODFrameThreadProc(void *parameter);
BOOL WINAPI utm_CloseHandle(HANDLE handle);
HANDLE WINAPI utm_CreateEventA(LPSECURITY_ATTRIBUTES attributes,BOOL manual,BOOL initial,LPCSTR name)
{UT_ASSERT_NULL(attributes);UT_ASSERT(manual);UT_ASSERT(!initial);UT_ASSERT_NULL(name);return(ut_event);}
tODResult utm_ODThreadCreate(tODThreadHandle *thread,ptODThreadProc *function,void *parameter)
{UT_ASSERT_EQ_PTR(utm_ODFrameThreadProc,function);UT_ASSERT_EQ_PTR((void *)(UINT_PTR)9,parameter);*thread=(HANDLE)(UINT_PTR)2;
 FrameStartResult=ut_frame_result;return(ut_create_result);}
DWORD WINAPI utm_WaitForSingleObject(HANDLE handle,DWORD timeout)
{unsigned call=ut_wait_calls++;UT_ASSERT_EQ_PTR(ut_event,handle);UT_ASSERT_EQ_UINT(call==0?OD_UI_THREAD_TIMEOUT:INFINITE,timeout);return(ut_wait_results[call]);}
BOOL WINAPI utm_CloseHandle(HANDLE handle)
{if(handle==ut_event)++ut_close_event_calls;else{UT_ASSERT_EQ_PTR((HANDLE)(UINT_PTR)2,handle);++ut_close_thread_calls;}return(TRUE);}
void utm_ODFrameShutdown(tODThreadHandle *thread){UT_ASSERT_EQ_PTR((HANDLE)(UINT_PTR)2,*thread);++ut_shutdown_calls;*thread=NULL;}
void utm_ODThreadWaitForExit(tODThreadHandle thread){UT_ASSERT_EQ_PTR((HANDLE)(UINT_PTR)2,thread);++ut_join_calls;}
static void reset_start(void){ut_event=(HANDLE)(UINT_PTR)1;ut_create_result=kODRCSuccess;ut_wait_results[0]=WAIT_OBJECT_0;ut_wait_results[1]=WAIT_OBJECT_0;ut_wait_calls=0;
 ut_close_event_calls=ut_close_thread_calls=ut_shutdown_calls=ut_join_calls=0;ut_frame_result=kODRCSuccess;}
static void reports_event_and_thread_creation_failures(void)
{tODThreadHandle thread=(HANDLE)(UINT_PTR)8;reset_start();ut_event=NULL;UT_ASSERT_EQ_INT(kODRCGeneralFailure,utt_ODFrameStart((HANDLE)(UINT_PTR)9,&thread));UT_ASSERT_EQ_UINT(0,ut_wait_calls);
 reset_start();ut_create_result=kODRCNoMemory;UT_ASSERT_EQ_INT(kODRCNoMemory,utt_ODFrameStart((HANDLE)(UINT_PTR)9,&thread));UT_ASSERT_EQ_UINT(1,ut_close_event_calls);UT_ASSERT_NULL(hFrameStartedEvent);}
static void cleans_up_a_late_success_after_the_startup_deadline(void)
{tODThreadHandle thread=NULL;reset_start();ut_wait_results[0]=WAIT_TIMEOUT;ut_frame_result=kODRCSuccess;
 UT_ASSERT_EQ_INT(kODRCGeneralFailure,utt_ODFrameStart((HANDLE)(UINT_PTR)9,&thread));UT_ASSERT_EQ_UINT(2,ut_wait_calls);UT_ASSERT_EQ_UINT(1,ut_shutdown_calls);UT_ASSERT_EQ_UINT(0,ut_join_calls);}
static void joins_a_late_failure_after_the_startup_deadline(void)
{tODThreadHandle thread=NULL;reset_start();ut_wait_results[0]=WAIT_FAILED;ut_frame_result=kODRCGeneralFailure;
 UT_ASSERT_EQ_INT(kODRCGeneralFailure,utt_ODFrameStart((HANDLE)(UINT_PTR)9,&thread));UT_ASSERT_EQ_UINT(2,ut_wait_calls);UT_ASSERT_EQ_UINT(1,ut_join_calls);UT_ASSERT_EQ_UINT(1,ut_close_thread_calls);UT_ASSERT_NULL(thread);}
static void returns_a_normal_success_without_thread_cleanup(void)
{tODThreadHandle thread=NULL;reset_start();UT_ASSERT_EQ_INT(kODRCSuccess,utt_ODFrameStart((HANDLE)(UINT_PTR)9,&thread));UT_ASSERT_EQ_PTR((HANDLE)(UINT_PTR)2,thread);UT_ASSERT_EQ_UINT(1,ut_close_event_calls);UT_ASSERT_EQ_UINT(0,ut_join_calls);}
static void cleans_up_a_normal_startup_failure(void)
{tODThreadHandle thread=NULL;reset_start();ut_frame_result=kODRCGeneralFailure;
 UT_ASSERT_EQ_INT(kODRCGeneralFailure,utt_ODFrameStart((HANDLE)(UINT_PTR)9,&thread));UT_ASSERT_NULL(thread);}
static const UTTestCase ut_cases[]={{"creation failures",reports_event_and_thread_creation_failures},{"late success",cleans_up_a_late_success_after_the_startup_deadline},{"late failure",joins_a_late_failure_after_the_startup_deadline},{"normal success",returns_a_normal_success_without_thread_cleanup},{"normal failure",cleans_up_a_normal_startup_failure}};
#endif
