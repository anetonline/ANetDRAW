#define UT_CUSTOM_MOCK_ODScrnGetTextInfo

static unsigned ut_info_calls;

void utm_ODScrnGetTextInfo(tODScrnTextInfo *pInfo)
{
   ++ut_info_calls;
   pInfo->winleft = 4;
   pInfo->winright = 13;
   pInfo->wintop = 2;
   pInfo->winbottom = 8;
}

static void uses_the_session_screen_when_available(void)
{
   INT width;
   INT height;
   bSessionScreenAvailable = TRUE;
   SessionScreen.nWidth = 132;
   SessionScreen.nHeight = 50;
   ut_info_calls = 0;
   UT_ASSERT_EQ_INT(TRUE, utt_ODSessionSnapshotDimensions(&width, &height));
   UT_ASSERT_EQ_INT(132, width);
   UT_ASSERT_EQ_INT(50, height);
   UT_ASSERT_EQ_UINT(0, ut_info_calls);
}

static void rejects_a_failed_remote_session_screen(void)
{
   INT width;
   INT height;
   bSessionScreenAvailable = FALSE;
   od_control.baud = 9600;
   nSessionScreenError = ERR_MEMORY;
   ut_info_calls = 0;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionSnapshotDimensions(&width, &height));
   UT_ASSERT_EQ_UINT(0, ut_info_calls);
}

static void falls_back_to_local_window_dimensions(void)
{
   INT width;
   INT height;
   bSessionScreenAvailable = FALSE;
   od_control.baud = 0;
   nSessionScreenError = ERR_MEMORY;
   ut_info_calls = 0;
   UT_ASSERT_EQ_INT(TRUE, utt_ODSessionSnapshotDimensions(&width, &height));
   UT_ASSERT_EQ_INT(10, width);
   UT_ASSERT_EQ_INT(7, height);
   UT_ASSERT_EQ_UINT(1, ut_info_calls);

   od_control.baud = 9600;
   nSessionScreenError = ERR_NONE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODSessionSnapshotDimensions(&width, &height));
   UT_ASSERT_EQ_UINT(2, ut_info_calls);
}

static const UTTestCase ut_cases[] = {
   {"session dimensions", uses_the_session_screen_when_available},
   {"failed remote screen", rejects_a_failed_remote_session_screen},
   {"local dimensions", falls_back_to_local_window_dimensions}
};
