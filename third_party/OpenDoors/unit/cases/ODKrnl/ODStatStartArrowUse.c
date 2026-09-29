static void increments_the_nested_arrow_key_owner_count(void)
{
   nArrowUseCount = 4; utt_ODStatStartArrowUse();
   UT_ASSERT_EQ_INT(5, nArrowUseCount);
}
static const UTTestCase ut_cases[] = {
   {"increment", increments_the_nested_arrow_key_owner_count}
};
