#define UT_CUSTOM_MOCK_ODEditGetActualCurPos
#define UT_CUSTOM_MOCK_ODEditGetCurrentLineInArea
#define UT_CUSTOM_MOCK_ODEditUpdateCursorPos
static UINT ut_actual_row;
static UINT ut_actual_column;
static unsigned ut_update_calls;

void utm_ODEditGetActualCurPos(tEditInstance *instance, UINT *row,
   UINT *column)
{
   UT_ASSERT_NOT_NULL(instance);
   *row = ut_actual_row;
   *column = ut_actual_column;
}

UINT utm_ODEditGetCurrentLineInArea(tEditInstance *instance)
{
   UT_ASSERT_NOT_NULL(instance);
   return(4);
}

void utm_ODEditUpdateCursorPos(tEditInstance *instance)
{
   ++ut_update_calls;
   UT_ASSERT_NOT_NULL(instance);
}

static void updates_only_when_either_coordinate_moved(void)
{
   tEditInstance instance;
   tODEditOptions options;

   instance.pUserOptions = &options;
   instance.unCurrentColumn = 7;
   options.nAreaTop = 3;
   options.nAreaLeft = 5;
   ut_update_calls = 0;
   ut_actual_row = 7;
   ut_actual_column = 12;
   utt_ODEditUpdateCursorIfMoved(&instance);
   UT_ASSERT_EQ_UINT(0, ut_update_calls);
   ut_actual_row = 8;
   utt_ODEditUpdateCursorIfMoved(&instance);
   UT_ASSERT_EQ_UINT(1, ut_update_calls);
   ut_actual_row = 7;
   ut_actual_column = 13;
   utt_ODEditUpdateCursorIfMoved(&instance);
   UT_ASSERT_EQ_UINT(2, ut_update_calls);
}

static const UTTestCase ut_cases[] = {
   {"cursor movement", updates_only_when_either_coordinate_moved}
};
