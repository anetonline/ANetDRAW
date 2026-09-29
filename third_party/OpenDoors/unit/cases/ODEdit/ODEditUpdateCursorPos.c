#define UT_CUSTOM_MOCK_ODEditGetCurrentLineInArea
#define UT_CUSTOM_MOCK_od_set_cursor
static unsigned ut_cursor_calls;

UINT utm_ODEditGetCurrentLineInArea(tEditInstance *instance)
{
   UT_ASSERT_NOT_NULL(instance);
   return(4);
}

void ODCALL utm_od_set_cursor(INT row, INT column)
{
   ++ut_cursor_calls;
   UT_ASSERT_EQ_INT(7, row);
   UT_ASSERT_EQ_INT(12, column);
}

static void positions_the_cursor_relative_to_the_area(void)
{
   tEditInstance instance;
   tODEditOptions options;

   instance.pUserOptions = &options;
   instance.unCurrentColumn = 7;
   options.nAreaTop = 3;
   options.nAreaLeft = 5;
   ut_cursor_calls = 0;
   utt_ODEditUpdateCursorPos(&instance);
   UT_ASSERT_EQ_UINT(1, ut_cursor_calls);
}

static const UTTestCase ut_cases[] = {
   {"relative cursor", positions_the_cursor_relative_to_the_area}
};
