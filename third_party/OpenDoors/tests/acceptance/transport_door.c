#include <stdlib.h>

#ifdef _WIN32 /* OpenDoor.h has not yet supplied ODPLAT_WIN32. */
#include <winsock2.h>
#include <windows.h>
#else
#if defined(__MSDOS__) || defined(MSDOS) || defined(__DOS__)
#include <dos.h>
#else
#include <unistd.h>
#endif
#endif

#define OD_ACCEPTANCE_NO_LOCAL_CONFIG
#ifdef __TURBOC__
#include "ATSUP.H"
#else
#include "test_support.h"
#endif

#ifdef ODPLAT_DOS32
#define OD_ACCEPTANCE_CALLBACK ODCALL
#else
#define OD_ACCEPTANCE_CALLBACK
#endif

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

static int before_chat_calls;
static int after_chat_calls;
static int chat_kernel_calls;
static int time_message_calls;
static int edit_menu_calls;
static int edit_realloc_calls;
static int before_exit_calls;

static void Marker(const char *text)
{
   od_disp(text, (INT)strlen(text), FALSE);
}

static void OD_ACCEPTANCE_CALLBACK BeforeChat(void)
{
   ++before_chat_calls;
}

static void OD_ACCEPTANCE_CALLBACK AfterChat(void)
{
   ++after_chat_calls;
}

static void OD_ACCEPTANCE_CALLBACK StopChat(void)
{
   if(++chat_kernel_calls >= 2)
      od_control.od_chat_active = FALSE;
}

static void OD_ACCEPTANCE_CALLBACK TimeMessage(char *message)
{
   (void)message;
   ++time_message_calls;
   Marker("TIME-MESSAGE");
}

static void OD_ACCEPTANCE_CALLBACK BeforeExit(void)
{
   ++before_exit_calls;
}

static tODEditMenuResult OD_ACCEPTANCE_CALLBACK EditMenu(void *unused)
{
   (void)unused;
   ++edit_menu_calls;
   if(edit_menu_calls == 1)
      Marker("EDIT-MENU-RESUMED");
   return(edit_menu_calls == 1 ? EDIT_MENU_DO_NOTHING
      : EDIT_MENU_EXIT_EDITOR);
}

static void * OD_ACCEPTANCE_CALLBACK EditRealloc(void *buffer, UINT size)
{
   ++edit_realloc_calls;
   return(realloc(buffer, size));
}

static void PauseWithoutOpenDoors(unsigned seconds)
{
#ifdef ODPLAT_WIN32
   Sleep((DWORD)seconds * 1000UL);
#else
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   delay(seconds * 1000U);
#else
   sleep(seconds);
#endif
#endif
}

static int FinishScenario(const char *marker)
{
   Marker(marker);
   OD_TEST_CHECK(od_get_key(TRUE) == 'X');
   od_exit(0, FALSE);
   return(0);
}

static int RunInputScenario(void)
{
   /* Turbo C 2.01 misaddresses function-local initialized const byte arrays
    * in optimized large-model builds. */
   static unsigned char burst[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";
   tODInputEvent event;
   DWORD first_seconds;
   DWORD second_seconds;
   WORD first_milliseconds;
   WORD second_milliseconds;
   size_t index;

   OD_TEST_CHECK(!od_get_input(&event, 0, GETIN_RAW));

   Marker("INPUT-CHAR");
   OD_TEST_CHECK(od_get_input(&event, OD_NO_TIMEOUT, GETIN_NORMAL));
   OD_TEST_CHECK(event.EventType == EVENT_CHARACTER);
   OD_TEST_CHECK(event.bFromRemote);
   OD_TEST_CHECK(event.chKeyPress == 'Q');

   Marker("INPUT-CTRL-NORMAL");
   OD_TEST_CHECK(od_get_input(&event, OD_NO_TIMEOUT, GETIN_NORMAL));
   OD_TEST_CHECK(event.EventType == EVENT_EXTENDED_KEY);
   OD_TEST_CHECK((unsigned char)event.chKeyPress == OD_KEY_UP);

   Marker("INPUT-CTRL-RAW");
   OD_TEST_CHECK(od_get_input(&event, OD_NO_TIMEOUT, GETIN_RAWCTRL));
   OD_TEST_CHECK(event.EventType == EVENT_CHARACTER);
   OD_TEST_CHECK((unsigned char)event.chKeyPress == 5U);

   Marker("INPUT-SEQUENCE");
   OD_TEST_CHECK(od_get_input(&event, OD_NO_TIMEOUT, GETIN_NORMAL));
   OD_TEST_CHECK(event.EventType == EVENT_EXTENDED_KEY);
   OD_TEST_CHECK((unsigned char)event.chKeyPress == OD_KEY_UP);

   Marker("INPUT-RAW-SEQUENCE");
   OD_TEST_CHECK(od_get_input(&event, OD_NO_TIMEOUT, GETIN_RAW));
   OD_TEST_CHECK(event.EventType == EVENT_CHARACTER && event.chKeyPress == 27);
   OD_TEST_CHECK(od_get_input(&event, OD_NO_TIMEOUT, GETIN_RAW));
   OD_TEST_CHECK(event.EventType == EVENT_CHARACTER && event.chKeyPress == '[');
   OD_TEST_CHECK(od_get_input(&event, OD_NO_TIMEOUT, GETIN_RAW));
   OD_TEST_CHECK(event.EventType == EVENT_CHARACTER && event.chKeyPress == 'A');

   Marker("INPUT-LINEFEED");
   OD_TEST_CHECK(od_get_key(TRUE) == 'R');
   OD_TEST_CHECK(od_control.od_last_input == 0);

   Marker("INPUT-BURST");
   od_sleep(250);
   for(index = 0; index < ARRAY_COUNT(burst) - 1; ++index)
   {
      OD_TEST_CHECK(od_get_input(&event, OD_NO_TIMEOUT, GETIN_RAW));
      OD_TEST_CHECK(event.EventType == EVENT_CHARACTER);
      OD_TEST_CHECK(event.bFromRemote);
      OD_TEST_CHECK((unsigned char)event.chKeyPress == burst[index]);
   }
   OD_TEST_CHECK(!od_key_pending());

   Marker("INPUT-CLEAR");
   od_get_time(&second_seconds, &second_milliseconds);
   second_seconds += 5;
   while(!od_key_pending())
   {
      od_get_time(&first_seconds, &first_milliseconds);
      OD_TEST_CHECK(first_seconds < second_seconds
         || (first_seconds == second_seconds
            && first_milliseconds < second_milliseconds));
      od_sleep(1);
   }
   od_clear_keybuffer();
   OD_TEST_CHECK(!od_key_pending());
   OD_TEST_CHECK(od_get_key(FALSE) == 0);

   od_get_time(&first_seconds, &first_milliseconds);
   OD_TEST_CHECK(first_milliseconds < 1000U);
   od_sleep(100);
   od_get_time(&second_seconds, &second_milliseconds);
   OD_TEST_CHECK(second_milliseconds < 1000U);
   OD_TEST_CHECK(second_seconds > first_seconds
      || (second_seconds == first_seconds
         && second_milliseconds > first_milliseconds));

   Marker("INPUT-UNTIL");
   od_get_time(&second_seconds, &second_milliseconds);
   second_milliseconds += 500U;
   if(second_milliseconds >= 1000U)
   {
      second_milliseconds -= 1000U;
      ++second_seconds;
   }
   OD_TEST_CHECK(od_get_input_until(&event, second_seconds,
      second_milliseconds, GETIN_RAW));
   OD_TEST_CHECK(event.EventType == EVENT_CHARACTER);
   OD_TEST_CHECK(event.bFromRemote && event.chKeyPress == 'U');

   Marker("INPUT-UNTIL-EXPIRED");
   od_get_time(&second_seconds, &second_milliseconds);
   OD_TEST_CHECK(!od_get_input_until(&event, second_seconds,
      second_milliseconds, GETIN_RAW));
   OD_TEST_CHECK(od_get_input(&event, OD_NO_TIMEOUT, GETIN_RAW));
   OD_TEST_CHECK(event.EventType == EVENT_CHARACTER);
   OD_TEST_CHECK(event.bFromRemote && event.chKeyPress == 'E');

   return(FinishScenario("INPUT-DONE"));
}

static int RunInteractiveScenario(void)
{
   char answer;
   char input[8];
   char edit_input[8];
   char multiline[64];
   WORD edit_result;
   INT menu_result;

   edit_input[0] = '\0';
   OD_TEST_CHECK(od_edit_str(edit_input, "A", 0, 0, L_WHITE,
      D_GREY, ' ', EDIT_FLAG_NORMAL) == EDIT_RETURN_ERROR);
   OD_TEST_CHECK(od_multiline_edit(NULL, 1, NULL) == OD_MULTIEDIT_ERROR);
   OD_TEST_CHECK(od_popup_menu("Bad", "One", 1, 1, 11, MENU_NORMAL) ==
      POPUP_ERROR);

   Marker("INTERACTIVE-ANSWER");
   answer = od_get_answer("YN");
   OD_TEST_CHECK(answer == 'Y');

   Marker("INTERACTIVE-STRING");
   memset(input, 0, sizeof(input));
   od_input_str(input, 4, 32, 126);
   OD_TEST_CHECK(strcmp(input, "aCDE") == 0);

   Marker("INTERACTIVE-HIGH");
   memset(input, 0, sizeof(input));
   od_input_str(input, 2, 0x80, 0xff);
   OD_TEST_CHECK((unsigned char)input[0] == 0x80U);
   OD_TEST_CHECK((unsigned char)input[1] == 0xffU);
   OD_TEST_CHECK(input[2] == '\0');

   Marker("INTERACTIVE-EDIT");
   edit_input[0] = '\0';
   edit_result = od_edit_str(edit_input, "UUU", 2, 2, L_WHITE,
      L_YELLOW, ' ', EDIT_FLAG_NORMAL);
   OD_TEST_CHECK(edit_result == EDIT_RETURN_ACCEPT);
   OD_TEST_CHECK(strcmp(edit_input, "ABC") == 0);

   Marker("INTERACTIVE-EDIT-CANCEL");
   strcpy(edit_input, "OLD");
   edit_result = od_edit_str(edit_input, "UUU", 2, 2, L_WHITE,
      L_YELLOW, ' ', EDIT_FLAG_EDIT_STRING | EDIT_FLAG_ALLOW_CANCEL);
   OD_TEST_CHECK(edit_result == EDIT_RETURN_CANCEL);
   OD_TEST_CHECK(strcmp(edit_input, "OLD") == 0);

   Marker("INTERACTIVE-MULTILINE");
   od_sleep(250);
   multiline[0] = '\0';
   OD_TEST_CHECK(od_multiline_edit(multiline, sizeof(multiline), NULL) ==
      OD_MULTIEDIT_SUCCESS);
   OD_TEST_CHECK(strstr(multiline, "Hi") != NULL);
   OD_TEST_CHECK(strstr(multiline, "There") != NULL);

   Marker("INTERACTIVE-POPUP-HOTKEY");
   menu_result = od_popup_menu("Menu", "^Save|^Load|E^xit", 2, 2, 0,
      MENU_NORMAL);
   OD_TEST_CHECK(menu_result == 1);

   Marker("INTERACTIVE-POPUP-CANCEL");
   menu_result = od_popup_menu("Menu", "^Save|^Load", 2, 2, 0,
      MENU_ALLOW_CANCEL);
   OD_TEST_CHECK(menu_result == POPUP_ESCAPE);

   Marker("INTERACTIVE-POPUP-KEEP");
   menu_result = od_popup_menu("Menu", "^Save|^Load", 2, 2, 0,
      MENU_KEEP);
   OD_TEST_CHECK(menu_result == 2);
   OD_TEST_CHECK(od_popup_menu(NULL, NULL, 0, 0, 0, MENU_DESTROY) ==
      POPUP_ESCAPE);

   od_control.od_okaytopage = PAGE_DISABLE;
   Marker("INTERACTIVE-PAGE-CANCEL");
   od_page();
   OD_TEST_CHECK(od_control.user_numpages == 0);

   Marker("INTERACTIVE-PAGE-DISABLED");
   od_page();
   OD_TEST_CHECK(od_control.user_numpages == 0);
   OD_TEST_CHECK(od_control.user_wantchat);

   od_control.od_cbefore_chat = BeforeChat;
   od_control.od_cafter_chat = AfterChat;
   Marker("INTERACTIVE-CHAT");
   OD_TEST_CHECK(od_get_key(TRUE) == 'A');
   od_control.od_ker_exec = StopChat;
   od_chat();
   od_control.od_ker_exec = NULL;
   OD_TEST_CHECK(before_chat_calls == 1);
   OD_TEST_CHECK(after_chat_calls == 1);
   OD_TEST_CHECK(!od_control.od_chat_active);

   return(FinishScenario("INTERACTIVE-DONE"));
}

static int RunEditScenario(void)
{
   char field[40];
   char multiline[80];
   char *growable;
   WORD edit_result;
   tODEditOptions options;

   Marker("EDIT-FORMATS");
   field[0] = '\0';
   edit_result = od_edit_str(field, "#%9*CADFHL'-'MTUWXY?", 2, 2,
      L_WHITE, L_YELLOW, '_', EDIT_FLAG_NORMAL);
   OD_TEST_CHECK(edit_result == EDIT_RETURN_ACCEPT);
   OD_TEST_CHECK(strcmp(field, "7 -!db/Faq-J+Rw3N@") == 0);

   Marker("EDIT-INSERT");
   strcpy(field, "ac");
   edit_result = od_edit_str(field, "????", 3, 2, L_WHITE, L_YELLOW,
      '_', EDIT_FLAG_EDIT_STRING);
   OD_TEST_CHECK(edit_result == EDIT_RETURN_ACCEPT);
   OD_TEST_CHECK(strcmp(field, "abc") == 0);

   Marker("EDIT-OVERWRITE");
   strcpy(field, "abc");
   edit_result = od_edit_str(field, "???", 4, 2, L_WHITE, L_YELLOW,
      '_', EDIT_FLAG_EDIT_STRING);
   OD_TEST_CHECK(edit_result == EDIT_RETURN_ACCEPT);
   OD_TEST_CHECK(strcmp(field, "xbc") == 0);

   Marker("EDIT-DELETE");
   strcpy(field, "abc");
   edit_result = od_edit_str(field, "???", 5, 2, L_WHITE, L_YELLOW,
      '_', EDIT_FLAG_EDIT_STRING);
   OD_TEST_CHECK(edit_result == EDIT_RETURN_ACCEPT);
   OD_TEST_CHECK(strcmp(field, "b") == 0);

   Marker("EDIT-KILL");
   strcpy(field, "old");
   edit_result = od_edit_str(field, "???", 6, 2, L_WHITE, L_YELLOW,
      '_', EDIT_FLAG_EDIT_STRING);
   OD_TEST_CHECK(edit_result == EDIT_RETURN_ACCEPT);
   OD_TEST_CHECK(strcmp(field, "x") == 0);

   Marker("EDIT-AUTO-DELETE");
   strcpy(field, "old");
   edit_result = od_edit_str(field, "???", 7, 2, L_WHITE, L_YELLOW,
      '_', EDIT_FLAG_EDIT_STRING | EDIT_FLAG_AUTO_DELETE);
   OD_TEST_CHECK(edit_result == EDIT_RETURN_ACCEPT);
   OD_TEST_CHECK(strcmp(field, "x") == 0);

   Marker("EDIT-PREVIOUS");
   strcpy(field, "A");
   edit_result = od_edit_str(field, "?", 8, 2, L_WHITE, L_YELLOW,
      '_', EDIT_FLAG_EDIT_STRING | EDIT_FLAG_FIELD_MODE);
   OD_TEST_CHECK(edit_result == EDIT_RETURN_PREVIOUS);

   Marker("EDIT-NEXT");
   strcpy(field, "A");
   edit_result = od_edit_str(field, "?", 9, 2, L_WHITE, L_YELLOW,
      '_', EDIT_FLAG_EDIT_STRING | EDIT_FLAG_FIELD_MODE);
   OD_TEST_CHECK(edit_result == EDIT_RETURN_NEXT);

   Marker("EDIT-FILL-AUTO");
   field[0] = '\0';
   edit_result = od_edit_str(field, "##", 10, 2, L_WHITE, L_YELLOW,
      '_', EDIT_FLAG_FILL_STRING | EDIT_FLAG_AUTO_ENTER);
   OD_TEST_CHECK(edit_result == EDIT_RETURN_ACCEPT);
   OD_TEST_CHECK(strcmp(field, "12") == 0);

   Marker("EDIT-PERMALITERAL");
   field[0] = '\0';
   edit_result = od_edit_str(field, "'-'?", 11, 2, L_WHITE, L_YELLOW,
      '_', EDIT_FLAG_PERMALITERAL | EDIT_FLAG_LEAVE_BLANK);
   OD_TEST_CHECK(edit_result == EDIT_RETURN_ACCEPT);
   OD_TEST_CHECK(field[0] == '\0');

   Marker("EDIT-STRICT");
   strcpy(field, "ab");
   edit_result = od_edit_str(field, "??", 12, 2, L_WHITE, L_YELLOW,
      '_', EDIT_FLAG_EDIT_STRING | EDIT_FLAG_STRICT_INPUT |
      EDIT_FLAG_NO_REDRAW);
   OD_TEST_CHECK(edit_result == EDIT_RETURN_ACCEPT);
   OD_TEST_CHECK(strcmp(field, "xb") == 0);

   Marker("EDIT-PASSWORD");
   strcpy(field, "secret");
   edit_result = od_edit_str(field, "??????", 13, 2, L_WHITE, L_YELLOW,
      '*', EDIT_FLAG_EDIT_STRING | EDIT_FLAG_PASSWORD_MODE |
      EDIT_FLAG_KEEP_BLANK | EDIT_FLAG_SHOW_SIZE);
   OD_TEST_CHECK(edit_result == EDIT_RETURN_ACCEPT);
   OD_TEST_CHECK(strcmp(field, "secret") == 0);

   Marker("EDIT-MULTILINE-MENU");
   memset(&options, 0, sizeof(options));
   options.nAreaLeft = 2;
   options.nAreaTop = 14;
   options.nAreaRight = 30;
   options.nAreaBottom = 19;
   options.TextFormat = FORMAT_NO_WORDWRAP;
   options.pfMenuCallback = EditMenu;
   multiline[0] = '\0';
   OD_TEST_CHECK(od_multiline_edit(multiline, sizeof(multiline), &options) ==
      OD_MULTIEDIT_SUCCESS);
   OD_TEST_CHECK(edit_menu_calls == 2);
   OD_TEST_CHECK(strncmp(multiline, "AB", 2) == 0);
   OD_TEST_CHECK(strchr(multiline, 'C') != NULL);

   Marker("EDIT-MULTILINE-GROW");
   memset(&options, 0, sizeof(options));
   options.TextFormat = FORMAT_LINE_BREAKS;
   options.pfBufferRealloc = EditRealloc;
   growable = (char *)malloc(4);
   OD_TEST_CHECK(growable != NULL);
   growable[0] = '\0';
   OD_TEST_CHECK(od_multiline_edit(growable, 4, &options) ==
      OD_MULTIEDIT_SUCCESS);
   OD_TEST_CHECK(edit_realloc_calls != 0);
   OD_TEST_CHECK(options.pszFinalBuffer != NULL);
   OD_TEST_CHECK(options.unFinalBufferSize > 4);
   OD_TEST_CHECK(strstr(options.pszFinalBuffer, "Growing text") != NULL);
   free(options.pszFinalBuffer);

   return(FinishScenario("EDIT-DONE"));
}

static int RunDisplayScenario(void)
{
   char menu_choice;
   DWORD pending_deadline_seconds;
   DWORD pending_seconds;
   WORD pending_deadline_milliseconds;
   WORD pending_milliseconds;

   Marker("DISPLAY-OUTPUT");
   od_disp("DISP", 4, FALSE);
   od_disp_str("-STRING");
   od_putch('-');
   od_repeat('R', 3);
   od_printf("-%s-%d", "PRINTF", 7);
   od_disp_emu("\x1b[31m-EMU-\x1b[0m", TRUE);

   od_control.od_page_pausing = FALSE;
   OD_TEST_CHECK(od_send_file("ODFILE"));
   OD_TEST_CHECK(od_send_file_section("ODSECT.ASC", "SECOND"));
   OD_TEST_CHECK(od_list_files("FILES.BBS"));

   od_control.od_error = ERR_NONE;
   OD_TEST_CHECK(!od_send_file("MISSING.ASC"));
   OD_TEST_CHECK(od_control.od_error == ERR_FILEOPEN);
   od_control.od_error = ERR_NONE;
   OD_TEST_CHECK(!od_send_file_section("ODSECT.ASC", "MISSING"));
   OD_TEST_CHECK(od_control.od_error == ERR_NONE);

   Marker("DISPLAY-HOTKEY-WAIT");
   menu_choice = od_hotkey_menu("ODMENU.ASC", "12Q", TRUE);
   OD_TEST_CHECK(menu_choice == '2');

   Marker("DISPLAY-HOTKEY-EARLY");
   OD_TEST_CHECK(od_get_key(TRUE) == 'A');
   Marker("DISPLAY-HOTKEY-QUEUED");
   od_get_time(&pending_deadline_seconds,
      &pending_deadline_milliseconds);
   pending_deadline_seconds += 5;
   while(!od_key_pending())
   {
      od_get_time(&pending_seconds, &pending_milliseconds);
      OD_TEST_CHECK(pending_seconds < pending_deadline_seconds
         || (pending_seconds == pending_deadline_seconds
            && pending_milliseconds < pending_deadline_milliseconds));
      od_sleep(1);
   }
   menu_choice = od_hotkey_menu("ODLONG.ASC", "Q", FALSE);
   OD_TEST_CHECK(menu_choice == 'Q');

   return(FinishScenario("DISPLAY-DONE"));
}

static BOOL ScreenMatches(INT column, INT row, const char *text,
   INT attribute)
{
   static unsigned char cells[128];
   size_t length;
   size_t index;

   length = strlen(text);
   if(length == 0 || length > sizeof(cells) / 2)
      return(FALSE);
   if(!od_gettext(column, row, column + (INT)length - 1, row, cells))
      return(FALSE);
   for(index = 0; index < length; ++index)
   {
      if(cells[index * 2] != (unsigned char)text[index])
         return(FALSE);
      if(attribute >= 0 && cells[index * 2 + 1] != (unsigned char)attribute)
         return(FALSE);
   }
   return(TRUE);
}

static int RunEmulationScenario(void)
{
   /* See the Turbo C note in RunInputScenario(). */
   static char avatar_position[] = {0x16, 0x08, 9, 15, 'V', 0};
   static char avatar_attribute[] = {0x16, 0x01, 0x2e, 'C', 0};
   static char avatar_repeat[] = {0x19, 'R', 3, 0};
   static char avatar_insert[] = {0x16, 0x09, 'X', 0};
   static char avatar_delete[] = {0x16, 0x0e, 0};
   static char ra_values[] = {0x06, 'A', '|', 0x06, 'H', '|',
      0x0b, 'A', 0};

   Marker("EMULATION-ANSI");
   od_disp_emu("\x1b[2J\x1b[3;5H\x1b[1;33;44mAB", TRUE);
   OD_TEST_CHECK(ScreenMatches(5, 3, "AB",
      L_YELLOW | (D_BLUE << 4)));
   od_disp_emu("\x1b[4;10H\x1b[s\x1b[7;20HX\x1b[uY", TRUE);
   OD_TEST_CHECK(ScreenMatches(10, 4, "Y", -1));
   OD_TEST_CHECK(ScreenMatches(20, 7, "X", -1));

   Marker("EMULATION-SPLIT");
   od_emulate(27);
   od_emulate('[');
   od_emulate('8');
   od_emulate(';');
   od_emulate('1');
   od_emulate('2');
   od_emulate('H');
   od_emulate('Z');
   OD_TEST_CHECK(ScreenMatches(12, 8, "Z", -1));

   Marker("EMULATION-AVATAR");
   od_disp_emu(avatar_position, TRUE);
   OD_TEST_CHECK(ScreenMatches(15, 9, "V", -1));
   od_disp_emu(avatar_attribute, TRUE);
   OD_TEST_CHECK(ScreenMatches(16, 9, "C", 0x2e));
   od_disp_emu(avatar_repeat, TRUE);
   OD_TEST_CHECK(ScreenMatches(17, 9, "RRR", 0x2e));

   od_disp_emu("\x1b[10;10HABC", TRUE);
   od_disp_emu("\x1b[10;11H", TRUE);
   od_disp_emu(avatar_insert, TRUE);
   OD_TEST_CHECK(ScreenMatches(10, 10, "AXBC", -1));
   od_disp_emu("\x1b[10;11H", TRUE);
   od_disp_emu(avatar_delete, TRUE);
   OD_TEST_CHECK(ScreenMatches(10, 10, "ABC ", -1));

   strcpy(od_control.user_name, "EMU-USER");
   od_control.user_flags[0] = 5;
   od_control.system_calls = 42;
   od_control.od_no_ra_codes = FALSE;
   Marker("EMULATION-RA");
   od_disp_emu(ra_values, TRUE);

   Marker("EMULATION-FORMATS");
   od_control.user_rip = TRUE;
   od_control.user_avatar = TRUE;
   od_control.user_ansi = TRUE;
   OD_TEST_CHECK(od_send_file("ODFMT"));
   od_control.user_rip = FALSE;
   OD_TEST_CHECK(od_send_file("ODFMT"));
   od_control.user_avatar = FALSE;
   OD_TEST_CHECK(od_send_file("ODFMT"));
   od_control.user_ansi = FALSE;
   OD_TEST_CHECK(od_send_file("ODFMT"));

   od_control.user_ansi = TRUE;
   od_control.user_screen_length = 4;
   od_control.od_page_pausing = TRUE;
   Marker("EMULATION-PAGING");
   OD_TEST_CHECK(od_send_file("ODPAGE"));
   od_control.od_page_pausing = FALSE;

   return(FinishScenario("EMULATION-DONE"));
}

static int RunListingScenario(void)
{
   char long_spec[101];
   char index_path[32];

   memset(long_spec, 'L', sizeof(long_spec) - 1);
   long_spec[sizeof(long_spec) - 1] = '\0';
   od_control.od_error = ERR_NONE;
   OD_TEST_CHECK(!od_list_files(long_spec));
   OD_TEST_CHECK(od_control.od_error == ERR_LIMIT);
   od_control.od_error = ERR_NONE;
   OD_TEST_CHECK(!od_list_files("MISSING.BBS"));
   OD_TEST_CHECK(od_control.od_error == ERR_FILEOPEN);
   od_control.od_error = ERR_NONE;
   OD_TEST_CHECK(!od_list_files("ODBADTOK.BBS"));
   OD_TEST_CHECK(od_control.od_error == ERR_LIMIT);

   Marker("LISTING-CURRENT");
   OD_TEST_CHECK(od_list_files(NULL));
   OD_TEST_CHECK(od_list_files(""));

   od_control.od_offline = "OFFLINE-MARK";
   Marker("LISTING-DIRECTORY");
   OD_TEST_CHECK(od_list_files("ODAREA"));
   strcpy(index_path, "ODAREA" DIRSEP_STR "FILES.BBS");
   Marker("LISTING-EXPLICIT");
   OD_TEST_CHECK(od_list_files(index_path));

   od_control.od_list_name_col = L_YELLOW | (D_BLUE << 4);
   od_control.od_list_size_col = L_WHITE | (D_RED << 4);
   od_control.od_list_comment_col = L_GREEN | (D_CYAN << 4);
   Marker("LISTING-COLORS");
   od_clr_scr();
   OD_TEST_CHECK(od_list_files("ODCOLOR.BBS"));
   OD_TEST_CHECK(ScreenMatches(1, 1, "COLOR.TXT   ",
      od_control.od_list_name_col));
   OD_TEST_CHECK(ScreenMatches(15, 1, "6", od_control.od_list_size_col));
   OD_TEST_CHECK(ScreenMatches(24, 1, "Color entry",
      od_control.od_list_comment_col));

   od_control.user_screen_length = 4;
   od_control.od_page_pausing = TRUE;
   Marker("LISTING-CANCEL");
   OD_TEST_CHECK(od_list_files("ODCANCEL.BBS"));
   Marker("LISTING-AFTER-CANCEL");
   Marker("LISTING-PAGING");
   OD_TEST_CHECK(od_list_files("ODPAGE.BBS"));
   od_control.od_page_pausing = FALSE;

   return(FinishScenario("LISTING-DONE"));
}

static int RunPopupScenario(void)
{
   static unsigned char original[744];
   static unsigned char restored[744];
   static unsigned char pattern[744];
   static unsigned char menu_original[90];
   static unsigned char menu_restored[90];
   void *first_window;
   void *second_window;
   void *window;
   INT result;
   int index;

   od_control.od_error = ERR_NONE;
   OD_TEST_CHECK(od_popup_menu(NULL, NULL, 1, 1, -1, MENU_NORMAL) ==
      POPUP_ERROR);
   OD_TEST_CHECK(od_control.od_error == ERR_LIMIT);
   od_control.od_error = ERR_NONE;
   OD_TEST_CHECK(od_popup_menu(NULL, "", 1, 1, 0, MENU_NORMAL) ==
      POPUP_ERROR);
   OD_TEST_CHECK(od_control.od_error == ERR_PARAMETER);

   od_control.user_ansi = FALSE;
   od_control.user_avatar = FALSE;
   od_control.od_error = ERR_NONE;
   OD_TEST_CHECK(od_popup_menu(NULL, "One", 1, 1, 0, MENU_NORMAL) ==
      POPUP_ERROR);
   OD_TEST_CHECK(od_control.od_error == ERR_NOGRAPHICS);
   od_control.od_error = ERR_NONE;
   OD_TEST_CHECK(od_window_create(2, 2, 12, 6, NULL, 0x17, 0x1e,
      0x1f, 0) == NULL);
   OD_TEST_CHECK(od_control.od_error == ERR_NOGRAPHICS);
   OD_TEST_CHECK(!od_draw_box(2, 2, 12, 6));
   OD_TEST_CHECK(od_control.od_error == ERR_NOGRAPHICS);
   od_control.user_ansi = TRUE;

   for(index = 0; index < 372; ++index)
   {
      pattern[index * 2] = (unsigned char)('A' + index % 26);
      pattern[index * 2 + 1] = (unsigned char)(0x10 + index % 8);
   }
   OD_TEST_CHECK(od_puttext(2, 2, 32, 13, pattern));
   OD_TEST_CHECK(od_gettext(2, 2, 32, 13, original));
   first_window = od_window_create(2, 2, 24, 10, NULL, 0x17, 0x1e,
      0x1f, 0);
   OD_TEST_CHECK(first_window != NULL);
   second_window = od_window_create(10, 5, 32, 13, "Second", 0x27,
      0x2e, 0x2f, 0);
   OD_TEST_CHECK(second_window != NULL);
   OD_TEST_CHECK(od_window_remove(second_window));
   OD_TEST_CHECK(od_window_remove(first_window));
   OD_TEST_CHECK(od_gettext(2, 2, 32, 13, restored));
   OD_TEST_CHECK(memcmp(original, restored, sizeof(original)) == 0);

   window = od_window_create(35, 2, 39, 6, "Narrow title", 0x37,
      0x3e, 0x3f, 0);
   OD_TEST_CHECK(window != NULL);
   OD_TEST_CHECK(od_window_remove(window));
   window = od_window_create(35, 2, 50, 7,
      "A title which must be truncated", 0x47, 0x4e, 0x4f, 0);
   OD_TEST_CHECK(window != NULL);
   OD_TEST_CHECK(od_window_remove(window));
   window = od_window_create(35, 2, 50, 7, "", 0x57, 0x5e, 0x5f, 0);
   OD_TEST_CHECK(window != NULL);
   OD_TEST_CHECK(od_window_remove(window));

   od_control.od_box_chars[BOX_BOTTOM] = 0;
   od_control.od_box_chars[BOX_RIGHT] = 0;
   OD_TEST_CHECK(od_draw_box(35, 9, 50, 14));
   OD_TEST_CHECK(od_control.od_box_chars[BOX_BOTTOM] ==
      od_control.od_box_chars[BOX_TOP]);
   OD_TEST_CHECK(od_control.od_box_chars[BOX_RIGHT] ==
      od_control.od_box_chars[BOX_LEFT]);
   od_control.user_avatar = TRUE;
   OD_TEST_CHECK(od_draw_box(52, 9, 65, 14));
   window = od_window_create(52, 16, 68, 22, "Avatar", 0x67, 0x6e,
      0x6f, 0);
   OD_TEST_CHECK(window != NULL);
   OD_TEST_CHECK(od_window_remove(window));
   od_control.user_avatar = FALSE;

   Marker("POPUP-DOWN");
   result = od_popup_menu("Arrows", "One|Two|Three|Four", 3, 3, 0,
      MENU_NORMAL);
   OD_TEST_CHECK(result == 3);
   Marker("POPUP-UP-WRAP");
   result = od_popup_menu(NULL, "One|Two|Three|Four", 3, 3, 0,
      MENU_NORMAL);
   OD_TEST_CHECK(result == 4);
   Marker("POPUP-IGNORED");
   result = od_popup_menu("Ignored", "Alpha|Beta|Gamma", 3, 3, 0,
      MENU_NORMAL);
   OD_TEST_CHECK(result == 2);
   Marker("POPUP-HOTKEY");
   result = od_popup_menu(NULL, "Alpha|Beta", 3, 3, 0, MENU_NORMAL);
   OD_TEST_CHECK(result == 2);

   Marker("POPUP-LEFT");
   OD_TEST_CHECK(od_popup_menu(NULL, "One|Two", 3, 3, 0,
      MENU_PULLDOWN) == POPUP_LEFT);
   Marker("POPUP-RIGHT");
   OD_TEST_CHECK(od_popup_menu(NULL, "One|Two", 3, 3, 0,
      MENU_PULLDOWN) == POPUP_RIGHT);
   Marker("POPUP-NUMERIC-LEFT");
   OD_TEST_CHECK(od_popup_menu(NULL, "One|Two", 3, 3, 0,
      MENU_PULLDOWN) == POPUP_LEFT);
   Marker("POPUP-NUMERIC-RIGHT");
   OD_TEST_CHECK(od_popup_menu(NULL, "One|Two", 3, 3, 0,
      MENU_PULLDOWN) == POPUP_RIGHT);

   od_clr_scr();
   OD_TEST_CHECK(od_gettext(10, 5, 18, 9, menu_original));
   Marker("POPUP-KEEP-FIRST");
   result = od_popup_menu("Keep", "One|Two|Three", 10, 5, 5,
      MENU_KEEP);
   OD_TEST_CHECK(result == 2);
   Marker("POPUP-KEEP-RESUME");
   result = od_popup_menu("Ignored", "Ignored", 1, 1, 5, MENU_NORMAL);
   OD_TEST_CHECK(result == 3);
   OD_TEST_CHECK(od_popup_menu(NULL, NULL, 0, 0, 5, MENU_DESTROY) ==
      POPUP_ESCAPE);
   OD_TEST_CHECK(od_gettext(10, 5, 18, 9, menu_restored));
   OD_TEST_CHECK(memcmp(menu_original, menu_restored,
      sizeof(menu_original)) == 0);

   Marker("POPUP-CANCEL-FIRST");
   result = od_popup_menu("Cancel", "One|Two", 20, 5, 6,
      MENU_KEEP | MENU_ALLOW_CANCEL);
   OD_TEST_CHECK(result == 1);
   Marker("POPUP-CANCEL-RESUME");
   result = od_popup_menu(NULL, NULL, 0, 0, 6, MENU_NORMAL);
   OD_TEST_CHECK(result == POPUP_ESCAPE);

   return(FinishScenario("POPUP-DONE"));
}

static int RunSessionScenario(void)
{
   tODInputEvent event;
   int count;

   od_control.user_ansi = FALSE;
   od_control.user_rip = FALSE;
   Marker("SESSION-AUTODETECT");
   od_autodetect(DETECT_NORMAL);
   OD_TEST_CHECK(od_control.user_ansi);
   OD_TEST_CHECK(od_control.user_rip);

   Marker("SESSION-AUTODETECT-KNOWN");
   od_autodetect(DETECT_NORMAL);
   OD_TEST_CHECK(od_control.user_ansi && od_control.user_rip);

   od_control.user_ansi = FALSE;
   od_control.user_rip = FALSE;
   Marker("SESSION-AUTODETECT-FAIL");
   od_autodetect(DETECT_NORMAL);
   OD_TEST_CHECK(!od_control.user_ansi);
   OD_TEST_CHECK(!od_control.user_rip);

   od_control.od_inactivity = 10;
   od_control.od_inactive_warning = 5;
   od_control.od_inactivity_warning = (char *)"warning";
   od_control.od_time_msg_func = TimeMessage;
   Marker("SESSION-TIMER-ARMED");
   PauseWithoutOpenDoors(6);
   OD_TEST_CHECK(od_get_input(&event, OD_NO_TIMEOUT, GETIN_RAW));
   OD_TEST_CHECK(event.EventType == EVENT_CHARACTER);
   OD_TEST_CHECK(event.bFromRemote && event.chKeyPress == 'Z');
   OD_TEST_CHECK(time_message_calls != 0);

   OD_TEST_CHECK(od_carrier());
   Marker("SESSION-DISCONNECT");
   for(count = 0; count < 100 && before_exit_calls == 0; ++count)
   {
      od_kernel();
      if(before_exit_calls == 0)
         od_sleep(50);
   }
   OD_TEST_CHECK(before_exit_calls == 1);
   return(0);
}

int main(int argc, char **argv)
{
   const char *scenario;
   DWORD carrier_deadline_seconds;
   DWORD carrier_seconds;
   WORD carrier_deadline_milliseconds;
   WORD carrier_milliseconds;
#if !defined(ODPLAT_DOS) && !defined(ODPLAT_DOS32)
   DWORD_PTR handle;
#endif
#ifdef ODPLAT_WIN32
   struct sockaddr_in address;
   WSADATA data;
   SOCKET connection;
#endif

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   OD_TEST_CHECK(argc == 2);
   scenario = argv[1];
#else
   OD_TEST_CHECK(argc == 3);
   scenario = argv[1];
#ifdef ODPLAT_WIN32
   OD_TEST_CHECK(WSAStartup(MAKEWORD(2, 0), &data) == 0);
   connection = socket(AF_INET, SOCK_STREAM, 0);
   OD_TEST_CHECK(connection != INVALID_SOCKET);
   memset(&address, 0, sizeof(address));
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = inet_addr("127.0.0.1");
   address.sin_port = htons((unsigned short)strtoul(argv[2], NULL, 10));
   OD_TEST_CHECK(connect(connection, (struct sockaddr *)&address,
      sizeof(address)) == 0);
   handle = (DWORD_PTR)connection;
#else
   handle = (DWORD_PTR)strtoul(argv[2], NULL, 10);
#endif
   OD_TEST_CHECK(handle != 0);
#endif

   memset(&od_control, 0, sizeof(od_control));
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   OD_TEST_CHECK(od_set_port(0));
#endif
   od_control.baud = 38400UL;
   od_control.od_connect_speed = 38400UL;
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
#ifndef OD_ACCEPTANCE_DOS_FOSSIL
   od_control.od_no_fossil = TRUE;
#endif
   od_control.od_com_flow_control = COM_NO_FLOW;
#else
   od_control.od_use_socket = TRUE;
   od_control.od_open_handle = handle;
#endif
   od_control.od_disable = DIS_INFOFILE | DIS_NAME_PROMPT | DIS_TIMEOUT |
      DIS_LOCAL_INPUT | DIS_SYSOP_KEYS;
   od_control.od_silent_mode = TRUE;
   od_control.od_nocopyright = TRUE;
   od_control.od_noexit = TRUE;
   od_control.od_before_exit = BeforeExit;
   od_control.user_ansi = TRUE;
   if(strcmp(scenario, "input") == 0)
      od_control.od_in_buf_size = 8;
   od_init();
   OD_TEST_CHECK(before_exit_calls == 0);

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
#ifdef OD_ACCEPTANCE_DOS_FOSSIL
   OD_TEST_CHECK(od_control.od_com_method == COM_FOSSIL);
#else
   OD_TEST_CHECK(od_control.od_com_method == COM_INTERNAL);
#endif
#else
   OD_TEST_CHECK(od_control.od_com_method == COM_SOCKET);
#endif
   od_get_time(&carrier_deadline_seconds,
      &carrier_deadline_milliseconds);
   carrier_deadline_seconds += 10;
   while(!od_carrier())
   {
      od_get_time(&carrier_seconds, &carrier_milliseconds);
      OD_TEST_CHECK(carrier_seconds < carrier_deadline_seconds
         || (carrier_seconds == carrier_deadline_seconds
            && carrier_milliseconds < carrier_deadline_milliseconds));
      od_sleep(10);
   }
   /* Let the emulated UART and its host TCP peer settle before emitting a
    * marker. Carrier state is exercised after the session is established. */
   od_sleep(500);

   if(strcmp(scenario, "input") == 0)
      return(RunInputScenario());
   if(strcmp(scenario, "interactive") == 0)
      return(RunInteractiveScenario());
   if(strcmp(scenario, "edit") == 0)
      return(RunEditScenario());
   if(strcmp(scenario, "display") == 0)
      return(RunDisplayScenario());
   if(strcmp(scenario, "emulation") == 0)
      return(RunEmulationScenario());
   if(strcmp(scenario, "listing") == 0)
      return(RunListingScenario());
   if(strcmp(scenario, "popup") == 0)
      return(RunPopupScenario());
   if(strcmp(scenario, "session") == 0)
      return(RunSessionScenario());

   fprintf(stderr, "unknown transport acceptance scenario: %s\n", scenario);
   return(2);
}
