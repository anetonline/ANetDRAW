#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK__close
#else
#define UT_CUSTOM_MOCK_close
#endif
#define UT_CUSTOM_MOCK_od_log_write
static int ut_close_calls;
static int ut_log_calls;
#ifdef ODPLAT_WIN32
int utm__close(int file)
#else
int utm_close(int file)
#endif
{ (void)file; ++ut_close_calls; return(0); }
BOOL ODCALL utm_od_log_write(const char *message)
{ (void)message; ++ut_log_calls; return(TRUE); }
static void detaches_and_clears_request_state(void)
{
   hODReserveFile = 7;
   bODReserveActive = TRUE;
   bODReserveRequested = TRUE;
   bODReserveAcquired = TRUE;
   eODLifecycleState = kODLifecycleInitializing;
   ut_close_calls = 0;
   utt_ODReserveDetach();
   UT_ASSERT_EQ_INT(1, ut_close_calls);
   UT_ASSERT(bODReserveDetached && !bODReserveActive);
   UT_ASSERT(!bODReserveRequested && !bODReserveAcquired);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);

   hODReserveFile = -1;
   eODLifecycleState = kODLifecycleActive;
   ut_log_calls = 0;
   utt_ODReserveDetach();
   UT_ASSERT_EQ_INT(1, ut_log_calls);
}
static const UTTestCase ut_cases[] = {
   {"detach", detaches_and_clears_request_state}
};
