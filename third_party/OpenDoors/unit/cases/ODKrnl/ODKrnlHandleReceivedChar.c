#define UT_CUSTOM_MOCK_ODInQueueAddEvent
static unsigned ut_add_calls;
static tODInputEvent ut_event;
tODResult utm_ODInQueueAddEvent(tODInQueueHandle queue, tODInputEvent *event)
{
   ++ut_add_calls; UT_ASSERT(queue == hODInputQueue); ut_event = *event;
   return kODRCSuccess;
}
static void reset_received(BOOL keyboard_on)
{
   memset(&od_control, 0, sizeof(od_control)); memset(&ut_event, 0, sizeof(ut_event));
   od_control.od_user_keyboard_on = keyboard_on; hODInputQueue = (tODInQueueHandle)1;
   ut_add_calls = 0;
}
static void ignores_only_remote_input_when_the_user_keyboard_is_off(void)
{
   reset_received(FALSE); utt_ODKrnlHandleReceivedChar('A', TRUE);
   UT_ASSERT_EQ_UINT(0, ut_add_calls);
   utt_ODKrnlHandleReceivedChar('B', FALSE); UT_ASSERT_EQ_UINT(1, ut_add_calls);
   UT_ASSERT_EQ_INT('B', ut_event.chKeyPress); UT_ASSERT(!ut_event.bFromRemote);
}
static void queues_remote_input_with_character_event_metadata(void)
{
   reset_received(TRUE); utt_ODKrnlHandleReceivedChar('A', TRUE);
   UT_ASSERT_EQ_UINT(1, ut_add_calls); UT_ASSERT_EQ_INT(EVENT_CHARACTER, ut_event.EventType);
   UT_ASSERT(ut_event.bFromRemote); UT_ASSERT_EQ_INT('A', ut_event.chKeyPress);
}
static const UTTestCase ut_cases[] = {
   {"disabled remote", ignores_only_remote_input_when_the_user_keyboard_is_off},
   {"remote event", queues_remote_input_with_character_event_metadata}
};
