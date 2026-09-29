#define UT_CUSTOM_MOCK_ODSessionScreenIsEmulating
#define UT_CUSTOM_MOCK_ODSessionScreenGetInfo
static BOOL ut_emulating;
static tODVScreenInfo ut_session_info;
BOOL utm_ODSessionScreenIsEmulating(void) { return ut_emulating; }
void utm_ODSessionScreenGetInfo(tODVScreenInfo *info) { *info = ut_session_info; }
static void clamps_emulated_values_to_the_legacy_byte_surface(void)
{
   tODScrnTextInfo info;
   memset(&ut_session_info, 0, sizeof(ut_session_info));
   ut_emulating = TRUE;
   ut_session_info.wintop = ut_session_info.winleft = 256;
   ut_session_info.winright = ut_session_info.winbottom = 300;
   ut_session_info.curx = ut_session_info.cury = 500;
   ut_session_info.attribute = 0x2e;
   utt_ODScrnGetTextInfo(&info);
   UT_ASSERT_EQ_UINT(255, info.wintop); UT_ASSERT_EQ_UINT(255, info.winleft);
   UT_ASSERT_EQ_UINT(255, info.winright); UT_ASSERT_EQ_UINT(255, info.winbottom);
   UT_ASSERT_EQ_UINT(255, info.curx); UT_ASSERT_EQ_UINT(255, info.cury);
   UT_ASSERT_EQ_UINT(0x2e, info.attribute);
   ut_session_info.wintop = 1; ut_session_info.winleft = 2;
   ut_session_info.winright = 3; ut_session_info.winbottom = 4;
   ut_session_info.curx = 5; ut_session_info.cury = 6;
   utt_ODScrnGetTextInfo(&info);
   UT_ASSERT_EQ_UINT(1, info.wintop); UT_ASSERT_EQ_UINT(2, info.winleft);
   UT_ASSERT_EQ_UINT(3, info.winright); UT_ASSERT_EQ_UINT(4, info.winbottom);
   UT_ASSERT_EQ_UINT(5, info.curx); UT_ASSERT_EQ_UINT(6, info.cury);
}
static void returns_fixed_local_screen_state(void)
{
   tODScrnTextInfo info;
   ut_emulating = FALSE;
   btTopBoundary = 2; btLeftBoundary = 3; btRightBoundary = 8; btBottomBoundary = 9;
   btCurrentAttribute = 0x17; btCursorColumn = 4; btCursorRow = 5;
   utt_ODScrnGetTextInfo(&info);
   UT_ASSERT_EQ_UINT(3, info.wintop); UT_ASSERT_EQ_UINT(4, info.winleft);
   UT_ASSERT_EQ_UINT(9, info.winright); UT_ASSERT_EQ_UINT(10, info.winbottom);
   UT_ASSERT_EQ_UINT(0x17, info.attribute);
   UT_ASSERT_EQ_UINT(5, info.curx); UT_ASSERT_EQ_UINT(6, info.cury);
}
static const UTTestCase ut_cases[] = {
   {"emulated info", clamps_emulated_values_to_the_legacy_byte_surface},
   {"local info", returns_fixed_local_screen_state}
};
