#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos
#define UT_CUSTOM_MOCK_ODScrnDisplayString
#define UT_CUSTOM_MOCK_ODScrnDisplayChar
#define UT_CUSTOM_MOCK_ODInQueueGetNextEvent
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#define UT_CUSTOM_MOCK_ODScrnPublish
#endif

static char ut_events[16];
static unsigned ut_event_count;
static unsigned ut_event_index;
static unsigned ut_cursor_calls;
static unsigned ut_string_calls;
static unsigned ut_char_calls;
static unsigned ut_publish_calls;
static unsigned ut_queue_calls;
static tODResult ut_queue_result;
static unsigned char ut_chars[24];

size_t utm_strlen(const char *string)
{
   size_t length;
   length = 0;
   while(string[length] != '\0') ++length;
   return length;
}

void ODCALL utm_ODScrnSetCursorPos(BYTE column, BYTE row)
{
   ++ut_cursor_calls;
   UT_ASSERT(column >= 10 && column <= 13);
   UT_ASSERT_EQ_UINT(5, row);
}

void ODCALL utm_ODScrnDisplayString(const char *string)
{
   ++ut_string_calls;
   UT_ASSERT(string != NULL);
}

void ODCALL utm_ODScrnDisplayChar(unsigned char character)
{
   if(ut_char_calls < DIM(ut_chars)) ut_chars[ut_char_calls] = character;
   ++ut_char_calls;
}

tODResult utm_ODInQueueGetNextEvent(tODInQueueHandle queue,
   tODInputEvent *event, tODMilliSec timeout)
{
   UT_ASSERT(queue == hODInputQueue); UT_ASSERT(event != NULL);
   UT_ASSERT_EQ_UINT(OD_NO_TIMEOUT, timeout);
   ++ut_queue_calls;
   if(ut_queue_result != kODRCSuccess)
   {
      event->chKeyPress = 'X';
      ut_queue_result = kODRCSuccess;
      return(kODRCGeneralFailure);
   }
   UT_ASSERT(ut_event_index < ut_event_count);
   memset(event, 0, sizeof(*event));
   event->chKeyPress = ut_events[ut_event_index++];
   return kODRCSuccess;
}

#ifdef ODPLAT_WIN32
static tODWindowsSubsystem ut_subsystem;
tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{ return(ut_subsystem); }
void utm_ODScrnPublish(void) { ++ut_publish_calls; }
#endif

static void reset_input(void)
{
   unsigned index;
   hODInputQueue = (tODInQueueHandle)1;
   ut_event_count = ut_event_index = 0;
   ut_cursor_calls = ut_string_calls = ut_char_calls = 0;
   ut_publish_calls = 0;
   ut_queue_calls = 0;
   ut_queue_result = kODRCSuccess;
#ifdef ODPLAT_WIN32
   ut_subsystem = kODWindowsSubsystemConsole;
#endif
   for(index = 0; index < DIM(ut_chars); ++index) ut_chars[index] = 0;
}

static void stops_without_reusing_an_event_after_queue_failure(void)
{
   char value[8];

   reset_input();
   strcpy(value, "OLD");
   ut_events[0] = '\r';
   ut_event_count = 1;
   ut_queue_result = kODRCGeneralFailure;

   utt_ODScrnLocalInput(10, 5, value, 3);

   UT_ASSERT(strcmp(value, "OLD") == 0);
   UT_ASSERT_EQ_UINT(1, ut_queue_calls);
   UT_ASSERT_EQ_UINT(0, ut_event_index);
}

#ifdef ODPLAT_WIN32
static void gui_input_does_not_publish_directly(void)
{
   char value[8];
   reset_input();
   ut_subsystem = kODWindowsSubsystemGUI;
   strcpy(value, "OLD");
   ut_queue_result = kODRCGeneralFailure;
   utt_ODScrnLocalInput(10, 5, value, 3);
   UT_ASSERT_EQ_UINT(0, ut_publish_calls);
}
#endif

static void edits_replaces_limits_and_handles_extended_input(void)
{
   char value[8];
   reset_input(); strcpy(value, "OLD");
   ut_events[0] = 'A'; ut_events[1] = 'B'; ut_events[2] = 'C';
   ut_events[3] = 'D'; ut_events[4] = '\b'; ut_events[5] = '\b';
   ut_events[6] = '\b'; ut_events[7] = '\b'; ut_events[8] = 1;
   ut_events[9] = '\0'; ut_events[10] = 'Z'; ut_events[11] = '\r';
   ut_event_count = 12;

   utt_ODScrnLocalInput(10, 5, value, 3);

   UT_ASSERT(strcmp(value, "") == 0);
   UT_ASSERT_EQ_UINT(12, ut_event_index);
   UT_ASSERT_EQ_UINT(1, ut_string_calls); UT_ASSERT_EQ_UINT(11, ut_char_calls);
   UT_ASSERT_EQ_UINT(177, ut_chars[0]); UT_ASSERT_EQ_UINT(177, ut_chars[4]);
   UT_ASSERT_EQ_UINT('A', ut_chars[5]); UT_ASSERT_EQ_UINT('C', ut_chars[7]);
   UT_ASSERT_EQ_UINT(177, ut_chars[10]);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(11, ut_publish_calls);
#endif
}

static void accepts_newline_with_an_initial_value_longer_than_the_limit(void)
{
   char value[4];
   reset_input(); strcpy(value, "AB");
   ut_events[0] = '\n'; ut_event_count = 1;
   utt_ODScrnLocalInput(10, 5, value, 1);
   UT_ASSERT(strcmp(value, "AB") == 0);
   UT_ASSERT_EQ_UINT(0, ut_char_calls);
   UT_ASSERT_EQ_UINT(1, ut_event_index);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(1, ut_publish_calls);
#endif
}

static const UTTestCase ut_cases[] = {
   {"edit input", edits_replaces_limits_and_handles_extended_input},
   {"newline", accepts_newline_with_an_initial_value_longer_than_the_limit},
   {"queue failure", stops_without_reusing_an_event_after_queue_failure}
#ifdef ODPLAT_WIN32
   ,{"GUI input", gui_input_does_not_publish_directly}
#endif
};
