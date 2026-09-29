#include "common.h"

static void zero_shift_does_nothing(void)
{
   ut_set_sequence("abcd");
   utt_ODShiftSeq(0);
   UT_ASSERT_EQ_INT(0, utm_strncmp(szCurrentSequence, "abcd", 5));
}

static void shift_past_end_does_nothing(void)
{
   ut_set_sequence("abcd");
   utt_ODShiftSeq(5);
   UT_ASSERT_EQ_INT(0, utm_strncmp(szCurrentSequence, "abcd", 5));
}

static void shifts_characters_and_the_terminator(void)
{
   ut_set_sequence("abcd");
   utt_ODShiftSeq(2);
   UT_ASSERT_EQ_INT(0, utm_strncmp(szCurrentSequence, "cd", 3));

   ut_set_sequence("abcd");
   utt_ODShiftSeq(4);
   UT_ASSERT_EQ_INT('\0', szCurrentSequence[0]);
}

static const UTTestCase ut_cases[] = {
   {"zero shift", zero_shift_does_nothing},
   {"past end", shift_past_end_does_nothing},
   {"ordinary shifts", shifts_characters_and_the_terminator}
};
