#define UT_CUSTOM_MOCK_ODEditRecommendFullRedraw
#define UT_CUSTOM_MOCK_od_scroll
#define UT_CUSTOM_MOCK_ODScrnEnableCaret
#define UT_CUSTOM_MOCK_ODEditDrawAreaLine
#define UT_CUSTOM_MOCK_ODEditRedrawArea

static BOOL ut_recommend_full;
static unsigned ut_recommend_calls;
static UINT ut_recommend_bytes;
static unsigned ut_scroll_calls;
static INT ut_scroll_distance;
static WORD ut_scroll_flags;
static BOOL ut_caret_states[2];
static unsigned ut_caret_calls;
static UINT ut_draw_lines[4];
static unsigned ut_draw_calls;
static unsigned ut_redraw_calls;

BOOL utm_ODEditRecommendFullRedraw(tEditInstance *instance, UINT bytes,
   BOOL default_value)
{
   ++ut_recommend_calls;
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT_EQ_INT(TRUE, default_value);
   ut_recommend_bytes = bytes;
   return(ut_recommend_full);
}

BOOL ODCALL utm_od_scroll(INT left, INT top, INT right, INT bottom,
   INT distance, WORD flags)
{
   ++ut_scroll_calls;
   UT_ASSERT_EQ_INT(2, left);
   UT_ASSERT_EQ_INT(3, top);
   UT_ASSERT_EQ_INT(20, right);
   UT_ASSERT_EQ_INT(8, bottom);
   ut_scroll_distance = distance;
   ut_scroll_flags = flags;
   return(TRUE);
}

void utm_ODScrnEnableCaret(BOOL enabled)
{
   UT_ASSERT(ut_caret_calls < 2);
   ut_caret_states[ut_caret_calls++] = enabled;
}

void utm_ODEditDrawAreaLine(tEditInstance *instance, UINT line)
{
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT(ut_draw_calls < 4);
   ut_draw_lines[ut_draw_calls++] = line;
}

void utm_ODEditRedrawArea(tEditInstance *instance)
{
   ++ut_redraw_calls;
   UT_ASSERT_NOT_NULL(instance);
}

static void reset_scroll(tEditInstance *instance, tODEditOptions *options)
{
   instance->pUserOptions = options;
   instance->unAreaWidth = 18;
   instance->unAreaHeight = 6;
   instance->unLineScrolledToTop = 5;
   instance->unLinesInBuffer = 20;
   options->nAreaLeft = 2;
   options->nAreaTop = 3;
   options->nAreaRight = 20;
   options->nAreaBottom = 8;
   ut_recommend_full = FALSE;
   ut_recommend_calls = 0;
   ut_recommend_bytes = 0;
   ut_scroll_calls = 0;
   ut_scroll_distance = 0;
   ut_scroll_flags = 0;
   ut_caret_calls = 0;
   ut_draw_calls = 0;
   ut_redraw_calls = 0;
}

static void accepts_a_zero_distance_without_work(void)
{
   tEditInstance instance;
   tODEditOptions options;
   reset_scroll(&instance, &options);
   od_control.user_avatar = FALSE;
   od_control.baud = 9600;
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditScrollArea(&instance, 0));
   UT_ASSERT_EQ_UINT(5, instance.unLineScrolledToTop);
   UT_ASSERT_EQ_UINT(0, ut_recommend_calls);
   UT_ASSERT_EQ_UINT(0, ut_scroll_calls);
   UT_ASSERT_EQ_UINT(0, ut_redraw_calls);
}

static void fully_redraws_remote_positive_and_negative_scrolls(void)
{
   tEditInstance instance;
   tODEditOptions options;

   reset_scroll(&instance, &options);
   od_control.user_avatar = FALSE;
   od_control.baud = 9600;
   UT_ASSERT_EQ_INT(FALSE, utt_ODEditScrollArea(&instance, 2));
   UT_ASSERT_EQ_UINT(7, instance.unLineScrolledToTop);
   UT_ASSERT_EQ_UINT(1, ut_redraw_calls);

   reset_scroll(&instance, &options);
   instance.unAreaHeight = 3;
   od_control.user_avatar = TRUE;
   od_control.baud = 9600;
   UT_ASSERT_EQ_INT(FALSE, utt_ODEditScrollArea(&instance, -2));
   UT_ASSERT_EQ_UINT(3, instance.unLineScrolledToTop);
   UT_ASSERT_EQ_UINT(0, ut_recommend_calls);
   UT_ASSERT_EQ_UINT(1, ut_redraw_calls);
}

static void uses_avatar_scroll_when_it_is_smaller_than_a_redraw(void)
{
   tEditInstance instance;
   tODEditOptions options;
   reset_scroll(&instance, &options);
   od_control.user_avatar = TRUE;
   od_control.baud = 9600;
   ut_recommend_full = FALSE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditScrollArea(&instance, 2));
   UT_ASSERT_EQ_UINT(1, ut_recommend_calls);
   UT_ASSERT_EQ_UINT((18 + 4) * 2 + 7, ut_recommend_bytes);
   UT_ASSERT_EQ_UINT(1, ut_scroll_calls);
   UT_ASSERT_EQ_INT(2, ut_scroll_distance);
   UT_ASSERT_EQ_UINT(SCROLL_NO_CLEAR, ut_scroll_flags);
   UT_ASSERT_EQ_UINT(7, instance.unLineScrolledToTop);
   UT_ASSERT_EQ_UINT(2, ut_draw_calls);
   UT_ASSERT_EQ_UINT(4, ut_draw_lines[0]);
   UT_ASSERT_EQ_UINT(5, ut_draw_lines[1]);
   UT_ASSERT_EQ_UINT(2, ut_caret_calls);
   UT_ASSERT_EQ_INT(FALSE, ut_caret_states[0]);
   UT_ASSERT_EQ_INT(TRUE, ut_caret_states[1]);
   UT_ASSERT_EQ_UINT(0, ut_redraw_calls);
}

static void honors_the_full_redraw_recommendation(void)
{
   tEditInstance instance;
   tODEditOptions options;
   reset_scroll(&instance, &options);
   od_control.user_avatar = TRUE;
   od_control.baud = 9600;
   ut_recommend_full = TRUE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODEditScrollArea(&instance, 2));
   UT_ASSERT_EQ_UINT(1, ut_recommend_calls);
   UT_ASSERT_EQ_UINT(0, ut_scroll_calls);
   UT_ASSERT_EQ_UINT(1, ut_redraw_calls);
}

static void uses_a_local_scroll_for_new_lines_at_the_top(void)
{
   tEditInstance instance;
   tODEditOptions options;
   reset_scroll(&instance, &options);
   od_control.user_avatar = FALSE;
   od_control.baud = 0;
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditScrollArea(&instance, -2));
   UT_ASSERT_EQ_UINT(1, ut_scroll_calls);
   UT_ASSERT_EQ_INT(-2, ut_scroll_distance);
   UT_ASSERT_EQ_UINT(3, instance.unLineScrolledToTop);
   UT_ASSERT_EQ_UINT(2, ut_draw_calls);
   UT_ASSERT_EQ_UINT(0, ut_draw_lines[0]);
   UT_ASSERT_EQ_UINT(1, ut_draw_lines[1]);
   UT_ASSERT_EQ_UINT(0, ut_redraw_calls);
}

static const UTTestCase ut_cases[] = {
   {"zero distance", accepts_a_zero_distance_without_work},
   {"remote full redraw", fully_redraws_remote_positive_and_negative_scrolls},
   {"avatar scroll", uses_avatar_scroll_when_it_is_smaller_than_a_redraw},
   {"recommended redraw", honors_the_full_redraw_recommendation},
   {"local scroll", uses_a_local_scroll_for_new_lines_at_the_top}
};
