#define UT_CUSTOM_MOCK_ODScrnCreateWin
#define UT_CUSTOM_MOCK_GetStockObject
#define UT_CUSTOM_MOCK_ODScrnSetCurrentFont
#define UT_CUSTOM_MOCK_DialogBoxParamA
#define UT_CUSTOM_MOCK_ODKrnlGetUIState
#define UT_CUSTOM_MOCK_ShowWindow
#define UT_CUSTOM_MOCK_SetFocus

static HANDLE ut_instance = (HANDLE)1;
static HWND ut_frame = (HWND)2;
static HWND ut_screen = (HWND)3;
static HFONT ut_font = (HFONT)4;
static BOOL ut_create_fails;
static INT_PTR ut_dialog_result;
static unsigned ut_font_calls;
static unsigned ut_dialog_calls;
static unsigned ut_show_calls;
static int ut_show_commands[2];
static unsigned ut_focus_calls;
static unsigned ut_state_calls;
static tODUIState ut_state;

HWND utm_ODScrnCreateWin(HWND frame, HANDLE instance)
{
   UT_ASSERT(frame == ut_frame); UT_ASSERT(instance == ut_instance);
   return ut_create_fails ? NULL : ut_screen;
}

HGDIOBJ WINAPI utm_GetStockObject(int object)
{
   UT_ASSERT_EQ_INT(OEM_FIXED_FONT, object); return ut_font;
}

void utm_ODScrnSetCurrentFont(HWND screen, HFONT font)
{
   ++ut_font_calls; UT_ASSERT(screen == ut_screen); UT_ASSERT(font == ut_font);
}

INT_PTR WINAPI utm_DialogBoxParamA(HINSTANCE instance, LPCSTR template_name,
   HWND parent, DLGPROC procedure, LPARAM parameter)
{
   ++ut_dialog_calls; UT_ASSERT(instance == ut_instance);
   UT_ASSERT(template_name == MAKEINTRESOURCE(IDD_LOGIN));
   UT_ASSERT(parent == ut_frame); UT_ASSERT(procedure == utm_ODInitLoginDlgProc);
   UT_ASSERT_EQ_INT(0, parameter); return ut_dialog_result;
}

void utm_ODKrnlGetUIState(tODUIState *state)
{
   ++ut_state_calls;
   *state = ut_state;
}

BOOL WINAPI utm_ShowWindow(HWND window, int command)
{
   UT_ASSERT(ut_show_calls < 2);
   UT_ASSERT(window == (ut_show_calls == 0 ? ut_frame : ut_screen));
   ut_show_commands[ut_show_calls++] = command;
   return TRUE;
}

HWND WINAPI utm_SetFocus(HWND window)
{
   ++ut_focus_calls; UT_ASSERT(window == ut_screen); return window;
}

static void reset_start(void)
{
   ut_create_fails = FALSE; ut_dialog_result = IDOK;
   ut_font_calls = ut_dialog_calls = 0;
   ut_show_calls = ut_focus_calls = ut_state_calls = 0;
   ut_show_commands[0] = ut_show_commands[1] = 0;
   memset(&ut_state, 0, sizeof(ut_state));
   bPromptForUserName = FALSE;
   ut_state.nCmdShow = SW_RESTORE;
}

static void reports_screen_creation_failure(void)
{
   reset_start(); ut_create_fails = TRUE;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODScrnStartWindow(ut_instance, ut_frame));
   UT_ASSERT_EQ_UINT(0, ut_font_calls); UT_ASSERT_EQ_UINT(0, ut_show_calls);
}

static void initializes_and_shows_both_windows_on_the_frame_thread(void)
{
   reset_start();
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODScrnStartWindow(ut_instance, ut_frame));
   UT_ASSERT_EQ_UINT(1, ut_font_calls);
   UT_ASSERT_EQ_UINT(1, ut_state_calls);
   UT_ASSERT_EQ_UINT(2, ut_show_calls);
   UT_ASSERT_EQ_INT(SW_RESTORE, ut_show_commands[0]);
   UT_ASSERT_EQ_INT(SW_SHOW, ut_show_commands[1]);
}

static void preserves_a_requested_minimized_frame_state(void)
{
   static const int commands[] = {
      SW_MINIMIZE, SW_SHOWMINIMIZED, SW_SHOWMINNOACTIVE
   };
   unsigned index;
   for(index = 0; index < DIM(commands); ++index)
   {
      reset_start(); ut_state.nCmdShow = commands[index];
      UT_ASSERT_EQ_INT(kODRCSuccess,
         utt_ODScrnStartWindow(ut_instance, ut_frame));
      UT_ASSERT_EQ_INT(SW_SHOWMINNOACTIVE, ut_show_commands[0]);
   }
}

static void prompts_and_focuses_the_screen_when_requested(void)
{
   reset_start(); bPromptForUserName = TRUE;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODScrnStartWindow(ut_instance, ut_frame));
   UT_ASSERT_EQ_UINT(1, ut_dialog_calls); UT_ASSERT_EQ_UINT(1, ut_focus_calls);
}

static void reports_failure_when_the_prompt_is_cancelled(void)
{
   reset_start(); bPromptForUserName = TRUE; ut_dialog_result = IDCANCEL;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODScrnStartWindow(ut_instance, ut_frame));
   UT_ASSERT_EQ_UINT(1, ut_state_calls);
   UT_ASSERT_EQ_UINT(0, ut_show_calls);
}

static const UTTestCase ut_cases[] = {
   {"creation failure", reports_screen_creation_failure},
   {"show windows", initializes_and_shows_both_windows_on_the_frame_thread},
   {"minimized", preserves_a_requested_minimized_frame_state},
   {"prompt", prompts_and_focuses_the_screen_when_requested},
   {"cancel", reports_failure_when_the_prompt_is_cancelled}
};
