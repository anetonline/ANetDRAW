#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_ODTimerElapsed
#define UT_CUSTOM_MOCK_ODTimerStart
#define UT_CUSTOM_MOCK_localtime
#define UT_CUSTOM_MOCK_od_clr_scr
#define UT_CUSTOM_MOCK_od_disp_str
#define UT_CUSTOM_MOCK_od_get_answer
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_input_str
#define UT_CUSTOM_MOCK_od_putch
#define UT_CUSTOM_MOCK_od_repeat
#define UT_CUSTOM_MOCK_od_set_attrib
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_time
#ifdef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_od_sleep
#endif
#define UT_CUSTOM_MOCK_od_kernel
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#endif
#ifdef OD_PERSONALITY_SUPPORT
#define UT_CUSTOM_MOCK_od_set_statusline
#endif

static char ut_reason[80];
static char ut_chat_reason[] = "Reason";
static char ut_no_sysop[] = "No sysop";
static char ut_press_key[] = "Press";
static char ut_paging[] = "Paging";
static char ut_no_response[] = "No response";
static struct tm ut_time_block;
static BOOL ut_localtime_fails;
static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static unsigned ut_clear_calls;
static INT ut_attributes[8];
static unsigned ut_attribute_calls;
static unsigned ut_display_calls;
static unsigned ut_denied_calls;
static unsigned ut_paging_calls;
static unsigned ut_no_response_calls;
static unsigned ut_answer_calls;
static unsigned ut_repeat_calls;
static char ut_repeat_character;
static unsigned ut_putch_calls;
static unsigned ut_chat_on_putch;
static unsigned ut_timer_starts;
static BOOL ut_elapsed_values[4];
static unsigned ut_elapsed_count;
static unsigned ut_elapsed_index;
static unsigned ut_log_calls;
#ifdef OD_THREAD_SUPPORT
static unsigned ut_sleep_calls;
static BOOL ut_shutdown_on_sleep;
#endif
static unsigned ut_kernel_calls;
static BOOL ut_init_succeeds;
#ifdef OD_PERSONALITY_SUPPORT
static unsigned ut_status_calls;
static INT ut_status_setting;
#endif
#ifdef ODPLAT_WIN32
static tODWindowsSubsystem ut_subsystem;
tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{
   return(ut_subsystem);
}
#endif

static void ut_copy(char *destination, const char *source)
{
   while((*destination++ = *source++) != '\0') { }
}

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }
void ODCALL utm_od_clr_scr(void) { ++ut_clear_calls; }

void ODCALL utm_od_set_attrib(INT attribute)
{
   UT_ASSERT(ut_attribute_calls < sizeof(ut_attributes) /
      sizeof(ut_attributes[0]));
   ut_attributes[ut_attribute_calls++] = attribute;
}

void ODCALL utm_od_disp_str(const char *text)
{
   ++ut_display_calls;
   if((text[0] == '\r' || text[0] == '\n')
      && (text[1] == '\r' || text[1] == '\n') && text[2] == '\0')
   {
      UT_ASSERT(text[0] == '\r' && text[1] == '\n');
   }
   if(text == ut_no_sysop) ++ut_denied_calls;
   if(text == ut_paging) ++ut_paging_calls;
   if(text == ut_no_response) ++ut_no_response_calls;
}

char ODCALL utm_od_get_answer(const char *options)
{
   UT_ASSERT(options != NULL);
   ++ut_answer_calls;
   return '\r';
}

void ODCALL utm_od_input_str(char *output, INT maximum,
   unsigned char minimum, unsigned char maximum_value)
{
   UT_ASSERT_EQ_PTR(od_control.user_reasonforchat, output);
   UT_ASSERT_EQ_INT(77, maximum);
   UT_ASSERT_EQ_UINT(32, minimum);
   UT_ASSERT_EQ_UINT(255, maximum_value);
   ut_copy(output, ut_reason);
}

void ODCALL utm_od_repeat(char value, BYTE count)
{
   UT_ASSERT_EQ_UINT(77, count);
   ++ut_repeat_calls;
   ut_repeat_character = value;
}

void ODCALL utm_od_putch(char value)
{
   (void)value;
   ++ut_putch_calls;
   if(ut_putch_calls == ut_chat_on_putch) bChatted = TRUE;
}

time_t utm_time(time_t *storage)
{
   UT_ASSERT(storage == NULL);
   return (time_t)123;
}

struct tm *utm_localtime(const time_t *value)
{
   UT_ASSERT(value != NULL && *value == (time_t)123);
   return ut_localtime_fails ? NULL : &ut_time_block;
}

void utm_ODTimerStart(tODTimer *timer, tODMilliSec duration)
{
   UT_ASSERT(timer != NULL);
   UT_ASSERT_EQ_UINT(1000, duration);
   ++ut_timer_starts;
}

BOOL utm_ODTimerElapsed(tODTimer *timer)
{
   UT_ASSERT(timer != NULL);
   UT_ASSERT(ut_elapsed_index < ut_elapsed_count);
   return ut_elapsed_values[ut_elapsed_index++];
}

#ifdef OD_THREAD_SUPPORT
void ODCALL utm_od_sleep(tODMilliSec duration)
{
   UT_ASSERT_EQ_UINT(0, duration);
   ++ut_sleep_calls;
   if(ut_shutdown_on_sleep) bODInitialized = FALSE;
}
#endif
void ODCALL utm_od_kernel(void) { ++ut_kernel_calls; }


#ifdef OD_PERSONALITY_SUPPORT
void ODCALL utm_od_set_statusline(INT setting)
{
   ++ut_status_calls;
   ut_status_setting = setting;
}
#endif

static BOOL ODCALL ut_log_write(INT type)
{
   UT_ASSERT_EQ_INT(8, type);
   ++ut_log_calls;
   return TRUE;
}

static void reset_page(void)
{
   bODInitialized = TRUE;
   od_control.od_cur_attrib = 0x1e;
   od_control.od_chat_color1 = 0x0f;
   od_control.od_chat_color2 = 0x07;
   od_control.od_chat_reason = ut_chat_reason;
   od_control.od_no_sysop = ut_no_sysop;
   od_control.od_press_key = ut_press_key;
   od_control.od_paging = ut_paging;
   od_control.od_no_response = ut_no_response;
   od_control.user_reasonforchat[0] = '\0';
   od_control.user_wantchat = FALSE;
   od_control.user_numpages = 0;
   od_control.user_ansi = FALSE;
   od_control.user_avatar = FALSE;
   od_control.od_pagestartmin = 0;
   od_control.od_pageendmin = 0;
   od_control.od_okaytopage = PAGE_USE_HOURS;
   od_control.od_page_len = 0;
   od_control.od_page_statusline = -1;
   btCurrentStatusLine = 0;
   bForceStatusUpdate = FALSE;
   bChatted = FALSE;
   pfLogWrite = NULL;
   ut_reason[0] = '\0';
   ut_localtime_fails = FALSE;
   ut_time_block.tm_hour = 2;
   ut_time_block.tm_min = 30;
   ut_init_calls = ut_entries = ut_exits = 0;
   ut_clear_calls = ut_attribute_calls = ut_display_calls = 0;
   ut_denied_calls = ut_paging_calls = ut_no_response_calls = 0;
   ut_answer_calls = ut_repeat_calls = ut_putch_calls = 0;
   ut_repeat_character = 0;
   ut_chat_on_putch = 0;
   ut_timer_starts = 0;
   ut_elapsed_count = ut_elapsed_index = 0;
   ut_log_calls = 0;
#ifdef OD_THREAD_SUPPORT
   ut_sleep_calls = 0;
   ut_shutdown_on_sleep = FALSE;
#endif
   ut_kernel_calls = 0;
#ifdef OD_PERSONALITY_SUPPORT
   ut_status_calls = 0;
   ut_status_setting = -1;
#endif
#ifdef ODPLAT_WIN32
   ut_subsystem = kODWindowsSubsystemConsole;
#endif
   ut_init_succeeds = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_page(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   utt_od_page();
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void set_reason(const char *reason)
{
   ut_copy(ut_reason, reason);
}

static void set_minute(INT minute)
{
   ut_time_block.tm_hour = minute / 60;
   ut_time_block.tm_min = minute % 60;
}

static void blank_reasons_use_each_line_style_and_stop(void)
{
   reset_page();
   bODInitialized = FALSE;
   utt_od_page();
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(1, ut_repeat_calls);
   UT_ASSERT_EQ_INT('-', ut_repeat_character);
   UT_ASSERT_EQ_INT(FALSE, od_control.user_wantchat);
   UT_ASSERT_EQ_UINT(0, ut_paging_calls);
   UT_ASSERT_EQ_INT(0x1e, ut_attributes[ut_attribute_calls - 1]);

   reset_page();
   od_control.user_ansi = TRUE;
   utt_od_page();
   UT_ASSERT_EQ_UINT(0xc4, (unsigned char)ut_repeat_character);

   reset_page();
   od_control.user_avatar = TRUE;
   utt_od_page();
   UT_ASSERT_EQ_UINT(0xc4, (unsigned char)ut_repeat_character);
}

static BOOL run_window(INT start, INT end, INT minute, INT policy)
{
   reset_page();
   set_reason("why");
   od_control.od_pagestartmin = start;
   od_control.od_pageendmin = end;
   od_control.od_okaytopage = policy;
   set_minute(minute);
   utt_od_page();
   return ut_denied_calls != 0;
}

static void daytime_windows_cover_before_inside_and_after(void)
{
   UT_ASSERT_EQ_INT(TRUE, run_window(100, 200, 50, PAGE_USE_HOURS));
   UT_ASSERT_EQ_INT(FALSE, run_window(100, 200, 150, PAGE_USE_HOURS));
   UT_ASSERT_EQ_INT(TRUE, run_window(100, 200, 250, PAGE_USE_HOURS));
}

static void overnight_windows_cover_both_allowed_sides_and_the_gap(void)
{
   UT_ASSERT_EQ_INT(FALSE, run_window(200, 100, 50, PAGE_USE_HOURS));
   UT_ASSERT_EQ_INT(TRUE, run_window(200, 100, 150, PAGE_USE_HOURS));
   UT_ASSERT_EQ_INT(FALSE, run_window(200, 100, 250, PAGE_USE_HOURS));
}

static void explicit_page_policy_overrides_or_disables_hours(void)
{
   UT_ASSERT_EQ_INT(FALSE, run_window(100, 200, 50, PAGE_ENABLE));
   UT_ASSERT_EQ_INT(TRUE, run_window(100, 100, 100, PAGE_DISABLE));
   UT_ASSERT_EQ_INT(FALSE, run_window(100, 100, 100, PAGE_USE_HOURS));
}

static void handles_a_failed_local_time_conversion(void)
{
   reset_page();
   set_reason("why");
   ut_localtime_fails = TRUE;
   utt_od_page();
   UT_ASSERT_EQ_UINT(1, ut_denied_calls);
   UT_ASSERT_EQ_UINT(0, ut_paging_calls);

   reset_page();
   set_reason("why");
   ut_localtime_fails = TRUE;
   od_control.od_okaytopage = PAGE_ENABLE;
   utt_od_page();
   UT_ASSERT_EQ_UINT(0, ut_denied_calls);
   UT_ASSERT_EQ_UINT(1, ut_paging_calls);
}

static void successful_paging_updates_hooks_status_and_timeout_message(void)
{
   reset_page();
   set_reason("why");
   pfLogWrite = ut_log_write;
#ifdef OD_PERSONALITY_SUPPORT
   od_control.od_page_statusline = 3;
   btCurrentStatusLine = 7;
#endif
   utt_od_page();
   UT_ASSERT_EQ_INT(TRUE, od_control.user_wantchat);
   UT_ASSERT_EQ_INT(TRUE, bForceStatusUpdate);
   UT_ASSERT_EQ_UINT(1, ut_log_calls);
   UT_ASSERT_EQ_UINT(1, ut_paging_calls);
   UT_ASSERT_EQ_UINT(1, od_control.user_numpages);
   UT_ASSERT_EQ_UINT(1, ut_no_response_calls);
   UT_ASSERT_EQ_UINT(1, ut_answer_calls);
   UT_ASSERT(ut_kernel_calls > 0);
#ifdef OD_PERSONALITY_SUPPORT
   UT_ASSERT_EQ_UINT(1, ut_status_calls);
   UT_ASSERT_EQ_INT(3, ut_status_setting);
#endif
}

#ifdef OD_PERSONALITY_SUPPORT
static void personality_status_line_conditions_are_independent(void)
{
   reset_page();
   set_reason("why");
   od_control.od_page_statusline = 3;
   btCurrentStatusLine = 8;
   utt_od_page();
   UT_ASSERT_EQ_UINT(0, ut_status_calls);
#ifdef ODPLAT_WIN32
   reset_page();
   set_reason("why");
   od_control.od_page_statusline = 3;
   ut_subsystem = kODWindowsSubsystemGUI;
   utt_od_page();
   UT_ASSERT_EQ_UINT(0, ut_status_calls);
#endif
}
#endif

static void chat_response_after_period_or_beep_stops_immediately(void)
{
   reset_page();
   set_reason("why");
   od_control.od_page_len = 1;
   ut_chat_on_putch = 2;
   utt_od_page();
   UT_ASSERT_EQ_UINT(1, ut_timer_starts);
   UT_ASSERT_EQ_UINT(2, ut_putch_calls);
   UT_ASSERT_EQ_UINT(0, ut_elapsed_index);
   UT_ASSERT_EQ_UINT(0, ut_no_response_calls);

   reset_page();
   set_reason("why");
   od_control.od_page_len = 1;
   ut_chat_on_putch = 3;
   utt_od_page();
   UT_ASSERT_EQ_UINT(3, ut_putch_calls);
   UT_ASSERT_EQ_UINT(0, ut_elapsed_index);
   UT_ASSERT_EQ_UINT(0, ut_no_response_calls);
}

static void unanswered_page_waits_for_the_timer_and_times_out(void)
{
   reset_page();
   set_reason("why");
   od_control.od_page_len = 1;
   ut_elapsed_values[0] = FALSE;
   ut_elapsed_values[1] = TRUE;
   ut_elapsed_count = 2;
   utt_od_page();
   UT_ASSERT_EQ_UINT(1, ut_timer_starts);
   UT_ASSERT_EQ_UINT(2, ut_elapsed_index);
   UT_ASSERT_EQ_UINT(1, ut_no_response_calls);
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT_EQ_UINT(1, ut_sleep_calls);
#else
   UT_ASSERT(ut_kernel_calls >= 2);
#endif
}

#ifdef OD_THREAD_SUPPORT
static void shutdown_during_the_timer_wait_exits_without_cleanup(void)
{
   reset_page();
   set_reason("why");
   od_control.od_page_len = 1;
   ut_elapsed_values[0] = FALSE;
   ut_elapsed_count = 1;
   ut_shutdown_on_sleep = TRUE;
   utt_od_page();
   UT_ASSERT_EQ_UINT(1, ut_sleep_calls);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(0, ut_no_response_calls);
   UT_ASSERT(ut_attributes[ut_attribute_calls - 1] != 0x1e);
}
#endif

static const UTTestCase ut_cases[] = {
   {"blank reason", blank_reasons_use_each_line_style_and_stop},
   {"daytime hours", daytime_windows_cover_before_inside_and_after},
   {"overnight hours", overnight_windows_cover_both_allowed_sides_and_the_gap},
   {"page policy", explicit_page_policy_overrides_or_disables_hours},
   {"local time failure", handles_a_failed_local_time_conversion},
   {"successful page", successful_paging_updates_hooks_status_and_timeout_message},
#ifdef OD_PERSONALITY_SUPPORT
   {"page status condition",
      personality_status_line_conditions_are_independent},
#endif
   {"answered page", chat_response_after_period_or_beep_stops_immediately},
   {"page timeout", unanswered_page_waits_for_the_timer_and_times_out},
#ifdef OD_THREAD_SUPPORT
   {"shutdown during page", shutdown_during_the_timer_wait_exits_without_cleanup},
#endif
   {"terminal session", terminal_session_is_rejected},
};
