#define UT_GRAPH_CLEAR_LINE
#include "common.h"

static void avatar_clears_virtual_screen_and_sends_avatar_sequence(void)
{
   reset_graph_fixture();
   bODInitialized = FALSE;
   ut_session_available = TRUE;
   ut_session_info.winleft = 5;
   ut_session_info.winright = 14;
   ut_session_info.curx = 3;
   od_control.user_avatar = TRUE;
   UT_ASSERT_EQ_INT(0, ut_display_calls);
   utt_od_clr_line();
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_clear_calls);
   UT_ASSERT_EQ_UINT(1, ut_present_calls);
   UT_ASSERT_EQ_INT(2, ut_display_length);
   UT_ASSERT(!ut_display_local);
   UT_ASSERT_EQ_INT(22, (unsigned char)ut_display_text[0]);
   UT_ASSERT_EQ_INT(7, (unsigned char)ut_display_text[1]);
}

static void ansi_clears_local_screen_without_remote_leakage(void)
{
   reset_graph_fixture();
   ut_text_info.curx = 75;
   od_control.user_ansi = TRUE;
   utt_od_clr_line();
   UT_ASSERT_EQ_UINT(1, ut_text_info_calls);
   UT_ASSERT_EQ_UINT(2, ut_scrolling_calls);
   UT_ASSERT(!ut_scrolling_values[0]);
   UT_ASSERT(ut_scrolling_values[1]);
   UT_ASSERT_EQ_UINT(1, ut_local_cursor_calls);
   UT_ASSERT_EQ_INT(3, ut_display_length);
   UT_ASSERT(!ut_display_local);
   UT_ASSERT_EQ_INT(27, (unsigned char)ut_display_text[0]);
}

static void plain_ascii_sends_spaces_and_backspaces_to_both_screens(void)
{
   INT index;
   reset_graph_fixture();
   ut_text_info.curx = 77;
   utt_od_clr_line();
   UT_ASSERT_EQ_INT(6, ut_display_length);
   UT_ASSERT(ut_display_local);
   for(index = 0; index < 3; ++index)
      UT_ASSERT_EQ_INT(' ', ut_display_text[index]);
   for(index = 3; index < 6; ++index)
      UT_ASSERT_EQ_INT(8, ut_display_text[index]);
   UT_ASSERT_EQ_UINT(0, ut_scrolling_calls);
}

static void both_graphics_modes_prefer_avatar(void)
{
   reset_graph_fixture();
   od_control.user_avatar = TRUE;
   od_control.user_ansi = TRUE;
   utt_od_clr_line();
   UT_ASSERT_EQ_INT(2, ut_display_length);
}

static void terminal_session_is_rejected(void)
{
   reset_graph_fixture(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   utt_od_clr_line();
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static const UTTestCase ut_cases[] = {
   {"AVATAR virtual clear", avatar_clears_virtual_screen_and_sends_avatar_sequence},
   {"ANSI local clear", ansi_clears_local_screen_without_remote_leakage},
   {"plain ASCII clear", plain_ascii_sends_spaces_and_backspaces_to_both_screens},
   {"AVATAR precedence", both_graphics_modes_prefer_avatar},
   {"terminal session", terminal_session_is_rejected}
};
