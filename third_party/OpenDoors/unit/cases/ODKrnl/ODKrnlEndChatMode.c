static void clears_the_public_chat_active_flag(void)
{
   memset(&od_control, 0, sizeof(od_control));
   od_control.od_chat_active = TRUE; utt_ODKrnlEndChatMode();
   UT_ASSERT_EQ_INT(FALSE, od_control.od_chat_active);
}
static const UTTestCase ut_cases[] = {
   {"clear", clears_the_public_chat_active_flag}
};
