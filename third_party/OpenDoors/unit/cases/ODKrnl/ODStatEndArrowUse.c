static void decrements_the_nested_arrow_key_owner_count(void)
{
   nArrowUseCount = 4; utt_ODStatEndArrowUse();
   UT_ASSERT_EQ_INT(3, nArrowUseCount);
}
static const UTTestCase ut_cases[] = {
   {"decrement", decrements_the_nested_arrow_key_owner_count}
};
