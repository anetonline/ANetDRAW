#define UT_CUSTOM_MOCK_ODScrnDisplayString
#define UT_CUSTOM_MOCK_ODSessionScreenAvailable
#define UT_CUSTOM_MOCK_ODSessionScreenDisplayString
#define UT_CUSTOM_MOCK_ODSessionScreenPresent
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_disp
#define UT_CUSTOM_MOCK_od_init

static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static BOOL ut_session_available;
static unsigned ut_session_calls;
static unsigned ut_present_calls;
static unsigned ut_local_calls;
static unsigned ut_disp_calls;
static char ut_disp_bytes[8];
static INT ut_disp_size;
static BOOL ut_disp_echo;
static BOOL ut_init_succeeds;

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }
BOOL utm_ODSessionScreenAvailable(void) { return ut_session_available; }

void utm_ODSessionScreenDisplayString(const char *text)
{
   UT_ASSERT_EQ_PTR(szODWorkString, text);
   ++ut_session_calls;
}

void utm_ODSessionScreenPresent(void) { ++ut_present_calls; }

void ODCALL utm_ODScrnDisplayString(const char *text)
{
   UT_ASSERT_EQ_PTR(szODWorkString, text);
   ++ut_local_calls;
}

void ODCALL utm_od_disp(const char *buffer, INT size, BOOL local_echo)
{
   INT index;
   UT_ASSERT(size >= 0 && size <= (INT)sizeof(ut_disp_bytes));
   ++ut_disp_calls;
   ut_disp_size = size;
   ut_disp_echo = local_echo;
   for(index = 0; index < size; ++index) ut_disp_bytes[index] = buffer[index];
}

static void reset_repeat(void)
{
   bODInitialized = TRUE;
   od_control.user_avatar = FALSE;
   szODWorkString[0] = '\0';
   ut_init_calls = 0;
   ut_entries = 0;
   ut_exits = 0;
   ut_session_available = FALSE;
   ut_session_calls = 0;
   ut_present_calls = 0;
   ut_local_calls = 0;
   ut_disp_calls = 0;
   ut_disp_size = 0;
   ut_disp_echo = TRUE;
   ut_init_succeeds = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_repeat(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   utt_od_repeat('x', 0);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void zero_repetitions_only_cross_the_api_boundary(void)
{
   reset_repeat();
   bODInitialized = FALSE;
   utt_od_repeat('x', 0);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(0, ut_session_calls);
   UT_ASSERT_EQ_UINT(0, ut_local_calls);
   UT_ASSERT_EQ_UINT(0, ut_disp_calls);
}

static void avatar_mode_uses_the_repeat_control_sequence(void)
{
   reset_repeat();
   od_control.user_avatar = TRUE;
   ut_session_available = TRUE;
   utt_od_repeat('q', 3);
   UT_ASSERT(szODWorkString[0] == 'q' && szODWorkString[1] == 'q'
      && szODWorkString[2] == 'q' && szODWorkString[3] == '\0');
   UT_ASSERT_EQ_UINT(1, ut_session_calls);
   UT_ASSERT_EQ_UINT(1, ut_present_calls);
   UT_ASSERT_EQ_UINT(0, ut_local_calls);
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
   UT_ASSERT_EQ_INT(3, ut_disp_size);
   UT_ASSERT_EQ_UINT(25, (unsigned char)ut_disp_bytes[0]);
   UT_ASSERT(ut_disp_bytes[1] == 'q');
   UT_ASSERT_EQ_UINT(3, (unsigned char)ut_disp_bytes[2]);
   UT_ASSERT_EQ_INT(FALSE, ut_disp_echo);
}

static void plain_mode_sends_the_complete_repeated_string(void)
{
   reset_repeat();
   utt_od_repeat('z', 2);
   UT_ASSERT_EQ_UINT(0, ut_session_calls);
   UT_ASSERT_EQ_UINT(0, ut_present_calls);
   UT_ASSERT_EQ_UINT(1, ut_local_calls);
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
   UT_ASSERT_EQ_INT(2, ut_disp_size);
   UT_ASSERT(ut_disp_bytes[0] == 'z' && ut_disp_bytes[1] == 'z');
   UT_ASSERT_EQ_INT(FALSE, ut_disp_echo);
}

static const UTTestCase ut_cases[] = {
   {"zero repeats", zero_repetitions_only_cross_the_api_boundary},
   {"AVATAR repeat", avatar_mode_uses_the_repeat_control_sequence},
   {"plain repeat", plain_mode_sends_the_complete_repeated_string},
   {"terminal session", terminal_session_is_rejected}
};
