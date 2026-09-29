#define UT_CUSTOM_MOCK_strlen
#include "display_common.h"

static const char ut_text[] = "hello";

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

static void sends_remote_text_and_updates_the_session_screen(void)
{
   ut_reset_display();
   bODInitialized = FALSE;
   od_control.baud = 9600;
   ut_session_available = TRUE;
   ut_timer_elapsed = TRUE;
   utt_od_disp_str(ut_text);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(1, ut_remote_calls);
   UT_ASSERT_EQ_PTR(ut_text, ut_remote_buffer);
   UT_ASSERT_EQ_INT(5, ut_remote_size);
   UT_ASSERT_EQ_UINT(1, ut_session_string_calls);
   UT_ASSERT_EQ_UINT(1, ut_present_calls);
   UT_ASSERT_EQ_UINT(0, ut_local_string_calls);
   UT_ASSERT_EQ_UINT(1, ut_timer_calls);
   UT_ASSERT_EQ_UINT(1, ut_kernel_calls);
}

static void local_mode_uses_the_platform_screen_fallback(void)
{
   ut_reset_display();
   ut_remote_buffer = ut_text;
   utt_od_disp_str(ut_text);
   UT_ASSERT_EQ_UINT(0, ut_remote_calls);
   UT_ASSERT_EQ_UINT(0, ut_session_string_calls);
   UT_ASSERT_EQ_UINT(0, ut_present_calls);
   UT_ASSERT_EQ_UINT(1, ut_local_string_calls);
   UT_ASSERT_EQ_UINT(1, ut_timer_calls);
   UT_ASSERT_EQ_UINT(0, ut_kernel_calls);
}

static void terminal_session_is_rejected(void)
{
   ut_reset_display(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   utt_od_disp_str(ut_text);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static const UTTestCase ut_cases[] = {
   {"remote session string", sends_remote_text_and_updates_the_session_screen},
   {"local string", local_mode_uses_the_platform_screen_fallback},
   {"terminal session", terminal_session_is_rejected}
};
