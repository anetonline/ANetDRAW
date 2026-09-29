#define UT_CUSTOM_MOCK_ODScrnGetText
#define UT_CUSTOM_MOCK_ODScrnGetTextInfo
#define UT_CUSTOM_MOCK_ODSessionScreenAvailable
#define UT_CUSTOM_MOCK_ODSessionScreenGetInfo
#define UT_CUSTOM_MOCK_ODSessionScreenGetText
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_init

#define MOCK_ENTRY 280
#define MOCK_EXIT 281
#define MOCK_INFO 282
#define MOCK_INIT 283
#define MOCK_LOCAL_GET 284
#define MOCK_SESSION_AVAILABLE 285
#define MOCK_SESSION_GET 286
#define MOCK_SESSION_INFO 287

static BOOL ut_session_available;
static BOOL ut_get_result;
static INT ut_width;
static INT ut_height;
static BOOL ut_init_succeeds;

static void reset_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
   memset(&ODTextInfo, 0, sizeof(ODTextInfo));
   bODInitialized = TRUE;
   od_control.user_ansi = TRUE;
   ut_session_available = TRUE;
   ut_get_result = TRUE;
   ut_width = 100;
   ut_height = 40;
   ut_init_succeeds = TRUE;
}

BOOL ODCALL utm_ODScrnGetText(BYTE left, BYTE top, BYTE right, BYTE bottom,
   void *buffer)
{
   UT_ASSERT_EQ_UINT(1, left);
   UT_ASSERT_EQ_UINT(2, top);
   UT_ASSERT_EQ_UINT(3, right);
   UT_ASSERT_EQ_UINT(4, bottom);
   UT_ASSERT_NOT_NULL(buffer);
   ut_mock_called(MOCK_LOCAL_GET);
   return ut_get_result;
}

void utm_ODScrnGetTextInfo(tODScrnTextInfo *info)
{
   memset(info, 0, sizeof(*info));
   info->winleft = 1;
   info->winright = ut_width;
   info->wintop = 1;
   info->winbottom = ut_height;
   ut_mock_called(MOCK_INFO);
}

BOOL utm_ODSessionScreenAvailable(void)
{
   ut_mock_called(MOCK_SESSION_AVAILABLE);
   return ut_session_available;
}

void utm_ODSessionScreenGetInfo(tODVScreenInfo *info)
{
   memset(info, 0, sizeof(*info));
   info->winleft = 1;
   info->winright = ut_width;
   info->wintop = 1;
   info->winbottom = ut_height;
   ut_mock_called(MOCK_SESSION_INFO);
}

BOOL utm_ODSessionScreenGetText(INT left, INT top, INT right, INT bottom,
   void *buffer)
{
   UT_ASSERT_EQ_INT(1, left);
   UT_ASSERT_EQ_INT(2, top);
   UT_ASSERT_EQ_INT(3, right);
   UT_ASSERT_EQ_INT(4, bottom);
   UT_ASSERT_NOT_NULL(buffer);
   ut_mock_called(MOCK_SESSION_GET);
   return ut_get_result;
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
   char buffer[24];
   reset_fixture(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_gettext(1, 2, 3, 4, buffer));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_ENTRY));
}

static void assert_invalid(INT left, INT top, INT right, INT bottom,
   void *buffer)
{
   reset_fixture();
   UT_ASSERT_EQ_INT(FALSE,
      utt_od_gettext(left, top, right, bottom, buffer));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_SESSION_GET));
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_LOCAL_GET));
}

static void session_screen_is_used(void)
{
   char buffer[24];
   reset_fixture();
   bODInitialized = FALSE;

   UT_ASSERT_EQ_INT(TRUE, utt_od_gettext(1, 2, 3, 4, buffer));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_INIT));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_SESSION_INFO));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_SESSION_GET));
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_LOCAL_GET));
}

static void local_screen_is_used_and_result_is_returned(void)
{
   char buffer[24];
   reset_fixture();
   ut_session_available = FALSE;
   ut_get_result = FALSE;

   UT_ASSERT_EQ_INT(FALSE, utt_od_gettext(1, 2, 3, 4, buffer));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_INFO));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_LOCAL_GET));
}

static void each_parameter_error_is_rejected(void)
{
   char buffer[24];
   assert_invalid(0, 2, 3, 4, buffer);
   assert_invalid(1, 0, 3, 4, buffer);
   assert_invalid(1, 2, 101, 4, buffer);
   assert_invalid(1, 2, 3, 41, buffer);
   assert_invalid(4, 2, 3, 4, buffer);
   assert_invalid(1, 5, 3, 4, buffer);
   assert_invalid(1, 2, 3, 4, NULL);
}

static void no_graphics_mode_is_rejected(void)
{
   char buffer[24];
   reset_fixture();
   od_control.user_ansi = FALSE;
   od_control.user_avatar = FALSE;

   UT_ASSERT_EQ_INT(FALSE, utt_od_gettext(1, 2, 3, 4, buffer));
   UT_ASSERT_EQ_INT(ERR_NOGRAPHICS, od_control.od_error);
}

static void avatar_alone_allows_graphics(void)
{
   char buffer[24];
   reset_fixture();
   od_control.user_ansi = FALSE;
   od_control.user_avatar = TRUE;
   UT_ASSERT_EQ_INT(TRUE, utt_od_gettext(1, 2, 3, 4, buffer));
}

static const UTTestCase ut_cases[] = {
   {"session screen", session_screen_is_used},
   {"local screen result", local_screen_is_used_and_result_is_returned},
   {"parameter validation", each_parameter_error_is_rejected},
   {"no graphics", no_graphics_mode_is_rejected},
   {"avatar graphics", avatar_alone_allows_graphics},
   {"terminal session", terminal_session_is_rejected}
};
