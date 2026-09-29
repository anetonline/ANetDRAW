#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_ODKrnlChatMode
static unsigned ut_init_calls, ut_entry_calls, ut_exit_calls, ut_chat_calls;
static BOOL ut_init_succeeds;
void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds)
      bODInitialized = TRUE;
}
void utm_ODSyncAPIEntry(void) { ++ut_entry_calls; }
void utm_ODSyncAPIExit(void) { ++ut_exit_calls; }
void utm_ODKrnlChatMode(void) { ++ut_chat_calls; }
static void enters_chat_after_initializing_when_needed(void)
{
   ut_init_calls = ut_entry_calls = ut_exit_calls = ut_chat_calls = 0;
   ut_init_succeeds = TRUE;
   bODInitialized = FALSE; od_control.od_chat_active = FALSE;
   utt_od_chat();
   UT_ASSERT_EQ_UINT(1, ut_init_calls); UT_ASSERT_EQ_UINT(1, ut_entry_calls);
   UT_ASSERT_EQ_UINT(1, ut_chat_calls); UT_ASSERT_EQ_UINT(1, ut_exit_calls);
   UT_ASSERT(od_control.od_chat_active);

   ut_init_calls = ut_entry_calls = ut_exit_calls = ut_chat_calls = 0;
   bODInitialized = TRUE; od_control.od_chat_active = FALSE;
   utt_od_chat();
   UT_ASSERT_EQ_UINT(0, ut_init_calls); UT_ASSERT_EQ_UINT(1, ut_chat_calls);

   ut_init_calls = ut_entry_calls = ut_exit_calls = ut_chat_calls = 0;
   bODInitialized = FALSE; od_control.od_chat_active = FALSE;
   ut_init_succeeds = FALSE;
   utt_od_chat();
   UT_ASSERT_EQ_UINT(1, ut_init_calls); UT_ASSERT_EQ_UINT(0, ut_entry_calls);
   UT_ASSERT_EQ_UINT(0, ut_chat_calls); UT_ASSERT_EQ_UINT(0, ut_exit_calls);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
}
static const UTTestCase ut_cases[] = {
   {"chat", enters_chat_after_initializing_when_needed}
};
