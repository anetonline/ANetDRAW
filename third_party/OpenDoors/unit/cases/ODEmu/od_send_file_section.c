#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_strncmp
#define UT_SEND_FILE_SUPPORT_ONLY
#include "od_send_file.c"

void *utm_memcpy(void *destination, const void *source, size_t count)
{
   BYTE *to = (BYTE *)destination;
   const BYTE *from = (const BYTE *)source;
   size_t index;
   for(index = 0; index < count; ++index) to[index] = from[index];
   return(destination);
}

int utm_strncmp(const char *left, const char *right, size_t count)
{
   size_t index;
   for(index = 0; index < count; ++index) {
      unsigned char a = (unsigned char)left[index];
      unsigned char b = (unsigned char)right[index];
      if(a != b) return(a < b ? -1 : 1);
      if(a == 0) return(0);
   }
   return(0);
}

static void terminal_session_is_rejected(void)
{
   reset_send(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT(!utt_od_send_file_section(NULL, "SEC"));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entry_calls);
}

static void rejects_invalid_section_requests(void)
{
   static char long_name[255];
   unsigned index;
   reset_send();
   bODInitialized = FALSE;
   UT_ASSERT(!utt_od_send_file_section(NULL, (char *)"SEC"));
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);

   reset_send();
   UT_ASSERT(!utt_od_send_file_section((char *)"screen.ans", NULL));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);

   for(index = 0; index < sizeof(long_name) - 1; ++index)
      long_name[index] = 'x';
   long_name[sizeof(long_name) - 1] = '\0';
   reset_send();
   UT_ASSERT(!utt_od_send_file_section((char *)"screen.ans", long_name));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);
}

static void covers_filename_selection_and_open_failures(void)
{
   reset_send();
   ut_extension = NULL;
   ut_auto_fits = FALSE;
   UT_ASSERT(!utt_od_send_file_section((char *)"long", (char *)"SEC"));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);

   reset_send();
   ut_extension = NULL;
   ut_find_count = 1;
   UT_ASSERT(!utt_od_send_file_section((char *)"missing", (char *)"SEC"));
   UT_ASSERT_EQ_INT(ERR_FILEOPEN, od_control.od_error);

   reset_send();
   ut_extension = NULL;
   ut_find_count = 1;
   ut_find_results[0] = (FILE *)&ut_remote_token;
   ut_find_levels[0] = LEVEL_NONE;
   UT_ASSERT(!utt_od_send_file_section((char *)"none", (char *)"SEC"));
   UT_ASSERT_EQ_INT(ERR_FILEOPEN, od_control.od_error);

   reset_send();
   ut_open_result = NULL;
   UT_ASSERT(!utt_od_send_file_section((char *)"none.ans", (char *)"SEC"));
   UT_ASSERT_EQ_INT(ERR_FILEOPEN, od_control.od_error);

   reset_send();
   ut_extension = NULL;
   ut_find_count = 1;
   ut_find_results[0] = (FILE *)&ut_remote_token;
   ut_find_levels[0] = LEVEL_ANSI;
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(!utt_od_send_file_section((char *)"screen", (char *)"SEC"));

   reset_send();
   ut_is_rip = TRUE;
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file_section((char *)"screen.rip", (char *)"SEC"));
   UT_ASSERT_EQ_UINT(1, ut_show_calls);
   UT_ASSERT_EQ_UINT(1, ut_remove_calls);
}

static void finds_and_sends_only_the_requested_section(void)
{
   reset_send();
   add_read((FILE *)&ut_remote_token, "preamble\n");
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, "content\n");
   add_read((FILE *)&ut_remote_token, "@#NEXT\n");
   UT_ASSERT(utt_od_send_file_section((char *)"screen.ans", (char *)"SEC"));
   UT_ASSERT_EQ_UINT(1, ut_emulate_calls);

   reset_send();
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, "content\n");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file_section((char *)"screen.ans", (char *)"SEC"));
   UT_ASSERT_EQ_UINT(1, ut_emulate_calls);

   reset_send();
   add_read((FILE *)&ut_remote_token, "other\n");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(!utt_od_send_file_section((char *)"screen.ans", (char *)"SEC"));
}

static void finds_the_section_while_finishing_a_local_fallback_file(void)
{
   reset_send();
   ut_extension = NULL;
   ut_find_count = 2;
   ut_find_results[0] = (FILE *)&ut_remote_token;
   ut_find_levels[0] = LEVEL_RIP;
   ut_find_results[1] = (FILE *)&ut_local_token;
   ut_find_levels[1] = LEVEL_ANSI;
   add_read((FILE *)&ut_remote_token, NULL);
   add_read((FILE *)&ut_local_token, "other\n");
   add_read((FILE *)&ut_local_token, "@#SEC\n");
   add_read((FILE *)&ut_local_token, "local content\n");
   add_read((FILE *)&ut_local_token, "@#NEXT\n");
   UT_ASSERT(utt_od_send_file_section((char *)"screen", (char *)"SEC"));
   UT_ASSERT_EQ_UINT(1, ut_emulate_calls);

   reset_send();
   ut_extension = NULL;
   ut_find_count = 2;
   ut_find_results[0] = (FILE *)&ut_remote_token;
   ut_find_levels[0] = LEVEL_RIP;
   ut_find_results[1] = (FILE *)&ut_local_token;
   ut_find_levels[1] = LEVEL_ASCII;
   add_read((FILE *)&ut_remote_token, NULL);
   add_read((FILE *)&ut_local_token, NULL);
   UT_ASSERT(!utt_od_send_file_section((char *)"screen", (char *)"SEC"));
}

static void handles_hotkeys_and_control_keys(void)
{
   char high_hotkeys[] = {(char)0x80, '\0'};

   reset_send();
   pszCurrentHotkeys = (char *)"AB";
   ut_keys[0] = 'x'; ut_keys[1] = 0; ut_key_count = 2;
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(!utt_od_send_file_section((char *)"screen.ans", (char *)"SEC"));

   reset_send();
   pszCurrentHotkeys = (char *)"AB";
   ut_keys[0] = 'b'; ut_key_count = 1;
   od_control.baud = 9600L;
   UT_ASSERT(!utt_od_send_file_section((char *)"screen.ans", (char *)"SEC"));
   UT_ASSERT_EQ_INT('B', chHotkeyPressed);

   reset_send();
   pszCurrentHotkeys = high_hotkeys;
   ut_keys[0] = (char)0x80; ut_key_count = 1;
   UT_ASSERT(!utt_od_send_file_section((char *)"screen.ans",
      (char *)"SEC"));
   UT_ASSERT_EQ_UINT(0x80, (unsigned char)chHotkeyPressed);

   reset_send();
   ut_control_after_emulate = 's';
   od_control.od_list_stop = TRUE;
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, "content");
   UT_ASSERT(utt_od_send_file_section((char *)"screen.ans", (char *)"SEC"));

   reset_send();
   ut_control_after_emulate = 's';
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, "content");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file_section((char *)"screen.ans", (char *)"SEC"));

   reset_send();
   ut_control_after_emulate = 'p';
   od_control.od_list_pause = FALSE;
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, "content");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file_section((char *)"screen.ans", (char *)"SEC"));

   reset_send();
   ut_control_after_emulate = 'p';
   od_control.od_list_pause = TRUE;
   ut_keys[0] = 'x'; ut_key_count = 1;
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, "content");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file_section((char *)"screen.ans", (char *)"SEC"));
   UT_ASSERT_EQ_UINT(1, ut_clear_key_calls);

   reset_send();
   ut_control_after_emulate = 'p';
   od_control.od_list_pause = TRUE;
   ut_keys[0] = 'x'; ut_key_count = 1;
   ut_uninitialize_on_wait = TRUE;
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, "content");
   UT_ASSERT(utt_od_send_file_section((char *)"screen.ans", (char *)"SEC"));

   reset_send();
   ut_control_after_emulate = 'x';
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, "content");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file_section((char *)"screen.ans", (char *)"SEC"));
}

static void applies_page_pausing_inside_the_selected_section(void)
{
   reset_send();
   od_control.od_page_pausing = TRUE;
   od_control.user_screen_length = 3;
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, "line\r");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file_section((char *)"screen.ans", (char *)"SEC"));
   UT_ASSERT_EQ_UINT(1, ut_page_calls);

   reset_send();
   od_control.od_page_pausing = TRUE;
   od_control.user_screen_length = 3;
   ut_page_result = TRUE;
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, "line\n");
   UT_ASSERT(utt_od_send_file_section((char *)"screen.ans", (char *)"SEC"));

   reset_send();
   od_control.od_page_pausing = TRUE;
   od_control.user_screen_length = 4;
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, "line\n");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file_section((char *)"screen.ans", (char *)"SEC"));
   UT_ASSERT_EQ_UINT(0, ut_page_calls);

   reset_send();
   od_control.od_page_pausing = FALSE;
   od_control.user_screen_length = 3;
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, "line\n");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file_section((char *)"screen.ans", (char *)"SEC"));

   reset_send();
   od_control.od_page_pausing = TRUE;
   od_control.user_screen_length = 3;
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, "line");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file_section((char *)"screen.ans", (char *)"SEC"));
}

static void handles_split_streams_and_remote_access_translation(void)
{
   reset_send();
   ut_extension = NULL;
   ut_find_count = 2;
   ut_find_results[0] = (FILE *)&ut_remote_token;
   ut_find_levels[0] = LEVEL_RIP;
   ut_find_results[1] = (FILE *)&ut_local_token;
   ut_find_levels[1] = LEVEL_ANSI;
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, "remote\n");
   add_read((FILE *)&ut_local_token, "local\n");
   add_read((FILE *)&ut_remote_token, NULL);
   add_read((FILE *)&ut_local_token, NULL);
   UT_ASSERT(utt_od_send_file_section((char *)"screen", (char *)"SEC"));
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
   UT_ASSERT_EQ_UINT(1, ut_emulate_calls);

   reset_send();
   ut_extension = NULL;
   ut_find_count = 2;
   ut_find_results[0] = (FILE *)&ut_remote_token;
   ut_find_levels[0] = LEVEL_RIP;
   ut_find_results[1] = (FILE *)&ut_local_token;
   ut_find_levels[1] = LEVEL_ASCII;
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, "remote 1\n");
   add_read((FILE *)&ut_local_token, NULL);
   add_read((FILE *)&ut_remote_token, "remote 2\n");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file_section((char *)"screen", (char *)"SEC"));
   UT_ASSERT_EQ_UINT(2, ut_disp_calls);

   reset_send();
   od_control.od_no_ra_codes = TRUE;
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, "line\n");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file_section((char *)"screen.ans", (char *)"SEC"));
   UT_ASSERT(!ut_emulate_remote[0]);
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
}

static void handles_rip_drain_shutdown_and_a_missing_window(void)
{
   reset_send();
   ut_extension = NULL;
   ut_find_count = 2;
   ut_find_results[0] = (FILE *)&ut_remote_token;
   ut_find_levels[0] = LEVEL_RIP;
   ut_find_results[1] = NULL;
   ut_find_levels[1] = LEVEL_NONE;
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, "rip content\n");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file_section((char *)"screen", (char *)"SEC"));
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);

   reset_send();
   ut_is_rip = TRUE;
   ut_show_returns_window = FALSE;
   od_control.baud = 9600L;
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, "rip content\n");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file_section((char *)"screen.rip", (char *)"SEC"));
   UT_ASSERT_EQ_UINT(1, ut_wait_calls);
   UT_ASSERT_EQ_UINT(0, ut_remove_calls);

   reset_send();
   ut_is_rip = TRUE;
   od_control.baud = 9600L;
   ut_uninitialize_on_wait = TRUE;
   add_read((FILE *)&ut_remote_token, "@#SEC\n");
   add_read((FILE *)&ut_remote_token, "rip content\n");
   add_read((FILE *)&ut_remote_token, NULL);
   UT_ASSERT(utt_od_send_file_section((char *)"screen.rip", (char *)"SEC"));
   UT_ASSERT_EQ_UINT(0, ut_remove_calls);
}

static const UTTestCase ut_cases[] = {
   {"invalid", rejects_invalid_section_requests},
   {"filenames", covers_filename_selection_and_open_failures},
   {"remote section", finds_and_sends_only_the_requested_section},
   {"local section", finds_the_section_while_finishing_a_local_fallback_file},
   {"keys", handles_hotkeys_and_control_keys},
   {"page pause", applies_page_pausing_inside_the_selected_section},
   {"split streams", handles_split_streams_and_remote_access_translation},
   {"RIP cleanup", handles_rip_drain_shutdown_and_a_missing_window},
   {"terminal session", terminal_session_is_rejected}
};
