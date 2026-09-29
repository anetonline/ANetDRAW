#define UT_CUSTOM_MOCK_ODScrnPutText
#define UT_CUSTOM_MOCK_free

static BYTE ut_window[12];
static unsigned ut_put_calls;
static unsigned ut_free_calls;

BOOL ODCALL utm_ODScrnPutText(BYTE left, BYTE top, BYTE right, BYTE bottom,
   void *buffer)
{
   ++ut_put_calls;
   UT_ASSERT_EQ_UINT(2, left); UT_ASSERT_EQ_UINT(3, top);
   UT_ASSERT_EQ_UINT(4, right); UT_ASSERT_EQ_UINT(5, bottom);
   UT_ASSERT(buffer == ut_window + 4);
   return FALSE;
}

void utm_free(void *allocation)
{
   ++ut_free_calls;
   UT_ASSERT(allocation == ut_window);
}

static void restores_the_saved_rectangle_and_releases_it(void)
{
   ut_window[0] = 2; ut_window[1] = 3;
   ut_window[2] = 4; ut_window[3] = 5;
   ut_put_calls = ut_free_calls = 0;
   utt_ODScrnDestroyWindow(ut_window);
   UT_ASSERT_EQ_UINT(1, ut_put_calls);
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
}

static const UTTestCase ut_cases[] = {
   {"restore window", restores_the_saved_rectangle_and_releases_it}
};
