#define UT_CUSTOM_MOCK_EndDialog
#define UT_CUSTOM_MOCK_GetDlgItem
#define UT_CUSTOM_MOCK_GetWindowTextA
#define UT_CUSTOM_MOCK_ODFrameCenterWindowInParent
#define UT_CUSTOM_MOCK_ODKrnlGetUIState
#define UT_CUSTOM_MOCK_ODStringCopy
#define UT_CUSTOM_MOCK_SendMessageA
#define UT_CUSTOM_MOCK_SetWindowTextA

enum
{
   MOCK_GET_STATE = 1,
   MOCK_COPY,
   MOCK_CENTER,
   MOCK_SET_TEXT,
   MOCK_GET_ITEM,
   MOCK_SEND,
   MOCK_GET_TEXT,
   MOCK_END
};

static int ut_dialog_token;
static int ut_control_token;
static HWND ut_dialog = (HWND)&ut_dialog_token;
static HWND ut_control = (HWND)&ut_control_token;
static const char *ut_entered_name;
static unsigned ut_copy_count;
static char *ut_copy_destinations[2];
static char ut_copy_sources[2][80];
static INT ut_copy_sizes[2];
static const char *ut_window_texts[2];
static unsigned ut_window_text_count;
static tODUIState ut_state;

void utm_ODKrnlGetUIState(tODUIState *state)
{
   ut_mock_called(MOCK_GET_STATE);
   *state = ut_state;
}

void utm_ODStringCopy(char *destination, const char *source, INT size)
{
   INT index;
   UT_ASSERT(ut_copy_count < 2);
   ut_copy_destinations[ut_copy_count] = destination;
   strcpy(ut_copy_sources[ut_copy_count], source);
   ut_copy_sizes[ut_copy_count] = size;
   ++ut_copy_count;
   ut_mock_called(MOCK_COPY);
   if(size <= 0)
      return;
   index = 0;
   while(index < size - 1 && source[index] != '\0')
   {
      destination[index] = source[index];
      ++index;
   }
   destination[index] = '\0';
}

void utm_ODFrameCenterWindowInParent(HWND window)
{
   ut_mock_called(MOCK_CENTER);
   UT_ASSERT_EQ_PTR(ut_dialog, window);
}

HWND WINAPI utm_GetDlgItem(HWND dialog, int identifier)
{
   ut_mock_called(MOCK_GET_ITEM);
   UT_ASSERT_EQ_PTR(ut_dialog, dialog);
   UT_ASSERT_EQ_INT(IDC_USER_NAME, identifier);
   return ut_control;
}

BOOL WINAPI utm_SetWindowTextA(HWND window, LPCSTR text)
{
   ut_mock_called(MOCK_SET_TEXT);
   UT_ASSERT(ut_window_text_count < 2);
   ut_window_texts[ut_window_text_count++] = text;
   if(ut_window_text_count == 1)
      UT_ASSERT_EQ_PTR(ut_dialog, window);
   else
      UT_ASSERT_EQ_PTR(ut_control, window);
   return TRUE;
}

LRESULT WINAPI utm_SendMessageA(HWND window, UINT message, WPARAM first,
   LPARAM second)
{
   ut_mock_called(MOCK_SEND);
   UT_ASSERT_EQ_PTR(ut_control, window);
   UT_ASSERT_EQ_UINT(EM_LIMITTEXT, message);
   UT_ASSERT_EQ_UINT(sizeof(od_control.user_name), first);
   UT_ASSERT_EQ_INT(0, second);
   return 0;
}

int WINAPI utm_GetWindowTextA(HWND window, LPSTR destination, int size)
{
   int index = 0;
   ut_mock_called(MOCK_GET_TEXT);
   UT_ASSERT_EQ_PTR(ut_control, window);
   UT_ASSERT_EQ_UINT(sizeof(od_control.user_name), size);
   while(index < size - 1 && ut_entered_name[index] != '\0')
   {
      destination[index] = ut_entered_name[index];
      ++index;
   }
   destination[index] = '\0';
   return index;
}

BOOL WINAPI utm_EndDialog(HWND dialog, INT_PTR result)
{
   ut_mock_called(MOCK_END);
   UT_ASSERT_EQ_PTR(ut_dialog, dialog);
   UT_ASSERT(result == IDOK || result == IDCANCEL);
   return TRUE;
}

static void reset_dialog_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
   memset(&ut_state, 0, sizeof(ut_state));
   memset(ut_copy_destinations, 0, sizeof(ut_copy_destinations));
   memset(ut_copy_sources, 0, sizeof(ut_copy_sources));
   memset(ut_copy_sizes, 0, sizeof(ut_copy_sizes));
   memset(ut_window_texts, 0, sizeof(ut_window_texts));
   ut_copy_count = 0;
   ut_window_text_count = 0;
   ut_entered_name = "Entered Name";
   strcpy(szWindowsStartupUserName, "Default user");
   bWindowsStartupCancelled = FALSE;
}

static void initializes_the_dialog_from_the_ui_state_cache(void)
{
   INT_PTR result;
   reset_dialog_fixture();
   strcpy(ut_state.szProgramName, "Door title");
   result = utt_ODInitLoginDlgProc(ut_dialog, WM_INITDIALOG, 0, 0);
   UT_ASSERT_EQ_INT(TRUE, result);
   UT_ASSERT_EQ_UINT(1, ut_copy_count);
   UT_ASSERT(strcmp("Door title", ut_copy_sources[0]) == 0);
   UT_ASSERT_EQ_UINT(sizeof(od_control.od_prog_name), ut_copy_sizes[0]);
   UT_ASSERT(strcmp("Door title", ut_window_texts[0]) == 0);
   UT_ASSERT(strcmp("Default user", ut_window_texts[1]) == 0);
   UT_ASSERT_EQ_UINT(8, ut_mock_call_count);
   UT_ASSERT_EQ_UINT(MOCK_GET_STATE, ut_mock_calls[0]);
   UT_ASSERT_EQ_UINT(MOCK_CENTER, ut_mock_calls[2]);
   UT_ASSERT_EQ_UINT(MOCK_SEND, ut_mock_calls[7]);
}

static void accepts_the_entered_name_for_owner_side_initialization(void)
{
   INT_PTR result;
   reset_dialog_fixture();
   result = utt_ODInitLoginDlgProc(ut_dialog, WM_COMMAND, IDOK, 0);
   UT_ASSERT_EQ_INT(TRUE, result);
   UT_ASSERT(strcmp("Entered Name", szWindowsStartupUserName) == 0);
   UT_ASSERT_EQ_UINT(0, ut_copy_count);
   UT_ASSERT_EQ_UINT(3, ut_mock_call_count);
   UT_ASSERT_EQ_UINT(MOCK_GET_ITEM, ut_mock_calls[0]);
   UT_ASSERT_EQ_UINT(MOCK_GET_TEXT, ut_mock_calls[1]);
   UT_ASSERT_EQ_UINT(MOCK_END, ut_mock_calls[2]);
}

static void closes_on_cancel(void)
{
   reset_dialog_fixture();
   UT_ASSERT_EQ_INT(TRUE,
      utt_ODInitLoginDlgProc(ut_dialog, WM_COMMAND, IDCANCEL, 0));
   UT_ASSERT(bWindowsStartupCancelled);
   UT_ASSERT_EQ_UINT(1, ut_mock_call_count);
   UT_ASSERT_EQ_UINT(MOCK_END, ut_mock_calls[0]);
}

static void consumes_an_unknown_command_without_side_effects(void)
{
   reset_dialog_fixture();
   UT_ASSERT_EQ_INT(TRUE,
      utt_ODInitLoginDlgProc(ut_dialog, WM_COMMAND, 12345, 0));
   UT_ASSERT_EQ_UINT(0, ut_mock_call_count);
}

static void declines_an_unknown_message(void)
{
   reset_dialog_fixture();
   UT_ASSERT_EQ_INT(FALSE,
      utt_ODInitLoginDlgProc(ut_dialog, WM_USER, 0, 0));
   UT_ASSERT_EQ_UINT(0, ut_mock_call_count);
}

static const UTTestCase ut_cases[] = {
   {"initialize", initializes_the_dialog_from_the_ui_state_cache},
   {"accept", accepts_the_entered_name_for_owner_side_initialization},
   {"cancel", closes_on_cancel},
   {"unknown command", consumes_an_unknown_command_without_side_effects},
   {"unknown message", declines_an_unknown_message}
};
