#define UT_CUSTOM_MOCK_ODSessionScreenIsEmulating
#define UT_CUSTOM_MOCK_ODSessionScreenDisplayBuffer
#define UT_CUSTOM_MOCK_ODScrnGetCursorPos
#define UT_CUSTOM_MOCK_ODScrnScrollUpOneLine
#define UT_CUSTOM_MOCK_ODScrnRingBell
#define UT_CUSTOM_MOCK_ODScrnUpdateCaretPos
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODScrnInvalidate
#endif

static BYTE ut_screen[4000];
static BOOL ut_emulating;
static unsigned ut_session_calls;
static unsigned ut_get_calls;
static unsigned ut_scroll_calls;
static unsigned ut_bell_calls;
static unsigned ut_caret_calls;
#ifdef ODPLAT_WIN32
static unsigned ut_invalidate_calls;
static BYTE ut_invalid_left;
static BYTE ut_invalid_top;
static BYTE ut_invalid_right;
static BYTE ut_invalid_bottom;
#endif

BOOL utm_ODSessionScreenIsEmulating(void) { return ut_emulating; }
void utm_ODSessionScreenDisplayBuffer(const char *buffer, INT count)
{
   ++ut_session_calls; UT_ASSERT(strcmp(buffer, "session") == 0);
   UT_ASSERT_EQ_INT(7, count);
}
void utm_ODScrnGetCursorPos(void) { ++ut_get_calls; }
void utm_ODScrnScrollUpOneLine(void) { ++ut_scroll_calls; }
void utm_ODScrnRingBell(void) { ++ut_bell_calls; }
void utm_ODScrnUpdateCaretPos(void) { ++ut_caret_calls; }
#ifdef ODPLAT_WIN32
void utm_ODScrnInvalidate(BYTE left, BYTE top, BYTE right, BYTE bottom)
{
   ++ut_invalidate_calls;
   ut_invalid_left = left; ut_invalid_top = top;
   ut_invalid_right = right; ut_invalid_bottom = bottom;
}
#endif

static void reset_display(void)
{
   unsigned index;
   for(index = 0; index < sizeof(ut_screen); ++index) ut_screen[index] = 0x55;
   pScrnBuffer = ut_screen; btLeftBoundary = 1; btTopBoundary = 1;
   btRightBoundary = 10; btBottomBoundary = 3; btCurrentAttribute = 0x2e;
   btCursorColumn = 0; btCursorRow = 0; bScrollEnabled = TRUE;
   od_control.od_silent_mode = FALSE; ut_emulating = FALSE;
   ut_session_calls = ut_get_calls = ut_scroll_calls = 0;
   ut_bell_calls = ut_caret_calls = 0;
#ifdef ODPLAT_WIN32
   ut_invalidate_calls = 0;
#endif
}

static void forwards_the_exact_buffer_during_session_emulation(void)
{
   reset_display(); ut_emulating = TRUE;
   utt_ODScrnDisplayBuffer("session", 7);
   UT_ASSERT_EQ_UINT(1, ut_session_calls); UT_ASSERT_EQ_UINT(0, ut_get_calls);
}

static void accepts_an_empty_buffer_and_clamps_each_cursor_edge(void)
{
   reset_display();
   utt_ODScrnDisplayBuffer("", 0);
   UT_ASSERT_EQ_UINT(1, ut_get_calls); UT_ASSERT_EQ_UINT(1, ut_caret_calls);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(0, ut_invalidate_calls);
#endif
   btCursorColumn = 20; btCursorRow = 20;
   utt_ODScrnDisplayBuffer("\r", 1);
   UT_ASSERT_EQ_UINT(0, btCursorColumn); UT_ASSERT_EQ_UINT(2, btCursorRow);
}

static void handles_carriage_return_and_both_backspace_edges(void)
{
   reset_display(); btCursorColumn = 2; btCursorRow = 1;
   utt_ODScrnDisplayBuffer("\b\b\b", 3);
   UT_ASSERT_EQ_UINT(0, btCursorColumn); UT_ASSERT_EQ_UINT(1, btCursorRow);
   btCursorColumn = 4;
   utt_ODScrnDisplayBuffer("\r", 1);
   UT_ASSERT_EQ_UINT(0, btCursorColumn);
}

static void handles_line_feed_with_advance_disabled_scroll_and_enabled_scroll(void)
{
   reset_display(); btCursorRow = 1;
   utt_ODScrnDisplayBuffer("\n", 1);
   UT_ASSERT_EQ_UINT(2, btCursorRow); UT_ASSERT_EQ_UINT(0, ut_scroll_calls);
   bScrollEnabled = FALSE;
   utt_ODScrnDisplayBuffer("\n", 1);
   UT_ASSERT_EQ_UINT(2, btCursorRow); UT_ASSERT_EQ_UINT(0, ut_scroll_calls);
   bScrollEnabled = TRUE;
   utt_ODScrnDisplayBuffer("\n", 1);
   UT_ASSERT_EQ_UINT(1, ut_scroll_calls);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(1, ut_invalidate_calls);
   UT_ASSERT_EQ_UINT(1, ut_invalid_left); UT_ASSERT_EQ_UINT(1, ut_invalid_top);
   UT_ASSERT_EQ_UINT(10, ut_invalid_right); UT_ASSERT_EQ_UINT(3, ut_invalid_bottom);
#endif
}

static void rings_when_audible_and_advances_past_a_silent_bell(void)
{
   reset_display();
   utt_ODScrnDisplayBuffer("\a", 1);
   UT_ASSERT_EQ_UINT(1, ut_bell_calls);
   reset_display(); od_control.od_silent_mode = TRUE;
   utt_ODScrnDisplayBuffer("\aX", 2);
   UT_ASSERT_EQ_UINT(0, ut_bell_calls); UT_ASSERT_EQ_UINT('X', ut_screen[162]);
   UT_ASSERT_EQ_UINT(1, btCursorColumn);
}

static void handles_tab_without_wrap_with_wrap_and_at_the_bottom(void)
{
   reset_display();
   utt_ODScrnDisplayBuffer("\tX", 2);
   UT_ASSERT_EQ_UINT(9, btCursorColumn); UT_ASSERT_EQ_UINT(0, btCursorRow);
   UT_ASSERT_EQ_UINT('X', ut_screen[178]);
   btCursorColumn = 8;
   utt_ODScrnDisplayBuffer("\t", 1);
   UT_ASSERT_EQ_UINT(0, btCursorColumn); UT_ASSERT_EQ_UINT(1, btCursorRow);
   btCursorColumn = 8; btCursorRow = 2; bScrollEnabled = FALSE;
   utt_ODScrnDisplayBuffer("\t", 1);
   UT_ASSERT_EQ_UINT(0, btCursorColumn); UT_ASSERT_EQ_UINT(2, btCursorRow);
   UT_ASSERT_EQ_UINT(0, ut_scroll_calls);
   btCursorColumn = 8; bScrollEnabled = TRUE;
   utt_ODScrnDisplayBuffer("\t", 1);
   UT_ASSERT_EQ_UINT(1, ut_scroll_calls);
}

static void expands_the_changed_rectangle_left_right_and_down(void)
{
   reset_display(); btCursorColumn = 2; btCursorRow = 1;
   utt_ODScrnDisplayBuffer("A\rB", 3);
   UT_ASSERT_EQ_UINT('A', ut_screen[326]); UT_ASSERT_EQ_UINT('B', ut_screen[322]);
   UT_ASSERT_EQ_UINT(1, btCursorColumn);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(1, ut_invalid_left); UT_ASSERT_EQ_UINT(2, ut_invalid_top);
   UT_ASSERT_EQ_UINT(3, ut_invalid_right); UT_ASSERT_EQ_UINT(2, ut_invalid_bottom);
#endif

   reset_display(); btCursorColumn = 1; btCursorRow = 0;
   utt_ODScrnDisplayBuffer("AB\nC", 4);
   UT_ASSERT_EQ_UINT('A', ut_screen[164]); UT_ASSERT_EQ_UINT('B', ut_screen[166]);
   UT_ASSERT_EQ_UINT('C', ut_screen[328]);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(2, ut_invalid_left); UT_ASSERT_EQ_UINT(1, ut_invalid_top);
   UT_ASSERT_EQ_UINT(4, ut_invalid_right); UT_ASSERT_EQ_UINT(2, ut_invalid_bottom);
#endif
}

static void wraps_printable_text_with_and_without_bottom_scroll(void)
{
   reset_display(); btCursorColumn = 9; btCursorRow = 1;
   utt_ODScrnDisplayBuffer("A", 1);
   UT_ASSERT_EQ_UINT(0, btCursorColumn); UT_ASSERT_EQ_UINT(2, btCursorRow);
   UT_ASSERT_EQ_UINT(0, ut_scroll_calls);
   btCursorColumn = 9; bScrollEnabled = FALSE;
   utt_ODScrnDisplayBuffer("B", 1);
   UT_ASSERT_EQ_UINT(0, btCursorColumn); UT_ASSERT_EQ_UINT(0, ut_scroll_calls);
   btCursorColumn = 9; bScrollEnabled = TRUE;
   utt_ODScrnDisplayBuffer("C", 1);
   UT_ASSERT_EQ_UINT(1, ut_scroll_calls);
}

static const UTTestCase ut_cases[] = {
   {"session forwarding", forwards_the_exact_buffer_during_session_emulation},
   {"empty and clamp", accepts_an_empty_buffer_and_clamps_each_cursor_edge},
   {"return and backspace", handles_carriage_return_and_both_backspace_edges},
   {"line feed", handles_line_feed_with_advance_disabled_scroll_and_enabled_scroll},
   {"bell", rings_when_audible_and_advances_past_a_silent_bell},
   {"tab", handles_tab_without_wrap_with_wrap_and_at_the_bottom},
   {"invalidation bounds", expands_the_changed_rectangle_left_right_and_down},
   {"printable wrap", wraps_printable_text_with_and_without_bottom_scroll}
};
