#include "common.h"

static void raw_input_has_no_code(void)
{
   ut_set_sequence("\177");
   UT_ASSERT_EQ_INT(NO_MATCH, utt_ODGetCodeIfLongest(GETIN_RAW));
}

static void partial_sequence_waits_for_more_input(void)
{
   ut_set_sequence("\033");
   UT_ASSERT_EQ_INT(NO_MATCH, utt_ODGetCodeIfLongest(0));
}

static void exact_sequence_is_returned_when_unambiguous(void)
{
   int match;
   ut_set_sequence("\177");
   match = utt_ODGetCodeIfLongest(0);
   UT_ASSERT(match != NO_MATCH);
   UT_ASSERT_EQ_INT(OD_KEY_DELETE, aKeySequences[match].chExtendedKey);

   ut_set_sequence("\033[A");
   match = utt_ODGetCodeIfLongest(0);
   UT_ASSERT(match != NO_MATCH);
   UT_ASSERT_EQ_INT(OD_KEY_UP, aKeySequences[match].chExtendedKey);
}

static void nonsequence_has_no_code(void)
{
   ut_set_sequence("ZZZZZZZZZ");
   UT_ASSERT_EQ_INT(NO_MATCH, utt_ODGetCodeIfLongest(0));
}

static void raw_control_filter_skips_control_aliases(void)
{
   ut_set_sequence("\026\t");
   UT_ASSERT_EQ_INT(NO_MATCH, utt_ODGetCodeIfLongest(GETIN_RAWCTRL));
   ut_set_sequence("\177");
   UT_ASSERT(utt_ODGetCodeIfLongest(GETIN_RAWCTRL) != NO_MATCH);
}

static const UTTestCase ut_cases[] = {
   {"raw input", raw_input_has_no_code},
   {"partial sequence", partial_sequence_waits_for_more_input},
   {"exact sequence", exact_sequence_is_returned_when_unambiguous},
   {"nonsequence", nonsequence_has_no_code},
   {"raw control filter", raw_control_filter_skips_control_aliases}
};
