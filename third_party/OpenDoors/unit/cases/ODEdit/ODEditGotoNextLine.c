#define UT_CUSTOM_MOCK_ODEditBufferGetTotalLines
#define UT_CUSTOM_MOCK_ODEditGetCurrentLineInArea
#define UT_CUSTOM_MOCK_ODEditScrollArea

static UINT ut_total_lines;
static UINT ut_area_line;
static unsigned ut_area_calls;
static INT ut_scroll_distance;
static unsigned ut_scroll_calls;

UINT utm_ODEditBufferGetTotalLines(tEditInstance *instance)
{
   UT_ASSERT_NOT_NULL(instance);
   return(ut_total_lines);
}

UINT utm_ODEditGetCurrentLineInArea(tEditInstance *instance)
{
   UT_ASSERT_NOT_NULL(instance);
   ++ut_area_calls;
   return(ut_area_line);
}

BOOL utm_ODEditScrollArea(tEditInstance *instance, INT distance)
{
   UT_ASSERT_NOT_NULL(instance);
   ++ut_scroll_calls;
   ut_scroll_distance = distance;
   return(TRUE);
}

static void reset_next(tEditInstance *instance, UINT line, UINT total,
   UINT area_line, UINT area_height, UINT scroll_distance)
{
   instance->unCurrentLine = line;
   instance->unAreaHeight = area_height;
   instance->unScrollDistance = scroll_distance;
   ut_total_lines = total;
   ut_area_line = area_line;
   ut_area_calls = 0;
   ut_scroll_distance = 0;
   ut_scroll_calls = 0;
}

static void stays_at_the_final_line(void)
{
   tEditInstance instance;
   reset_next(&instance, 2, 3, 1, 3, 2);
   utt_ODEditGotoNextLine(&instance);
   UT_ASSERT_EQ_UINT(2, instance.unCurrentLine);
   UT_ASSERT_EQ_UINT(0, ut_area_calls);
   UT_ASSERT_EQ_UINT(0, ut_scroll_calls);
}

static void moves_without_scrolling_inside_the_area(void)
{
   tEditInstance instance;
   reset_next(&instance, 2, 6, 1, 3, 2);
   utt_ODEditGotoNextLine(&instance);
   UT_ASSERT_EQ_UINT(3, instance.unCurrentLine);
   UT_ASSERT_EQ_UINT(1, ut_area_calls);
   UT_ASSERT_EQ_UINT(0, ut_scroll_calls);
}

static void scrolls_by_the_smaller_available_distance(void)
{
   tEditInstance instance;

   reset_next(&instance, 2, 10, 2, 3, 3);
   utt_ODEditGotoNextLine(&instance);
   UT_ASSERT_EQ_UINT(3, instance.unCurrentLine);
   UT_ASSERT_EQ_UINT(1, ut_scroll_calls);
   UT_ASSERT_EQ_INT(3, ut_scroll_distance);

   reset_next(&instance, 7, 10, 2, 3, 5);
   utt_ODEditGotoNextLine(&instance);
   UT_ASSERT_EQ_UINT(8, instance.unCurrentLine);
   UT_ASSERT_EQ_UINT(1, ut_scroll_calls);
   UT_ASSERT_EQ_INT(3, ut_scroll_distance);
}

static const UTTestCase ut_cases[] = {
   {"final line", stays_at_the_final_line},
   {"visible next", moves_without_scrolling_inside_the_area},
   {"scroll distance", scrolls_by_the_smaller_available_distance}
};
