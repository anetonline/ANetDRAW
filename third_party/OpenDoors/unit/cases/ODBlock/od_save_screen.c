#define UT_CUSTOM_MOCK_ODScrnGetText
#define UT_CUSTOM_MOCK_ODScrnGetTextInfo
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_init

#define MOCK_ENTRY 260
#define MOCK_EXIT 261
#define MOCK_GET_TEXT 262
#define MOCK_INFO 263
#define MOCK_INIT 264

static tODScrnTextInfo ut_text_info;
static BOOL ut_get_text_result;
static char ut_screen_buffer[4004];
static BOOL ut_init_succeeds;

static void reset_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
   memset(&ut_text_info, 0, sizeof(ut_text_info));
   ut_text_info.winleft = 1;
   ut_text_info.winright = 80;
   ut_text_info.wintop = 1;
   ut_text_info.winbottom = 25;
   ut_text_info.curx = 12;
   ut_text_info.cury = 7;
   ut_text_info.attribute = 31;
   ut_get_text_result = TRUE;
   bODInitialized = TRUE;
   ut_init_succeeds = TRUE;
}

BOOL ODCALL utm_ODScrnGetText(BYTE left, BYTE top, BYTE right, BYTE bottom,
   void *buffer)
{
   UT_ASSERT_EQ_UINT(1, left);
   UT_ASSERT_EQ_UINT(1, top);
   UT_ASSERT_EQ_UINT(80, right);
   UT_ASSERT_EQ_UINT(25, bottom);
   UT_ASSERT_NOT_NULL(buffer);
   ut_mock_called(MOCK_GET_TEXT);
   return ut_get_text_result;
}

void utm_ODScrnGetTextInfo(tODScrnTextInfo *info)
{
   UT_ASSERT_NOT_NULL(info);
   *info = ut_text_info;
   ut_mock_called(MOCK_INFO);
}

void utm_ODSyncAPIEntry(void) { ut_mock_called(MOCK_ENTRY); }
void utm_ODSyncAPIExit(void) { ut_mock_called(MOCK_EXIT); }
void ODCALL utm_od_init(void)
{
   ut_mock_called(MOCK_INIT);
   if(ut_init_succeeds) bODInitialized = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_fixture(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_save_screen(ut_screen_buffer));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_ENTRY));
}

static void valid_window_stores_header_and_screen(void)
{
   reset_fixture();
   bODInitialized = FALSE;
   memset(ut_screen_buffer, 0, sizeof(ut_screen_buffer));

   UT_ASSERT_EQ_INT(TRUE, utt_od_save_screen(ut_screen_buffer));
   UT_ASSERT_EQ_INT(12, (unsigned char)ut_screen_buffer[0]);
   UT_ASSERT_EQ_INT(7, (unsigned char)ut_screen_buffer[1]);
   UT_ASSERT_EQ_INT(31, (unsigned char)ut_screen_buffer[2]);
   UT_ASSERT_EQ_INT(25, (unsigned char)ut_screen_buffer[3]);
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_INIT));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_GET_TEXT));
   UT_ASSERT_EQ_UINT(MOCK_EXIT, ut_mock_calls[ut_mock_call_count - 2]);
}

static void screen_read_failure_is_returned(void)
{
   reset_fixture();
   ut_get_text_result = FALSE;

   UT_ASSERT_EQ_INT(FALSE, utt_od_save_screen(ut_screen_buffer));
}

static void invalid_left_edge_is_rejected(void)
{
   reset_fixture();
   ut_text_info.winleft = 2;
   UT_ASSERT_EQ_INT(FALSE, utt_od_save_screen(ut_screen_buffer));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_GET_TEXT));
}

static void invalid_right_edge_is_rejected(void)
{
   reset_fixture();
   ut_text_info.winright = 79;
   UT_ASSERT_EQ_INT(FALSE, utt_od_save_screen(ut_screen_buffer));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
}

static void null_buffer_is_rejected(void)
{
   reset_fixture();
   UT_ASSERT_EQ_INT(FALSE, utt_od_save_screen(NULL));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
}

static const UTTestCase ut_cases[] = {
   {"valid save", valid_window_stores_header_and_screen},
   {"screen failure", screen_read_failure_is_returned},
   {"invalid left edge", invalid_left_edge_is_rejected},
   {"invalid right edge", invalid_right_edge_is_rejected},
   {"null buffer", null_buffer_is_rejected},
   {"terminal session", terminal_session_is_rejected}
};
