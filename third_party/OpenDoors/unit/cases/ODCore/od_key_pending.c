#define UT_CUSTOM_MOCK_ODInQueueWaiting
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_kernel
#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed

static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static unsigned ut_waiting_calls;
static BOOL ut_waiting;
static unsigned ut_kernel_calls;
static BOOL ut_public_call_allowed;
static BOOL ut_init_succeeds;

BOOL utm_ODSyncPublicCallAllowed(void)
{
   return(ut_public_call_allowed);
}

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

BOOL utm_ODInQueueWaiting(tODInQueueHandle queue)
{
   UT_ASSERT_EQ_PTR(hODInputQueue, queue);
   ++ut_waiting_calls;
   return ut_waiting;
}

void ODCALL utm_od_kernel(void) { ++ut_kernel_calls; }

static void reset_pending(void)
{
   bODInitialized = TRUE;
   hODInputQueue = (tODInQueueHandle)1;
   ut_init_calls = 0;
   ut_entries = 0;
   ut_exits = 0;
   ut_waiting_calls = 0;
   ut_waiting = FALSE;
   ut_kernel_calls = 0;
   ut_public_call_allowed = TRUE;
   ut_init_succeeds = TRUE;
}

static void reports_an_empty_queue_after_initialization(void)
{
   reset_pending();
   bODInitialized = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_key_pending());
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(1, ut_waiting_calls);
   UT_ASSERT_EQ_UINT(1, ut_kernel_calls);
}

static void reports_a_waiting_event(void)
{
   reset_pending();
   ut_waiting = TRUE;
   UT_ASSERT_EQ_INT(TRUE, utt_od_key_pending());
   UT_ASSERT_EQ_UINT(0, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(1, ut_waiting_calls);
}

static void rejects_terminal_sessions(void)
{
   reset_pending();
   ut_public_call_allowed = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_key_pending());
   UT_ASSERT_EQ_UINT(0, ut_init_calls);
   UT_ASSERT_EQ_UINT(0, ut_entries);

   reset_pending();
   bODInitialized = FALSE;
   ut_init_succeeds = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_key_pending());
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static const UTTestCase ut_cases[] = {
   {"empty queue", reports_an_empty_queue_after_initialization},
   {"waiting event", reports_a_waiting_event},
   {"terminal sessions", rejects_terminal_sessions}
};
