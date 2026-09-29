#define UT_CUSTOM_MOCK_SetConsoleCursorInfo
#define UT_CUSTOM_MOCK_SetConsoleCursorPosition
#define UT_CUSTOM_MOCK_WriteConsoleOutputA
#define UT_CUSTOM_MOCK_realloc

static CHAR_INFO ut_cells[4];
static BOOL ut_realloc_succeeds;
static BOOL ut_write_succeeds;
static unsigned ut_cursor_position_calls;
static BOOL ut_cursor_visible;

void *utm_realloc(void *memory, size_t size)
{
   (void)memory; UT_ASSERT_EQ_UINT(sizeof(ut_cells), size);
   return(ut_realloc_succeeds ? ut_cells : NULL);
}
BOOL WINAPI utm_WriteConsoleOutputA(HANDLE handle,
   const CHAR_INFO *buffer, COORD size, COORD origin, PSMALL_RECT region)
{
   (void)handle; (void)origin; (void)region;
   UT_ASSERT_EQ_INT(2, size.X); UT_ASSERT_EQ_INT(2, size.Y);
   UT_ASSERT_EQ_INT('A', buffer[0].Char.AsciiChar);
   UT_ASSERT_EQ_UINT(0x1e, buffer[0].Attributes);
   return(ut_write_succeeds);
}
BOOL WINAPI utm_SetConsoleCursorInfo(HANDLE handle,
   const CONSOLE_CURSOR_INFO *info)
{
   (void)handle; ut_cursor_visible = info->bVisible; return(TRUE);
}
BOOL WINAPI utm_SetConsoleCursorPosition(HANDLE handle, COORD position)
{
   (void)handle; UT_ASSERT_EQ_INT(1, position.X); UT_ASSERT_EQ_INT(1, position.Y);
   ++ut_cursor_position_calls; return(TRUE);
}

static void reset_fixture(void)
{
   bConsoleActive = TRUE; hConsoleOutput = (HANDLE)(UINT_PTR)2;
   pConsoleCells = NULL; nConsoleCellCapacity = 0;
   SavedCursorInfo.dwSize = 25;
   ut_realloc_succeeds = ut_write_succeeds = TRUE;
   ut_cursor_position_calls = 0; ut_cursor_visible = FALSE;
}

static void rejects_each_invalid_input(void)
{
   BYTE cells[8] = {'A', 0x1e};
   reset_fixture(); bConsoleActive = FALSE;
   UT_ASSERT(!utt_ODConsoleWrite(cells, 2, 2, 0, 0, TRUE));
   reset_fixture(); UT_ASSERT(!utt_ODConsoleWrite(NULL, 2, 2, 0, 0, TRUE));
   reset_fixture(); UT_ASSERT(!utt_ODConsoleWrite(cells, 0, 2, 0, 0, TRUE));
   reset_fixture(); UT_ASSERT(!utt_ODConsoleWrite(cells, 2, 0, 0, 0, TRUE));
}

static void allocates_converts_and_positions_cursor(void)
{
   BYTE cells[8] = {'A', 0x1e, 'B', 0x2f, 'C', 0x30, 'D', 0x41};
   reset_fixture();
   UT_ASSERT(utt_ODConsoleWrite(cells, 2, 2, 1, 1, TRUE));
   UT_ASSERT_EQ_UINT(4, nConsoleCellCapacity);
   UT_ASSERT(ut_cursor_visible);
   UT_ASSERT_EQ_UINT(1, ut_cursor_position_calls);
   UT_ASSERT_EQ_INT('D', pConsoleCells[3].Char.AsciiChar);
}

static void handles_allocation_and_output_failure(void)
{
   BYTE cells[8] = {'A', 0x1e};
   reset_fixture(); ut_realloc_succeeds = FALSE;
   UT_ASSERT(!utt_ODConsoleWrite(cells, 2, 2, 0, 0, TRUE));
   reset_fixture(); ut_write_succeeds = FALSE;
   UT_ASSERT(!utt_ODConsoleWrite(cells, 2, 2, 0, 0, TRUE));
}

static void reuses_storage_and_ignores_offscreen_cursor(void)
{
   BYTE cells[8] = {'A', 0x1e};
   reset_fixture(); pConsoleCells = ut_cells; nConsoleCellCapacity = 4;
   UT_ASSERT(utt_ODConsoleWrite(cells, 2, 2, -1, 1, FALSE));
   UT_ASSERT(!ut_cursor_visible); UT_ASSERT_EQ_UINT(0, ut_cursor_position_calls);
   UT_ASSERT(utt_ODConsoleWrite(cells, 2, 2, 1, -1, FALSE));
   UT_ASSERT(utt_ODConsoleWrite(cells, 2, 2, 2, 1, FALSE));
   UT_ASSERT(utt_ODConsoleWrite(cells, 2, 2, 1, 2, FALSE));
}

static const UTTestCase ut_cases[] = {
   {"invalid input", rejects_each_invalid_input},
   {"write", allocates_converts_and_positions_cursor},
   {"failures", handles_allocation_and_output_failure},
   {"cursor bounds", reuses_storage_and_ignores_offscreen_cursor}
};
