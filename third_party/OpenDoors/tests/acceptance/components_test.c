#include "test_support.h"

#define CONFIG_FILE "ODACCEPT.CFG"
#define LOG_FILE "ODACCEPT.LOG"
#define CONFIG_COLOR (L_CYAN | (D_BLUE << 4))

static int custom_calls;
static int personality_initialize_calls;

static void ConfigComplete(void)
{
}

static void CustomConfig(char *keyword, char *options)
{
   if(strcmp(keyword, "ACCEPTANCEOPTION") == 0 &&
      strcmp(options, "recognized value") == 0)
      ++custom_calls;
}

static OD_PERSONALITY_PROC TestPersonality;

static void TestPersonality(BYTE operation)
{
   if(operation == PEROP_INITIALIZE)
      ++personality_initialize_calls;
}

int main(void)
{
   FILE *file;
   char log_contents[1024];
   size_t length;

   remove(CONFIG_FILE);
   remove(LOG_FILE);
   file = fopen(CONFIG_FILE, "w");
   OD_TEST_CHECK(file != NULL);
   OD_TEST_CHECK(fputs("; public configuration acceptance fixture\n\t \n",
      file) >= 0);
   OD_TEST_CHECK(fputs("Node 7\nBBSDir ACCEPTANCE-BBS\n", file) >= 0);
   OD_TEST_CHECK(fputs("LogFileName " LOG_FILE "\nDisableLogging\n",
      file) >= 0);
   OD_TEST_CHECK(fputs("SundayPagingHours 1:02 3:04\n", file) >= 0);
   OD_TEST_CHECK(fputs("MondayPagingHours 1:02 3:04\n", file) >= 0);
   OD_TEST_CHECK(fputs("TuesdayPagingHours 1:02 3:04\n", file) >= 0);
   OD_TEST_CHECK(fputs("WednesdayPagingHours 1:02 3:04\n", file) >= 0);
   OD_TEST_CHECK(fputs("ThursdayPagingHours 1:02 3:04\n", file) >= 0);
   OD_TEST_CHECK(fputs("FridayPagingHours 1:02 3:04\n", file) >= 0);
   OD_TEST_CHECK(fputs("SaturdayPagingHours 1:02 3:04\n", file) >= 0);
   OD_TEST_CHECK(fputs("MaximumDoorTime 37\n", file) >= 0);
   OD_TEST_CHECK(fputs("SysopName Acceptance Sysop\n", file) >= 0);
   OD_TEST_CHECK(fputs("SystemName Acceptance System\n", file) >= 0);
   OD_TEST_CHECK(fputs("SwappingDisable\nSwappingDir ACCEPTANCE-SWAP\n",
      file) >= 0);
   OD_TEST_CHECK(fputs("SwappingNoEMS\nLockedBPS 1\n",
      file) >= 0);
   OD_TEST_CHECK(fputs("InactivityTimeout 23\nPageDuration 9\n",
      file) >= 0);
   OD_TEST_CHECK(fputs("ChatUserColour Bright Cyan on Blue\n", file) >= 0);
   OD_TEST_CHECK(fputs("ChatSysopColour Bright Cyan on Blue\n", file) >= 0);
   OD_TEST_CHECK(fputs("FileListTitleColour Bright Cyan on Blue\n",
      file) >= 0);
   OD_TEST_CHECK(fputs("FileListNameColour Bright Cyan on Blue\n",
      file) >= 0);
   OD_TEST_CHECK(fputs("FileListSizeColour Bright Cyan on Blue\n",
      file) >= 0);
   OD_TEST_CHECK(fputs(
      "FileListDescriptionColour Bright Cyan on Blue\n", file) >= 0);
   OD_TEST_CHECK(fputs("FileListOfflineColour Bright Cyan on Blue\n",
      file) >= 0);
   OD_TEST_CHECK(fputs("Personality STANDARD\n", file) >= 0);
#ifndef ODPLAT_NIX
   OD_TEST_CHECK(fputs("NoFossil\n", file) >= 0);
#endif
   OD_TEST_CHECK(fputs("PortAddress port=2f8\nPortIRQ 5\n", file) >= 0);
   OD_TEST_CHECK(fputs("ReceiveBuffer 512\nTransmitBuffer 768\n",
      file) >= 0);
   OD_TEST_CHECK(fputs("PagePromptColour Bright Cyan on Blue\n",
      file) >= 0);
   OD_TEST_CHECK(fputs("LocalMode\n", file) >= 0);
   OD_TEST_CHECK(fputs("PopupMenuTitleColour Bright Cyan on Blue\n",
      file) >= 0);
   OD_TEST_CHECK(fputs("PopupMenuBorderColour Bright Cyan on Blue\n",
      file) >= 0);
   OD_TEST_CHECK(fputs("PopupMenuTextColour Bright Cyan on Blue\n",
      file) >= 0);
   OD_TEST_CHECK(fputs("PopupMenuKeyColour Bright Cyan on Blue\n",
      file) >= 0);
   OD_TEST_CHECK(fputs("PopupMenuHighlightColour Bright Cyan on Blue\n",
      file) >= 0);
   OD_TEST_CHECK(fputs("PopupMenuHighKeyColour Bright Cyan on Blue\n",
      file) >= 0);
   OD_TEST_CHECK(fputs("NoFIFO\nFIFOTriggerSize 8\n", file) >= 0);
   OD_TEST_CHECK(fputs("DisableDTR DROP DTR\nNoDTRDisable\n", file) >= 0);
   OD_TEST_CHECK(fputs("AcceptanceOption recognized value\n", file) >= 0);
   OD_TEST_CHECK(fclose(file) == 0);

   ODTestConfigureLocal();
   od_control.od_config_file = INCLUDE_CONFIG_FILE;
   od_control.od_config_filename = CONFIG_FILE;
   od_control.od_config_function = CustomConfig;
   od_control.od_config_callback = ConfigComplete;
   od_control.od_logfile = INCLUDE_LOGFILE;
   strcpy(od_control.od_logfile_name, LOG_FILE);
   od_control.od_mps = INCLUDE_MPS;
   od_init();

   OD_TEST_CHECK(od_control.od_maxtime == 37);
   OD_TEST_CHECK(strcmp(od_control.info_path, "ACCEPTANCE-BBS") == 0);
   OD_TEST_CHECK(strcmp(od_control.od_logfile_name, LOG_FILE) == 0);
   OD_TEST_CHECK(od_control.od_logfile_disable);
   OD_TEST_CHECK(od_control.od_pagestartmin == 62);
   OD_TEST_CHECK(od_control.od_pageendmin == 184);
   OD_TEST_CHECK(strcmp(od_control.sysop_name, "Acceptance Sysop") == 0);
   OD_TEST_CHECK(strcmp(od_control.system_name, "Acceptance System") == 0);
   OD_TEST_CHECK(od_control.od_swapping_disable);
   OD_TEST_CHECK(od_control.od_swapping_noems);
   OD_TEST_CHECK(strcmp(od_control.od_swapping_path,
      "ACCEPTANCE-SWAP") == 0);
   OD_TEST_CHECK(od_control.od_inactivity == 23);
   OD_TEST_CHECK(od_control.od_page_len == 9);
   OD_TEST_CHECK(od_control.od_chat_color1 == CONFIG_COLOR);
   OD_TEST_CHECK(od_control.od_chat_color2 == CONFIG_COLOR);
   OD_TEST_CHECK(od_control.od_list_title_col == CONFIG_COLOR);
   OD_TEST_CHECK(od_control.od_list_name_col == CONFIG_COLOR);
   OD_TEST_CHECK(od_control.od_list_size_col == CONFIG_COLOR);
   OD_TEST_CHECK(od_control.od_list_comment_col == CONFIG_COLOR);
   OD_TEST_CHECK(od_control.od_list_offline_col == CONFIG_COLOR);
#ifndef ODPLAT_NIX
   OD_TEST_CHECK(od_control.od_no_fossil);
#endif
   OD_TEST_CHECK((WORD)od_control.od_com_address == 0x2f8U);
   OD_TEST_CHECK(od_control.od_com_irq == 5);
   OD_TEST_CHECK(od_control.od_com_rx_buf == 512);
   OD_TEST_CHECK(od_control.od_com_tx_buf == 768);
   OD_TEST_CHECK(od_control.od_continue_col == CONFIG_COLOR);
   OD_TEST_CHECK(od_control.od_force_local);
   OD_TEST_CHECK(od_control.od_menu_title_col == CONFIG_COLOR);
   OD_TEST_CHECK(od_control.od_menu_border_col == CONFIG_COLOR);
   OD_TEST_CHECK(od_control.od_menu_text_col == CONFIG_COLOR);
   OD_TEST_CHECK(od_control.od_menu_key_col == CONFIG_COLOR);
   OD_TEST_CHECK(od_control.od_menu_highlight_col == CONFIG_COLOR);
   OD_TEST_CHECK(od_control.od_menu_highkey_col == CONFIG_COLOR);
   OD_TEST_CHECK(od_control.od_com_no_fifo);
   OD_TEST_CHECK(od_control.od_com_fifo_trigger == 8);
   OD_TEST_CHECK(strcmp(od_control.od_disable_dtr, "DROP DTR") == 0);
   OD_TEST_CHECK((od_control.od_disable & DIS_DTR_DISABLE) != 0);
   OD_TEST_CHECK(custom_calls == 1);
   OD_TEST_CHECK(od_control.od_config_callback == ConfigComplete);
   od_control.od_logfile_disable = TRUE;
   OD_TEST_CHECK(od_log_write("disabled acceptance log record"));
   od_control.od_logfile_disable = FALSE;
   OD_TEST_CHECK(od_log_open());
   OD_TEST_CHECK(od_log_open());
   OD_TEST_CHECK(od_log_write("acceptance log record"));
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) || defined(ODPLAT_WIN32)
   OD_TEST_CHECK(od_add_personality("Acceptance", 1, 1, TestPersonality));
   OD_TEST_CHECK(od_set_personality("Acceptance"));
   OD_TEST_CHECK(personality_initialize_calls == 1);
   OD_TEST_CHECK(!od_set_personality("Missing"));
   OD_TEST_CHECK(od_control.od_error == ERR_LIMIT);
#else
   OD_TEST_CHECK(!od_add_personality("Acceptance", 1, 1, TestPersonality));
   OD_TEST_CHECK(od_control.od_error == ERR_UNSUPPORTED);
#endif

   od_exit(0, FALSE);
   file = fopen(LOG_FILE, "r");
   OD_TEST_CHECK(file != NULL);
   length = fread(log_contents, 1, sizeof(log_contents) - 1, file);
   OD_TEST_CHECK(!ferror(file));
   log_contents[length] = '\0';
   OD_TEST_CHECK(fclose(file) == 0);
   OD_TEST_CHECK(strstr(log_contents, "acceptance log record") != NULL);
   pdef_opendoors(PEROP_INITIALIZE);
   pdef_opendoors(PEROP_DEINITIALIZE);
   pdef_od_onerow(PEROP_INITIALIZE);
   pdef_od_onerow(PEROP_DEINITIALIZE);
   pdef_pcboard(PEROP_INITIALIZE);
   pdef_pcboard(PEROP_DEINITIALIZE);
   pdef_ra(PEROP_INITIALIZE);
   pdef_ra(PEROP_DEINITIALIZE);
   pdef_wildcat(PEROP_INITIALIZE);
   pdef_wildcat(PEROP_DEINITIALIZE);
   remove(CONFIG_FILE);
   remove(LOG_FILE);
   return(0);
}
