#define UT_CUSTOM_MOCK_ODTimerStart
#define UT_CUSTOM_MOCK_ODTimerElapsed
#define UT_CUSTOM_MOCK_ODTimerLeft
#define UT_CUSTOM_MOCK_ODInQueueGetNextEvent
#define UT_CUSTOM_MOCK_ODSyncAPIRelease
#define UT_CUSTOM_MOCK_ODSyncAPIReacquire
#define UT_CUSTOM_MOCK_ODSyncAPICheckpoint
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_strstr
#define UT_CUSTOM_MOCK_memmove
#ifdef OD_DIAGNOSTICS
#define UT_CUSTOM_MOCK_strcat
#endif

static tODInputEvent ut_events[48];
static tODResult ut_results[48];
static unsigned ut_event_count;
static unsigned ut_event_index;
static unsigned ut_elapsed_calls;
static unsigned ut_elapsed_after;
static unsigned ut_start_calls;
static unsigned ut_release_calls;
static unsigned ut_reacquire_calls;
static unsigned ut_checkpoint_calls;
static tODMilliSec ut_timer_left;
static BOOL ut_checkpoint_result;

size_t utm_strlen(const char *text)
{
   size_t length = 0; while(text[length] != 0) ++length; return(length);
}
char *utm_strstr(const char *haystack, const char *needle)
{
   size_t nlen = utm_strlen(needle); size_t index;
   if(nlen == 0) return((char *)haystack);
   for(index = 0; haystack[index] != 0; ++index)
      if(memcmp(haystack + index, needle, nlen) == 0)
         return((char *)(haystack + index));
   return(NULL);
}
void *utm_memmove(void *destination, const void *source, size_t size)
{
   BYTE *out = destination; const BYTE *in = source; size_t index;
   if(out < in) for(index = 0; index < size; ++index) out[index] = in[index];
   else for(index = size; index > 0; --index) out[index - 1] = in[index - 1];
   return(destination);
}
#ifdef OD_DIAGNOSTICS
char *utm_strcat(char *destination, const char *source)
{
   size_t offset = utm_strlen(destination);
   memcpy(destination + offset, source, utm_strlen(source) + 1);
   return(destination);
}
#endif
void utm_ODTimerStart(tODTimer *timer, tODMilliSec duration)
{
   UT_ASSERT_NOT_NULL(timer); UT_ASSERT(duration == 2000); ++ut_start_calls;
}
BOOL utm_ODTimerElapsed(tODTimer *timer)
{
   UT_ASSERT_NOT_NULL(timer); return(ut_elapsed_calls++ >= ut_elapsed_after);
}
tODMilliSec utm_ODTimerLeft(tODTimer *timer)
{
   UT_ASSERT_NOT_NULL(timer); return(ut_timer_left);
}
tODResult utm_ODInQueueGetNextEvent(tODInQueueHandle queue,
   tODInputEvent *event, tODMilliSec timeout)
{
   UT_ASSERT_EQ_PTR((tODInQueueHandle)(DWORD_PTR)41, queue);
   UT_ASSERT_NOT_NULL(event);
   UT_ASSERT(timeout == (ut_timer_left > 50 ? 50 : ut_timer_left));
   UT_ASSERT(ut_event_index < ut_event_count);
   *event = ut_events[ut_event_index]; return(ut_results[ut_event_index++]);
}
unsigned utm_ODSyncAPIRelease(void) { ++ut_release_calls; return(3); }
void utm_ODSyncAPIReacquire(unsigned level)
{
   UT_ASSERT_EQ_UINT(3, level); ++ut_reacquire_calls;
}
BOOL utm_ODSyncAPICheckpoint(void)
{
   ++ut_checkpoint_calls;
   return(ut_checkpoint_result);
}

static void reset_wait(void)
{
   memset(ut_events, 0, sizeof(ut_events));
   memset(ut_results, 0, sizeof(ut_results));
   ut_event_count = ut_event_index = ut_elapsed_calls = 0;
   ut_elapsed_after = 0; ut_start_calls = 0;
   ut_release_calls = ut_reacquire_calls = 0;
   ut_checkpoint_calls = 0;
   ut_timer_left = 123;
   ut_checkpoint_result = TRUE;
   hODInputQueue = (tODInQueueHandle)(DWORD_PTR)41;
#ifdef OD_DIAGNOSTICS
   szDebugWorkString[0] = 0;
#endif
}

static void accepts_empty_response_without_starting_timer(void)
{
   reset_wait(); UT_ASSERT(utt_ODWaitForString("", 2000));
   UT_ASSERT_EQ_UINT(0, ut_start_calls);
}

static void expires_and_yields_after_queue_failure(void)
{
   reset_wait(); ut_elapsed_after = 0;
   UT_ASSERT(!utt_ODWaitForString("OK", 2000));
   UT_ASSERT_EQ_UINT(1, ut_start_calls);
   reset_wait(); ut_elapsed_after = 1; ut_event_count = 1;
   ut_results[0] = kODRCNothingWaiting;
   UT_ASSERT(!utt_ODWaitForString("OK", 2000));
   UT_ASSERT_EQ_UINT(1, ut_checkpoint_calls);
   UT_ASSERT_EQ_UINT(1, ut_release_calls);
   UT_ASSERT_EQ_UINT(1, ut_reacquire_calls);

   reset_wait(); ut_elapsed_after = 2; ut_event_count = 1;
   ut_results[0] = kODRCNothingWaiting;
   ut_checkpoint_result = FALSE;
   UT_ASSERT(!utt_ODWaitForString("OK", 2000));
   UT_ASSERT_EQ_UINT(1, ut_checkpoint_calls);
}

static void ignores_local_and_noncharacter_events_then_matches(void)
{
   reset_wait(); ut_timer_left = 25; ut_elapsed_after = 4; ut_event_count = 4;
   ut_results[0] = ut_results[1] = ut_results[2] = ut_results[3] = kODRCSuccess;
   ut_events[0].bFromRemote = FALSE; ut_events[0].EventType = EVENT_CHARACTER;
   ut_events[1].bFromRemote = TRUE; ut_events[1].EventType = EVENT_EXTENDED_KEY;
   ut_events[2].bFromRemote = TRUE; ut_events[2].EventType = EVENT_CHARACTER;
   ut_events[2].chKeyPress = 'O';
   ut_events[3].bFromRemote = TRUE; ut_events[3].EventType = EVENT_CHARACTER;
   ut_events[3].chKeyPress = 'K';
   UT_ASSERT(utt_ODWaitForString("OK", 2000));
   UT_ASSERT_EQ_UINT(4, ut_release_calls);
   UT_ASSERT_EQ_UINT(4, ut_reacquire_calls);
}

static void slides_the_bounded_receive_window(void)
{
   unsigned index;
   reset_wait(); ut_elapsed_after = MAX_RESPONSE_LEN + 1;
   ut_event_count = MAX_RESPONSE_LEN + 1;
   for(index = 0; index < MAX_RESPONSE_LEN; ++index) {
      ut_results[index] = kODRCSuccess; ut_events[index].bFromRemote = TRUE;
      ut_events[index].EventType = EVENT_CHARACTER; ut_events[index].chKeyPress = 'A';
   }
   ut_results[MAX_RESPONSE_LEN] = kODRCSuccess;
   ut_events[MAX_RESPONSE_LEN].bFromRemote = TRUE;
   ut_events[MAX_RESPONSE_LEN].EventType = EVENT_CHARACTER;
   ut_events[MAX_RESPONSE_LEN].chKeyPress = 'Z';
   UT_ASSERT(utt_ODWaitForString("AZ", 2000));
}

static const UTTestCase ut_cases[] = {
   {"empty", accepts_empty_response_without_starting_timer},
   {"timeout", expires_and_yields_after_queue_failure},
   {"event filtering", ignores_local_and_noncharacter_events_then_matches},
   {"sliding window", slides_the_bounded_receive_window}
};
