#define UT_CUSTOM_MOCK_ODScrnGetTextInfo
static unsigned ut_info_calls;

void utm_ODScrnGetTextInfo(tODScrnTextInfo *information)
{
   ++ut_info_calls;
   UT_ASSERT_NOT_NULL(information);
   information->cury = 23;
   information->curx = 79;
}

static void obtains_the_screen_cursor_position(void)
{
   tEditInstance instance;
   UINT row = 0;
   UINT column = 0;

   ut_info_calls = 0;
   utt_ODEditGetActualCurPos(&instance, &row, &column);
   UT_ASSERT_EQ_UINT(23, row);
   UT_ASSERT_EQ_UINT(79, column);
   UT_ASSERT_EQ_UINT(1, ut_info_calls);
}

static const UTTestCase ut_cases[] = {
   {"screen cursor", obtains_the_screen_cursor_position}
};
