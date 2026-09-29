#define UT_CUSTOM_MOCK_ODSessionScreenIsEmulating
#define UT_CUSTOM_MOCK_ODSessionScreenDisplayChar
#define UT_CUSTOM_MOCK_ODScrnGetCursorPos
#define UT_CUSTOM_MOCK_ODScrnScrollUpAndInvalidate
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
void utm_ODSessionScreenDisplayChar(unsigned char character)
{
   ++ut_session_calls; UT_ASSERT_EQ_UINT('S', character);
}
void utm_ODScrnGetCursorPos(void) { ++ut_get_calls; }
void utm_ODScrnScrollUpAndInvalidate(void) { ++ut_scroll_calls; }
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
   btCursorColumn = 0; btCursorRow = 0; ut_emulating = FALSE;
   ut_session_calls = ut_get_calls = ut_scroll_calls = 0;
   ut_bell_calls = ut_caret_calls = 0;
#ifdef ODPLAT_WIN32
   ut_invalidate_calls = 0;
#endif
}

static void forwards_during_session_emulation(void)
{
   reset_display(); ut_emulating = TRUE;
   utt_ODScrnDisplayChar('S');
   UT_ASSERT_EQ_UINT(1, ut_session_calls); UT_ASSERT_EQ_UINT(0, ut_get_calls);
   UT_ASSERT_EQ_UINT(0, ut_caret_calls);
}

static void clamps_the_cursor_and_handles_carriage_return(void)
{
   reset_display(); btCursorColumn = 20; btCursorRow = 20;
   utt_ODScrnDisplayChar('\r');
   UT_ASSERT_EQ_UINT(0, btCursorColumn); UT_ASSERT_EQ_UINT(2, btCursorRow);
   UT_ASSERT_EQ_UINT(1, ut_get_calls); UT_ASSERT_EQ_UINT(1, ut_caret_calls);
}

static void handles_line_feed_at_and_above_the_bottom(void)
{
   reset_display(); btCursorRow = 1;
   utt_ODScrnDisplayChar('\n');
   UT_ASSERT_EQ_UINT(2, btCursorRow); UT_ASSERT_EQ_UINT(0, ut_scroll_calls);
   utt_ODScrnDisplayChar('\n');
   UT_ASSERT_EQ_UINT(2, btCursorRow); UT_ASSERT_EQ_UINT(1, ut_scroll_calls);
}

static void handles_backspace_at_and_after_the_left_edge(void)
{
   reset_display();
   utt_ODScrnDisplayChar('\b'); UT_ASSERT_EQ_UINT(0, btCursorColumn);
   btCursorColumn = 2;
   utt_ODScrnDisplayChar('\b'); UT_ASSERT_EQ_UINT(1, btCursorColumn);
}

static void handles_tabs_without_wrap_with_wrap_and_with_scroll(void)
{
   reset_display();
   utt_ODScrnDisplayChar('\t');
   UT_ASSERT_EQ_UINT(8, btCursorColumn); UT_ASSERT_EQ_UINT(0, btCursorRow);
   utt_ODScrnDisplayChar('\t');
   UT_ASSERT_EQ_UINT(0, btCursorColumn); UT_ASSERT_EQ_UINT(1, btCursorRow);
   btCursorColumn = 8; btCursorRow = 2;
   utt_ODScrnDisplayChar('\t');
   UT_ASSERT_EQ_UINT(0, btCursorColumn); UT_ASSERT_EQ_UINT(2, btCursorRow);
   UT_ASSERT_EQ_UINT(1, ut_scroll_calls);
}

static void rings_the_bell(void)
{
   reset_display(); utt_ODScrnDisplayChar('\a');
   UT_ASSERT_EQ_UINT(1, ut_bell_calls); UT_ASSERT_EQ_UINT(1, ut_caret_calls);
}

static void writes_advances_wraps_and_scrolls_printable_characters(void)
{
   reset_display(); btCursorColumn = 1; btCursorRow = 1;
   utt_ODScrnDisplayChar('A');
   UT_ASSERT_EQ_UINT('A', ut_screen[324]); UT_ASSERT_EQ_UINT(0x2e, ut_screen[325]);
   UT_ASSERT_EQ_UINT(2, btCursorColumn); UT_ASSERT_EQ_UINT(1, btCursorRow);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(1, ut_invalidate_calls);
   UT_ASSERT_EQ_UINT(2, ut_invalid_left); UT_ASSERT_EQ_UINT(2, ut_invalid_top);
   UT_ASSERT_EQ_UINT(2, ut_invalid_right); UT_ASSERT_EQ_UINT(2, ut_invalid_bottom);
#endif

   btCursorColumn = 9; btCursorRow = 1;
   utt_ODScrnDisplayChar('B');
   UT_ASSERT_EQ_UINT('B', ut_screen[340]);
   UT_ASSERT_EQ_UINT(0, btCursorColumn); UT_ASSERT_EQ_UINT(2, btCursorRow);
   UT_ASSERT_EQ_UINT(0, ut_scroll_calls);

   btCursorColumn = 9; btCursorRow = 2;
   utt_ODScrnDisplayChar('C');
   UT_ASSERT_EQ_UINT('C', ut_screen[500]);
   UT_ASSERT_EQ_UINT(0, btCursorColumn); UT_ASSERT_EQ_UINT(2, btCursorRow);
   UT_ASSERT_EQ_UINT(1, ut_scroll_calls);
}

static const UTTestCase ut_cases[] = {
   {"session forwarding", forwards_during_session_emulation},
   {"cursor clamp and return", clamps_the_cursor_and_handles_carriage_return},
   {"line feed", handles_line_feed_at_and_above_the_bottom},
   {"backspace", handles_backspace_at_and_after_the_left_edge},
   {"tab", handles_tabs_without_wrap_with_wrap_and_with_scroll},
   {"bell", rings_the_bell},
   {"printable", writes_advances_wraps_and_scrolls_printable_characters}
};
