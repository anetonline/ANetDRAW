#define UT_CUSTOM_MOCK_ODScrnGetTextInfo
#define UT_CUSTOM_MOCK_ODSessionScreenAvailable
#define UT_CUSTOM_MOCK_ODSessionScreenGetInfo
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_clr_scr
#define UT_CUSTOM_MOCK_od_disp
#define UT_CUSTOM_MOCK_od_disp_str
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_putch
#define UT_CUSTOM_MOCK_od_puttext
#define UT_CUSTOM_MOCK_od_set_attrib
#define UT_CUSTOM_MOCK_od_set_cursor

#define MOCK_CLEAR 300
#define MOCK_DISP 301
#define MOCK_DISP_STR 302
#define MOCK_ENTRY 303
#define MOCK_EXIT 304
#define MOCK_INIT 305
#define MOCK_PUTCH 306
#define MOCK_PUTTEXT 307
#define MOCK_SET_ATTRIB 308
#define MOCK_SET_CURSOR 309

static unsigned char ut_buffer[4004];
static tODScrnTextInfo ut_text_info;
static BOOL ut_session_available;
static INT ut_session_width;
static BOOL ut_puttext_result;
static INT ut_puttext_height;
static INT ut_cursor_row;
static INT ut_cursor_column;
static INT ut_attribute;
static char ut_output[1024];
static unsigned short ut_output_length;
static BOOL ut_init_succeeds;

static void reset_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
   memset(&ut_text_info, 0, sizeof(ut_text_info));
   memset(ut_buffer, 0, sizeof(ut_buffer));
   memset(ut_output, 0, sizeof(ut_output));
   ut_text_info.winleft = 1;
   ut_text_info.winright = 80;
   ut_text_info.wintop = 1;
   ut_text_info.winbottom = 3;
   ut_buffer[0] = 4;
   ut_buffer[1] = 2;
   ut_buffer[2] = 31;
   ut_buffer[3] = 3;
   ut_session_available = TRUE;
   ut_session_width = 80;
   ut_puttext_result = TRUE;
   ut_puttext_height = 0;
   ut_cursor_row = 0;
   ut_cursor_column = 0;
   ut_attribute = 0;
   ut_output_length = 0;
   bODInitialized = TRUE;
   ut_init_succeeds = TRUE;
}

static void set_character(INT row, INT column, char value)
{
   size_t offset = 4U + (size_t)(row - 1) * 160U +
      (size_t)(column - 1) * 2U;
   ut_buffer[offset] = (unsigned char)value;
   ut_buffer[offset + 1] = 7;
}

void utm_ODScrnGetTextInfo(tODScrnTextInfo *info)
{
   *info = ut_text_info;
}

BOOL utm_ODSessionScreenAvailable(void)
{
   return ut_session_available;
}

void utm_ODSessionScreenGetInfo(tODVScreenInfo *info)
{
   memset(info, 0, sizeof(*info));
   info->winleft = 1;
   info->winright = ut_session_width;
}

void utm_ODSyncAPIEntry(void) { ut_mock_called(MOCK_ENTRY); }
void utm_ODSyncAPIExit(void) { ut_mock_called(MOCK_EXIT); }
void ODCALL utm_od_clr_scr(void) { ut_mock_called(MOCK_CLEAR); }

void ODCALL utm_od_disp(const char *buffer, INT size, BOOL local_echo)
{
   UT_ASSERT_EQ_INT(2, size);
   UT_ASSERT_EQ_INT(FALSE, local_echo);
   UT_ASSERT(buffer[0] == '\r' && buffer[1] == '\n');
   ut_mock_called(MOCK_DISP);
}

void ODCALL utm_od_disp_str(const char *text)
{
   UT_ASSERT(text[0] == '\r' && text[1] == '\n' && text[2] == '\0');
   ut_mock_called(MOCK_DISP_STR);
}

void ODCALL utm_od_init(void)
{
   ut_mock_called(MOCK_INIT);
   if(ut_init_succeeds) bODInitialized = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_fixture(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_restore_screen(ut_buffer));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_ENTRY));
}

void ODCALL utm_od_putch(char value)
{
   UT_ASSERT(ut_output_length < sizeof(ut_output));
   ut_output[ut_output_length++] = value;
   ut_mock_called(MOCK_PUTCH);
}

BOOL ODCALL utm_od_puttext(INT left, INT top, INT right, INT bottom,
   void *buffer)
{
   UT_ASSERT_EQ_INT(1, left);
   UT_ASSERT_EQ_INT(1, top);
   UT_ASSERT_EQ_INT(80, right);
   UT_ASSERT_NOT_NULL(buffer);
   ut_puttext_height = bottom;
   ut_mock_called(MOCK_PUTTEXT);
   return ut_puttext_result;
}

void ODCALL utm_od_set_attrib(INT attribute)
{
   ut_attribute = attribute;
   ut_mock_called(MOCK_SET_ATTRIB);
}

void ODCALL utm_od_set_cursor(INT row, INT column)
{
   ut_cursor_row = row;
   ut_cursor_column = column;
   ut_mock_called(MOCK_SET_CURSOR);
}

static void assert_parameter_failure(void)
{
   UT_ASSERT_EQ_INT(FALSE, utt_od_restore_screen(ut_buffer));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_CLEAR));
}

static void invalid_window_and_buffer_are_rejected(void)
{
   reset_fixture();
   ut_text_info.winleft = 2;
   assert_parameter_failure();
   reset_fixture();
   ut_text_info.winright = 79;
   assert_parameter_failure();
   reset_fixture();
   UT_ASSERT_EQ_INT(FALSE, utt_od_restore_screen(NULL));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
}

static void graphics_restore_truncates_top_and_clamps_cursor(void)
{
   reset_fixture();
   bODInitialized = FALSE;
   od_control.user_avatar = TRUE;
   ut_text_info.winbottom = 2;
   ut_buffer[1] = 1;
   ut_buffer[3] = 4;

   UT_ASSERT_EQ_INT(TRUE, utt_od_restore_screen(ut_buffer));
   UT_ASSERT_EQ_INT(2, ut_puttext_height);
   UT_ASSERT_EQ_INT(1, ut_cursor_row);
   UT_ASSERT_EQ_INT(4, ut_cursor_column);
   UT_ASSERT_EQ_INT(31, ut_attribute);
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_INIT));
}

static void graphics_restore_limits_height_and_propagates_failure(void)
{
   reset_fixture();
   od_control.user_ansi = TRUE;
   ut_text_info.winbottom = 5;
   ut_buffer[1] = 5;
   ut_buffer[3] = 3;
   ut_puttext_result = FALSE;

   UT_ASSERT_EQ_INT(FALSE, utt_od_restore_screen(ut_buffer));
   UT_ASSERT_EQ_INT(3, ut_puttext_height);
   UT_ASSERT_EQ_INT(3, ut_cursor_row);
}

static void session_rows_trim_space_zero_and_nonblank_characters(void)
{
   reset_fixture();
   od_control.user_ansi = FALSE;
   od_control.user_avatar = FALSE;
   ut_session_width = 5;
   ut_buffer[0] = 4;
   ut_buffer[1] = 3;
   set_character(1, 3, 'X');
   set_character(1, 4, 0);
   set_character(1, 5, ' ');
   set_character(2, 1, 'A');
   set_character(2, 2, 'B');
   set_character(2, 4, 'D');
   set_character(3, 5, 'Z');

   UT_ASSERT_EQ_INT(TRUE, utt_od_restore_screen(ut_buffer));
   UT_ASSERT(ut_output_length > 5);
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_DISP));
   UT_ASSERT_EQ_UINT(2, ut_mock_count(MOCK_DISP_STR));
}

static void session_last_column_is_settled_by_next_character(void)
{
   reset_fixture();
   od_control.user_ansi = FALSE;
   od_control.user_avatar = FALSE;
   ut_session_width = 5;
   ut_buffer[0] = 4;
   ut_buffer[1] = 2;
   set_character(1, 5, 'X');
   set_character(2, 1, 'A');
   set_character(2, 3, 'C');

   UT_ASSERT_EQ_INT(TRUE, utt_od_restore_screen(ut_buffer));
   UT_ASSERT(ut_output_length >= 6);
   UT_ASSERT(ut_output[5] == 'A');
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_DISP_STR));
}

static void settled_last_column_handles_zero_cursor_width(void)
{
   reset_fixture();
   od_control.user_ansi = FALSE;
   od_control.user_avatar = FALSE;
   ut_session_width = 5;
   ut_buffer[0] = 1;
   ut_buffer[1] = 2;
   set_character(1, 5, 'X');
   set_character(2, 1, 'Y');

   UT_ASSERT_EQ_INT(TRUE, utt_od_restore_screen(ut_buffer));
   UT_ASSERT(ut_output_length >= 7);
   UT_ASSERT(ut_output[ut_output_length - 2] == ' ');
   UT_ASSERT(ut_output[ut_output_length - 1] == '\b');
}

static void local_short_line_uses_explicit_newline(void)
{
   reset_fixture();
   ut_session_available = FALSE;
   ut_buffer[1] = 2;
   set_character(1, 2, 'X');

   UT_ASSERT_EQ_INT(TRUE, utt_od_restore_screen(ut_buffer));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_DISP_STR));
}

static void zero_saved_height_skips_ascii_row_loop(void)
{
   reset_fixture();
   od_control.user_ansi = FALSE;
   od_control.user_avatar = FALSE;
   ut_buffer[3] = 0;

   UT_ASSERT_EQ_INT(TRUE, utt_od_restore_screen(ut_buffer));
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_PUTCH));
}

static void local_last_column_remote_advance_is_conditional(void)
{
   reset_fixture();
   ut_session_available = FALSE;
   ut_buffer[1] = 2;
   set_character(1, 80, 'X');
   od_control.baud = 0;
   UT_ASSERT_EQ_INT(TRUE, utt_od_restore_screen(ut_buffer));
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_DISP));

   reset_fixture();
   ut_session_available = FALSE;
   ut_buffer[1] = 2;
   set_character(1, 80, 'X');
   od_control.baud = 38400;
   od_control.user_screenwidth = 80;
   UT_ASSERT_EQ_INT(TRUE, utt_od_restore_screen(ut_buffer));
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_DISP));

   reset_fixture();
   ut_session_available = FALSE;
   ut_buffer[1] = 2;
   set_character(1, 80, 'X');
   od_control.baud = 38400;
   od_control.user_screenwidth = 132;
   UT_ASSERT_EQ_INT(TRUE, utt_od_restore_screen(ut_buffer));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_DISP));
}

static const UTTestCase ut_cases[] = {
   {"invalid parameters", invalid_window_and_buffer_are_rejected},
   {"graphics truncation", graphics_restore_truncates_top_and_clamps_cursor},
   {"graphics height and failure",
      graphics_restore_limits_height_and_propagates_failure},
   {"session row trimming",
      session_rows_trim_space_zero_and_nonblank_characters},
   {"session last column", session_last_column_is_settled_by_next_character},
   {"zero-width cursor line", settled_last_column_handles_zero_cursor_width},
   {"local short line", local_short_line_uses_explicit_newline},
   {"zero saved height", zero_saved_height_skips_ascii_row_loop},
   {"local last column", local_last_column_remote_advance_is_conditional},
   {"terminal session", terminal_session_is_rejected}
};
