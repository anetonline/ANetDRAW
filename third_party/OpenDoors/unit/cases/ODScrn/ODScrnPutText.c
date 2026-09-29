#define UT_CUSTOM_MOCK_ODSessionScreenIsEmulating
#define UT_CUSTOM_MOCK_ODSessionScreenPutText
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODScrnInvalidate
#endif
static BOOL ut_emulating;
static BOOL ut_session_result;
static WORD ut_screen[2000];
static unsigned ut_session_calls;
static unsigned ut_invalidate_calls;
BOOL utm_ODSessionScreenIsEmulating(void) { return ut_emulating; }
BOOL utm_ODSessionScreenPutText(INT left, INT top, INT right, INT bottom,
   const void *buffer)
{
   ++ut_session_calls;
   UT_ASSERT_EQ_INT(2, left); UT_ASSERT_EQ_INT(2, top);
   UT_ASSERT_EQ_INT(3, right); UT_ASSERT_EQ_INT(3, bottom);
   UT_ASSERT(buffer != NULL); return ut_session_result;
}
#ifdef ODPLAT_WIN32
void utm_ODScrnInvalidate(BYTE left, BYTE top, BYTE right, BYTE bottom)
{
   ++ut_invalidate_calls;
   UT_ASSERT_EQ_UINT(2, left); UT_ASSERT_EQ_UINT(2, top);
   UT_ASSERT_EQ_UINT(3, right); UT_ASSERT_EQ_UINT(3, bottom);
}
#endif
static void forwards_or_copies_interleaved_words(void)
{
   WORD input[4];
   unsigned index;
   input[0] = 0x1111; input[1] = 0x2222; input[2] = 0x3333; input[3] = 0x4444;
   for(index = 0; index < DIM(ut_screen); ++index) ut_screen[index] = 0x5555;
   pScrnBuffer = ut_screen; btLeftBoundary = 1; btTopBoundary = 1;
   btRightBoundary = 5; btBottomBoundary = 5;
   ut_session_calls = ut_invalidate_calls = 0; ut_emulating = TRUE;
   ut_session_result = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODScrnPutText(2, 2, 3, 3, input));
   ut_session_result = TRUE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODScrnPutText(2, 2, 3, 3, input));
   ut_emulating = FALSE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODScrnPutText(2, 2, 3, 3, input));
   UT_ASSERT_EQ_UINT(0x1111, ut_screen[162]); UT_ASSERT_EQ_UINT(0x2222, ut_screen[163]);
   UT_ASSERT_EQ_UINT(0x3333, ut_screen[242]); UT_ASSERT_EQ_UINT(0x4444, ut_screen[243]);
   UT_ASSERT_EQ_UINT(0x5555, ut_screen[164]);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(1, ut_invalidate_calls);
#endif
}
static const UTTestCase ut_cases[] = {{"put text", forwards_or_copies_interleaved_words}};
