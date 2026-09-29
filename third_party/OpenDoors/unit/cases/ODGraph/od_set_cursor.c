#include "common.h"

static void rejects_each_nonpositive_coordinate(void)
{
   reset_graph_fixture();
   bODInitialized = FALSE;
   utt_od_set_cursor(0, 1);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);

   reset_graph_fixture();
   utt_od_set_cursor(1, 0);
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
}

static void rejects_each_out_of_window_coordinate(void)
{
   reset_graph_fixture();
   utt_od_set_cursor(1, 81);
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);

   reset_graph_fixture();
   utt_od_set_cursor(26, 1);
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
}

static void avatar_rejects_each_unrepresentable_coordinate(void)
{
   reset_graph_fixture();
   ut_session_available = TRUE;
   ut_session_info.winright = 300;
   ut_session_info.winbottom = 300;
   od_control.user_avatar = TRUE;
   utt_od_set_cursor(1, 256);
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);

   reset_graph_fixture();
   ut_session_available = TRUE;
   ut_session_info.winright = 300;
   ut_session_info.winbottom = 300;
   od_control.user_avatar = TRUE;
   utt_od_set_cursor(256, 1);
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);

   reset_graph_fixture();
   ut_session_available = TRUE;
   ut_session_info.winright = 300;
   ut_session_info.winbottom = 300;
   od_control.user_ansi = TRUE;
   utt_od_set_cursor(256, 256);
   UT_ASSERT_EQ_UINT(1, ut_session_cursor_calls);
}

static void sends_avatar_cursor_to_virtual_and_local_screens(void)
{
   reset_graph_fixture();
   ut_session_available = TRUE;
   od_control.user_avatar = TRUE;
   utt_od_set_cursor(7, 9);
   UT_ASSERT_EQ_UINT(1, ut_session_cursor_calls);
   UT_ASSERT_EQ_UINT(1, ut_present_calls);
   UT_ASSERT_EQ_INT(9, ut_cursor_column);
   UT_ASSERT_EQ_INT(7, ut_cursor_row);
   UT_ASSERT_EQ_INT(4, ut_display_length);

   reset_graph_fixture();
   od_control.user_avatar = TRUE;
   utt_od_set_cursor(7, 9);
   UT_ASSERT_EQ_UINT(1, ut_local_cursor_calls);
}

static void sends_ansi_cursor_to_virtual_and_local_screens(void)
{
   reset_graph_fixture();
   ut_session_available = TRUE;
   od_control.user_ansi = TRUE;
   utt_od_set_cursor(12, 34);
   UT_ASSERT_EQ_UINT(1, ut_session_cursor_calls);
   UT_ASSERT_EQ_UINT(1, ut_present_calls);
   UT_ASSERT_EQ_INT(27, (unsigned char)ut_display_text[0]);
   UT_ASSERT_EQ_INT(8, ut_display_length);

   reset_graph_fixture();
   od_control.user_ansi = TRUE;
   utt_od_set_cursor(2, 3);
   UT_ASSERT_EQ_UINT(1, ut_local_cursor_calls);
}

static void reports_lack_of_graphics(void)
{
   reset_graph_fixture();
   utt_od_set_cursor(1, 1);
   UT_ASSERT_EQ_INT(ERR_NOGRAPHICS, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_display_calls);
}

static void terminal_session_is_rejected(void)
{
   reset_graph_fixture(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   utt_od_set_cursor(1, 1);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static const UTTestCase ut_cases[] = {
   {"nonpositive coordinates", rejects_each_nonpositive_coordinate},
   {"outside window", rejects_each_out_of_window_coordinate},
   {"AVATAR byte limit", avatar_rejects_each_unrepresentable_coordinate},
   {"AVATAR cursor", sends_avatar_cursor_to_virtual_and_local_screens},
   {"ANSI cursor", sends_ansi_cursor_to_virtual_and_local_screens},
   {"graphics required", reports_lack_of_graphics},
   {"terminal session", terminal_session_is_rejected}
};
