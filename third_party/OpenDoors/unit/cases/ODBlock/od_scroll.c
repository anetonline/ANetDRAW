#define UT_CUSTOM_MOCK_ODScrnCopyText
#define UT_CUSTOM_MOCK_ODScrnDisplayString
#define UT_CUSTOM_MOCK_ODScrnGetTextInfo
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos
#define UT_CUSTOM_MOCK_ODSessionScreenAvailable
#define UT_CUSTOM_MOCK_ODSessionScreenCopyText
#define UT_CUSTOM_MOCK_ODSessionScreenDisplayString
#define UT_CUSTOM_MOCK_ODSessionScreenGetInfo
#define UT_CUSTOM_MOCK_ODSessionScreenPresent
#define UT_CUSTOM_MOCK_ODSessionScreenSetCursorPos
#define UT_CUSTOM_MOCK_ODSizeMultiply
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_od_clr_line
#define UT_CUSTOM_MOCK_od_disp
#define UT_CUSTOM_MOCK_od_gettext
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_puttext
#define UT_CUSTOM_MOCK_od_repeat
#define UT_CUSTOM_MOCK_od_set_cursor

#define MOCK_CLEAR 340
#define MOCK_COPY 341
#define MOCK_DISPLAY 342
#define MOCK_FREE 343
#define MOCK_GET 344
#define MOCK_MALLOC 345
#define MOCK_PRESENT 346
#define MOCK_PUT 347
#define MOCK_REPEAT 348
#define MOCK_SET_CURSOR 349

static unsigned char ut_allocation[4096];
static BOOL ut_session_available;
static INT ut_width;
static INT ut_height;
static BOOL ut_size_result;
static BOOL ut_malloc_fails;
static INT ut_repeat_calls;
static INT ut_last_repeat;
static BOOL ut_init_succeeds;

static void reset_fixture(void)
{
   ut_mock_call_count = 0;
   memset(&od_control, 0, sizeof(od_control));
   memset(ut_allocation, 0, sizeof(ut_allocation));
   bODInitialized = TRUE;
   bScrollAction = TRUE;
   od_control.user_ansi = TRUE;
   ut_session_available = TRUE;
   ut_width = 300;
   ut_height = 300;
   ut_size_result = TRUE;
   ut_malloc_fails = FALSE;
   ut_repeat_calls = 0;
   ut_last_repeat = 0;
   ut_init_succeeds = TRUE;
}

BOOL utm_ODScrnCopyText(BYTE left, BYTE top, BYTE right, BYTE bottom,
   BYTE destination_column, BYTE destination_row)
{
   (void)left; (void)top; (void)right; (void)bottom;
   (void)destination_column; (void)destination_row;
   ut_mock_called(MOCK_COPY);
   return TRUE;
}

void ODCALL utm_ODScrnDisplayString(const char *text)
{
   UT_ASSERT_NOT_NULL(text);
   ut_mock_called(MOCK_DISPLAY);
}

void utm_ODScrnGetTextInfo(tODScrnTextInfo *info)
{
   memset(info, 0, sizeof(*info));
   info->winleft = 1;
   info->winright = ut_width;
   info->wintop = 1;
   info->winbottom = ut_height;
   info->curx = 9;
   info->cury = 8;
}

void ODCALL utm_ODScrnSetCursorPos(BYTE column, BYTE row)
{
   (void)column; (void)row;
   ut_mock_called(MOCK_SET_CURSOR);
}

BOOL utm_ODSessionScreenAvailable(void)
{
   return ut_session_available;
}

BOOL utm_ODSessionScreenCopyText(INT left, INT top, INT right, INT bottom,
   INT destination_column, INT destination_row)
{
   (void)left; (void)top; (void)right; (void)bottom;
   (void)destination_column; (void)destination_row;
   ut_mock_called(MOCK_COPY);
   return TRUE;
}

void utm_ODSessionScreenDisplayString(const char *text)
{
   UT_ASSERT_NOT_NULL(text);
   ut_mock_called(MOCK_DISPLAY);
}

void utm_ODSessionScreenGetInfo(tODVScreenInfo *info)
{
   memset(info, 0, sizeof(*info));
   info->winleft = 1;
   info->winright = ut_width;
   info->wintop = 1;
   info->winbottom = ut_height;
   info->curx = 9;
   info->cury = 8;
}

void utm_ODSessionScreenPresent(void) { ut_mock_called(MOCK_PRESENT); }

void utm_ODSessionScreenSetCursorPos(INT column, INT row)
{
   (void)column; (void)row;
   ut_mock_called(MOCK_SET_CURSOR);
}

int utm_ODSizeMultiply(size_t left, size_t right, size_t *result)
{
   if(!ut_size_result) return FALSE;
   *result = left * right;
   return TRUE;
}

void utm_ODSyncAPIEntry(void) { }
void utm_ODSyncAPIExit(void) { }

void utm_free(void *memory)
{
   UT_ASSERT_EQ_PTR(ut_allocation, memory);
   ut_mock_called(MOCK_FREE);
}

void *utm_malloc(size_t size)
{
   UT_ASSERT(size <= sizeof(ut_allocation));
   ut_mock_called(MOCK_MALLOC);
   return ut_malloc_fails ? NULL : ut_allocation;
}

void ODCALL utm_od_clr_line(void) { ut_mock_called(MOCK_CLEAR); }

void ODCALL utm_od_disp(const char *buffer, INT size, BOOL local_echo)
{
   UT_ASSERT_NOT_NULL(buffer);
   UT_ASSERT_EQ_INT(7, size);
   UT_ASSERT_EQ_INT(FALSE, local_echo);
}

BOOL ODCALL utm_od_gettext(INT left, INT top, INT right, INT bottom,
   void *block)
{
   (void)left; (void)top; (void)right; (void)bottom;
   UT_ASSERT_EQ_PTR(ut_allocation, block);
   ut_mock_called(MOCK_GET);
   return TRUE;
}

void ODCALL utm_od_init(void)
{
   if(ut_init_succeeds) bODInitialized = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_fixture(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_scroll(1, 1, 3, 4, 1, 0));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_COPY));
}

BOOL ODCALL utm_od_puttext(INT left, INT top, INT right, INT bottom,
   void *block)
{
   (void)left; (void)top; (void)right; (void)bottom;
   UT_ASSERT_EQ_PTR(ut_allocation, block);
   UT_ASSERT_EQ_INT(FALSE, bScrollAction);
   ut_mock_called(MOCK_PUT);
   return TRUE;
}

void ODCALL utm_od_repeat(char value, BYTE times)
{
   UT_ASSERT_EQ_INT(' ', value);
   ++ut_repeat_calls;
   ut_last_repeat = times;
   ut_mock_called(MOCK_REPEAT);
}

void ODCALL utm_od_set_cursor(INT row, INT column)
{
   (void)row; (void)column;
   ut_mock_called(MOCK_SET_CURSOR);
}

static void assert_invalid(INT left, INT top, INT right, INT bottom)
{
   reset_fixture();
   UT_ASSERT_EQ_INT(FALSE, utt_od_scroll(left, top, right, bottom, 1, 0));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
}

static void parameter_and_graphics_validation(void)
{
   assert_invalid(0, 1, 1, 1);
   assert_invalid(1, 0, 1, 1);
   assert_invalid(1, 1, 301, 1);
   assert_invalid(1, 1, 1, 301);
   assert_invalid(2, 1, 1, 1);
   assert_invalid(1, 2, 1, 1);

   reset_fixture();
   od_control.user_avatar = TRUE;
   od_control.user_ansi = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_scroll(1, 1, 256, 1, 1, 0));
   reset_fixture();
   od_control.user_avatar = TRUE;
   od_control.user_ansi = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_scroll(1, 1, 1, 256, 1, 0));
   reset_fixture();
   od_control.user_ansi = FALSE;
   od_control.user_avatar = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_scroll(1, 1, 1, 1, 1, 0));
   UT_ASSERT_EQ_INT(ERR_NOGRAPHICS, od_control.od_error);
}

static void zero_distance_returns_without_scrolling(void)
{
   reset_fixture();
   bODInitialized = FALSE;
   UT_ASSERT_EQ_INT(TRUE, utt_od_scroll(1, 1, 3, 3, 0, 0));
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_COPY));
}

static void avatar_scrolls_both_directions_and_backends(void)
{
   reset_fixture();
   od_control.user_ansi = FALSE;
   od_control.user_avatar = TRUE;
   UT_ASSERT_EQ_INT(TRUE, utt_od_scroll(1, 1, 3, 4, 1, 0));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_COPY));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_PRESENT));

   reset_fixture();
   od_control.user_ansi = FALSE;
   od_control.user_avatar = TRUE;
   ut_session_available = FALSE;
   UT_ASSERT_EQ_INT(TRUE, utt_od_scroll(1, 1, 3, 4, 1, 0));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_COPY));

   reset_fixture();
   od_control.user_ansi = FALSE;
   od_control.user_avatar = TRUE;
   ut_session_available = FALSE;
   UT_ASSERT_EQ_INT(TRUE, utt_od_scroll(1, 1, 3, 4, -1, 0));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_COPY));
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_PRESENT));
}

static void avatar_clamps_distance_and_reports_allocation_failure(void)
{
   reset_fixture();
   od_control.user_ansi = FALSE;
   od_control.user_avatar = TRUE;
   ut_malloc_fails = TRUE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_scroll(1, 1, 3, 4, 9, 0));
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);

   reset_fixture();
   od_control.user_ansi = FALSE;
   od_control.user_avatar = TRUE;
   UT_ASSERT_EQ_INT(TRUE, utt_od_scroll(1, 1, 3, 4, -9, 0));
}

static void ansi_moves_retained_text_in_both_directions(void)
{
   reset_fixture();
   UT_ASSERT_EQ_INT(TRUE, utt_od_scroll(1, 1, 3, 4, 1,
      SCROLL_NO_CLEAR));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_GET));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_PUT));
   UT_ASSERT_EQ_INT(TRUE, bScrollAction);

   reset_fixture();
   UT_ASSERT_EQ_INT(TRUE, utt_od_scroll(1, 1, 3, 4, -1,
      SCROLL_NO_CLEAR));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_GET));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_PUT));
}

static void ansi_allocation_failures_and_complete_clear(void)
{
   reset_fixture();
   ut_size_result = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_scroll(1, 1, 3, 4, 1, 0));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);

   reset_fixture();
   ut_malloc_fails = TRUE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_scroll(1, 1, 3, 4, 1, 0));
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);

   reset_fixture();
   ut_width = 3;
   UT_ASSERT_EQ_INT(TRUE, utt_od_scroll(1, 1, 3, 4, 4, 0));
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_MALLOC));
   UT_ASSERT_EQ_UINT(4, ut_mock_count(MOCK_CLEAR));
}

static void ansi_manual_clear_chunks_wide_regions(void)
{
   reset_fixture();
   ut_width = 400;
   UT_ASSERT_EQ_INT(TRUE, utt_od_scroll(1, 1, 300, 2, 2, 0));
   UT_ASSERT_EQ_INT(4, ut_repeat_calls);
   UT_ASSERT_EQ_INT(45, ut_last_repeat);
}

static const UTTestCase ut_cases[] = {
   {"validation", parameter_and_graphics_validation},
   {"zero distance", zero_distance_returns_without_scrolling},
   {"AVATAR directions", avatar_scrolls_both_directions_and_backends},
   {"AVATAR clamping", avatar_clamps_distance_and_reports_allocation_failure},
   {"ANSI directions", ansi_moves_retained_text_in_both_directions},
   {"ANSI failures and clear", ansi_allocation_failures_and_complete_clear},
   {"ANSI wide clear", ansi_manual_clear_chunks_wide_regions},
   {"terminal session", terminal_session_is_rejected}
};
