#include <stdlib.h>

#include "test_support.h"

static char custom_keyword[32];
static char custom_options[80];
static INT custom_calls;
static INT flag_calls;

static BOOL FlagHandler(const char *argument)
{
   ++flag_calls;
   return(strcmp(argument, "-FAST") == 0);
}

static void CustomHandler(char *keyword, char *options)
{
   strncpy(custom_keyword, keyword, sizeof(custom_keyword) - 1);
   custom_keyword[sizeof(custom_keyword) - 1] = '\0';
   strncpy(custom_options, options, sizeof(custom_options) - 1);
   custom_options[sizeof(custom_options) - 1] = '\0';
   ++custom_calls;
}

int main(void)
{
   INT count;
   char **arguments;

   memset(&od_control, 0, sizeof(od_control));
   od_control.od_cmd_line_flag_handler = FlagHandler;
   od_control.od_cmd_line_handler = CustomHandler;
#ifdef ODPLAT_WIN32
   {
      char *argv[] = { "door", "-USERNAME", "Jane", "Smith",
         "-LOCATION", "Test", "Lab", "-BBSNAME", "Acceptance", "BBS",
         "-FAST", "-CUSTOM", "alpha", "beta", "-NODE", "7",
         "-LOCAL", "-BPS", "57600", "-PORT", "COM2", "-MAXTIME", "44",
         "-ADDRESS", "2f8", "-IRQ", "3", "-NOFOSSIL", "-NOFIFO",
         "-DROPFILE", "fixture", "-TIMELEFT", "12", "-SECURITY", "99",
         "-GRAPHICS", "N", "-SOCKET", "+123", "-SILENT", "-CP437UTF8" };
      od_parse_cmd_line_cons((INT)(sizeof(argv) / sizeof(argv[0])), argv);
   }
#else
   {
      char *argv[] = { "door", "-USERNAME", "Jane", "Smith",
         "-LOCATION", "Test", "Lab", "-BBSNAME", "Acceptance", "BBS",
         "-FAST", "-CUSTOM", "alpha", "beta", "-NODE", "7",
         "-LOCAL", "-BPS", "57600", "-PORT", "COM2", "-MAXTIME", "44",
         "-ADDRESS", "2f8", "-IRQ", "3", "-NOFOSSIL", "-NOFIFO",
         "-DROPFILE", "fixture", "-TIMELEFT", "12", "-SECURITY", "99",
         "-GRAPHICS", "N", "-SOCKET", "+123", "-SILENT", "-CP437UTF8" };
      od_parse_cmd_line((INT)(sizeof(argv) / sizeof(argv[0])), argv);
   }
#endif
   OD_TEST_CHECK(strcmp(od_control.user_name, "Jane Smith") == 0);
   OD_TEST_CHECK(strcmp(od_control.user_location, "Test Lab") == 0);
   OD_TEST_CHECK(strcmp(od_control.system_name, "Acceptance BBS") == 0);
   OD_TEST_CHECK(od_control.od_node == 7);
   OD_TEST_CHECK(flag_calls == 2);
   OD_TEST_CHECK(custom_calls == 1);
   OD_TEST_CHECK(strcmp(custom_keyword, "-CUSTOM") == 0);
   OD_TEST_CHECK(strcmp(custom_options, "alpha beta") == 0);
   OD_TEST_CHECK(od_control.od_force_local);
   OD_TEST_CHECK(od_control.baud == 57600);
   OD_TEST_CHECK(od_control.port == 1);
   OD_TEST_CHECK(od_control.od_maxtime == 44);
   OD_TEST_CHECK(od_control.od_com_address == 0x2f8);
   OD_TEST_CHECK(od_control.od_com_irq == 3);
   OD_TEST_CHECK(od_control.od_no_fossil);
   OD_TEST_CHECK(od_control.od_com_no_fifo);
   OD_TEST_CHECK(strcmp(od_control.info_path, "fixture") == 0);
   OD_TEST_CHECK(od_control.user_timelimit == 12);
   OD_TEST_CHECK(od_control.user_security == 99);
   OD_TEST_CHECK(!od_control.user_ansi);
   OD_TEST_CHECK(od_control.od_use_socket);
   OD_TEST_CHECK(od_control.od_open_handle == (DWORD_PTR)123);
   OD_TEST_CHECK(od_control.od_silent_mode);
   OD_TEST_CHECK(od_control.od_cp437_to_utf8_out);

   arguments = od_split_cmd_line("-NAME \"Jane Smith\" -LOCAL", &count);
   OD_TEST_CHECK(arguments != NULL);
#ifdef ODPLAT_WIN32
   OD_TEST_CHECK(count == 4);
   OD_TEST_CHECK(strcmp(arguments[1], "-NAME") == 0);
   OD_TEST_CHECK(strcmp(arguments[2], "Jane Smith") == 0);
   OD_TEST_CHECK(strcmp(arguments[3], "-LOCAL") == 0);
   OD_TEST_CHECK(arguments[4] == NULL);
#else
   OD_TEST_CHECK(count == 5);
   OD_TEST_CHECK(strcmp(arguments[1], "-NAME") == 0);
   OD_TEST_CHECK(strcmp(arguments[2], "\"Jane") == 0);
   OD_TEST_CHECK(strcmp(arguments[3], "Smith\"") == 0);
   OD_TEST_CHECK(arguments[5] == NULL);
#endif
   od_free_split_cmd_line(arguments);

   od_control.od_error = ERR_NONE;
#ifdef ODPLAT_WIN32
   od_parse_cmd_line_cons(1, NULL);
#else
   od_parse_cmd_line(1, NULL);
#endif
   OD_TEST_CHECK(od_control.od_error == ERR_PARAMETER);
   return(0);
}
