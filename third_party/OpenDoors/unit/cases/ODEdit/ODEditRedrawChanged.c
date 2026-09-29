#define UT_CUSTOM_MOCK_ODEditDetermineChanged
#define UT_CUSTOM_MOCK_ODEditUpdateCursorIfMoved
#define UT_CUSTOM_MOCK_ODEditEstDrawBytes
#define UT_CUSTOM_MOCK_ODEditRecommendFullRedraw
#define UT_CUSTOM_MOCK_ODEditRedrawArea
#define UT_CUSTOM_MOCK_ODEditUpdateCursorPos
#define UT_CUSTOM_MOCK_ODEditRedrawSubArea

static BOOL ut_changed;
static BOOL ut_full_redraw;
static unsigned ut_determine_calls;
static unsigned ut_update_if_moved_calls;
static unsigned ut_estimate_calls;
static unsigned ut_recommend_calls;
static unsigned ut_redraw_area_calls;
static unsigned ut_update_position_calls;
static unsigned ut_redraw_subarea_calls;

BOOL utm_ODEditDetermineChanged(tEditInstance *instance, void *area,
   UINT upper, UINT lower, UINT *start_line, UINT *start_column,
   UINT *finish_line, UINT *finish_column)
{
   ++ut_determine_calls;
   UT_ASSERT_EQ_PTR(instance->pRememberBuffer, area);
   UT_ASSERT_EQ_UINT(2, upper);
   UT_ASSERT_EQ_UINT(7, lower);
   *start_line = 1;
   *start_column = 2;
   *finish_line = 3;
   *finish_column = 4;
   return(ut_changed);
}

void utm_ODEditUpdateCursorIfMoved(tEditInstance *instance)
{
   ++ut_update_if_moved_calls;
   UT_ASSERT_NOT_NULL(instance);
}

UINT utm_ODEditEstDrawBytes(tEditInstance *instance, UINT start_line,
   UINT start_column, UINT finish_line, UINT finish_column)
{
   ++ut_estimate_calls;
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT_EQ_UINT(1, start_line);
   UT_ASSERT_EQ_UINT(2, start_column);
   UT_ASSERT_EQ_UINT(3, finish_line);
   UT_ASSERT_EQ_UINT(4, finish_column);
   return(19);
}

BOOL utm_ODEditRecommendFullRedraw(tEditInstance *instance, UINT bytes,
   BOOL forced)
{
   ++ut_recommend_calls;
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT_EQ_UINT(19, bytes);
   UT_ASSERT_EQ_INT(FALSE, forced);
   return(ut_full_redraw);
}

void utm_ODEditRedrawArea(tEditInstance *instance)
{
   ++ut_redraw_area_calls;
   UT_ASSERT_NOT_NULL(instance);
}

void utm_ODEditUpdateCursorPos(tEditInstance *instance)
{
   ++ut_update_position_calls;
   UT_ASSERT_NOT_NULL(instance);
}

void utm_ODEditRedrawSubArea(tEditInstance *instance, UINT start_line,
   UINT start_column, UINT finish_line, UINT finish_column)
{
   ++ut_redraw_subarea_calls;
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT_EQ_UINT(1, start_line);
   UT_ASSERT_EQ_UINT(2, start_column);
   UT_ASSERT_EQ_UINT(3, finish_line);
   UT_ASSERT_EQ_UINT(4, finish_column);
}

static void reset_changed(tEditInstance *instance, void *remembered)
{
   instance->pRememberBuffer = remembered;
   ut_changed = TRUE;
   ut_full_redraw = FALSE;
   ut_determine_calls = 0;
   ut_update_if_moved_calls = 0;
   ut_estimate_calls = 0;
   ut_recommend_calls = 0;
   ut_redraw_area_calls = 0;
   ut_update_position_calls = 0;
   ut_redraw_subarea_calls = 0;
}

static void only_restores_the_cursor_when_nothing_changed(void)
{
   char remembered;
   tEditInstance instance;
   reset_changed(&instance, &remembered);
   ut_changed = FALSE;
   utt_ODEditRedrawChanged(&instance, &remembered, 2, 7);
   UT_ASSERT_EQ_UINT(1, ut_determine_calls);
   UT_ASSERT_EQ_UINT(1, ut_update_if_moved_calls);
   UT_ASSERT_EQ_UINT(0, ut_estimate_calls);
   UT_ASSERT_EQ_UINT(0, ut_recommend_calls);
   UT_ASSERT_EQ_UINT(0, ut_redraw_area_calls);
   UT_ASSERT_EQ_UINT(0, ut_update_position_calls);
   UT_ASSERT_EQ_UINT(0, ut_redraw_subarea_calls);
}

static void performs_the_recommended_full_redraw(void)
{
   char remembered;
   tEditInstance instance;
   reset_changed(&instance, &remembered);
   ut_full_redraw = TRUE;
   utt_ODEditRedrawChanged(&instance, &remembered, 2, 7);
   UT_ASSERT_EQ_UINT(1, ut_estimate_calls);
   UT_ASSERT_EQ_UINT(1, ut_recommend_calls);
   UT_ASSERT_EQ_UINT(1, ut_redraw_area_calls);
   UT_ASSERT_EQ_UINT(1, ut_update_position_calls);
   UT_ASSERT_EQ_UINT(0, ut_redraw_subarea_calls);
   UT_ASSERT_EQ_UINT(0, ut_update_if_moved_calls);
}

static void performs_an_incremental_redraw(void)
{
   char remembered;
   tEditInstance instance;
   reset_changed(&instance, &remembered);
   utt_ODEditRedrawChanged(&instance, &remembered, 2, 7);
   UT_ASSERT_EQ_UINT(1, ut_estimate_calls);
   UT_ASSERT_EQ_UINT(1, ut_recommend_calls);
   UT_ASSERT_EQ_UINT(0, ut_redraw_area_calls);
   UT_ASSERT_EQ_UINT(0, ut_update_position_calls);
   UT_ASSERT_EQ_UINT(1, ut_redraw_subarea_calls);
   UT_ASSERT_EQ_UINT(1, ut_update_if_moved_calls);
}

static const UTTestCase ut_cases[] = {
   {"unchanged", only_restores_the_cursor_when_nothing_changed},
   {"full redraw", performs_the_recommended_full_redraw},
   {"partial redraw", performs_an_incremental_redraw}
};
