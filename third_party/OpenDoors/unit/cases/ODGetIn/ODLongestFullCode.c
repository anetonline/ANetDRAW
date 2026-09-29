#include "common.h"

static void raw_input_has_no_translation(void)
{
   ut_set_sequence("\033[A");
   UT_ASSERT_EQ_INT(NO_MATCH, utt_ODLongestFullCode(GETIN_RAW));
}

static void finds_the_longest_complete_sequence(void)
{
   int match;
   ut_set_sequence("\033[H\033[2J");
   match = utt_ODLongestFullCode(0);
   UT_ASSERT(match != NO_MATCH);
   UT_ASSERT_EQ_INT(0, utm_strncmp(aKeySequences[match].pszSequence,
      "\033[H\033[2J", utm_strlen("\033[H\033[2J") + 1));

   ut_set_sequence("not a key sequence");
   UT_ASSERT_EQ_INT(NO_MATCH, utt_ODLongestFullCode(0));
}

static void raw_control_filter_skips_only_control_aliases(void)
{
   int match;
   ut_set_sequence("\026\t");
   UT_ASSERT_EQ_INT(NO_MATCH, utt_ODLongestFullCode(GETIN_RAWCTRL));

   ut_set_sequence("\033[A");
   match = utt_ODLongestFullCode(GETIN_RAWCTRL);
   UT_ASSERT(match != NO_MATCH);
   UT_ASSERT(!aKeySequences[match].bIsControlKey);
}

static const UTTestCase ut_cases[] = {
   {"raw input", raw_input_has_no_translation},
   {"longest complete sequence", finds_the_longest_complete_sequence},
   {"raw control filter", raw_control_filter_skips_only_control_aliases}
};
