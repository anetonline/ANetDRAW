#define UT_CUSTOM_MOCK_ODEditGetCurrentLineInArea
#define UT_CUSTOM_MOCK_ODEditScrollArea

static UINT ut_area_line;
static unsigned ut_area_calls;
static INT ut_scroll_distance;
static unsigned ut_scroll_calls;

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

static void reset_previous(tEditInstance *instance, UINT line,
   UINT scroll_distance, UINT area_line)
{
   instance->unCurrentLine = line;
   instance->unScrollDistance = scroll_distance;
   ut_area_line = area_line;
   ut_area_calls = 0;
   ut_scroll_distance = 0;
   ut_scroll_calls = 0;
}

static void stays_at_the_first_line(void)
{
   tEditInstance instance;
   reset_previous(&instance, 0, 3, 0);
   utt_ODEditGotoPreviousLine(&instance);
   UT_ASSERT_EQ_UINT(0, instance.unCurrentLine);
   UT_ASSERT_EQ_UINT(0, ut_area_calls);
   UT_ASSERT_EQ_UINT(0, ut_scroll_calls);
}

static void moves_without_scrolling_inside_the_area(void)
{
   tEditInstance instance;
   reset_previous(&instance, 3, 2, 1);
   utt_ODEditGotoPreviousLine(&instance);
   UT_ASSERT_EQ_UINT(2, instance.unCurrentLine);
   UT_ASSERT_EQ_UINT(1, ut_area_calls);
   UT_ASSERT_EQ_UINT(0, ut_scroll_calls);
}

static void scrolls_by_the_smaller_available_distance(void)
{
   tEditInstance instance;

   reset_previous(&instance, 4, 2, 0);
   utt_ODEditGotoPreviousLine(&instance);
   UT_ASSERT_EQ_UINT(3, instance.unCurrentLine);
   UT_ASSERT_EQ_UINT(1, ut_scroll_calls);
   UT_ASSERT_EQ_INT(-2, ut_scroll_distance);

   reset_previous(&instance, 2, 5, 0);
   utt_ODEditGotoPreviousLine(&instance);
   UT_ASSERT_EQ_UINT(1, instance.unCurrentLine);
   UT_ASSERT_EQ_UINT(1, ut_scroll_calls);
   UT_ASSERT_EQ_INT(-2, ut_scroll_distance);
}

static const UTTestCase ut_cases[] = {
   {"first line", stays_at_the_first_line},
   {"visible previous", moves_without_scrolling_inside_the_area},
   {"scroll distance", scrolls_by_the_smaller_available_distance}
};
