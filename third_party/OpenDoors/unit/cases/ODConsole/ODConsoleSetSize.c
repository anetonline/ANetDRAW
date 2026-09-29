#define UT_CUSTOM_MOCK_GetConsoleScreenBufferInfo
#define UT_CUSTOM_MOCK_GetLargestConsoleWindowSize
#define UT_CUSTOM_MOCK_SetConsoleScreenBufferSize
#define UT_CUSTOM_MOCK_SetConsoleWindowInfo

static COORD ut_largest;
static CONSOLE_SCREEN_BUFFER_INFO ut_info;
static unsigned ut_info_calls;
static unsigned ut_window_calls;
static unsigned ut_buffer_calls;
static BOOL ut_buffer_result;
static BOOL ut_window_result;
static BOOL ut_first_info_result;
static BOOL ut_second_info_result;
static COORD ut_buffer_size;
static COORD ut_buffer_sizes[2];
static SMALL_RECT ut_window;
static SMALL_RECT ut_windows[2];

COORD WINAPI utm_GetLargestConsoleWindowSize(HANDLE handle)
{ (void)handle; return(ut_largest); }
BOOL WINAPI utm_GetConsoleScreenBufferInfo(HANDLE handle,
   PCONSOLE_SCREEN_BUFFER_INFO info)
{
   BOOL result;
   (void)handle;
   result = ut_info_calls == 0 ? ut_first_info_result
      : ut_second_info_result;
   ++ut_info_calls;
   if(result) *info = ut_info;
   return(result);
}
BOOL WINAPI utm_SetConsoleScreenBufferSize(HANDLE handle, COORD size)
{
   (void)handle;
   UT_ASSERT(ut_buffer_calls < 2);
   UT_ASSERT(size.X > 0 && size.Y > 0);
   ut_buffer_sizes[ut_buffer_calls] = size;
   ++ut_buffer_calls;
   ut_buffer_size = size;
   if(ut_buffer_result)
      ut_info.dwSize = size;
   return(ut_buffer_result);
}
BOOL WINAPI utm_SetConsoleWindowInfo(HANDLE handle, BOOL absolute,
   const SMALL_RECT *window)
{
   (void)handle; UT_ASSERT(absolute); UT_ASSERT_NOT_NULL(window);
   UT_ASSERT(ut_window_calls < 2);
   ut_windows[ut_window_calls] = *window;
   ++ut_window_calls; ut_window = *window; return(ut_window_result);
}

static void reset_fixture(void)
{
   memset(&ut_info, 0, sizeof(ut_info));
   ut_info.dwSize.X = 80; ut_info.dwSize.Y = 25;
   ut_info.srWindow.Right = 79; ut_info.srWindow.Bottom = 24;
   ut_largest.X = 120; ut_largest.Y = 60;
   memset(&ut_buffer_size, 0, sizeof(ut_buffer_size));
   memset(ut_buffer_sizes, 0, sizeof(ut_buffer_sizes));
   memset(&ut_window, 0, sizeof(ut_window));
   memset(ut_windows, 0, sizeof(ut_windows));
   ut_info_calls = ut_window_calls = ut_buffer_calls = 0;
   ut_buffer_result = ut_window_result = TRUE;
   ut_first_info_result = ut_second_info_result = TRUE;
   hConsoleOutput = (HANDLE)(UINT_PTR)2;
   bConsoleActive = TRUE;
}

static void inactive_console_preserves_requested_minimum(void)
{
   INT width, height;
   reset_fixture(); bConsoleActive = FALSE;
   utt_ODConsoleSetSize(0, -1, &width, &height);
   UT_ASSERT_EQ_INT(1, width); UT_ASSERT_EQ_INT(1, height);
   UT_ASSERT_EQ_UINT(0, ut_buffer_calls);
}

static void preserves_the_buffer_size_and_clamps_the_visible_window(void)
{
   INT width, height;
   reset_fixture();
   utt_ODConsoleSetSize(200, 100, &width, &height);
   UT_ASSERT_EQ_INT(200, width); UT_ASSERT_EQ_INT(100, height);
   UT_ASSERT_EQ_INT(200, ut_buffer_size.X);
   UT_ASSERT_EQ_INT(100, ut_buffer_size.Y);
   UT_ASSERT_EQ_INT(119, ut_window.Right);
   UT_ASSERT_EQ_INT(59, ut_window.Bottom);
   UT_ASSERT_EQ_UINT(1, ut_window_calls);
   UT_ASSERT_EQ_UINT(1, ut_buffer_calls);

   reset_fixture(); ut_largest.X = ut_largest.Y = 0;
   utt_ODConsoleSetSize(40000, 40000, &width, &height);
   UT_ASSERT_EQ_INT(SHRT_MAX, width); UT_ASSERT_EQ_INT(SHRT_MAX, height);
   UT_ASSERT_EQ_INT(SHRT_MAX, ut_buffer_size.X);
   UT_ASSERT_EQ_INT(SHRT_MAX, ut_buffer_size.Y);
}

static void avoids_unneeded_shrink(void)
{
   INT width, height;
   reset_fixture();
   utt_ODConsoleSetSize(100, 30, &width, &height);
   UT_ASSERT_EQ_UINT(1, ut_window_calls);
   UT_ASSERT_EQ_INT(100, width); UT_ASSERT_EQ_INT(30, height);

   reset_fixture();
   utt_ODConsoleSetSize(80, 25, &width, &height);
   UT_ASSERT_EQ_UINT(0, ut_buffer_calls);
   UT_ASSERT_EQ_UINT(1, ut_window_calls);
}

static void expands_before_a_mixed_resize_then_trims_the_buffer(void)
{
   INT width, height;
   reset_fixture();
   ut_info.dwSize.X = 120;
   ut_info.dwSize.Y = 25;
   ut_info.srWindow.Right = 119;
   ut_info.srWindow.Bottom = 24;
   utt_ODConsoleSetSize(100, 32, &width, &height);
   UT_ASSERT_EQ_UINT(2, ut_buffer_calls);
   UT_ASSERT_EQ_INT(120, ut_buffer_sizes[0].X);
   UT_ASSERT_EQ_INT(32, ut_buffer_sizes[0].Y);
   UT_ASSERT_EQ_UINT(1, ut_window_calls);
   UT_ASSERT_EQ_INT(99, ut_window.Right);
   UT_ASSERT_EQ_INT(31, ut_window.Bottom);
   UT_ASSERT_EQ_INT(100, ut_buffer_sizes[1].X);
   UT_ASSERT_EQ_INT(32, ut_buffer_sizes[1].Y);
   UT_ASSERT_EQ_INT(100, width);
   UT_ASSERT_EQ_INT(32, height);
}

static void reports_current_size_after_each_resize_failure(void)
{
   INT width, height;
   reset_fixture(); ut_buffer_result = FALSE;
   utt_ODConsoleSetSize(100, 30, &width, &height);
   UT_ASSERT_EQ_INT(80, width); UT_ASSERT_EQ_INT(25, height);
   reset_fixture(); ut_window_result = FALSE;
   utt_ODConsoleSetSize(100, 30, &width, &height);
   UT_ASSERT_EQ_INT(100, width); UT_ASSERT_EQ_INT(30, height);
}

static void handles_console_queries_and_each_shrink_dimension(void)
{
   INT width, height;
   reset_fixture(); ut_first_info_result = FALSE;
   utt_ODConsoleSetSize(100, 30, &width, &height);
   UT_ASSERT_EQ_UINT(1, ut_info_calls);

   reset_fixture();
   utt_ODConsoleSetSize(40, 30, &width, &height);
   UT_ASSERT_EQ_UINT(1, ut_window_calls);
   UT_ASSERT_EQ_INT(39, ut_windows[0].Right);
   UT_ASSERT_EQ_INT(29, ut_windows[0].Bottom);
   reset_fixture();
   utt_ODConsoleSetSize(100, 10, &width, &height);
   UT_ASSERT_EQ_UINT(1, ut_window_calls);
   UT_ASSERT_EQ_INT(99, ut_windows[0].Right);
   UT_ASSERT_EQ_INT(9, ut_windows[0].Bottom);

   reset_fixture(); ut_buffer_result = FALSE;
   ut_second_info_result = FALSE;
   utt_ODConsoleSetSize(100, 30, &width, &height);
   UT_ASSERT_EQ_UINT(2, ut_info_calls);

   reset_fixture(); ut_first_info_result = FALSE;
   ut_buffer_result = FALSE;
   utt_ODConsoleSetSize(100, 30, &width, &height);
   UT_ASSERT_EQ_UINT(2, ut_info_calls);
}

static const UTTestCase ut_cases[] = {
   {"inactive", inactive_console_preserves_requested_minimum},
   {"window clamping",
      preserves_the_buffer_size_and_clamps_the_visible_window},
   {"no shrink", avoids_unneeded_shrink},
   {"mixed resize", expands_before_a_mixed_resize_then_trims_the_buffer},
   {"resize failures", reports_current_size_after_each_resize_failure},
   {"console queries", handles_console_queries_and_each_shrink_dimension}
};
