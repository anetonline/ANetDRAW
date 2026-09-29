#ifdef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock
void utm_ODMutexLock(tODMutex *mutex) { UT_ASSERT_EQ_PTR(&KernelStateLock, mutex); }
void utm_ODMutexUnlock(tODMutex *mutex) { UT_ASSERT_EQ_PTR(&KernelStateLock, mutex); }
#endif
static void replaces_the_pending_or_current_time_value(void)
{
#ifdef OD_THREAD_SUPPORT
   bTimeValuePending = FALSE; nPendingTimeAdjustment = 99;
   utt_ODKrnlRequestTimeValue(30); UT_ASSERT(bTimeValuePending);
   UT_ASSERT_EQ_INT(30, nPendingTimeValue); UT_ASSERT_EQ_INT(0, nPendingTimeAdjustment);
   utt_ODKrnlRequestTimeValue(-1); UT_ASSERT_EQ_INT(OD_MIN_USER_TIME_MINUTES, nPendingTimeValue);
   utt_ODKrnlRequestTimeValue(2000); UT_ASSERT_EQ_INT(OD_MAX_USER_TIME_MINUTES, nPendingTimeValue);
#else
   memset(&od_control, 0, sizeof(od_control)); utt_ODKrnlRequestTimeValue(-7);
   UT_ASSERT_EQ_INT(-7, od_control.user_timelimit);
#endif
}
static const UTTestCase ut_cases[] = {{"replace", replaces_the_pending_or_current_time_value}};
