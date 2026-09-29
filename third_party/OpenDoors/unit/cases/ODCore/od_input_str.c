#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_disp_str
#define UT_CUSTOM_MOCK_od_get_key
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_putch

static char ut_keys[16];
static unsigned ut_key_count;
static unsigned ut_key_index;
static unsigned ut_shutdown_key;
static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static unsigned ut_putch_calls;
static char ut_echoed[8];
static unsigned ut_disp_calls;
static const char *ut_last_display;
static BOOL ut_init_succeeds;

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

char ODCALL utm_od_get_key(BOOL wait)
{
   UT_ASSERT_EQ_INT(TRUE, wait);
   UT_ASSERT(ut_key_index < ut_key_count);
   ++ut_key_index;
   if(ut_key_index == ut_shutdown_key) bODInitialized = FALSE;
   return ut_keys[ut_key_index - 1];
}

void ODCALL utm_od_putch(char value)
{
   UT_ASSERT(ut_putch_calls < sizeof(ut_echoed));
   ut_echoed[ut_putch_calls++] = value;
}

void ODCALL utm_od_disp_str(const char *text)
{
   ++ut_disp_calls;
   ut_last_display = text;
}

static void reset_input(void)
{
   bODInitialized = TRUE;
   od_control.od_error = 0;
   ut_key_count = 0;
   ut_key_index = 0;
   ut_shutdown_key = 0;
   ut_init_calls = 0;
   ut_entries = 0;
   ut_exits = 0;
   ut_putch_calls = 0;
   ut_disp_calls = 0;
   ut_last_display = NULL;
   ut_init_succeeds = TRUE;
}

static void terminal_session_is_rejected(void)
{
   char output[2];
   reset_input(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   utt_od_input_str(output, 1, 'a', 'z');
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void rejects_each_invalid_parameter(void)
{
   char output[2];
   reset_input();
   bODInitialized = FALSE;
   utt_od_input_str(NULL, 1, 'a', 'z');
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);

   reset_input();
   utt_od_input_str(output, 0, 'a', 'z');
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);

   reset_input();
   utt_od_input_str(output, 1, 'z', 'a');
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
}

static void filters_edits_limits_and_terminates_on_line_feed(void)
{
   char output[3] = {'?', '?', '?'};
   reset_input();
   ut_keys[0] = 8;
   ut_keys[1] = '@';
   ut_keys[2] = '[';
   ut_keys[3] = 'A';
   ut_keys[4] = 'B';
   ut_keys[5] = 'C';
   ut_keys[6] = 8;
   ut_keys[7] = 'C';
   ut_keys[8] = '\n';
   ut_key_count = 9;
   utt_od_input_str(output, 2, 'A', 'Z');
   UT_ASSERT(output[0] == 'A' && output[1] == 'C' && output[2] == '\0');
   UT_ASSERT_EQ_UINT(3, ut_putch_calls);
   UT_ASSERT(ut_echoed[0] == 'A' && ut_echoed[1] == 'B'
      && ut_echoed[2] == 'C');
   UT_ASSERT_EQ_UINT(2, ut_disp_calls);
   UT_ASSERT(ut_last_display[0] == '\r' && ut_last_display[1] == '\n'
      && ut_last_display[2] == '\0');
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static void carriage_return_terminates_an_empty_string(void)
{
   char output[2] = {'?', '?'};
   reset_input();
   ut_keys[0] = '\r';
   ut_key_count = 1;
   utt_od_input_str(output, 1, 0, 255);
   UT_ASSERT(output[0] == '\0');
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
}

static void session_shutdown_returns_without_echo_or_termination(void)
{
   char output[2] = {'?', '?'};
   reset_input();
   ut_keys[0] = 'A';
   ut_key_count = 1;
   ut_shutdown_key = 1;
   utt_od_input_str(output, 1, 'A', 'Z');
   UT_ASSERT(output[0] == '?');
   UT_ASSERT_EQ_UINT(0, ut_putch_calls);
   UT_ASSERT_EQ_UINT(0, ut_disp_calls);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static const UTTestCase ut_cases[] = {
   {"invalid parameters", rejects_each_invalid_parameter},
   {"filtered editing", filters_edits_limits_and_terminates_on_line_feed},
   {"carriage return", carriage_return_terminates_an_empty_string},
   {"session shutdown", session_shutdown_returns_without_echo_or_termination},
   {"terminal session", terminal_session_is_rejected}
};
