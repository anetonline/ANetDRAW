#define UT_CUSTOM_MOCK_ODKrnlDispatchPending
#define UT_CUSTOM_MOCK_ODSyncAPIReacquire
#define UT_CUSTOM_MOCK_ODSyncAPIRelease
#define UT_CUSTOM_MOCK_od_kernel

static unsigned ut_release_calls;
static unsigned ut_dispatch_calls;
static unsigned ut_reacquire_calls;
static unsigned ut_kernel_calls;
static BOOL ut_allow_callbacks;
static unsigned ut_reacquire_level;
static BOOL ut_shutdown_on_dispatch;

unsigned utm_ODSyncAPIRelease(void)
{
   ++ut_release_calls;
   return 3;
}

void utm_ODKrnlDispatchPending(BOOL allow_callbacks)
{
   ++ut_dispatch_calls;
   ut_allow_callbacks = allow_callbacks;
   if(ut_shutdown_on_dispatch)
      eODLifecycleState = kODLifecycleExitPending;
}

void ODCALL utm_od_kernel(void) { ++ut_kernel_calls; }

void utm_ODSyncAPIReacquire(unsigned level)
{
   ++ut_reacquire_calls;
   ut_reacquire_level = level;
}

static void reset_checkpoint(void)
{
   ut_release_calls = 0;
   ut_dispatch_calls = 0;
   ut_reacquire_calls = 0;
   ut_kernel_calls = 0;
   ut_allow_callbacks = TRUE;
   ut_reacquire_level = 0;
   eODLifecycleState = kODLifecycleActive;
   ut_shutdown_on_dispatch = FALSE;
}

static void returns_state_without_releasing_outside_an_api_call(void)
{
   reset_checkpoint();
   nAPILevel = 0;
   UT_ASSERT_EQ_INT(TRUE, utt_ODSyncAPICheckpoint());
   UT_ASSERT_EQ_UINT(0, ut_release_calls);
}

static void releases_dispatches_and_reacquires_the_api_level(void)
{
   reset_checkpoint();
   nAPILevel = 1;
   UT_ASSERT_EQ_INT(TRUE, utt_ODSyncAPICheckpoint());
   UT_ASSERT_EQ_UINT(1, ut_release_calls);
   UT_ASSERT_EQ_UINT(1, ut_dispatch_calls);
   UT_ASSERT_EQ_INT(TRUE, ut_allow_callbacks);
   UT_ASSERT_EQ_UINT(1, ut_reacquire_calls);
   UT_ASSERT_EQ_UINT(3, ut_reacquire_level);
   UT_ASSERT_EQ_UINT(1, ut_kernel_calls);

   reset_checkpoint();
   nAPILevel = 1;
   eODLifecycleState = kODLifecycleExitPending;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSyncAPICheckpoint());
   UT_ASSERT_EQ_UINT(0, ut_dispatch_calls);
   UT_ASSERT_EQ_UINT(0, ut_kernel_calls);

   reset_checkpoint();
   nAPILevel = 1;
   ut_shutdown_on_dispatch = TRUE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSyncAPICheckpoint());
   UT_ASSERT_EQ_UINT(1, ut_dispatch_calls);
   UT_ASSERT_EQ_UINT(0, ut_kernel_calls);
}

static const UTTestCase ut_cases[] = {
   {"inactive checkpoint", returns_state_without_releasing_outside_an_api_call},
   {"active checkpoint", releases_dispatches_and_reacquires_the_api_level}
};
