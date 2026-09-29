#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock
#define UT_CUSTOM_MOCK_ODScrnSetWinCaretPos
#define UT_CUSTOM_MOCK_SaveDC
#define UT_CUSTOM_MOCK_SetBkMode
#define UT_CUSTOM_MOCK_SelectObject
#define UT_CUSTOM_MOCK_SetTextColor
#define UT_CUSTOM_MOCK_SetBkColor
#define UT_CUSTOM_MOCK_TextOutA
#define UT_CUSTOM_MOCK_RestoreDC

static BYTE ut_screen[OD_SCREEN_WIDTH * OD_SCREEN_HEIGHT * 2];
static HDC ut_dc = (HDC)1;
static HFONT ut_font = (HFONT)2;
static unsigned ut_lock_calls;
static unsigned ut_unlock_calls;
static unsigned ut_text_calls;
static unsigned ut_text_lengths[4];
static int ut_text_x[4];
static int ut_text_y[4];
static char ut_text[4][4];
static COLORREF ut_foreground[4];
static COLORREF ut_background[4];
static unsigned ut_foreground_calls;
static unsigned ut_background_calls;
static unsigned ut_caret_calls;

void utm_ODMutexLock(tODMutex *mutex)
{
   UT_ASSERT(mutex == &ScreenPresentationMutex); ++ut_lock_calls;
}
void utm_ODMutexUnlock(tODMutex *mutex)
{
   UT_ASSERT(mutex == &ScreenPresentationMutex); ++ut_unlock_calls;
}
void utm_ODScrnSetWinCaretPos(void) { ++ut_caret_calls; }

int WINAPI utm_SaveDC(HDC dc)
{
   UT_ASSERT(dc == ut_dc); return 31;
}

int WINAPI utm_SetBkMode(HDC dc, int mode)
{
   UT_ASSERT(dc == ut_dc); UT_ASSERT_EQ_INT(OPAQUE, mode); return 0;
}

HGDIOBJ WINAPI utm_SelectObject(HDC dc, HGDIOBJ object)
{
   UT_ASSERT(dc == ut_dc); UT_ASSERT(object == ut_font); return NULL;
}

COLORREF WINAPI utm_SetTextColor(HDC dc, COLORREF color)
{
   UT_ASSERT(dc == ut_dc); UT_ASSERT(ut_foreground_calls < 4);
   ut_foreground[ut_foreground_calls++] = color; return 0;
}

COLORREF WINAPI utm_SetBkColor(HDC dc, COLORREF color)
{
   UT_ASSERT(dc == ut_dc); UT_ASSERT(ut_background_calls < 4);
   ut_background[ut_background_calls++] = color; return 0;
}

BOOL WINAPI utm_TextOutA(HDC dc, int x, int y, LPCSTR text, int length)
{
   int index;
   UT_ASSERT(dc == ut_dc); UT_ASSERT(ut_text_calls < 4);
   UT_ASSERT(length >= 0 && length < 4);
   ut_text_x[ut_text_calls] = x; ut_text_y[ut_text_calls] = y;
   ut_text_lengths[ut_text_calls] = (unsigned)length;
   for(index = 0; index < length; ++index)
      ut_text[ut_text_calls][index] = text[index];
   ut_text[ut_text_calls][length] = '\0';
   ++ut_text_calls; return TRUE;
}

BOOL WINAPI utm_RestoreDC(HDC dc, int saved)
{
   UT_ASSERT(dc == ut_dc); UT_ASSERT_EQ_INT(31, saved); return TRUE;
}

static void reset_paint(void)
{
   unsigned index;
   for(index = 0; index < sizeof(ut_screen); index += 2)
   {
      ut_screen[index] = ' '; ut_screen[index + 1] = 0;
   }
   memset(ut_text, 0, sizeof(ut_text));
   ut_lock_calls = ut_unlock_calls = ut_text_calls = 0;
   ut_foreground_calls = ut_background_calls = 0;
   pDisplayBuffer = ut_screen; hCurrentFont = ut_font;
   nFontCellWidth = 8; nFontCellHeight = 16;
   acrPCTextColors[0] = 100; acrPCTextColors[1] = 101;
   acrPCTextColors[2] = 102; acrPCTextColors[3] = 103;
   ut_caret_calls = 0;
}

static void groups_adjacent_cells_with_the_same_attribute(void)
{
   reset_paint();
   ut_screen[0] = 'A'; ut_screen[1] = 0x21;
   ut_screen[2] = 'B'; ut_screen[3] = 0x21;
   ut_screen[4] = 'C'; ut_screen[5] = 0x32;
   utt_ODScrnPaint(ut_dc, 0, 0, 2, 0);
   UT_ASSERT_EQ_UINT(1, ut_lock_calls); UT_ASSERT_EQ_UINT(1, ut_unlock_calls);
   UT_ASSERT_EQ_UINT(1, ut_caret_calls);
   UT_ASSERT_EQ_UINT(2, ut_text_calls);
   UT_ASSERT_EQ_INT(0, ut_text_x[0]); UT_ASSERT_EQ_INT(0, ut_text_y[0]);
   UT_ASSERT_EQ_UINT(2, ut_text_lengths[0]); UT_ASSERT(strcmp("AB", ut_text[0]) == 0);
   UT_ASSERT_EQ_INT(16, ut_text_x[1]); UT_ASSERT_EQ_UINT(1, ut_text_lengths[1]);
   UT_ASSERT(strcmp("C", ut_text[1]) == 0);
   UT_ASSERT_EQ_UINT(101, ut_foreground[0]); UT_ASSERT_EQ_UINT(102, ut_background[0]);
   UT_ASSERT_EQ_UINT(102, ut_foreground[1]); UT_ASSERT_EQ_UINT(103, ut_background[1]);
}

static void clips_an_oversized_rectangle_to_the_bottom_right_cell(void)
{
   unsigned offset = ((OD_SCREEN_HEIGHT - 1) * OD_SCREEN_WIDTH +
      (OD_SCREEN_WIDTH - 1)) * 2;
   reset_paint(); ut_screen[offset] = 'Z'; ut_screen[offset + 1] = 0x01;
   utt_ODScrnPaint(ut_dc, OD_SCREEN_WIDTH - 1, OD_SCREEN_HEIGHT - 1,
      OD_SCREEN_WIDTH + 20, OD_SCREEN_HEIGHT + 20);
   UT_ASSERT_EQ_UINT(1, ut_text_calls); UT_ASSERT(strcmp("Z", ut_text[0]) == 0);
   UT_ASSERT_EQ_INT((OD_SCREEN_WIDTH - 1) * 8, ut_text_x[0]);
   UT_ASSERT_EQ_INT((OD_SCREEN_HEIGHT - 1) * 16, ut_text_y[0]);
}

static const UTTestCase ut_cases[] = {
   {"attribute runs", groups_adjacent_cells_with_the_same_attribute},
   {"clip", clips_an_oversized_rectangle_to_the_bottom_right_cell}
};
