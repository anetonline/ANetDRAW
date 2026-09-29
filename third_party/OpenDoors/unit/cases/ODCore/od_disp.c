#include "display_common.h"

static const char ut_text[] = "abc";

static void can_suppress_both_remote_and_local_output(void)
{
   ut_reset_display();
   bODInitialized = FALSE;
   utt_od_disp(ut_text, 3, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(0, ut_remote_calls);
   UT_ASSERT_EQ_UINT(0, ut_session_buffer_calls);
   UT_ASSERT_EQ_UINT(0, ut_local_buffer_calls);
}

static void sends_remote_output_and_runs_an_elapsed_kernel_checkpoint(void)
{
   ut_reset_display();
   od_control.baud = 9600;
   ut_timer_elapsed = TRUE;
   utt_od_disp(ut_text, 3, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_remote_calls);
   UT_ASSERT_EQ_PTR(ut_text, ut_remote_buffer);
   UT_ASSERT_EQ_INT(3, ut_remote_size);
   UT_ASSERT_EQ_UINT(1, ut_timer_calls);
   UT_ASSERT_EQ_UINT(1, ut_kernel_calls);
}

static void echoes_to_the_authoritative_session_screen(void)
{
   ut_reset_display();
   od_control.baud = 9600;
   ut_session_available = TRUE;
   utt_od_disp(ut_text, 3, TRUE);
   UT_ASSERT_EQ_UINT(1, ut_remote_calls);
   UT_ASSERT_EQ_UINT(1, ut_session_buffer_calls);
   UT_ASSERT_EQ_UINT(1, ut_present_calls);
   UT_ASSERT_EQ_UINT(0, ut_local_buffer_calls);
}

static void falls_back_to_the_platform_local_screen(void)
{
   ut_reset_display();
   ut_remote_buffer = ut_text;
   ut_remote_size = 0;
   utt_od_disp(ut_text, 0, TRUE);
   UT_ASSERT_EQ_UINT(0, ut_remote_calls);
   UT_ASSERT_EQ_UINT(0, ut_session_buffer_calls);
   UT_ASSERT_EQ_UINT(0, ut_present_calls);
   UT_ASSERT_EQ_UINT(1, ut_local_buffer_calls);
}

static void terminal_session_is_rejected(void)
{
   ut_reset_display(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   utt_od_disp(ut_text, 3, FALSE);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static const UTTestCase ut_cases[] = {
   {"suppressed output", can_suppress_both_remote_and_local_output},
   {"remote output", sends_remote_output_and_runs_an_elapsed_kernel_checkpoint},
   {"session echo", echoes_to_the_authoritative_session_screen},
   {"local echo", falls_back_to_the_platform_local_screen},
   {"terminal session", terminal_session_is_rejected}
};
