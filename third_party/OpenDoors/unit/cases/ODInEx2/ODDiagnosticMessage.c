#define UT_CUSTOM_MOCK_ODSyncAPILevelActive
#define UT_CUSTOM_MOCK_ODSyncAPIRelease
#define UT_CUSTOM_MOCK_ODSyncAPIReacquire
#define UT_CUSTOM_MOCK_MessageBoxA
static BOOL ut_api_active;
static unsigned ut_release_calls;
static unsigned ut_reacquire_calls;
static unsigned ut_message_calls;
BOOL utm_ODSyncAPILevelActive(void) { return(ut_api_active); }
unsigned utm_ODSyncAPIRelease(void) { ++ut_release_calls; return(3); }
void utm_ODSyncAPIReacquire(unsigned level)
{
   UT_ASSERT_EQ_UINT(3, level); ++ut_reacquire_calls;
}
int WINAPI utm_MessageBoxA(HWND window, LPCSTR text, LPCSTR title, UINT type)
{
   UT_ASSERT_NULL(window); UT_ASSERT(strcmp(text, "diagnostic") == 0);
   UT_ASSERT(strcmp(title, "title") == 0); UT_ASSERT_EQ_UINT(MB_OK, type);
   ++ut_message_calls; return(IDOK);
}

static void displays_with_and_without_releasing_the_api_writer(void)
{
   ut_api_active = FALSE; ut_release_calls = ut_reacquire_calls = 0;
   ut_message_calls = 0;
   utt_ODDiagnosticMessage("diagnostic", "title");
   UT_ASSERT_EQ_UINT(1, ut_message_calls);
   UT_ASSERT_EQ_UINT(0, ut_release_calls); UT_ASSERT_EQ_UINT(0, ut_reacquire_calls);
   ut_api_active = TRUE;
   utt_ODDiagnosticMessage("diagnostic", "title");
   UT_ASSERT_EQ_UINT(1, ut_release_calls); UT_ASSERT_EQ_UINT(1, ut_reacquire_calls);
}

static const UTTestCase ut_cases[] = {
   {"diagnostic message", displays_with_and_without_releasing_the_api_writer}
};
