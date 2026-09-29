#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_ODEmulatePathExtension
#define UT_CUSTOM_MOCK_ODEmulateAutoNameFits
#define UT_CUSTOM_MOCK_ODEmulateFindCompatFile
#define UT_CUSTOM_MOCK_fopen
#define UT_CUSTOM_MOCK_ODEmulateExtensionIsRIP
#define UT_CUSTOM_MOCK_ODEmulateBuildRIPMessage
#define UT_CUSTOM_MOCK_ODScrnShowMessage
#define UT_CUSTOM_MOCK_od_get_key
#define UT_CUSTOM_MOCK_tolower
#define UT_CUSTOM_MOCK_od_clear_keybuffer
#define UT_CUSTOM_MOCK_fgets
#define UT_CUSTOM_MOCK_ODEmulateFromBuffer
#define UT_CUSTOM_MOCK_ODPagePrompt
#define UT_CUSTOM_MOCK_od_disp
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_fclose
#define UT_CUSTOM_MOCK_ODWaitDrain
#define UT_CUSTOM_MOCK_ODScrnRemoveMessage
#define UT_CUSTOM_MOCK_od_kernel
#define UT_CUSTOM_MOCK_ODInQueueExchangeLastControlKey

#define UT_SEND_MAX_EVENTS 20
typedef struct UTReadEvent {
   FILE *stream;
   const char *text;
} UTReadEvent;

static char ut_remote_token, ut_local_token, ut_window_token;
static unsigned ut_init_calls, ut_entry_calls, ut_exit_calls;
static const char *ut_extension;
static BOOL ut_auto_fits;
static FILE *ut_find_results[2];
static INT ut_find_levels[2];
static unsigned ut_find_count, ut_find_index;
static FILE *ut_open_result;
static unsigned ut_open_calls;
static BOOL ut_is_rip;
static BOOL ut_show_returns_window;
static unsigned ut_message_build_calls, ut_show_calls, ut_remove_calls;
static UTReadEvent ut_reads[UT_SEND_MAX_EVENTS];
static unsigned ut_read_count, ut_read_index;
static char ut_keys[4];
static unsigned ut_key_count, ut_key_index;
static unsigned ut_clear_key_calls;
static unsigned ut_emulate_calls;
static char ut_control_after_emulate;
static BOOL ut_emulate_remote[UT_SEND_MAX_EVENTS];
static BOOL ut_emulate_session[UT_SEND_MAX_EVENTS];
static unsigned ut_page_calls;
static BOOL ut_page_result;
static unsigned ut_disp_calls;
static unsigned ut_close_calls;
static FILE *ut_closed[2];
static unsigned ut_wait_calls;
static BOOL ut_uninitialize_on_wait;
static unsigned ut_kernel_calls;
static char ut_last_control_key;
static BOOL ut_init_succeeds;

size_t utm_strlen(const char *text);
void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}
void utm_ODSyncAPIEntry(void) { ++ut_entry_calls; }
void utm_ODSyncAPIExit(void) { ++ut_exit_calls; }

static const char *utm_ODEmulatePathExtension(const char *path)
{
   UT_ASSERT(path != NULL);
   return(ut_extension);
}
static BOOL utm_ODEmulateAutoNameFits(const char *path)
{
   UT_ASSERT(path != NULL);
   return(ut_auto_fits);
}
static FILE *utm_ODEmulateFindCompatFile(const char *base, INT *level)
{
   UT_ASSERT(base != NULL);
   UT_ASSERT(level != NULL);
   UT_ASSERT(ut_find_index < ut_find_count);
   *level = ut_find_levels[ut_find_index];
   return(ut_find_results[ut_find_index++]);
}
FILE *utm_fopen(const char *path, const char *mode)
{
   UT_ASSERT(path != NULL);
   UT_ASSERT(strcmp("rb", mode) == 0);
   ++ut_open_calls;
   return(ut_open_result);
}
static BOOL utm_ODEmulateExtensionIsRIP(const char *extension)
{
   UT_ASSERT_EQ_PTR(ut_extension, extension);
   return(ut_is_rip);
}
static void utm_ODEmulateBuildRIPMessage(char *message, INT size,
   const char *name)
{
   UT_ASSERT(message != NULL);
   UT_ASSERT_EQ_INT(74, size);
   UT_ASSERT(name != NULL);
   message[0] = 'R'; message[1] = '\0';
   ++ut_message_build_calls;
}
void *utm_ODScrnShowMessage(char *text, int flags)
{
   UT_ASSERT(strcmp("R", text) == 0);
   UT_ASSERT_EQ_INT(0, flags);
   ++ut_show_calls;
   return(ut_show_returns_window ? (void *)&ut_window_token : NULL);
}
char ODCALL utm_od_get_key(BOOL wait)
{
   char value;
   if(ut_key_index == ut_key_count)
      return(0);
   value = ut_keys[ut_key_index++];
   if(wait && ut_uninitialize_on_wait)
      bODInitialized = FALSE;
   return(value);
}
int utm_tolower(int value)
{
   UT_ASSERT_EQ_INT((int)(unsigned char)value, value);
   if(value >= 'A' && value <= 'Z') return(value - 'A' + 'a');
   return(value);
}
void ODCALL utm_od_clear_keybuffer(void) { ++ut_clear_key_calls; }

static void add_read(FILE *stream, const char *text)
{
   UT_ASSERT(ut_read_count < UT_SEND_MAX_EVENTS);
   ut_reads[ut_read_count].stream = stream;
   ut_reads[ut_read_count].text = text;
   ++ut_read_count;
}
char *utm_fgets(char *buffer, int size, FILE *stream)
{
   const char *text;
   unsigned index = 0;
   UT_ASSERT(buffer != NULL);
   UT_ASSERT_EQ_INT(OD_GLOBAL_WORK_STRING_SIZE - 1, size);
   UT_ASSERT(ut_read_index < ut_read_count);
   UT_ASSERT_EQ_PTR(ut_reads[ut_read_index].stream, stream);
   text = ut_reads[ut_read_index++].text;
   if(text == NULL) return(NULL);
   while(text[index] != '\0') {
      buffer[index] = text[index];
      ++index;
   }
   buffer[index] = '\0';
   return(buffer);
}
static void utm_ODEmulateFromBuffer(const char *text, BOOL remote,
   BOOL session)
{
   UT_ASSERT(text != NULL);
   UT_ASSERT(ut_emulate_calls < UT_SEND_MAX_EVENTS);
   ut_emulate_remote[ut_emulate_calls] = remote;
   ut_emulate_session[ut_emulate_calls] = session;
   ++ut_emulate_calls;
   if(ut_control_after_emulate != 0)
      ut_last_control_key = ut_control_after_emulate;
}
char utm_ODInQueueExchangeLastControlKey(tODInQueueHandle queue,
   char replacement)
{
   char previous;
   UT_ASSERT_EQ_PTR(hODInputQueue, queue);
   previous = ut_last_control_key;
   ut_last_control_key = replacement;
   return(previous);
}
BOOL utm_ODPagePrompt(BOOL *pausing)
{
   UT_ASSERT(pausing != NULL);
   ++ut_page_calls;
   return(ut_page_result);
}
void ODCALL utm_od_disp(const char *buffer, INT size, BOOL local)
{
   UT_ASSERT(buffer != NULL);
   UT_ASSERT_EQ_INT((INT)utm_strlen(buffer), size);
   UT_ASSERT(!local);
   ++ut_disp_calls;
}
size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return(length);
}
int utm_fclose(FILE *stream)
{
   UT_ASSERT(ut_close_calls < 2);
   ut_closed[ut_close_calls++] = stream;
   return(0);
}
void utm_ODWaitDrain(tODMilliSec timeout)
{
   UT_ASSERT_EQ_UINT(OD_NO_TIMEOUT, timeout);
   ++ut_wait_calls;
   if(ut_uninitialize_on_wait) bODInitialized = FALSE;
}
void utm_ODScrnRemoveMessage(void *window)
{
   UT_ASSERT_EQ_PTR(&ut_window_token, window);
   ++ut_remove_calls;
}
void ODCALL utm_od_kernel(void) { ++ut_kernel_calls; }

static void reset_send(void)
{
   unsigned index;
   bODInitialized = TRUE;
   ut_init_calls = ut_entry_calls = ut_exit_calls = 0;
   ut_extension = (const char *)".ans";
   ut_auto_fits = TRUE;
   ut_find_count = ut_find_index = 0;
   ut_open_result = (FILE *)&ut_remote_token;
   ut_open_calls = 0;
   ut_is_rip = FALSE;
   ut_show_returns_window = TRUE;
   ut_message_build_calls = ut_show_calls = ut_remove_calls = 0;
   ut_read_count = ut_read_index = 0;
   ut_key_count = ut_key_index = 0;
   ut_clear_key_calls = 0;
   ut_emulate_calls = ut_page_calls = ut_disp_calls = 0;
   ut_control_after_emulate = 0;
   ut_page_result = FALSE;
   ut_close_calls = ut_wait_calls = 0;
   ut_uninitialize_on_wait = FALSE;
   ut_kernel_calls = 0;
   pszCurrentHotkeys = NULL;
   chHotkeyPressed = 0;
   ut_last_control_key = 0;
   od_control.od_error = 0;
   od_control.od_page_pausing = FALSE;
   od_control.od_list_stop = FALSE;
   od_control.od_list_pause = FALSE;
   od_control.baud = 0;
   od_control.user_screen_length = 24;
   od_control.od_no_ra_codes = FALSE;
   ut_init_succeeds = TRUE;
   for(index = 0; index < 2; ++index) {
      ut_find_results[index] = NULL;
      ut_find_levels[index] = LEVEL_NONE;
      ut_closed[index] = NULL;
   }
}

#ifndef UT_SEND_FILE_SUPPORT_ONLY
static void terminal_session_is_rejected(void)
{
   reset_send(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT(!utt_od_send_file(NULL));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entry_calls);
}

static void rejects_invalid_names_and_failed_opens(void)
{
   reset_send();
   bODInitialized = FALSE;
   UT_ASSERT(!utt_od_send_file(NULL));
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);

   reset_send();
   ut_extension = NULL;
   ut_auto_fits = FALSE;
   UT_ASSERT(!utt_od_send_file("too-long"));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);

   reset_send();
   ut_extension = NULL;
   ut_find_count = 1;
   UT_ASSERT(!utt_od_send_file("missing"));
   UT_ASSERT_EQ_INT(ERR_FILEOPEN, od_control.od_error);

   reset_send();
   ut_extension = NULL;
   ut_find_count = 1;
   ut_find_results[0] = (FILE *)&ut_remote_token;
   ut_find_levels[0] = LEVEL_NONE;
   UT_ASSERT(!utt_od_send_file("none"));
   UT_ASSERT_EQ_INT(ERR_FILEOPEN, od_control.od_error);

   reset_send();
   ut_open_result = NULL;
   UT_ASSERT(!utt_od_send_file("missing.ans"));
   UT_ASSERT_EQ_INT(ERR_FILEOPEN, od_control.od_error);
}

static void sends_an_explicit_local_file_to_eof(void)
{
   reset_send();
   add_read((FILE *)&ut_remote_token, "line\n");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file("screen.ans"));
   UT_ASSERT_EQ_UINT(1, ut_emulate_calls);
   UT_ASSERT(ut_emulate_remote[0]);
   UT_ASSERT(ut_emulate_session[0]);
   UT_ASSERT_EQ_UINT(1, ut_close_calls);
   UT_ASSERT_EQ_PTR(&ut_remote_token, ut_closed[0]);
   UT_ASSERT_EQ_UINT(1, ut_exit_calls);
   UT_ASSERT(ut_kernel_calls >= 1);
}

static void handles_auto_selected_remote_and_local_files(void)
{
   reset_send();
   ut_extension = NULL;
   ut_find_count = 2;
   ut_find_results[0] = (FILE *)&ut_remote_token;
   ut_find_levels[0] = LEVEL_RIP;
   ut_find_results[1] = (FILE *)&ut_local_token;
   ut_find_levels[1] = LEVEL_ANSI;
   add_read((FILE *)&ut_remote_token, "remote 1\n");
   add_read((FILE *)&ut_local_token, "local 1\n");
   add_read((FILE *)&ut_remote_token, NULL);
   add_read((FILE *)&ut_local_token, "local 2\n");
   add_read((FILE *)&ut_local_token, NULL);
   UT_ASSERT(utt_od_send_file("screen"));
   UT_ASSERT_EQ_UINT(2, ut_emulate_calls);
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
   UT_ASSERT_EQ_UINT(2, ut_close_calls);

   reset_send();
   ut_extension = NULL;
   ut_find_count = 2;
   ut_find_results[0] = (FILE *)&ut_remote_token;
   ut_find_levels[0] = LEVEL_RIP;
   ut_find_results[1] = (FILE *)&ut_local_token;
   ut_find_levels[1] = LEVEL_ASCII;
   add_read((FILE *)&ut_remote_token, "remote 1\n");
   add_read((FILE *)&ut_local_token, NULL);
   add_read((FILE *)&ut_remote_token, "remote 2\n");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file("screen"));
   UT_ASSERT_EQ_UINT(2, ut_disp_calls);
   UT_ASSERT_EQ_UINT(2, ut_close_calls);

   reset_send();
   ut_extension = NULL;
   ut_find_count = 1;
   ut_find_results[0] = (FILE *)&ut_remote_token;
   ut_find_levels[0] = LEVEL_ANSI;
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file("screen"));
   UT_ASSERT_EQ_UINT(1, ut_find_index);
}

static void sends_rip_without_local_emulation_and_drains_remote_output(void)
{
   reset_send();
   ut_is_rip = TRUE;
   add_read((FILE *)&ut_remote_token, "rip data");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file("screen.rip"));
   UT_ASSERT_EQ_UINT(1, ut_message_build_calls);
   UT_ASSERT_EQ_UINT(1, ut_show_calls);
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
   UT_ASSERT_EQ_UINT(0, ut_wait_calls);
   UT_ASSERT_EQ_UINT(1, ut_remove_calls);

   reset_send();
   ut_extension = NULL;
   ut_find_count = 2;
   ut_find_results[0] = (FILE *)&ut_remote_token;
   ut_find_levels[0] = LEVEL_RIP;
   ut_find_results[1] = NULL;
   ut_find_levels[1] = LEVEL_NONE;
   od_control.baud = 9600L;
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file("screen"));
   UT_ASSERT_EQ_UINT(1, ut_wait_calls);
   UT_ASSERT_EQ_UINT(1, ut_remove_calls);

   reset_send();
   ut_is_rip = TRUE;
   od_control.baud = 9600L;
   ut_uninitialize_on_wait = TRUE;
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file("screen.rip"));
   UT_ASSERT_EQ_UINT(1, ut_wait_calls);
   UT_ASSERT_EQ_UINT(0, ut_remove_calls);
}

static void handles_hotkeys_during_transmission(void)
{
   char high_hotkeys[] = {(char)0x80, '\0'};

   reset_send();
   pszCurrentHotkeys = (char *)"AB";
   ut_keys[0] = 'x'; ut_keys[1] = 0; ut_key_count = 2;
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file("screen.ans"));
   UT_ASSERT_EQ_INT(0, chHotkeyPressed);

   reset_send();
   pszCurrentHotkeys = (char *)"AB";
   ut_keys[0] = 'b'; ut_key_count = 1;
   od_control.baud = 9600L;
   UT_ASSERT(utt_od_send_file("screen.ans"));
   UT_ASSERT_EQ_INT('B', chHotkeyPressed);

   reset_send();
   pszCurrentHotkeys = (char *)"AB";
   ut_keys[0] = 'a'; ut_key_count = 1;
   UT_ASSERT(utt_od_send_file("screen.ans"));
   UT_ASSERT_EQ_INT('A', chHotkeyPressed);

   reset_send();
   pszCurrentHotkeys = high_hotkeys;
   ut_keys[0] = (char)0x80; ut_key_count = 1;
   UT_ASSERT(utt_od_send_file("screen.ans"));
   UT_ASSERT_EQ_UINT(0x80, (unsigned char)chHotkeyPressed);
}

static void honors_stop_pause_and_other_control_keys(void)
{
   reset_send();
   ut_control_after_emulate = 's';
   od_control.od_list_stop = TRUE;
   add_read((FILE *)&ut_remote_token, "line");
   UT_ASSERT(utt_od_send_file("screen.ans"));

   reset_send();
   ut_control_after_emulate = 's';
   add_read((FILE *)&ut_remote_token, "line");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file("screen.ans"));
   UT_ASSERT_EQ_INT(0, ut_last_control_key);

   reset_send();
   ut_control_after_emulate = 'p';
   od_control.od_list_pause = FALSE;
   add_read((FILE *)&ut_remote_token, "line");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file("screen.ans"));

   reset_send();
   ut_control_after_emulate = 'p';
   od_control.od_list_pause = TRUE;
   ut_keys[0] = 'x'; ut_key_count = 1;
   add_read((FILE *)&ut_remote_token, "line");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file("screen.ans"));
   UT_ASSERT_EQ_UINT(1, ut_clear_key_calls);

   reset_send();
   ut_control_after_emulate = 'p';
   od_control.od_list_pause = TRUE;
   ut_keys[0] = 'x'; ut_key_count = 1;
   ut_uninitialize_on_wait = TRUE;
   add_read((FILE *)&ut_remote_token, "line");
   UT_ASSERT(utt_od_send_file("screen.ans"));
   UT_ASSERT_EQ_UINT(1, ut_close_calls);

   reset_send();
   ut_control_after_emulate = 'x';
   add_read((FILE *)&ut_remote_token, "line");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file("screen.ans"));
}

static void applies_page_pause_conditions_and_result(void)
{
   reset_send();
   od_control.od_page_pausing = TRUE;
   od_control.user_screen_length = 3;
   add_read((FILE *)&ut_remote_token, "line\r");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file("screen.ans"));
   UT_ASSERT_EQ_UINT(1, ut_page_calls);

   reset_send();
   od_control.od_page_pausing = TRUE;
   od_control.user_screen_length = 3;
   ut_page_result = TRUE;
   add_read((FILE *)&ut_remote_token, "line\n");
   UT_ASSERT(utt_od_send_file("screen.ans"));
   UT_ASSERT_EQ_UINT(1, ut_page_calls);

   reset_send();
   od_control.od_page_pausing = TRUE;
   od_control.user_screen_length = 4;
   add_read((FILE *)&ut_remote_token, "line\n");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file("screen.ans"));
   UT_ASSERT_EQ_UINT(0, ut_page_calls);

   reset_send();
   od_control.od_page_pausing = FALSE;
   od_control.user_screen_length = 3;
   add_read((FILE *)&ut_remote_token, "line\n");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file("screen.ans"));
   UT_ASSERT_EQ_UINT(0, ut_page_calls);

   reset_send();
   od_control.od_page_pausing = TRUE;
   od_control.user_screen_length = 3;
   add_read((FILE *)&ut_remote_token, "line");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file("screen.ans"));
   UT_ASSERT_EQ_UINT(0, ut_page_calls);
}

static void selects_remote_access_translation_policy(void)
{
   reset_send();
   od_control.od_no_ra_codes = TRUE;
   add_read((FILE *)&ut_remote_token, "line\n");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file("screen.ans"));
   UT_ASSERT_EQ_UINT(1, ut_emulate_calls);
   UT_ASSERT(!ut_emulate_remote[0]);
   UT_ASSERT(ut_emulate_session[0]);
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
}

static const UTTestCase ut_cases[] = {
   {"invalid input", rejects_invalid_names_and_failed_opens},
   {"explicit file", sends_an_explicit_local_file_to_eof},
   {"auto files", handles_auto_selected_remote_and_local_files},
   {"RIP", sends_rip_without_local_emulation_and_drains_remote_output},
   {"hotkeys", handles_hotkeys_during_transmission},
   {"control keys", honors_stop_pause_and_other_control_keys},
   {"page pause", applies_page_pause_conditions_and_result},
   {"RA codes", selects_remote_access_translation_policy},
   {"terminal session", terminal_session_is_rejected}
};
#endif
