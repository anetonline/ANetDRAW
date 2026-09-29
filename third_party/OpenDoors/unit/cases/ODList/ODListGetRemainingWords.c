static void finds_the_second_word_after_all_spaces(void)
{
   char input[] = "first   second third";
   UT_ASSERT_EQ_PTR(input + 8, utt_ODListGetRemainingWords(input));
   UT_ASSERT(strcmp(utt_ODListGetRemainingWords(input), "second third") == 0);
}

static void returns_the_terminator_when_no_words_remain(void)
{
   char one[] = "first";
   char trailing[] = "first   ";
   char empty[] = "";
   UT_ASSERT_EQ_PTR(one + 5, utt_ODListGetRemainingWords(one));
   UT_ASSERT_EQ_PTR(trailing + 8, utt_ODListGetRemainingWords(trailing));
   UT_ASSERT_EQ_PTR(empty, utt_ODListGetRemainingWords(empty));
}

static void treats_leading_spaces_as_an_empty_first_word(void)
{
   char input[] = "  second";
   UT_ASSERT_EQ_PTR(input + 2, utt_ODListGetRemainingWords(input));
}

static const UTTestCase ut_cases[] = {
   {"remaining words", finds_the_second_word_after_all_spaces},
   {"no remainder", returns_the_terminator_when_no_words_remain},
   {"empty first word", treats_leading_spaces_as_an_empty_first_word}
};
