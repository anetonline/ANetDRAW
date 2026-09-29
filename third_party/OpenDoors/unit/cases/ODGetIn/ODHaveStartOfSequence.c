#include "common.h"

static void raw_input_never_starts_a_sequence(void)
{
   ut_set_sequence("\033[");
   UT_ASSERT(!utt_ODHaveStartOfSequence(GETIN_RAW));
}

static void recognizes_empty_partial_and_complete_prefixes(void)
{
   ut_set_sequence("");
   UT_ASSERT(utt_ODHaveStartOfSequence(0));
   ut_set_sequence("\033[");
   UT_ASSERT(utt_ODHaveStartOfSequence(0));
   ut_set_sequence("\177");
   UT_ASSERT(utt_ODHaveStartOfSequence(0));
}

static void rejects_nonmatching_short_and_long_strings(void)
{
   ut_set_sequence("Z");
   UT_ASSERT(!utt_ODHaveStartOfSequence(0));
   ut_set_sequence("ZZZZZZZZZ");
   UT_ASSERT(!utt_ODHaveStartOfSequence(0));
}

static void raw_control_filter_keeps_noncontrol_sequences(void)
{
   ut_set_sequence("\026\t");
   UT_ASSERT(!utt_ODHaveStartOfSequence(GETIN_RAWCTRL));
   ut_set_sequence("\033[");
   UT_ASSERT(utt_ODHaveStartOfSequence(GETIN_RAWCTRL));
}

static const UTTestCase ut_cases[] = {
   {"raw input", raw_input_never_starts_a_sequence},
   {"valid prefixes", recognizes_empty_partial_and_complete_prefixes},
   {"invalid prefixes", rejects_nonmatching_short_and_long_strings},
   {"raw control filter", raw_control_filter_keeps_noncontrol_sequences}
};
