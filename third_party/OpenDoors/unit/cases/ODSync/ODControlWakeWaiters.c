#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_SetEvent
static HANDLE ut_changed;
static unsigned ut_calls;

BOOL WINAPI utm_SetEvent(HANDLE changed)
{
   ut_changed = changed;
   ++ut_calls;
   return TRUE;
}
#else
#define UT_CUSTOM_MOCK_pthread_cond_broadcast
static pthread_cond_t *ut_changed;
static unsigned ut_calls;

int utm_pthread_cond_broadcast(pthread_cond_t *changed)
{
   ut_changed = changed;
   ++ut_calls;
   return 0;
}
#endif

static void wakes_every_waiter_on_the_platform_condition(void)
{
   ut_calls = 0;
#ifdef ODPLAT_WIN32
   ControlLock.changed = (HANDLE)1;
   ut_changed = NULL;
   utt_ODControlWakeWaiters();
   UT_ASSERT_EQ_PTR(ControlLock.changed, ut_changed);
#else
   ut_changed = NULL;
   utt_ODControlWakeWaiters();
   UT_ASSERT_EQ_PTR(&ControlLock.changed, ut_changed);
#endif
   UT_ASSERT_EQ_UINT(1, ut_calls);
}

static const UTTestCase ut_cases[] = {
   {"wake waiters", wakes_every_waiter_on_the_platform_condition}
};
