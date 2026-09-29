#define UT_CUSTOM_MOCK_ODEmulateFromBuffer
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_disp
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_strlen

static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static unsigned ut_disp_calls;
static unsigned ut_emulate_calls;
static BOOL ut_translate;
static BOOL ut_session_echo;
static const char *ut_text;
static BOOL ut_init_succeeds;

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

size_t utm_strlen(const char *text)
{
   const char *end = text;
   while(*end != '\0')
      ++end;
   return (size_t)(end - text);
}

void ODCALL utm_od_disp(const char *text, INT size, BOOL local)
{
   ++ut_disp_calls;
   UT_ASSERT_EQ_PTR(ut_text, text);
   UT_ASSERT_EQ_INT(3, size);
   UT_ASSERT_EQ_INT(FALSE, local);
}

void utm_ODEmulateFromBuffer(const char *text, BOOL translate,
   BOOL session_echo)
{
   ++ut_emulate_calls;
   UT_ASSERT_EQ_PTR(ut_text, text);
   ut_translate = translate;
   ut_session_echo = session_echo;
}

static void reset_display(void)
{
   memset(&od_control, 0, sizeof(od_control));
   bODInitialized = TRUE;
   ut_init_calls = ut_entries = ut_exits = 0;
   ut_disp_calls = ut_emulate_calls = 0;
   ut_translate = ut_session_echo = FALSE;
   ut_text = "abc";
   ut_init_succeeds = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_display(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   utt_od_disp_emu(ut_text, FALSE);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void local_only_output_needs_no_remote_translation(void)
{
   reset_display();
   bODInitialized = FALSE;
   utt_od_disp_emu(ut_text, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(0, ut_disp_calls);
   UT_ASSERT_EQ_UINT(1, ut_emulate_calls);
   UT_ASSERT_EQ_INT(FALSE, ut_translate);
   UT_ASSERT_EQ_INT(FALSE, ut_session_echo);
}

static void raw_remote_output_is_sent_before_local_emulation(void)
{
   reset_display();
   od_control.od_no_ra_codes = TRUE;
   utt_od_disp_emu(ut_text, TRUE);
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
   UT_ASSERT_EQ_UINT(1, ut_emulate_calls);
   UT_ASSERT_EQ_INT(FALSE, ut_translate);
   UT_ASSERT_EQ_INT(TRUE, ut_session_echo);
}

static void translated_remote_output_is_emulated_once(void)
{
   reset_display();
   od_control.od_no_ra_codes = FALSE;
   utt_od_disp_emu(ut_text, TRUE);
   UT_ASSERT_EQ_UINT(0, ut_disp_calls);
   UT_ASSERT_EQ_UINT(1, ut_emulate_calls);
   UT_ASSERT_EQ_INT(TRUE, ut_translate);
   UT_ASSERT_EQ_INT(TRUE, ut_session_echo);
}

static const UTTestCase ut_cases[] = {
   {"local only", local_only_output_needs_no_remote_translation},
   {"raw remote", raw_remote_output_is_sent_before_local_emulation},
   {"translated remote", translated_remote_output_is_emulated_once},
   {"terminal session", terminal_session_is_rejected}
};
