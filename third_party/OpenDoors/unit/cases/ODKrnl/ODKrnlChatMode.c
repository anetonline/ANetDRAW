#define UT_CUSTOM_MOCK_od_get_key
#define UT_CUSTOM_MOCK_od_set_attrib
#define UT_CUSTOM_MOCK_od_disp_str
#define UT_CUSTOM_MOCK_od_putch
#define UT_CUSTOM_MOCK_ODKrnlChatCleanup
#define UT_CUSTOM_MOCK_ODTimerStart
#define UT_CUSTOM_MOCK_ODTimerElapsed
#define UT_CUSTOM_MOCK_od_sleep
#define UT_CUSTOM_MOCK_od_kernel

#define UT_MAX_KEYS 160
#define UT_CHAT_MAX_CALLS 200

static BYTE ut_keys[UT_MAX_KEYS];
static BOOL ut_sources[UT_MAX_KEYS];
static unsigned ut_key_count;
static unsigned ut_key_index;
static unsigned ut_stop_after;
static unsigned ut_nonwaiting_calls;
static INT ut_attributes[UT_CHAT_MAX_CALLS];
static unsigned ut_attribute_calls;
static BYTE ut_output[UT_CHAT_MAX_CALLS];
static unsigned ut_output_calls;
static unsigned ut_display_calls;
static unsigned ut_before_calls;
static unsigned ut_newline_calls;
static unsigned ut_backspace_calls;
static unsigned ut_erase_calls;
static unsigned ut_word_calls;
static unsigned ut_cleanup_calls;
static unsigned ut_before_callback_calls;
static unsigned ut_log_calls;
static BOOL ut_disable_in_before_callback;
static unsigned ut_timer_starts;
static unsigned ut_timer_checks;
static unsigned ut_sleep_calls;
static unsigned ut_kernel_calls;
static BOOL ut_timer_elapsed;

static void add_key(BYTE key, BOOL local)
{
   UT_ASSERT(ut_key_count < UT_MAX_KEYS);
   ut_keys[ut_key_count] = key;
   ut_sources[ut_key_count] = local;
   ++ut_key_count;
}

char ODCALL utm_od_get_key(BOOL wait)
{
   unsigned position = ut_key_index;
   UT_ASSERT(position < ut_key_count);
   UT_ASSERT(!wait);
   ++ut_nonwaiting_calls;
   od_control.od_last_input = ut_sources[position];
   ++ut_key_index;
   if(ut_key_index == ut_stop_after)
      od_control.od_chat_active = FALSE;
   return((char)ut_keys[position]);
}

void ODCALL utm_od_set_attrib(INT attribute)
{
   UT_ASSERT(ut_attribute_calls < UT_CHAT_MAX_CALLS);
   ut_attributes[ut_attribute_calls++] = attribute;
}

void ODCALL utm_od_disp_str(const char *text)
{
   unsigned index;
   ++ut_display_calls;
   if(text == od_control.od_before_chat)
      ++ut_before_calls;
   else if(text == szBackspaceWithDelete)
      ++ut_backspace_calls;
   else if(strcmp(text, "\r\n") == 0)
      ++ut_newline_calls;
   else {
      for(index = 0; text[index] == '\b'; ++index) { }
      if(index != 0)
         ++ut_erase_calls;
      else
         ++ut_word_calls;
   }
}

void ODCALL utm_od_putch(char value)
{
   UT_ASSERT(ut_output_calls < UT_CHAT_MAX_CALLS);
   ut_output[ut_output_calls++] = (BYTE)value;
}

void utm_ODKrnlChatCleanup(void)
{
   ++ut_cleanup_calls;
   od_control.od_chat_active = FALSE;
}

#ifdef ODPLAT_DOS32
static void ODCALL ut_before_callback(void)
#else
static void ut_before_callback(void)
#endif
{
   ++ut_before_callback_calls;
   UT_ASSERT(bShellChatActive);
   if(ut_disable_in_before_callback)
      od_control.od_chat_active = FALSE;
}

static BOOL ODCALL ut_log(INT event)
{
   ++ut_log_calls;
   UT_ASSERT_EQ_INT(9, event);
   return(TRUE);
}

void utm_ODTimerStart(tODTimer *timer, tODMilliSec duration)
{
   UT_ASSERT(timer != NULL);
   UT_ASSERT_EQ_UINT(CHAT_YIELD_PERIOD, duration);
   ++ut_timer_starts;
}

BOOL utm_ODTimerElapsed(tODTimer *timer)
{
   UT_ASSERT(timer != NULL);
   ++ut_timer_checks;
   return(ut_timer_elapsed);
}

void ODCALL utm_od_sleep(tODMilliSec duration)
{
   UT_ASSERT_EQ_UINT(0, duration);
   ++ut_sleep_calls;
}

void ODCALL utm_od_kernel(void) { ++ut_kernel_calls; }


static void reset_chat(void)
{
   unsigned index;
   ut_key_count = ut_key_index = 0;
   ut_stop_after = UT_MAX_KEYS;
   ut_nonwaiting_calls = 0;
   ut_attribute_calls = ut_output_calls = ut_display_calls = 0;
   ut_before_calls = ut_newline_calls = ut_backspace_calls = 0;
   ut_erase_calls = ut_word_calls = ut_cleanup_calls = 0;
   ut_before_callback_calls = ut_log_calls = 0;
   ut_disable_in_before_callback = FALSE;
   for(index = 0; index < UT_CHAT_MAX_CALLS; ++index)
      ut_attributes[index] = 0;
   ut_timer_starts = ut_timer_checks = ut_sleep_calls = ut_kernel_calls = 0;
   ut_timer_elapsed = FALSE;
   bChatted = FALSE;
   bSysopColor = FALSE;
   bShellChatActive = FALSE;
   bForceStatusUpdate = FALSE;
   od_control.od_cur_attrib = 3;
   od_control.od_chat_color1 = 4;
   od_control.od_chat_color2 = 5;
   od_control.od_chat_active = FALSE;
   od_control.user_wantchat = TRUE;
   od_control.od_last_input = TRUE;
   od_control.od_before_chat = NULL;
   od_control.od_cbefore_chat = NULL;
   pfLogWrite = NULL;
}

static void initializes_chat_and_handles_optional_entry_actions(void)
{
   reset_chat();
   add_key(27, TRUE);
   utt_ODKrnlChatMode();
   UT_ASSERT(bChatted);
   UT_ASSERT(!od_control.user_wantchat);
   UT_ASSERT(bForceStatusUpdate);
   UT_ASSERT_EQ_INT(3, nChatOriginalAttrib);
   UT_ASSERT_EQ_UINT(1, ut_cleanup_calls);
   UT_ASSERT_EQ_UINT(1, ut_attribute_calls);
   UT_ASSERT_EQ_INT(4, ut_attributes[0]);
   UT_ASSERT_EQ_UINT(1, ut_kernel_calls);
   UT_ASSERT_EQ_UINT(1, ut_timer_starts);

   reset_chat();
   od_control.od_before_chat = (char *)"before";
   od_control.od_cbefore_chat = ut_before_callback;
   pfLogWrite = ut_log;
   add_key(27, TRUE);
   utt_ODKrnlChatMode();
   UT_ASSERT_EQ_UINT(1, ut_before_callback_calls);
   UT_ASSERT(!bShellChatActive);
   UT_ASSERT_EQ_UINT(1, ut_before_calls);
   UT_ASSERT_EQ_UINT(1, ut_log_calls);
}

static void honors_a_prechat_callback_that_cancels_chat(void)
{
   reset_chat();
   od_control.od_cbefore_chat = ut_before_callback;
   ut_disable_in_before_callback = TRUE;
   utt_ODKrnlChatMode();
   UT_ASSERT_EQ_UINT(1, ut_before_callback_calls);
   UT_ASSERT_EQ_UINT(1, ut_cleanup_calls);
   UT_ASSERT_EQ_UINT(0, ut_attribute_calls);
   UT_ASSERT_EQ_UINT(0, ut_key_index);
}

static void switches_colors_for_local_and_remote_input(void)
{
   reset_chat();
   add_key('a', TRUE);
   add_key('b', FALSE);
   add_key('c', FALSE);
   add_key('d', TRUE);
   add_key(27, FALSE);
   add_key(27, TRUE);
   utt_ODKrnlChatMode();
   UT_ASSERT_EQ_UINT(5, ut_attribute_calls);
   UT_ASSERT_EQ_INT(4, ut_attributes[0]);
   UT_ASSERT_EQ_INT(5, ut_attributes[1]);
   UT_ASSERT_EQ_INT(4, ut_attributes[2]);
   UT_ASSERT_EQ_INT(5, ut_attributes[3]);
   UT_ASSERT_EQ_INT(4, ut_attributes[4]);
   UT_ASSERT_EQ_UINT(4, ut_output_calls);
   UT_ASSERT_EQ_UINT(1, ut_cleanup_calls);
}

static void handles_spaces_backspace_and_enter(void)
{
   reset_chat();
   add_key('a', TRUE);
   add_key(8, TRUE);
   add_key(8, TRUE);
   add_key(' ', TRUE);
   add_key(13, TRUE);
   add_key(27, TRUE);
   utt_ODKrnlChatMode();
   UT_ASSERT_EQ_UINT(2, ut_output_calls);
   UT_ASSERT_EQ_UINT(2, ut_backspace_calls);
   UT_ASSERT_EQ_UINT(1, ut_newline_calls);
}

static void observes_chat_deactivation_at_the_loop_boundary(void)
{
   reset_chat();
   add_key(0, TRUE);
   ut_stop_after = 1;
   utt_ODKrnlChatMode();
   UT_ASSERT_EQ_UINT(1, ut_key_index);
   UT_ASSERT_EQ_UINT(1, ut_cleanup_calls);
}

static void wraps_a_short_word_at_the_chat_boundary(void)
{
   unsigned index;
   reset_chat();
   for(index = 0; index < 70; ++index)
      add_key('a', TRUE);
   add_key(' ', TRUE);
   add_key('w', TRUE);
   add_key('o', TRUE);
   add_key('r', TRUE);
   add_key('d', TRUE);
   add_key('x', TRUE);
   add_key('y', TRUE);
   add_key(27, TRUE);
   utt_ODKrnlChatMode();
   UT_ASSERT_EQ_UINT(1, ut_erase_calls);
   UT_ASSERT_EQ_UINT(1, ut_newline_calls);
   UT_ASSERT_EQ_UINT(1, ut_word_calls);
}

static void starts_a_new_line_without_wrapping_an_empty_or_long_word(void)
{
   unsigned index;
   reset_chat();
   for(index = 0; index < 76; ++index)
      add_key(' ', TRUE);
   add_key(27, TRUE);
   utt_ODKrnlChatMode();
   UT_ASSERT_EQ_UINT(1, ut_newline_calls);
   UT_ASSERT_EQ_UINT(0, ut_erase_calls);

   reset_chat();
   for(index = 0; index < 76; ++index)
      add_key('x', TRUE);
   add_key(27, TRUE);
   utt_ODKrnlChatMode();
   UT_ASSERT_EQ_UINT(1, ut_newline_calls);
   UT_ASSERT_EQ_UINT(0, ut_erase_calls);
}

static void yields_only_after_the_chat_timer_elapses(void)
{
   reset_chat();
   add_key(0, TRUE);
   ut_stop_after = 1;
   utt_ODKrnlChatMode();
   UT_ASSERT_EQ_UINT(1, ut_timer_checks);
   UT_ASSERT_EQ_UINT(0, ut_sleep_calls);
   UT_ASSERT_EQ_UINT(1, ut_timer_starts);

   reset_chat();
   ut_timer_elapsed = TRUE;
   add_key(0, TRUE);
   ut_stop_after = 1;
   utt_ODKrnlChatMode();
   UT_ASSERT_EQ_UINT(1, ut_timer_checks);
   UT_ASSERT_EQ_UINT(1, ut_sleep_calls);
   UT_ASSERT_EQ_UINT(2, ut_timer_starts);
}

static const UTTestCase ut_cases[] = {
   {"entry", initializes_chat_and_handles_optional_entry_actions},
   {"prechat cancellation", honors_a_prechat_callback_that_cancels_chat},
   {"colors", switches_colors_for_local_and_remote_input},
   {"editing", handles_spaces_backspace_and_enter},
   {"external end", observes_chat_deactivation_at_the_loop_boundary},
   {"word wrap", wraps_a_short_word_at_the_chat_boundary},
   {"hard wrap", starts_a_new_line_without_wrapping_an_empty_or_long_word},
   {"yield", yields_only_after_the_chat_timer_elapses}
};
