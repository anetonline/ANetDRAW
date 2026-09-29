#ifdef _MSC_VER
#define UT_CUSTOM_MOCK__InterlockedExchange
#else
#define UT_CUSTOM_MOCK_InterlockedExchange
#endif
#define UT_CUSTOM_MOCK_PostMessageA

static unsigned ut_post_calls;
static BOOL ut_post_result;

#ifdef _MSC_VER
LONG utm__InterlockedExchange(LONG volatile *pValue, LONG value)
#else
LONG WINAPI utm_InterlockedExchange(LONG volatile *pValue, LONG value)
#endif
{
   LONG old = *pValue;
   *pValue = value;
   return(old);
}

BOOL WINAPI utm_PostMessageA(HWND hwnd, UINT message, WPARAM wParam,
   LPARAM lParam)
{
   UT_ASSERT_EQ_PTR(hwndCurrentFrame, hwnd);
   UT_ASSERT_EQ_UINT(WM_OD_CONTROL_STATE, message);
   UT_ASSERT_EQ_UINT(0, wParam);
   UT_ASSERT_EQ_INT(0, lParam);
   ++ut_post_calls;
   return(ut_post_result);
}

static void coalesces_state_notifications(void)
{
   hwndCurrentFrame = NULL;
   lControlStateDirty = 0;
   ut_post_calls = 0;
   ut_post_result = TRUE;
   utt_ODFrameControlStateChanged();
   UT_ASSERT_EQ_UINT(0, ut_post_calls);

   hwndCurrentFrame = (HWND)(UINT_PTR)1;
   utt_ODFrameControlStateChanged();
   utt_ODFrameControlStateChanged();
   UT_ASSERT_EQ_UINT(1, ut_post_calls);
   UT_ASSERT_EQ_INT(1, lControlStateDirty);

   lControlStateDirty = 0;
   ut_post_result = FALSE;
   utt_ODFrameControlStateChanged();
   UT_ASSERT_EQ_UINT(2, ut_post_calls);
   UT_ASSERT_EQ_INT(0, lControlStateDirty);
}

static const UTTestCase ut_cases[] = {
   {"coalesced notification", coalesces_state_notifications}
};
