#define UT_CUSTOM_MOCK_od_set_attrib
#define UT_CUSTOM_MOCK_od_disp_str
static unsigned ut_attrib_calls, ut_display_calls, ut_callback_calls;
static unsigned ut_log_calls;
static INT ut_attribs[2];
void ODCALL utm_od_set_attrib(INT attrib)
{
   UT_ASSERT(ut_attrib_calls < 2);
   ut_attribs[ut_attrib_calls++] = attrib;
}
void ODCALL utm_od_disp_str(const char *text)
{
   ++ut_display_calls;
   UT_ASSERT(strcmp("after", text) == 0);
}
#ifdef ODPLAT_DOS32
static void ODCALL ut_after_chat(void)
#else
static void ut_after_chat(void)
#endif
{
   ++ut_callback_calls;
   UT_ASSERT(bShellChatActive);
}
static BOOL ODCALL ut_log(INT event)
{
   ++ut_log_calls;
   UT_ASSERT_EQ_UINT(10, event);
   return(TRUE);
}
static void reset_cleanup(void)
{
   ut_attrib_calls = ut_display_calls = ut_callback_calls = ut_log_calls = 0;
   od_control.od_chat_color1 = 7;
   od_control.od_after_chat = NULL;
   od_control.od_cafter_chat = NULL;
   od_control.od_chat_active = TRUE;
   nChatOriginalAttrib = 3;
   bShellChatActive = FALSE;
   pfLogWrite = NULL;
}
static void restores_state_with_no_optional_notifications(void)
{
   reset_cleanup();
   utt_ODKrnlChatCleanup();
   UT_ASSERT_EQ_UINT(2, ut_attrib_calls);
   UT_ASSERT_EQ_INT(7, ut_attribs[0]); UT_ASSERT_EQ_INT(3, ut_attribs[1]);
   UT_ASSERT_EQ_UINT(0, ut_display_calls); UT_ASSERT_EQ_UINT(0, ut_callback_calls);
   UT_ASSERT_EQ_UINT(0, ut_log_calls); UT_ASSERT(!od_control.od_chat_active);
}
static void invokes_each_optional_notification(void)
{
   reset_cleanup();
   od_control.od_after_chat = (char *)"after";
   od_control.od_cafter_chat = ut_after_chat;
   pfLogWrite = ut_log;
   utt_ODKrnlChatCleanup();
   UT_ASSERT_EQ_UINT(1, ut_display_calls); UT_ASSERT_EQ_UINT(1, ut_callback_calls);
   UT_ASSERT_EQ_UINT(1, ut_log_calls); UT_ASSERT(!bShellChatActive);
}
static const UTTestCase ut_cases[] = {
   {"plain", restores_state_with_no_optional_notifications},
   {"notifications", invokes_each_optional_notification}
};
