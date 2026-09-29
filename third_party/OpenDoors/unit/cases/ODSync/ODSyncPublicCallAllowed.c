static void allows_live_states_and_rejects_shutdown_states(void)
{
   od_control.od_error = ERR_NONE;
   eODLifecycleState = kODLifecycleNeverStarted;
   UT_ASSERT(utt_ODSyncPublicCallAllowed());
   eODLifecycleState = kODLifecycleInitializing;
   UT_ASSERT(utt_ODSyncPublicCallAllowed());
   eODLifecycleState = kODLifecycleActive;
   UT_ASSERT(utt_ODSyncPublicCallAllowed());

   eODLifecycleState = kODLifecycleExitPending;
   UT_ASSERT(!utt_ODSyncPublicCallAllowed());
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   od_control.od_error = ERR_NONE;
   eODLifecycleState = kODLifecycleFinalizing;
   UT_ASSERT(!utt_ODSyncPublicCallAllowed());
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   od_control.od_error = ERR_NONE;
   eODLifecycleState = kODLifecycleTerminal;
   UT_ASSERT(!utt_ODSyncPublicCallAllowed());
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
}

static const UTTestCase ut_cases[] = {
   {"lifecycle guard", allows_live_states_and_rejects_shutdown_states}
};
