static void copies_a_word_and_stops_at_the_first_space(void)
{
   char input[] = "first second";
   char output[8];
   UT_ASSERT(utt_ODListGetFirstWord(input, output, sizeof(output)));
   UT_ASSERT(strcmp(output, "first") == 0);
}

static void accepts_empty_and_exact_fit_words(void)
{
   char empty[] = "";
   char exact[] = "abc";
   char output[4];
   UT_ASSERT(utt_ODListGetFirstWord(empty, output, sizeof(output)));
   UT_ASSERT_EQ_INT('\0', output[0]);
   UT_ASSERT(utt_ODListGetFirstWord(exact, output, sizeof(output)));
   UT_ASSERT(strcmp(output, "abc") == 0);
}

static void rejects_a_word_which_does_not_fit(void)
{
   char input[] = "abcd";
   char output[4] = "old";
   UT_ASSERT(!utt_ODListGetFirstWord(input, output, sizeof(output)));
   UT_ASSERT_EQ_INT('\0', output[0]);
}

static const UTTestCase ut_cases[] = {
   {"first word", copies_a_word_and_stops_at_the_first_space},
   {"short words", accepts_empty_and_exact_fit_words},
   {"capacity", rejects_a_word_which_does_not_fit}
};
