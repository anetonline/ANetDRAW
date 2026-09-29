#define UT_CUSTOM_MOCK_ODInQueueGetNextEvent
#define UT_CUSTOM_MOCK_ODInQueueWaiting

static int ut_waiting_calls;
static int ut_get_calls;

BOOL utm_ODInQueueWaiting(tODInQueueHandle handle)
{
   UT_ASSERT(handle == (tODInQueueHandle)123);
   return ut_waiting_calls++ < 2;
}

tODResult utm_ODInQueueGetNextEvent(tODInQueueHandle handle,
   tODInputEvent *event, tODMilliSec timeout)
{
   UT_ASSERT(handle == (tODInQueueHandle)123);
   UT_ASSERT(event != NULL);
   UT_ASSERT_EQ_INT(0, timeout);
   ++ut_get_calls;
   return kODRCSuccess;
}

static void removes_events_until_empty(void)
{
   ut_waiting_calls = 0;
   ut_get_calls = 0;
   utt_ODInQueueEmpty((tODInQueueHandle)123);
   UT_ASSERT_EQ_INT(3, ut_waiting_calls);
   UT_ASSERT_EQ_INT(2, ut_get_calls);
}

static void accepts_already_empty_queue(void)
{
   ut_waiting_calls = 2;
   ut_get_calls = 0;
   utt_ODInQueueEmpty((tODInQueueHandle)123);
   UT_ASSERT_EQ_INT(3, ut_waiting_calls);
   UT_ASSERT_EQ_INT(0, ut_get_calls);
}

static const UTTestCase ut_cases[] = {
   {"drain queue", removes_events_until_empty},
   {"already empty", accepts_already_empty_queue}
};
