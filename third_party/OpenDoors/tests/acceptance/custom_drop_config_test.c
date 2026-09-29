#include "test_support.h"

#define CONFIG_FILE "ODCUSTOM.CFG"
#define DROP_FILE "ODCUSTOM.DAT"

static int WriteLine(FILE *file, const char *text)
{
   return(fputs(text, file) >= 0 && fputc('\n', file) != EOF);
}

int main(void)
{
   static const char *drop_lines[] = {
      "2", "4", "57600", "no", "mixed USER", "jane", "DOE",
      "HANDLE", "2", "15", "1:30", "120", "2:120", "1:02:180",
      "yes", "no", "true", "false", "yes", "no", "31", "yes",
      "no", "42", "detroit city", "7", "boss person", "alice",
      "SMITH", "acceptance bbs", "yes", "no", "yes", "0"
   };
   static const char *mappings[] = {
      "COMPORT", "FOSSILPORT", "MODEMBPS", "LOCALMODE", "USERNAME",
      "USERFIRSTNAME", "USERLASTNAME", "ALIAS", "HOURSLEFT",
      "MINUTESLEFT", "MINUTESLEFT", "SECONDSLEFT", "SECONDSLEFT",
      "SECONDSLEFT", "ANSI", "ANSI", "AVATAR", "AVATAR",
      "PAGEPAUSING", "PAGEPAUSING", "SCREENLENGTH", "SCREENCLEARING",
      "SCREENCLEARING", "SECURITY", "CITY", "NODE", "SYSOPNAME",
      "SYSOPFIRSTNAME", "SYSOPLASTNAME", "SYSTEMNAME", "RIP", "RIP",
      "LOCALMODE", "COMPORT"
   };
   FILE *file;
   unsigned index;

   OD_TEST_CHECK(sizeof(drop_lines) / sizeof(drop_lines[0]) ==
      sizeof(mappings) / sizeof(mappings[0]));
   remove(CONFIG_FILE);
   remove(DROP_FILE);
   file = fopen(DROP_FILE, "w");
   OD_TEST_CHECK(file != NULL);
   for(index = 0; index < sizeof(drop_lines) / sizeof(drop_lines[0]); ++index)
      OD_TEST_CHECK(WriteLine(file, drop_lines[index]));
   OD_TEST_CHECK(fclose(file) == 0);

   file = fopen(CONFIG_FILE, "w");
   OD_TEST_CHECK(file != NULL);
   OD_TEST_CHECK(WriteLine(file, "CustomFileLine IGNORE"));
   OD_TEST_CHECK(WriteLine(file, "CustomFileName " DROP_FILE));
   for(index = 0; index < sizeof(mappings) / sizeof(mappings[0]); ++index)
   {
      OD_TEST_CHECK(fputs("CustomFileLine ", file) >= 0);
      OD_TEST_CHECK(WriteLine(file, mappings[index]));
   }
   OD_TEST_CHECK(fclose(file) == 0);

   ODTestConfigureLocal();
   od_control.od_force_local = FALSE;
   od_control.od_disable &= ~DIS_INFOFILE;
   od_control.od_config_file = INCLUDE_CONFIG_FILE;
   od_control.od_config_filename = CONFIG_FILE;
   od_init();

   OD_TEST_CHECK(od_control.od_info_type == CUSTOM);
   OD_TEST_CHECK(od_control.port == -1);
   OD_TEST_CHECK(strcmp(od_control.user_name, "Jane Doe") == 0);
   OD_TEST_CHECK(strcmp(od_control.user_handle, "Handle") == 0);
   OD_TEST_CHECK(strcmp(od_control.user_location, "Detroit City") == 0);
   OD_TEST_CHECK(od_control.user_timelimit == 296);
   OD_TEST_CHECK(!od_control.user_ansi);
   OD_TEST_CHECK(!od_control.user_avatar);
   OD_TEST_CHECK(!od_control.od_page_pausing);
   OD_TEST_CHECK(od_control.user_screen_length == 31);
   OD_TEST_CHECK((od_control.user_attribute & 0x02) == 0);
   OD_TEST_CHECK(od_control.user_security == 42);
   OD_TEST_CHECK(strcmp(od_control.sysop_name, "Alice Smith") == 0);
   OD_TEST_CHECK(strcmp(od_control.system_name, "acceptance bbs") == 0);
   OD_TEST_CHECK(!od_control.user_rip);

   od_exit(0, FALSE);
   remove(CONFIG_FILE);
   remove(DROP_FILE);
   return(0);
}
