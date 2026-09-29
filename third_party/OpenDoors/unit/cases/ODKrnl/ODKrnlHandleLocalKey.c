#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODKrnlGetUIState
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#endif
#define UT_CUSTOM_MOCK_ODKrnlHandleReceivedChar
#define UT_CUSTOM_MOCK_ODInQueueAddEvents
tODControl od_control;
tODInQueueHandle hODInputQueue;
static char ut_characters[3];
static BOOL ut_remote[3];
static unsigned ut_calls;
static unsigned ut_add_events_calls;
static INT ut_add_event_count;
static tODInputEvent ut_added_events[2];
#ifdef ODPLAT_WIN32
static tODWindowsSubsystem ut_subsystem;
static WORD ut_disable;
void utm_ODKrnlGetUIState(tODUIState *state)
{ memset(state, 0, sizeof(*state)); state->wDisable = ut_disable; }
tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{ return(ut_subsystem); }
#endif
void utm_ODKrnlHandleReceivedChar(char character, BOOL remote)
{
   UT_ASSERT(ut_calls < 3); ut_characters[ut_calls] = character;
   ut_remote[ut_calls++] = remote;
}
tODResult utm_ODInQueueAddEvents(tODInQueueHandle queue,
   const tODInputEvent *events, INT count)
{
   INT index;
   UT_ASSERT(queue == hODInputQueue);
   UT_ASSERT(count <= 2);
   ++ut_add_events_calls;
   ut_add_event_count = count;
   for(index = 0; index < count; ++index)
      ut_added_events[index] = events[index];
   return(kODRCSuccess);
}
static void ignores_local_keys_when_local_input_is_disabled(void)
{
   memset(&od_control, 0, sizeof(od_control));
#ifdef ODPLAT_WIN32
   ut_subsystem = kODWindowsSubsystemGUI;
   ut_disable = DIS_LOCAL_INPUT;
#else
   od_control.od_disable = DIS_LOCAL_INPUT; ut_calls = 0;
#endif
   ut_calls = 0;
   utt_ODKrnlHandleLocalKey((WORD)'A'); UT_ASSERT_EQ_UINT(0, ut_calls);
}
static void queues_an_extended_key_as_two_local_characters(void)
{
   memset(&od_control, 0, sizeof(od_control)); ut_calls = 0;
   ut_add_events_calls = 0; ut_add_event_count = 0;
#ifdef ODPLAT_WIN32
   ut_subsystem = kODWindowsSubsystemGUI;
   ut_disable = 0;
#endif
   utt_ODKrnlHandleLocalKey((WORD)((WORD)OD_KEY_LEFT << 8));
   UT_ASSERT_EQ_UINT(0, ut_calls);
   UT_ASSERT_EQ_UINT(1, ut_add_events_calls);
   UT_ASSERT_EQ_INT(2, ut_add_event_count);
   UT_ASSERT_EQ_INT(EVENT_CHARACTER, ut_added_events[0].EventType);
   UT_ASSERT_EQ_INT(0, ut_added_events[0].chKeyPress);
   UT_ASSERT_EQ_INT((char)OD_KEY_LEFT, ut_added_events[1].chKeyPress);
   UT_ASSERT(!ut_added_events[0].bFromRemote
      && !ut_added_events[1].bFromRemote);
}
static void queues_an_ordinary_key_as_one_local_character(void)
{
   memset(&od_control, 0, sizeof(od_control)); ut_calls = 0;
#ifdef ODPLAT_WIN32
   ut_subsystem = kODWindowsSubsystemGUI;
   ut_disable = 0;
#endif
   utt_ODKrnlHandleLocalKey((WORD)(((WORD)0x1e << 8) | (WORD)'A'));
   UT_ASSERT_EQ_UINT(1, ut_calls); UT_ASSERT_EQ_INT('A', ut_characters[0]);
   UT_ASSERT(!ut_remote[0]);
}
#ifdef ODPLAT_WIN32
static void console_uses_the_public_disable_bits(void)
{
   memset(&od_control, 0, sizeof(od_control)); ut_calls = 0;
   ut_subsystem = kODWindowsSubsystemConsole;
   od_control.od_disable = DIS_LOCAL_INPUT;
   utt_ODKrnlHandleLocalKey((WORD)'A');
   UT_ASSERT_EQ_UINT(0, ut_calls);

   od_control.od_disable = 0;
   utt_ODKrnlHandleLocalKey((WORD)'A');
   UT_ASSERT_EQ_UINT(1, ut_calls);
}
#endif
static const UTTestCase ut_cases[] = {
   {"disabled", ignores_local_keys_when_local_input_is_disabled},
   {"extended", queues_an_extended_key_as_two_local_characters},
   {"ordinary", queues_an_ordinary_key_as_one_local_character}
#ifdef ODPLAT_WIN32
   ,{"console disable", console_uses_the_public_disable_bits}
#endif
};
