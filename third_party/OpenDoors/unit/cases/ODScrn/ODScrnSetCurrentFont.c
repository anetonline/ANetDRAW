#define UT_CUSTOM_MOCK_GetDC
#define UT_CUSTOM_MOCK_SaveDC
#define UT_CUSTOM_MOCK_SelectObject
#define UT_CUSTOM_MOCK_GetTextMetricsA
#define UT_CUSTOM_MOCK_RestoreDC
#define UT_CUSTOM_MOCK_ReleaseDC
#define UT_CUSTOM_MOCK_ODScrnAdjustWindows

static HWND ut_window = (HWND)1;
static HDC ut_dc = (HDC)2;
static HFONT ut_font = (HFONT)3;
static BOOL ut_dc_fails;
static unsigned ut_save_calls;
static unsigned ut_select_calls;
static unsigned ut_metrics_calls;
static unsigned ut_restore_calls;
static unsigned ut_release_calls;
static unsigned ut_adjust_calls;

HDC WINAPI utm_GetDC(HWND window)
{
   UT_ASSERT(window == ut_window);
   return ut_dc_fails ? NULL : ut_dc;
}

int WINAPI utm_SaveDC(HDC dc)
{
   ++ut_save_calls; UT_ASSERT(dc == ut_dc); return 19;
}

HGDIOBJ WINAPI utm_SelectObject(HDC dc, HGDIOBJ object)
{
   ++ut_select_calls; UT_ASSERT(dc == ut_dc); UT_ASSERT(object == ut_font);
   return NULL;
}

BOOL WINAPI utm_GetTextMetricsA(HDC dc, LPTEXTMETRICA metrics)
{
   ++ut_metrics_calls; UT_ASSERT(dc == ut_dc); UT_ASSERT_NOT_NULL(metrics);
   memset(metrics, 0, sizeof(*metrics));
   metrics->tmMaxCharWidth = 9; metrics->tmHeight = 17;
   return TRUE;
}

BOOL WINAPI utm_RestoreDC(HDC dc, int saved)
{
   ++ut_restore_calls; UT_ASSERT(dc == ut_dc); UT_ASSERT_EQ_INT(19, saved);
   return TRUE;
}

int WINAPI utm_ReleaseDC(HWND window, HDC dc)
{
   ++ut_release_calls; UT_ASSERT(window == ut_window); UT_ASSERT(dc == ut_dc);
   return 1;
}

void utm_ODScrnAdjustWindows(void)
{
   ++ut_adjust_calls;
}

static void reset_font(void)
{
   ut_dc_fails = FALSE;
   ut_save_calls = ut_select_calls = ut_metrics_calls = 0;
   ut_restore_calls = ut_release_calls = ut_adjust_calls = 0;
   hCurrentFont = NULL; nFontCellWidth = 1; nFontCellHeight = 2;
}

static void leaves_the_current_font_unchanged_without_a_device_context(void)
{
   reset_font(); ut_dc_fails = TRUE;
   utt_ODScrnSetCurrentFont(ut_window, ut_font);
   UT_ASSERT(hCurrentFont == NULL); UT_ASSERT_EQ_INT(1, nFontCellWidth);
   UT_ASSERT_EQ_INT(2, nFontCellHeight); UT_ASSERT_EQ_UINT(0, ut_save_calls);
   UT_ASSERT_EQ_UINT(0, ut_adjust_calls);
}

static void selects_the_font_updates_metrics_and_adjusts_both_windows(void)
{
   reset_font();
   utt_ODScrnSetCurrentFont(ut_window, ut_font);
   UT_ASSERT(hCurrentFont == ut_font); UT_ASSERT_EQ_INT(9, nFontCellWidth);
   UT_ASSERT_EQ_INT(17, nFontCellHeight); UT_ASSERT_EQ_UINT(1, ut_save_calls);
   UT_ASSERT_EQ_UINT(1, ut_select_calls); UT_ASSERT_EQ_UINT(1, ut_metrics_calls);
   UT_ASSERT_EQ_UINT(1, ut_restore_calls); UT_ASSERT_EQ_UINT(1, ut_release_calls);
   UT_ASSERT_EQ_UINT(2, ut_adjust_calls);
}

static const UTTestCase ut_cases[] = {
   {"no context", leaves_the_current_font_unchanged_without_a_device_context},
   {"update", selects_the_font_updates_metrics_and_adjusts_both_windows}
};
