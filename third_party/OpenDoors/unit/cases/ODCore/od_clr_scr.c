#define UT_CUSTOM_MOCK_ODScrnClear
#define UT_CUSTOM_MOCK_ODSessionScreenAvailable
#define UT_CUSTOM_MOCK_ODSessionScreenClear
#define UT_CUSTOM_MOCK_ODSessionScreenPresent
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_disp
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_set_attrib

static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static unsigned ut_disp_calls;
static char ut_disp_text[3][16];
static INT ut_disp_sizes[3];
static BOOL ut_session_available;
static unsigned ut_session_clears;
static unsigned ut_present_calls;
static unsigned ut_local_clears;
static unsigned ut_attrib_calls;
static INT ut_seen_attrib;
static BOOL ut_init_succeeds;

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

void ODCALL utm_od_disp(const char *buffer, INT size, BOOL local_echo)
{
   INT index;
   UT_ASSERT(ut_disp_calls < 3);
   UT_ASSERT(size >= 0 && size < (INT)sizeof(ut_disp_text[0]));
   UT_ASSERT_EQ_INT(FALSE, local_echo);
   ut_disp_sizes[ut_disp_calls] = size;
   for(index = 0; index < size; ++index)
      ut_disp_text[ut_disp_calls][index] = buffer[index];
   ut_disp_text[ut_disp_calls][size] = '\0';
   ++ut_disp_calls;
}

BOOL utm_ODSessionScreenAvailable(void) { return ut_session_available; }
void utm_ODSessionScreenClear(void) { ++ut_session_clears; }
void utm_ODSessionScreenPresent(void) { ++ut_present_calls; }
void utm_ODScrnClear(void) { ++ut_local_clears; }

void ODCALL utm_od_set_attrib(INT attribute)
{
   ++ut_attrib_calls;
   ut_seen_attrib = attribute;
}

static void reset_clear_screen(void)
{
   bODInitialized = TRUE;
   od_control.od_always_clear = FALSE;
   od_control.user_attribute = 0;
   od_control.od_extended_info = FALSE;
   od_control.od_info_type = 0;
   od_control.user_rip = FALSE;
   od_control.od_default_rip_win = FALSE;
   od_control.user_ansi = FALSE;
   od_control.od_cur_attrib = 0x1e;
   ut_init_calls = 0;
   ut_entries = 0;
   ut_exits = 0;
   ut_disp_calls = 0;
   ut_session_available = FALSE;
   ut_session_clears = 0;
   ut_present_calls = 0;
   ut_local_clears = 0;
   ut_attrib_calls = 0;
   ut_seen_attrib = 0;
   ut_init_succeeds = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_clear_screen(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   utt_od_clr_scr();
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void assert_skipped(void)
{
   UT_ASSERT_EQ_UINT(0, ut_disp_calls);
   UT_ASSERT_EQ_UINT(0, ut_session_clears);
   UT_ASSERT_EQ_UINT(0, ut_local_clears);
   UT_ASSERT_EQ_UINT(0, ut_attrib_calls);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static void extended_sessions_can_suppress_clearing(void)
{
   reset_clear_screen();
   bODInitialized = FALSE;
   od_control.od_extended_info = TRUE;
   utt_od_clr_scr();
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   assert_skipped();
}

static void custom_sessions_can_suppress_clearing(void)
{
   reset_clear_screen();
   od_control.od_info_type = CUSTOM;
   utt_od_clr_scr();
   assert_skipped();
}

static void always_clear_overrides_the_suppression_rule(void)
{
   reset_clear_screen();
   od_control.od_always_clear = TRUE;
   od_control.od_extended_info = TRUE;
   utt_od_clr_scr();
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
   UT_ASSERT_EQ_UINT(1, ut_local_clears);
}

static void graphics_capability_overrides_the_suppression_rule(void)
{
   reset_clear_screen();
   od_control.user_attribute = 2;
   od_control.od_extended_info = TRUE;
   utt_od_clr_scr();
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
   UT_ASSERT_EQ_UINT(1, ut_local_clears);
}

static void ordinary_sessions_clear_even_without_overrides(void)
{
   reset_clear_screen();
   utt_od_clr_scr();
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
   UT_ASSERT_EQ_INT(1, ut_disp_sizes[0]);
   UT_ASSERT_EQ_UINT(12, (unsigned char)ut_disp_text[0][0]);
   UT_ASSERT_EQ_UINT(1, ut_local_clears);
   UT_ASSERT_EQ_UINT(1, ut_attrib_calls);
   UT_ASSERT_EQ_INT(0x1e, ut_seen_attrib);
   UT_ASSERT_EQ_INT(-1, od_control.od_cur_attrib);
}

static void rip_and_ansi_clear_the_virtual_session_screen(void)
{
   reset_clear_screen();
   od_control.user_rip = TRUE;
   od_control.user_ansi = TRUE;
   ut_session_available = TRUE;
   utt_od_clr_scr();
   UT_ASSERT_EQ_UINT(3, ut_disp_calls);
   UT_ASSERT_EQ_INT(3, ut_disp_sizes[0]);
   UT_ASSERT(ut_disp_text[0][0] == '!' && ut_disp_text[0][1] == '|'
      && ut_disp_text[0][2] == '*');
   UT_ASSERT_EQ_INT(13, ut_disp_sizes[1]);
   UT_ASSERT_EQ_INT(10, ut_disp_sizes[2]);
   UT_ASSERT_EQ_UINT(1, ut_session_clears);
   UT_ASSERT_EQ_UINT(1, ut_present_calls);
   UT_ASSERT_EQ_UINT(0, ut_local_clears);
}

static void default_rip_window_omits_the_window_reset(void)
{
   reset_clear_screen();
   od_control.user_rip = TRUE;
   od_control.od_default_rip_win = TRUE;
   utt_od_clr_scr();
   UT_ASSERT_EQ_UINT(2, ut_disp_calls);
   UT_ASSERT_EQ_INT(3, ut_disp_sizes[0]);
   UT_ASSERT_EQ_INT(1, ut_disp_sizes[1]);
}

static const UTTestCase ut_cases[] = {
   {"extended suppression", extended_sessions_can_suppress_clearing},
   {"custom suppression", custom_sessions_can_suppress_clearing},
   {"always-clear override", always_clear_overrides_the_suppression_rule},
   {"graphics override", graphics_capability_overrides_the_suppression_rule},
   {"ordinary clear", ordinary_sessions_clear_even_without_overrides},
   {"RIP ANSI clear", rip_and_ansi_clear_the_virtual_session_screen},
   {"default RIP window", default_rip_window_omits_the_window_reset},
   {"terminal session", terminal_session_is_rejected}
};
