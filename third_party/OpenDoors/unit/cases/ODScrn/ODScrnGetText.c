#define UT_CUSTOM_MOCK_ODSessionScreenIsEmulating
#define UT_CUSTOM_MOCK_ODSessionScreenGetText
static BOOL ut_emulating;
static BOOL ut_session_result;
static WORD ut_screen[2000];
static unsigned ut_session_calls;
BOOL utm_ODSessionScreenIsEmulating(void) { return ut_emulating; }
BOOL utm_ODSessionScreenGetText(INT left, INT top, INT right, INT bottom, void *buffer)
{
   ++ut_session_calls;
   UT_ASSERT_EQ_INT(2, left); UT_ASSERT_EQ_INT(2, top);
   UT_ASSERT_EQ_INT(3, right); UT_ASSERT_EQ_INT(3, bottom);
   UT_ASSERT(buffer != NULL); return ut_session_result;
}
static void forwards_or_copies_interleaved_words(void)
{
   WORD output[4];
   pScrnBuffer = ut_screen; btLeftBoundary = 1; btTopBoundary = 1;
   btRightBoundary = 5; btBottomBoundary = 5;
   ut_screen[162] = 0x1111; ut_screen[163] = 0x2222;
   ut_screen[242] = 0x3333; ut_screen[243] = 0x4444;
   ut_session_calls = 0; ut_emulating = TRUE; ut_session_result = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODScrnGetText(2, 2, 3, 3, output));
   ut_session_result = TRUE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODScrnGetText(2, 2, 3, 3, output));
   UT_ASSERT_EQ_UINT(2, ut_session_calls);
   ut_emulating = FALSE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODScrnGetText(2, 2, 3, 3, output));
   UT_ASSERT_EQ_UINT(0x1111, output[0]); UT_ASSERT_EQ_UINT(0x2222, output[1]);
   UT_ASSERT_EQ_UINT(0x3333, output[2]); UT_ASSERT_EQ_UINT(0x4444, output[3]);
}
static const UTTestCase ut_cases[] = {{"get text", forwards_or_copies_interleaved_words}};
