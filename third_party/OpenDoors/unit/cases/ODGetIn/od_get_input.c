#define UT_CUSTOM_MOCK_ODGetCodeIfLongest
#define UT_CUSTOM_MOCK_ODGetInputWait
#define UT_CUSTOM_MOCK_ODGetInputWaitUntil
#define UT_CUSTOM_MOCK_ODHaveStartOfSequence
#define UT_CUSTOM_MOCK_ODLongestFullCode
#define UT_CUSTOM_MOCK_ODShiftSeq
#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_od_kernel

#include "common.h"

#define utt_od_get_input(event, timeout, flags) \
   utt_ODGetInputCore((event), (timeout), (flags), NULL)

#define UT_INPUT_CAPACITY 4
#define UT_RESULT_CAPACITY 8

static tODResult ut_wait_results[UT_INPUT_CAPACITY];
static tODInputEvent ut_wait_events[UT_INPUT_CAPACITY];
static BOOL ut_have_results[UT_RESULT_CAPACITY];
static int ut_code_results[UT_RESULT_CAPACITY];
static int ut_longest_results[UT_RESULT_CAPACITY];
static unsigned ut_wait_count;
static unsigned ut_wait_index;
static unsigned ut_have_count;
static unsigned ut_have_index;
static unsigned ut_code_count;
static unsigned ut_code_index;
static unsigned ut_longest_count;
static unsigned ut_longest_index;
static unsigned ut_kernel_calls;
static unsigned ut_shift_calls;
static int ut_last_shift;
static const tODInputDeadline *ut_expected_deadline;
static tODMilliSec ut_seen_timeout;

static void reset_input_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
   memset(ut_wait_results, 0, sizeof(ut_wait_results));
   memset(ut_wait_events, 0, sizeof(ut_wait_events));
   memset(ut_have_results, 0, sizeof(ut_have_results));
   memset(ut_code_results, 0xff, sizeof(ut_code_results));
   memset(ut_longest_results, 0xff, sizeof(ut_longest_results));
   ut_set_sequence("");
   bDoorwaySequence = FALSE;
   bDoorwaySequencePending = FALSE;
   ut_wait_count = ut_wait_index = 0;
   ut_have_count = ut_have_index = 0;
   ut_code_count = ut_code_index = 0;
   ut_longest_count = ut_longest_index = 0;
   ut_kernel_calls = ut_shift_calls = 0;
   ut_last_shift = -1;
   ut_expected_deadline = NULL;
   ut_seen_timeout = 0;
}

void ODCALL utm_od_kernel(void) { ++ut_kernel_calls; }

void *utm_memcpy(void *destination, const void *source, size_t size)
{
   unsigned char *output = (unsigned char *)destination;
   const unsigned char *input = (const unsigned char *)source;
   size_t index;
   for(index = 0; index < size; ++index) output[index] = input[index];
   return destination;
}

static void queue_input(tODResult result, char character, BOOL remote)
{
   unsigned index = ut_wait_count++;
   UT_ASSERT(index < UT_INPUT_CAPACITY);
   ut_wait_results[index] = result;
   ut_wait_events[index].EventType = EVENT_CHARACTER;
   ut_wait_events[index].chKeyPress = character;
   ut_wait_events[index].bFromRemote = remote;
}

tODResult utm_ODGetInputWait(tODInputEvent *event, tODMilliSec timeout)
{
   unsigned index = ut_wait_index++;
   UT_ASSERT(index < ut_wait_count);
   if(index != 0) UT_ASSERT_EQ_UINT(MAX_CHARACTER_LATENCY, timeout);
   if(ut_wait_results[index] == kODRCSuccess)
      utm_memcpy(event, &ut_wait_events[index], sizeof(*event));
   return ut_wait_results[index];
}

tODResult utm_ODGetInputWaitUntil(tODInputEvent *event,
   const tODInputDeadline *deadline, tODMilliSec timeout)
{
   UT_ASSERT_EQ_PTR(ut_expected_deadline, deadline);
   ut_seen_timeout = timeout;
   return(utm_ODGetInputWait(event, timeout));
}

int utm_ODHaveStartOfSequence(WORD flags)
{
   (void)flags;
   UT_ASSERT(ut_have_index < ut_have_count);
   return ut_have_results[ut_have_index++];
}

int utm_ODGetCodeIfLongest(WORD flags)
{
   (void)flags;
   UT_ASSERT(ut_code_index < ut_code_count);
   return ut_code_results[ut_code_index++];
}

int utm_ODLongestFullCode(WORD flags)
{
   (void)flags;
   UT_ASSERT(ut_longest_index < ut_longest_count);
   return ut_longest_results[ut_longest_index++];
}

void utm_ODShiftSeq(int characters)
{
   size_t length = utm_strlen(szCurrentSequence);
   size_t index;
   ++ut_shift_calls;
   ut_last_shift = characters;
   if(characters <= 0 || (size_t)characters > length) return;
   for(index = 0; index + (size_t)characters <= length; ++index)
      szCurrentSequence[index] = szCurrentSequence[index + characters];
}

static void rejects_null_output(void)
{
   reset_input_fixture();
   UT_ASSERT(!utt_od_get_input(NULL, 0, 0));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
}

static void initial_wait_can_fail(void)
{
   tODInputEvent output;
   reset_input_fixture();
   queue_input(kODRCTimeout, 0, TRUE);
   UT_ASSERT(!utt_od_get_input(&output, 123, 0));
}

static void absolute_deadline_uses_the_same_translation_paths(void)
{
   tODInputEvent output;
   tODInputDeadline deadline;

   reset_input_fixture();
   deadline.dwSeconds = 12;
   deadline.wMilliseconds = 345;
   ut_expected_deadline = &deadline;
   queue_input(kODRCSuccess, 'L', FALSE);
   UT_ASSERT(utt_ODGetInputCore(&output, OD_NO_TIMEOUT, 0, &deadline));
   UT_ASSERT_EQ_INT('L', output.chKeyPress);

   reset_input_fixture();
   deadline.dwSeconds = 12;
   deadline.wMilliseconds = 345;
   ut_expected_deadline = &deadline;
   ut_set_sequence("\033[");
   ut_have_results[0] = TRUE;
   ut_have_count = 1;
   ut_code_results[0] = NO_MATCH;
   ut_code_results[1] = 15;
   ut_code_count = 2;
   queue_input(kODRCSuccess, 'A', TRUE);
   UT_ASSERT(utt_ODGetInputCore(&output, OD_NO_TIMEOUT, 0, &deadline));
   UT_ASSERT_EQ_INT((char)OD_KEY_UP, output.chKeyPress);
   UT_ASSERT_EQ_UINT(MAX_CHARACTER_LATENCY, ut_seen_timeout);
}

static void returns_an_initial_local_character(void)
{
   tODInputEvent output;
   reset_input_fixture();
   queue_input(kODRCSuccess, 'L', FALSE);
   UT_ASSERT(utt_od_get_input(&output, 0, 0));
   UT_ASSERT_EQ_INT('L', output.chKeyPress);
   UT_ASSERT(!output.bFromRemote);
   UT_ASSERT_EQ_INT(EVENT_CHARACTER, output.EventType);
}

static void returns_an_untranslated_remote_character(void)
{
   tODInputEvent output;
   reset_input_fixture();
   queue_input(kODRCSuccess, 'x', TRUE);
   ut_have_results[0] = FALSE;
   ut_have_count = 1;
   UT_ASSERT(utt_od_get_input(&output, 0, 7));
   UT_ASSERT_EQ_INT('x', output.chKeyPress);
   UT_ASSERT(output.bFromRemote);
   UT_ASSERT_EQ_UINT(1, ut_shift_calls);
   UT_ASSERT_EQ_INT(1, ut_last_shift);
}

static void local_null_starts_but_does_not_complete_doorway_input(void)
{
   tODInputEvent output;
   reset_input_fixture();
   queue_input(kODRCSuccess, 0, FALSE);
   queue_input(kODRCTimeout, 0, TRUE);
   ut_code_results[0] = NO_MATCH;
   ut_code_count = 1;
   UT_ASSERT(utt_od_get_input(&output, 0, 0));
   UT_ASSERT_EQ_INT(0, output.chKeyPress);
   UT_ASSERT_EQ_INT(EVENT_CHARACTER, output.EventType);
   UT_ASSERT(!bDoorwaySequence);
}

static void returns_an_immediately_complete_control_sequence(void)
{
   tODInputEvent output;
   reset_input_fixture();
   ut_set_sequence("\033A");
   ut_have_results[0] = TRUE;
   ut_have_count = 1;
   ut_code_results[0] = 0;
   ut_code_count = 1;
   UT_ASSERT(utt_od_get_input(&output, 0, 0));
   UT_ASSERT_EQ_INT((char)OD_KEY_UP, output.chKeyPress);
   UT_ASSERT_EQ_INT(EVENT_EXTENDED_KEY, output.EventType);
   UT_ASSERT_EQ_INT(2, ut_last_shift);
}

static void completes_a_doorway_sequence(void)
{
   tODInputEvent output;
   reset_input_fixture();
   bDoorwaySequence = TRUE;
   ut_code_results[0] = NO_MATCH;
   ut_code_count = 1;
   queue_input(kODRCSuccess, 'D', TRUE);
   UT_ASSERT(utt_od_get_input(&output, 0, 0));
   UT_ASSERT_EQ_INT('D', output.chKeyPress);
   UT_ASSERT_EQ_INT(EVENT_EXTENDED_KEY, output.EventType);
   UT_ASSERT(!bDoorwaySequence);
}

static void accumulated_remote_input_can_complete_a_sequence(void)
{
   tODInputEvent output;
   reset_input_fixture();
   ut_set_sequence("\033[");
   ut_have_results[0] = TRUE;
   ut_have_count = 1;
   ut_code_results[0] = NO_MATCH;
   ut_code_results[1] = 15;
   ut_code_count = 2;
   queue_input(kODRCSuccess, 'A', TRUE);
   UT_ASSERT(utt_od_get_input(&output, 0, 0));
   UT_ASSERT_EQ_INT((char)OD_KEY_UP, output.chKeyPress);
   UT_ASSERT_EQ_INT(EVENT_EXTENDED_KEY, output.EventType);
}

static void accumulated_local_input_is_returned_immediately(void)
{
   tODInputEvent output;
   reset_input_fixture();
   ut_set_sequence("\033[");
   ut_have_results[0] = TRUE;
   ut_have_count = 1;
   ut_code_results[0] = NO_MATCH;
   ut_code_count = 1;
   queue_input(kODRCSuccess, 'Q', FALSE);
   UT_ASSERT(utt_od_get_input(&output, 0, 0));
   UT_ASSERT_EQ_INT('Q', output.chKeyPress);
   UT_ASSERT(!output.bFromRemote);
}

static void unmatched_accumulated_input_falls_back_after_timeout(void)
{
   tODInputEvent output;
   reset_input_fixture();
   ut_set_sequence("\033[");
   ut_have_results[0] = TRUE;
   ut_have_count = 1;
   ut_code_results[0] = NO_MATCH;
   ut_code_results[1] = NO_MATCH;
   ut_code_count = 2;
   ut_longest_results[0] = NO_MATCH;
   ut_longest_count = 1;
   queue_input(kODRCSuccess, 'Z', TRUE);
   queue_input(kODRCTimeout, 0, TRUE);
   UT_ASSERT(utt_od_get_input(&output, 0, 0));
   UT_ASSERT_EQ_INT('\033', output.chKeyPress);
   UT_ASSERT_EQ_INT(EVENT_CHARACTER, output.EventType);
}

static void remote_null_defers_doorway_mode_until_buffer_drains(void)
{
   tODInputEvent output;
   reset_input_fixture();
   ut_set_sequence("x");
   ut_have_results[0] = TRUE;
   ut_have_count = 1;
   ut_code_results[0] = NO_MATCH;
   ut_code_count = 1;
   ut_longest_results[0] = NO_MATCH;
   ut_longest_count = 1;
   queue_input(kODRCSuccess, 0, TRUE);
   UT_ASSERT(utt_od_get_input(&output, 0, 0));
   UT_ASSERT_EQ_INT('x', output.chKeyPress);
   UT_ASSERT(bDoorwaySequencePending);
}

static void timeout_can_settle_for_a_complete_sequence(void)
{
   tODInputEvent output;
   reset_input_fixture();
   ut_set_sequence("\033A");
   ut_have_results[0] = TRUE;
   ut_have_count = 1;
   ut_code_results[0] = NO_MATCH;
   ut_code_count = 1;
   ut_longest_results[0] = 0;
   ut_longest_count = 1;
   queue_input(kODRCTimeout, 0, TRUE);
   UT_ASSERT(utt_od_get_input(&output, 0, 0));
   UT_ASSERT_EQ_INT((char)OD_KEY_UP, output.chKeyPress);
   UT_ASSERT_EQ_INT(EVENT_EXTENDED_KEY, output.EventType);
}

static void pending_state_exercises_each_promotion_condition(void)
{
   tODInputEvent output;
   reset_input_fixture();
   bDoorwaySequencePending = TRUE;
   queue_input(kODRCSuccess, 'P', TRUE);
   ut_code_results[0] = NO_MATCH;
   ut_code_count = 1;
   UT_ASSERT(utt_od_get_input(&output, 0, 0));
   UT_ASSERT_EQ_INT(EVENT_EXTENDED_KEY, output.EventType);

   reset_input_fixture();
   bDoorwaySequence = TRUE;
   bDoorwaySequencePending = TRUE;
   ut_code_results[0] = NO_MATCH;
   ut_code_count = 1;
   UT_ASSERT(utt_od_get_input(&output, 0, 0));
   UT_ASSERT_EQ_INT(EVENT_CHARACTER, output.EventType);

   reset_input_fixture();
   bDoorwaySequencePending = TRUE;
   ut_set_sequence("z");
   ut_have_results[0] = TRUE;
   ut_have_count = 1;
   ut_code_results[0] = NO_MATCH;
   ut_code_count = 1;
   ut_longest_results[0] = NO_MATCH;
   ut_longest_count = 1;
   UT_ASSERT(utt_od_get_input(&output, 0, 0));
   UT_ASSERT_EQ_INT('z', output.chKeyPress);
}

static const UTTestCase ut_cases[] = {
   {"invalid output", rejects_null_output},
   {"initial timeout", initial_wait_can_fail},
   {"initial local character", returns_an_initial_local_character},
   {"untranslated remote character", returns_an_untranslated_remote_character},
   {"incomplete doorway input", local_null_starts_but_does_not_complete_doorway_input},
   {"complete sequence", returns_an_immediately_complete_control_sequence},
   {"doorway completion", completes_a_doorway_sequence},
   {"accumulated remote sequence", accumulated_remote_input_can_complete_a_sequence},
   {"accumulated local input", accumulated_local_input_is_returned_immediately},
   {"unmatched accumulated input", unmatched_accumulated_input_falls_back_after_timeout},
   {"deferred doorway input", remote_null_defers_doorway_mode_until_buffer_drains},
   {"settled complete sequence", timeout_can_settle_for_a_complete_sequence},
   {"pending promotion conditions", pending_state_exercises_each_promotion_condition},
   {"absolute deadline", absolute_deadline_uses_the_same_translation_paths}
};
