static void returns_initialization_error(void)
{
   nSessionScreenError = ERR_MEMORY;
   UT_ASSERT_EQ_INT(ERR_MEMORY, utt_ODSessionScreenError());
   nSessionScreenError = ERR_NONE;
   UT_ASSERT_EQ_INT(ERR_NONE, utt_ODSessionScreenError());
}

static const UTTestCase ut_cases[] = {
   {"screen error", returns_initialization_error}
};
