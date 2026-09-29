#define UT_CUSTOM_MOCK_ODConsoleAvailable
#define UT_CUSTOM_MOCK_ODConsoleSetSize
#define UT_CUSTOM_MOCK_ODConsoleWrite
#define UT_CUSTOM_MOCK_ODScrnConsoleCopyPersonalityRow
#define UT_CUSTOM_MOCK_ODSessionScreenCell
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_memcpy

static BYTE ut_allocation[512];
static BYTE ut_remote[12];
static BOOL ut_console_available;
static BOOL ut_allocate;
static BOOL ut_write_result;
static BOOL ut_local_composition;
static unsigned ut_personality_rows;
static unsigned ut_free_calls;
static INT ut_actual_width;
static INT ut_actual_height;
static INT ut_expected_width;
static INT ut_expected_height;
static INT ut_expected_cursor_column;
static INT ut_expected_cursor_row;
static BOOL ut_check_cells;
static BOOL ut_expect_blank_fixed_screen_overflow;

void *utm_memcpy(void *destination, const void *source, size_t size)
{
   BYTE *out = (BYTE *)destination;
   const BYTE *in = (const BYTE *)source;
   size_t index;
   for(index = 0; index < size; ++index)
      out[index] = in[index];
   return(destination);
}

BOOL utm_ODConsoleAvailable(void) { return(ut_console_available); }
void utm_ODConsoleSetSize(INT requested_width, INT requested_height,
   INT *width, INT *height)
{
   *width = ut_actual_width != 0 ? ut_actual_width : requested_width;
   *height = ut_actual_height != 0 ? ut_actual_height : requested_height;
}
void *utm_malloc(size_t size)
{
   UT_ASSERT(size <= sizeof(ut_allocation));
   return(ut_allocate ? ut_allocation : NULL);
}
void utm_free(void *memory)
{
   UT_ASSERT_EQ_PTR(ut_allocation, memory); ++ut_free_calls;
}
void utm_ODScrnConsoleCopyPersonalityRow(BYTE *destination, INT width,
   INT source_row)
{
   INT column; (void)source_row; ++ut_personality_rows;
   for(column = 0; column < width; ++column)
   { destination[column * 2] = 'P'; destination[column * 2 + 1] = 0x17; }
}
BYTE *utm_ODSessionScreenCell(INT column, INT row)
{
   return(ut_remote + (row * 3 + column) * 2);
}
BOOL utm_ODConsoleWrite(const BYTE *cells, INT width, INT height,
   INT cursor_column, INT cursor_row, BOOL cursor_on)
{
   UT_ASSERT_EQ_INT(ut_expected_width, width);
   UT_ASSERT_EQ_INT(ut_expected_height, height);
   if(ut_check_cells)
   {
      UT_ASSERT_EQ_INT('P', cells[0]);
      UT_ASSERT_EQ_INT(ut_local_composition ? 'P' : 'R', cells[6]);
      UT_ASSERT_EQ_INT('P', cells[18]);
   }
   if(ut_expect_blank_fixed_screen_overflow)
      UT_ASSERT_EQ_INT(' ', cells[(height - 1) * width * 2]);
   UT_ASSERT_EQ_INT(ut_expected_cursor_column, cursor_column);
   UT_ASSERT_EQ_INT(ut_expected_cursor_row, cursor_row);
   UT_ASSERT(cursor_on);
   return(ut_write_result);
}

static void reset_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
   memset(&SessionScreen, 0, sizeof(SessionScreen));
   memset(ut_remote, 0, sizeof(ut_remote));
   ut_remote[0] = ut_remote[6] = 'R';
   ut_console_available = ut_allocate = ut_write_result = TRUE;
   ut_local_composition = FALSE;
   ut_check_cells = TRUE;
   ut_personality_rows = ut_free_calls = 0;
   ut_actual_width = ut_actual_height = 0;
   ut_expected_width = 3; ut_expected_height = 4;
   ut_expected_cursor_column = 1; ut_expected_cursor_row = 2;
   ut_expect_blank_fixed_screen_overflow = FALSE;
   od_control.user_screenwidth = 3; od_control.user_screen_length = 2;
   btOutputTop = 2; btOutputBottom = 24;
   bSessionScreenAvailable = TRUE;
   SessionScreen.nWidth = 3; SessionScreen.nHeight = 2;
   SessionScreen.nLeft = 0; SessionScreen.nTop = 0;
   SessionScreen.nCursorColumn = 1; SessionScreen.nCursorRow = 1;
   bCaretOn = TRUE;
}

static void composes_the_fixed_local_screen_without_a_session_buffer(void)
{
   reset_fixture();
   bSessionScreenAvailable = FALSE;
   btCursorColumn = 2; btCursorRow = 3;
   ut_local_composition = TRUE;
   ut_expected_cursor_column = 2; ut_expected_cursor_row = 3;
   UT_ASSERT(utt_ODScrnPublishConsole());
   UT_ASSERT_EQ_UINT(4, ut_personality_rows);
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
}

static void leaves_rows_beyond_the_fixed_local_screen_blank(void)
{
   reset_fixture();
   bSessionScreenAvailable = FALSE;
   btOutputTop = 1; btOutputBottom = 23;
   btCursorColumn = 1; btCursorRow = 1;
   od_control.user_screen_length = 24;
   ut_actual_height = 26;
   ut_expected_height = 26;
   ut_expected_cursor_row = 1;
   ut_check_cells = FALSE;
   ut_expect_blank_fixed_screen_overflow = TRUE;
   UT_ASSERT(utt_ODScrnPublishConsole());
   UT_ASSERT_EQ_UINT(25, ut_personality_rows);
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
}

static void unavailable_or_silent_console_needs_no_write(void)
{
   reset_fixture(); ut_console_available = FALSE;
   UT_ASSERT(utt_ODScrnPublishConsole()); UT_ASSERT_EQ_UINT(0, ut_free_calls);
   reset_fixture(); od_control.od_silent_mode = TRUE;
   UT_ASSERT(utt_ODScrnPublishConsole()); UT_ASSERT_EQ_UINT(0, ut_free_calls);
}

static void composes_remote_and_personality_rows(void)
{
   reset_fixture();
   UT_ASSERT(utt_ODScrnPublishConsole());
   UT_ASSERT_EQ_UINT(2, ut_personality_rows);
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
}

static void reports_allocation_and_write_failures(void)
{
   reset_fixture(); ut_allocate = FALSE;
   UT_ASSERT(!utt_ODScrnPublishConsole()); UT_ASSERT_EQ_UINT(0, ut_free_calls);
   reset_fixture(); ut_write_result = FALSE;
   UT_ASSERT(!utt_ODScrnPublishConsole()); UT_ASSERT_EQ_UINT(1, ut_free_calls);
}

static void covers_console_dimension_boundaries(void)
{
   reset_fixture();
   btOutputTop = 0; btOutputBottom = OD_SCREEN_HEIGHT;
   ut_expected_height = 2;
   ut_expected_cursor_row = 1;
   ut_check_cells = FALSE;
   UT_ASSERT(utt_ODScrnPublishConsole());

   reset_fixture();
   ut_actual_width = 2;
   ut_expected_width = 2;
   ut_check_cells = FALSE;
   UT_ASSERT(utt_ODScrnPublishConsole());

   reset_fixture();
   bSessionScreenAvailable = FALSE;
   od_control.user_screen_length = 30;
   ut_expected_height = 32;
   ut_expected_cursor_column = 0;
   ut_expected_cursor_row = 0;
   ut_check_cells = FALSE;
   UT_ASSERT(utt_ODScrnPublishConsole());

   reset_fixture();
   ut_actual_height = 5;
   ut_expected_height = 5;
   ut_check_cells = FALSE;
   UT_ASSERT(utt_ODScrnPublishConsole());
}

static const UTTestCase ut_cases[] = {
   {"inactive presentation", unavailable_or_silent_console_needs_no_write},
   {"composition", composes_remote_and_personality_rows},
   {"local composition",
      composes_the_fixed_local_screen_without_a_session_buffer},
   {"local fixed-screen overflow",
      leaves_rows_beyond_the_fixed_local_screen_blank},
   {"failures", reports_allocation_and_write_failures},
   {"dimension boundaries", covers_console_dimension_boundaries}
};
