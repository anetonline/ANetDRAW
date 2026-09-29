#define UT_CUSTOM_MOCK_ODConsoleShiftStatus
#define UT_CUSTOM_MOCK_PeekConsoleInputA
#define UT_CUSTOM_MOCK_ReadConsoleInputA

static INPUT_RECORD ut_records[12];
static unsigned ut_record_count;
static unsigned ut_record_index;
static BOOL ut_peek_succeeds;
static BOOL ut_read_succeeds;
static BOOL ut_read_is_empty;

BYTE utm_ODConsoleShiftStatus(DWORD state)
{
   UT_ASSERT_EQ_UINT(0x1234, state);
   return(0x42);
}
BOOL WINAPI utm_PeekConsoleInputA(HANDLE handle, PINPUT_RECORD record,
   DWORD length, LPDWORD read_count)
{
   (void)handle; (void)record; UT_ASSERT_EQ_UINT(1, length);
   if(!ut_peek_succeeds) return(FALSE);
   *read_count = ut_record_index < ut_record_count ? 1 : 0;
   return(TRUE);
}
BOOL WINAPI utm_ReadConsoleInputA(HANDLE handle, PINPUT_RECORD record,
   DWORD length, LPDWORD read_count)
{
   (void)handle; UT_ASSERT_EQ_UINT(1, length);
   if(!ut_read_succeeds) return(FALSE);
   if(ut_read_is_empty) { *read_count = 0; return(TRUE); }
   if(ut_record_index >= ut_record_count) { *read_count = 0; return(TRUE); }
   *record = ut_records[ut_record_index++]; *read_count = 1; return(TRUE);
}

static void reset_fixture(void)
{
   memset(ut_records, 0, sizeof(ut_records));
   ut_record_count = ut_record_index = 0;
   ut_peek_succeeds = ut_read_succeeds = TRUE;
   ut_read_is_empty = FALSE;
   bConsoleActive = TRUE; hConsoleInput = (HANDLE)(UINT_PTR)1;
   wRepeatCount = 0;
}

static void rejects_invalid_or_empty_input(void)
{
   WORD key; BYTE shift;
   reset_fixture(); bConsoleActive = FALSE;
   UT_ASSERT(!utt_ODConsoleReadKey(&key, &shift));
   reset_fixture(); UT_ASSERT(!utt_ODConsoleReadKey(NULL, &shift));
   reset_fixture(); UT_ASSERT(!utt_ODConsoleReadKey(&key, NULL));
   reset_fixture(); ut_peek_succeeds = FALSE;
   UT_ASSERT(!utt_ODConsoleReadKey(&key, &shift));
   reset_fixture(); UT_ASSERT(!utt_ODConsoleReadKey(&key, &shift));
}

static void returns_saved_key_repetitions(void)
{
   WORD key; BYTE shift;
   reset_fixture(); wRepeatedKey = 0x1e41; btRepeatedShiftStatus = 2;
   wRepeatCount = 2;
   UT_ASSERT(utt_ODConsoleReadKey(&key, &shift));
   UT_ASSERT_EQ_UINT(0x1e41, key); UT_ASSERT_EQ_UINT(2, shift);
   UT_ASSERT_EQ_UINT(1, wRepeatCount);
}

static void skips_non_keys_keyups_and_modifiers(void)
{
   static const WORD modifiers[] = {
      VK_SHIFT, VK_CONTROL, VK_MENU, VK_CAPITAL, VK_NUMLOCK, VK_SCROLL
   };
   WORD key; BYTE shift; unsigned index;
   reset_fixture();
   ut_records[0].EventType = WINDOW_BUFFER_SIZE_EVENT;
   ut_records[1].EventType = KEY_EVENT;
   ut_records[1].Event.KeyEvent.bKeyDown = FALSE;
   for(index = 0; index < sizeof(modifiers) / sizeof(modifiers[0]); ++index)
   {
      ut_records[index + 2].EventType = KEY_EVENT;
      ut_records[index + 2].Event.KeyEvent.bKeyDown = TRUE;
      ut_records[index + 2].Event.KeyEvent.wVirtualKeyCode = modifiers[index];
   }
   ut_records[8].EventType = KEY_EVENT;
   ut_records[8].Event.KeyEvent.bKeyDown = TRUE;
   ut_records[8].Event.KeyEvent.wVirtualKeyCode = 'A';
   ut_records[8].Event.KeyEvent.wVirtualScanCode = 0x1e;
   ut_records[8].Event.KeyEvent.uChar.AsciiChar = 'A';
   ut_records[8].Event.KeyEvent.dwControlKeyState = 0x1234;
   ut_records[8].Event.KeyEvent.wRepeatCount = 3;
   ut_record_count = 9;
   UT_ASSERT(utt_ODConsoleReadKey(&key, &shift));
   UT_ASSERT_EQ_UINT(0x1e41, key); UT_ASSERT_EQ_UINT(0x42, shift);
   UT_ASSERT_EQ_UINT(2, wRepeatCount);
}

static void handles_read_failure_and_zero_repeat(void)
{
   WORD key; BYTE shift;
   reset_fixture(); ut_record_count = 1; ut_read_succeeds = FALSE;
   UT_ASSERT(!utt_ODConsoleReadKey(&key, &shift));
   reset_fixture(); ut_record_count = 1; ut_read_is_empty = TRUE;
   UT_ASSERT(!utt_ODConsoleReadKey(&key, &shift));
   reset_fixture(); ut_record_count = 1;
   ut_records[0].EventType = KEY_EVENT;
   ut_records[0].Event.KeyEvent.bKeyDown = TRUE;
   ut_records[0].Event.KeyEvent.wVirtualKeyCode = 'B';
   ut_records[0].Event.KeyEvent.wVirtualScanCode = 0x30;
   ut_records[0].Event.KeyEvent.uChar.AsciiChar = 'B';
   ut_records[0].Event.KeyEvent.dwControlKeyState = 0x1234;
   ut_records[0].Event.KeyEvent.wRepeatCount = 0;
   UT_ASSERT(utt_ODConsoleReadKey(&key, &shift));
   UT_ASSERT_EQ_UINT(0, wRepeatCount);
}

static const UTTestCase ut_cases[] = {
   {"invalid and empty", rejects_invalid_or_empty_input},
   {"repeat", returns_saved_key_repetitions},
   {"event filtering", skips_non_keys_keyups_and_modifiers},
   {"read and repeat edge", handles_read_failure_and_zero_repeat}
};
