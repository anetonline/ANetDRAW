static void recognized_true_initials_are_true(void)
{
   char values[] = "1tTyYgG";
   unsigned index;
   for(index = 0; values[index] != '\0'; ++index)
      UT_ASSERT_EQ_INT(TRUE, utt_ODCfgIsTrue(&values[index]));
}

static void leading_horizontal_space_is_skipped(void)
{
   char text[] = " \t  Yes";
   UT_ASSERT_EQ_INT(TRUE, utt_ODCfgIsTrue(text));
}

static void other_and_empty_values_are_false(void)
{
   char no[] = "No";
   char empty[] = "";
   char spaces[] = " \t";
   UT_ASSERT_EQ_INT(FALSE, utt_ODCfgIsTrue(no));
   UT_ASSERT_EQ_INT(FALSE, utt_ODCfgIsTrue(empty));
   UT_ASSERT_EQ_INT(FALSE, utt_ODCfgIsTrue(spaces));
}

static const UTTestCase ut_cases[] = {
   {"recognized true initials", recognized_true_initials_are_true},
   {"leading whitespace", leading_horizontal_space_is_skipped},
   {"false values", other_and_empty_values_are_false}
};
