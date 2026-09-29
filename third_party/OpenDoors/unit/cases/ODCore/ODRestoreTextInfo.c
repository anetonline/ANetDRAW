#define UT_CUSTOM_MOCK_ODScrnSetAttribute
#define UT_CUSTOM_MOCK_ODScrnSetBoundary
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos

static unsigned ut_boundary_calls;
static unsigned ut_attribute_calls;
static unsigned ut_cursor_calls;

void utm_ODScrnSetBoundary(BYTE left, BYTE top, BYTE right, BYTE bottom)
{
   ++ut_boundary_calls;
   UT_ASSERT_EQ_UINT(2, left);
   UT_ASSERT_EQ_UINT(3, top);
   UT_ASSERT_EQ_UINT(79, right);
   UT_ASSERT_EQ_UINT(24, bottom);
}

void ODCALL utm_ODScrnSetAttribute(BYTE attribute)
{
   ++ut_attribute_calls;
   UT_ASSERT_EQ_UINT(0x1e, attribute);
}

void ODCALL utm_ODScrnSetCursorPos(BYTE column, BYTE row)
{
   ++ut_cursor_calls;
   UT_ASSERT_EQ_UINT(12, column);
   UT_ASSERT_EQ_UINT(9, row);
}

static void restores_every_saved_text_setting(void)
{
   ODTextInfo.winleft = 2;
   ODTextInfo.wintop = 3;
   ODTextInfo.winright = 79;
   ODTextInfo.winbottom = 24;
   ODTextInfo.attribute = 0x1e;
   ODTextInfo.curx = 12;
   ODTextInfo.cury = 9;
   ut_boundary_calls = 0;
   ut_attribute_calls = 0;
   ut_cursor_calls = 0;
   utt_ODRestoreTextInfo();
   UT_ASSERT_EQ_UINT(1, ut_boundary_calls);
   UT_ASSERT_EQ_UINT(1, ut_attribute_calls);
   UT_ASSERT_EQ_UINT(1, ut_cursor_calls);
}

static const UTTestCase ut_cases[] = {
   {"restore text info", restores_every_saved_text_setting}
};
