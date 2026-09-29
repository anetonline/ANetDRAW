#define UT_CUSTOM_MOCK_ODScrnGetTextInfo
#define UT_CUSTOM_MOCK_ODSessionScreenAvailable
#define UT_CUSTOM_MOCK_ODSessionScreenGetInfo

static BOOL ut_session_available;

BOOL utm_ODSessionScreenAvailable(void)
{
   return ut_session_available;
}

void utm_ODSessionScreenGetInfo(tODVScreenInfo *info)
{
   memset(info, 0, sizeof(*info));
   info->winleft = 3;
   info->winright = 102;
   info->wintop = 4;
   info->winbottom = 33;
}

void utm_ODScrnGetTextInfo(tODScrnTextInfo *info)
{
   memset(info, 0, sizeof(*info));
   info->winleft = 2;
   info->winright = 81;
   info->wintop = 5;
   info->winbottom = 28;
}

static void obtains_virtual_window_dimensions(void)
{
   INT width = 0;
   INT height = 0;
   ut_session_available = TRUE;
   utt_ODEditGetWindowSize(&width, &height);
   UT_ASSERT_EQ_INT(100, width);
   UT_ASSERT_EQ_INT(30, height);
}

static void obtains_local_window_dimensions(void)
{
   INT width = 0;
   INT height = 0;
   ut_session_available = FALSE;
   utt_ODEditGetWindowSize(&width, &height);
   UT_ASSERT_EQ_INT(80, width);
   UT_ASSERT_EQ_INT(24, height);
}

static const UTTestCase ut_cases[] = {
   {"virtual dimensions", obtains_virtual_window_dimensions},
   {"local dimensions", obtains_local_window_dimensions}
};
