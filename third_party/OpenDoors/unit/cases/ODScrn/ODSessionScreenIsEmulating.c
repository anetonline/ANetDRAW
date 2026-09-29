static void returns_emulation_state(void)
{
   bSessionScreenEmulating = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenIsEmulating());
   bSessionScreenEmulating = TRUE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODSessionScreenIsEmulating());
}

static const UTTestCase ut_cases[] = {
   {"emulation state", returns_emulation_state}
};
