#define UT_CUSTOM_MOCK_ODKrnlDispatchPending
static unsigned ut_dispatches;
static unsigned ut_dispatch_level;
static BOOL ut_dispatch_allow_callbacks;

void utm_ODKrnlDispatchPending(BOOL allow_callbacks)
{
   ++ut_dispatches;
   ut_dispatch_level = nAPILevel;
   ut_dispatch_allow_callbacks = allow_callbacks;
}

static void dispatches_pending_work_only_at_the_outermost_entry(void)
{
   nAPILevel = 0;
   bSyncActive = TRUE;
   eODLifecycleState = kODLifecycleActive;
   ut_dispatches = 0;
   ut_dispatch_level = 0;
   ut_dispatch_allow_callbacks = FALSE;
   utt_ODSyncAPIEntry();
   UT_ASSERT_EQ_UINT(1, nAPILevel);
   UT_ASSERT_EQ_UINT(1, ut_dispatches);
   UT_ASSERT_EQ_UINT(1, ut_dispatch_level);
   UT_ASSERT(ut_dispatch_allow_callbacks);
   utt_ODSyncAPIEntry();
   UT_ASSERT_EQ_UINT(2, nAPILevel);
   UT_ASSERT_EQ_UINT(1, ut_dispatches);

   nAPILevel = 0;
   eODLifecycleState = kODLifecycleExitPending;
   utt_ODSyncAPIEntry();
   UT_ASSERT_EQ_UINT(1, nAPILevel);
   UT_ASSERT_EQ_UINT(1, ut_dispatches);
}

static const UTTestCase ut_cases[] = {
   {"nested entry", dispatches_pending_work_only_at_the_outermost_entry}
};
