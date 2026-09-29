#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_init

#if defined(OD_TEXTMODE) || defined(ODPLAT_WIN32)
#define UT_CUSTOM_MOCK_ODScrnCopyText
#define UT_CUSTOM_MOCK_ODScrnDisplayString
#define UT_CUSTOM_MOCK_ODScrnEnableCaret
#define UT_CUSTOM_MOCK_ODScrnEnableScrolling
#define UT_CUSTOM_MOCK_ODScrnPutText
#define UT_CUSTOM_MOCK_ODScrnSetAttribute
#define UT_CUSTOM_MOCK_ODScrnSetBoundary
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos
#define UT_CUSTOM_MOCK_ODStoreTextInfo
#endif

#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#endif

#ifdef ODPLAT_DOS32
#define UT_APPLICATION_CALLBACK ODCALL
#else
#define UT_APPLICATION_CALLBACK
#endif

static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static BOOL ut_init_succeeds;

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

#if !defined(OD_TEXTMODE) && !defined(ODPLAT_WIN32)

static void reports_status_lines_as_unsupported(void)
{
   ut_init_succeeds = TRUE;
   bODInitialized = FALSE;
   od_control.od_error = 0;
   ut_init_calls = 0;
   ut_entries = 0;
   ut_exits = 0;
   utt_od_set_statusline(1);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_INT(ERR_UNSUPPORTED, od_control.od_error);

   bODInitialized = TRUE;
   od_control.od_error = 0;
   utt_od_set_statusline(2);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(2, ut_entries);
   UT_ASSERT_EQ_UINT(2, ut_exits);
   UT_ASSERT_EQ_INT(ERR_UNSUPPORTED, od_control.od_error);
}

static void terminal_session_is_rejected(void)
{
   bODInitialized = FALSE; ut_init_succeeds = FALSE;
   od_control.od_error = 0; ut_entries = 0;
   utt_od_set_statusline(1);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static const UTTestCase ut_cases[] = {
   {"unsupported status line", reports_status_lines_as_unsupported},
   {"terminal session", terminal_session_is_rejected}
};

#else

#ifdef ODPLAT_WIN32
static tODWindowsSubsystem ut_subsystem;

tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{
   return(ut_subsystem);
}
#endif

static tODScrnTextInfo ut_stored_info;
static unsigned ut_store_calls;
static unsigned ut_boundary_calls;
static unsigned ut_copy_calls;
static unsigned ut_attribute_calls;
static unsigned ut_cursor_calls;
static unsigned ut_display_calls;
static unsigned ut_put_calls;
static unsigned ut_caret_calls;
static BOOL ut_caret_values[2];
static unsigned ut_scrolling_calls;
static BOOL ut_scrolling_values[2];
static unsigned ut_personality_calls;
static BYTE ut_personality_setting;

void utm_ODStoreTextInfo(void)
{
   ODTextInfo = ut_stored_info;
   ++ut_store_calls;
}

void utm_ODScrnSetBoundary(BYTE left, BYTE top, BYTE right, BYTE bottom)
{
   (void)left; (void)top; (void)right; (void)bottom;
   ++ut_boundary_calls;
}

BOOL utm_ODScrnCopyText(BYTE left, BYTE top, BYTE right, BYTE bottom,
   BYTE destination_column, BYTE destination_row)
{
   (void)left; (void)top; (void)right; (void)bottom;
   (void)destination_column; (void)destination_row;
   ++ut_copy_calls;
   return TRUE;
}

void ODCALL utm_ODScrnSetAttribute(BYTE attribute)
{
   (void)attribute;
   ++ut_attribute_calls;
}

void ODCALL utm_ODScrnSetCursorPos(BYTE column, BYTE row)
{
   (void)column; (void)row;
   ++ut_cursor_calls;
}

void ODCALL utm_ODScrnDisplayString(const char *text)
{
   UT_ASSERT(text != NULL);
   ++ut_display_calls;
}

BOOL ODCALL utm_ODScrnPutText(BYTE left, BYTE top, BYTE right, BYTE bottom,
   void *text)
{
   (void)left; (void)top; (void)right; (void)bottom;
   UT_ASSERT_EQ_PTR(abtBlackBlock, text);
   ++ut_put_calls;
   return TRUE;
}

void utm_ODScrnEnableCaret(BOOL enabled)
{
   UT_ASSERT(ut_caret_calls < 2);
   ut_caret_values[ut_caret_calls++] = enabled;
}

void utm_ODScrnEnableScrolling(BOOL enabled)
{
   UT_ASSERT(ut_scrolling_calls < 2);
   ut_scrolling_values[ut_scrolling_calls++] = enabled;
}

static void UT_APPLICATION_CALLBACK ut_personality(BYTE setting)
{
   ++ut_personality_calls;
   ut_personality_setting = setting;
}

static void reset_status(void)
{
#ifdef ODPLAT_WIN32
   ut_subsystem = kODWindowsSubsystemConsole;
#endif
   bODInitialized = TRUE;
   od_control.od_status_on = TRUE;
   od_control.od_update_status_now = FALSE;
   od_control.od_current_statusline = 1;
   btCurrentStatusLine = 1;
   btOutputTop = 2;
   btOutputBottom = 23;
   pfCurrentPersonality = ut_personality;
   ut_stored_info.winleft = 1;
   ut_stored_info.wintop = 2;
   ut_stored_info.winright = 80;
   ut_stored_info.winbottom = 23;
   ut_stored_info.attribute = 0x1e;
   ut_stored_info.curx = 5;
   ut_stored_info.cury = 10;
   ut_init_calls = ut_entries = ut_exits = 0;
   ut_store_calls = ut_boundary_calls = ut_copy_calls = 0;
   ut_attribute_calls = ut_cursor_calls = ut_display_calls = 0;
   ut_put_calls = ut_caret_calls = ut_scrolling_calls = 0;
   ut_personality_calls = 0;
   ut_personality_setting = 255;
   ut_init_succeeds = TRUE;
}

#ifdef ODPLAT_WIN32
static void gui_status_lines_remain_unsupported(void)
{
   reset_status();
   ut_subsystem = kODWindowsSubsystemGUI;
   od_control.od_error = ERR_NONE;
   utt_od_set_statusline(2);
   UT_ASSERT_EQ_INT(ERR_UNSUPPORTED, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_store_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}
#endif

static void terminal_session_is_rejected(void)
{
   reset_status(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   utt_od_set_statusline(1);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void disabled_and_unchanged_status_lines_return_early(void)
{
   reset_status();
   bODInitialized = FALSE;
   od_control.od_status_on = FALSE;
   utt_od_set_statusline(1);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(0, ut_store_calls);

   reset_status();
   utt_od_set_statusline(1);
   UT_ASSERT_EQ_UINT(0, ut_store_calls);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static void invalid_settings_are_normalized_to_zero(void)
{
   reset_status();
   utt_od_set_statusline(-1);
   UT_ASSERT_EQ_UINT(0, btCurrentStatusLine);
   UT_ASSERT_EQ_UINT(1, ut_personality_calls);
   UT_ASSERT_EQ_UINT(0, ut_personality_setting);

   reset_status();
   utt_od_set_statusline(9);
   UT_ASSERT_EQ_UINT(0, btCurrentStatusLine);
}

static void forced_updates_redraw_an_unchanged_status_line(void)
{
   reset_status();
   od_control.od_update_status_now = TRUE;
   utt_od_set_statusline(1);
   UT_ASSERT_EQ_UINT(1, ut_store_calls);
   UT_ASSERT_EQ_UINT(1, ut_personality_calls);
   UT_ASSERT_EQ_UINT(2, ut_caret_calls);
   UT_ASSERT_EQ_INT(FALSE, ut_caret_values[0]);
   UT_ASSERT_EQ_INT(TRUE, ut_caret_values[1]);
   UT_ASSERT_EQ_UINT(2, ut_scrolling_calls);
}

static void enabling_from_none_repairs_each_cursor_position(void)
{
   reset_status();
   btCurrentStatusLine = STATUS_NONE;
   ut_stored_info.cury = 24;
   utt_od_set_statusline(2);
   UT_ASSERT_EQ_UINT(1, ut_copy_calls);
   UT_ASSERT_EQ_UINT(22, ODTextInfo.cury);

   reset_status();
   btCurrentStatusLine = STATUS_NONE;
   ut_stored_info.cury = 1;
   utt_od_set_statusline(2);
   UT_ASSERT_EQ_UINT(1, ut_copy_calls);
   UT_ASSERT_EQ_UINT(2, ODTextInfo.cury);

   reset_status();
   btCurrentStatusLine = STATUS_NONE;
   ut_stored_info.cury = 10;
   utt_od_set_statusline(2);
   UT_ASSERT_EQ_UINT(0, ut_copy_calls);
   UT_ASSERT_EQ_UINT(10, ODTextInfo.cury);
}

static void setting_eight_clears_every_area_outside_the_output_window(void)
{
   reset_status();
   utt_od_set_statusline(8);
   UT_ASSERT_EQ_UINT(8, btCurrentStatusLine);
   UT_ASSERT_EQ_UINT(8, od_control.od_current_statusline);
   UT_ASSERT_EQ_UINT(1, ut_put_calls);
   UT_ASSERT_EQ_UINT(3, ut_display_calls);
   UT_ASSERT_EQ_UINT(2, ut_attribute_calls);
   UT_ASSERT_EQ_UINT(4, ut_cursor_calls);
   UT_ASSERT_EQ_UINT(0, ut_personality_calls);
}

static const UTTestCase ut_cases[] = {
   {"early status return", disabled_and_unchanged_status_lines_return_early},
   {"invalid status", invalid_settings_are_normalized_to_zero},
   {"forced status", forced_updates_redraw_an_unchanged_status_line},
   {"cursor repair", enabling_from_none_repairs_each_cursor_position},
   {"clear status", setting_eight_clears_every_area_outside_the_output_window},
#ifdef ODPLAT_WIN32
   {"GUI unsupported", gui_status_lines_remain_unsupported},
#endif
   {"terminal session", terminal_session_is_rejected}
};

#endif
