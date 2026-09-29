#include "common.h"

static void rejects_two_null_outputs_after_initializing(void)
{
   reset_graph_fixture();
   bODInitialized = FALSE;
   utt_od_get_cursor(NULL, NULL);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static void obtains_each_virtual_cursor_component(void)
{
   INT row = -1;
   INT column = -1;
   reset_graph_fixture();
   ut_session_available = TRUE;
   ut_session_info.curx = 123;
   ut_session_info.cury = 45;
   utt_od_get_cursor(&row, &column);
   UT_ASSERT_EQ_INT(45, row);
   UT_ASSERT_EQ_INT(123, column);

   row = -1;
   utt_od_get_cursor(&row, NULL);
   UT_ASSERT_EQ_INT(45, row);

   column = -1;
   utt_od_get_cursor(NULL, &column);
   UT_ASSERT_EQ_INT(123, column);
}

static void obtains_each_local_cursor_component(void)
{
   INT row = -1;
   INT column = -1;
   reset_graph_fixture();
   ut_text_info.curx = 12;
   ut_text_info.cury = 7;
   utt_od_get_cursor(&row, &column);
   UT_ASSERT_EQ_INT(7, row);
   UT_ASSERT_EQ_INT(12, column);

   row = -1;
   utt_od_get_cursor(&row, NULL);
   UT_ASSERT_EQ_INT(7, row);

   column = -1;
   utt_od_get_cursor(NULL, &column);
   UT_ASSERT_EQ_INT(12, column);
}

static void terminal_session_is_rejected(void)
{
   INT row;
   INT column;
   reset_graph_fixture(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   utt_od_get_cursor(&row, &column);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static const UTTestCase ut_cases[] = {
   {"invalid outputs", rejects_two_null_outputs_after_initializing},
   {"virtual cursor", obtains_each_virtual_cursor_component},
   {"local cursor", obtains_each_local_cursor_component},
   {"terminal session", terminal_session_is_rejected}
};
