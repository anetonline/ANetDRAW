#include <setjmp.h>

#define UT_CUSTOM_MOCK_ODCfgGetDWordDecimal
#define UT_CUSTOM_MOCK_ODCfgGetNextTime
#define UT_CUSTOM_MOCK_ODCfgGetWordDecimal
#define UT_CUSTOM_MOCK_ODCfgGetWordHex
#define UT_CUSTOM_MOCK_ODCfgIsTrue
#define UT_CUSTOM_MOCK_ODDirChangeCurrent
#define UT_CUSTOM_MOCK_ODDirGetCurrent
#define UT_CUSTOM_MOCK_ODInitError
#define UT_CUSTOM_MOCK_ODSearchForDropFile
#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed
#define UT_CUSTOM_MOCK_ODStringCopy
#define UT_CUSTOM_MOCK_ODStringNormalizeLine
#define UT_CUSTOM_MOCK_ODStringToName
#define UT_CUSTOM_MOCK_exit
#define UT_CUSTOM_MOCK_fclose
#define UT_CUSTOM_MOCK_fgetc
#define UT_CUSTOM_MOCK_fgets
#define UT_CUSTOM_MOCK_fopen
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_localtime
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_od_color_config
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_strupr
#define UT_CUSTOM_MOCK_strupr
#define UT_CUSTOM_MOCK_strcat
#define UT_CUSTOM_MOCK_strchr
#define UT_CUSTOM_MOCK_strcmp
#define UT_CUSTOM_MOCK_strcpy
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_strncat
#define UT_CUSTOM_MOCK_strncpy
#define UT_CUSTOM_MOCK_time

FILE *fopen(const char *name, const char *mode);
char *fgets(char *text, int size, FILE *stream);
int fgetc(FILE *stream);
int fclose(FILE *stream);

static struct tm ut_time_block;
static unsigned ut_init_calls;
static unsigned ut_error_calls;
static unsigned ut_exit_calls;
static int ut_exit_status;
static jmp_buf ut_exit_target;
static BOOL ut_open_enabled;
static BOOL ut_fail_first_open;
static BOOL ut_inject_cr_line;
static unsigned ut_fopen_calls;
static BOOL ut_allow_allocation;
static char ut_allocation[257];
static char ut_current_directory[32];
static char ut_changed_directory[80];
static INT ut_search_result;
static char ut_search_path[80];
static unsigned ut_callback_calls;
static unsigned ut_color_calls;
static BOOL ut_clear_original_during_init;
static BOOL ut_public_call_allowed;

BOOL utm_ODSyncPublicCallAllowed(void)
{
   if(!ut_public_call_allowed)
      od_control.od_error = ERR_GENERALFAILURE;
   return ut_public_call_allowed;
}

static char *ut_text_keywords[TEXT_SIZE] = {
   "NODE", "BBSDIR", "DOORDIR", "LOGFILENAME", "DISABLELOGGING",
   "SUNDAYPAGINGHOURS", "MONDAYPAGINGHOURS", "TUESDAYPAGINGHOURS",
   "WEDNESDAYPAGINGHOURS", "THURSDAYPAGINGHOURS", "FRIDAYPAGINGHOURS",
   "SATURDAYPAGINGHOURS", "MAXIMUMDOORTIME", "SYSOPNAME", "SYSTEMNAME",
   "SWAPPINGDISABLE", "SWAPPINGDIR", "SWAPPINGNOEMS", "LOCKEDBPS",
   "SERIALPORT", "CUSTOMFILENAME", "CUSTOMFILELINE", "INACTIVITYTIMEOUT",
   "PAGEDURATION", "CHATUSERCOLOUR", "CHATSYSOPCOLOUR",
   "FILELISTTITLECOLOUR", "FILELISTNAMECOLOUR", "FILELISTSIZECOLOUR",
   "FILELISTDESCRIPTIONCOLOUR", "FILELISTOFFLINECOLOUR", "PERSONALITY",
   "NOFOSSIL", "PORTADDRESS", "PORTIRQ", "RECEIVEBUFFER",
   "TRANSMITBUFFER", "PAGEPROMPTCOLOUR", "LOCALMODE",
   "POPUPMENUTITLECOLOUR", "POPUPMENUBORDERCOLOUR", "POPUPMENUTEXTCOLOUR",
   "POPUPMENUKEYCOLOUR", "POPUPMENUHIGHLIGHTCOLOUR",
   "POPUPMENUHIGHKEYCOLOUR", "NOFIFO", "FIFOTRIGGERSIZE", "DISABLEDTR",
   "NODTRDISABLE"
};

static char *ut_line_keywords[LINES_SIZE] = {
   "IGNORE", "COMPORT", "FOSSILPORT", "MODEMBPS", "LOCALMODE",
   "USERNAME", "USERFIRSTNAME", "USERLASTNAME", "ALIAS", "HOURSLEFT",
   "MINUTESLEFT", "SECONDSLEFT", "ANSI", "AVATAR", "PAGEPAUSING",
   "SCREENLENGTH", "SCREENCLEARING", "SECURITY", "CITY", "NODE",
   "SYSOPNAME", "SYSOPFIRSTNAME", "SYSOPLASTNAME", "SYSTEMNAME", "RIP"
};

static size_t ut_text_length(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

static char *ut_copy(char *destination, const char *source)
{
   char *result = destination;
   while((*destination++ = *source++) != '\0') { }
   return result;
}

size_t utm_strlen(const char *text) { return ut_text_length(text); }

int utm_strcmp(const char *left, const char *right)
{
   while(*left != '\0' && *left == *right)
   {
      ++left;
      ++right;
   }
   return (unsigned char)*left - (unsigned char)*right;
}

char *utm_strcpy(char *destination, const char *source)
{
   return ut_copy(destination, source);
}

char *utm_strncpy(char *destination, const char *source, size_t count)
{
   char *result = destination;
   while(count != 0 && *source != '\0')
   {
      *destination++ = *source++;
      --count;
   }
   while(count-- != 0) *destination++ = '\0';
   return result;
}

char *utm_strcat(char *destination, const char *source)
{
   ut_copy(destination + ut_text_length(destination), source);
   return destination;
}

char *utm_strncat(char *destination, const char *source, size_t count)
{
   char *position = destination + ut_text_length(destination);
   while(count-- != 0 && *source != '\0') *position++ = *source++;
   *position = '\0';
   return destination;
}

static char *ut_upper_string(char *text)
{
   char *position;
   for(position = text; *position != '\0'; ++position)
      if(*position >= 'a' && *position <= 'z')
         *position = (char)(*position - 'a' + 'A');
   return text;
}

char *utm_od_strupr(char *text) { return ut_upper_string(text); }
char *utm_strupr(char *text) { return ut_upper_string(text); }

static unsigned long ut_decimal(const char *text)
{
   unsigned long value = 0;
   while(*text != '\0' && (*text < '0' || *text > '9')) ++text;
   while(*text >= '0' && *text <= '9')
   {
      value = value * 10UL + (unsigned long)(*text - '0');
      ++text;
   }
   return value;
}

static WORD utm_ODCfgGetWordDecimal(char *text)
{
   return (WORD)ut_decimal(text);
}

static DWORD utm_ODCfgGetDWordDecimal(char *text)
{
   return (DWORD)ut_decimal(text);
}

static BOOL utm_ODCfgGetWordHex(char *text, WORD *result)
{
   WORD value = 0;
   BOOL found = FALSE;
   int digit;
   while(*text != '\0')
   {
      if(*text >= '0' && *text <= '9') digit = *text - '0';
      else if(*text >= 'a' && *text <= 'f') digit = *text - 'a' + 10;
      else if(*text >= 'A' && *text <= 'F') digit = *text - 'A' + 10;
      else
      {
         if(found) break;
         ++text;
         continue;
      }
      found = TRUE;
      value = (WORD)(value * 16U + (WORD)digit);
      ++text;
   }
   if(found) *result = value;
   return found;
}

static void utm_ODCfgGetNextTime(char **text)
{
   char *position = *text;
   btTimeNumVals = 0;
   awTimeVal[0] = awTimeVal[1] = awTimeVal[2] = 0;
   while(*position == ' ' || *position == '\t') ++position;
   while(*position >= '0' && *position <= '9' && btTimeNumVals < 3)
   {
      awTimeVal[btTimeNumVals++] = (WORD)ut_decimal(position);
      while(*position >= '0' && *position <= '9') ++position;
      if(*position == ':' || *position == '.' || *position == ','
         || *position == ';') ++position;
   }
   *text = position;
}

static BOOL utm_ODCfgIsTrue(char *text)
{
   while(*text == ' ' || *text == '\t') ++text;
   return *text == '1' || *text == 't' || *text == 'T'
      || *text == 'y' || *text == 'Y' || *text == 'g' || *text == 'G';
}

FILE *utm_fopen(const char *name, const char *mode)
{
   ++ut_fopen_calls;
   if(!ut_open_enabled || (ut_fail_first_open && ut_fopen_calls == 1))
      return NULL;
   return fopen(name, mode);
}

char *utm_fgets(char *text, int size, FILE *stream)
{
   if(ut_inject_cr_line)
   {
      UT_ASSERT(size > 16);
      ut_inject_cr_line = FALSE;
      ut_copy(text, "NODE 8\rIGNORED");
      return text;
   }
   return fgets(text, size, stream);
}

int utm_fgetc(FILE *stream) { return fgetc(stream); }
int utm_fclose(FILE *stream) { return fclose(stream); }

void *utm_malloc(size_t size)
{
   if(!ut_allow_allocation || size > sizeof(ut_allocation)) return NULL;
   return ut_allocation;
}

void utm_free(void *memory) { (void)memory; }

void utm_ODStringCopy(char *destination, const char *source, INT size)
{
   INT index = 0;
   if(size <= 0) return;
   while(index + 1 < size && source[index] != '\0')
   {
      destination[index] = source[index];
      ++index;
   }
   destination[index] = '\0';
}

void utm_ODDirGetCurrent(char *path, INT size)
{
   utm_ODStringCopy(path, ut_current_directory, size);
}

void utm_ODDirChangeCurrent(char *path)
{
   utm_ODStringCopy(ut_changed_directory, path,
      (INT)sizeof(ut_changed_directory));
}

INT utm_ODSearchForDropFile(char **names, INT count, char *found,
   char *directory)
{
   (void)names;
   (void)count;
   (void)directory;
   if(ut_search_result != -1)
      utm_ODStringCopy(found, ut_search_path, 256);
   return ut_search_result;
}

size_t utm_ODStringNormalizeLine(char *line, BOOL *complete)
{
   size_t length = ut_text_length(line);
   *complete = length > 0 && line[length - 1] == '\n';
   if(*complete) line[--length] = '\0';
   if(length > 0 && line[length - 1] == '\r') line[--length] = '\0';
   return length;
}

void utm_ODStringToName(char *text)
{
   BOOL capitalize = TRUE;
   while(*text != '\0')
   {
      if(*text == '\r' || *text == '\n')
      {
         *text = '\0';
         break;
      }
      if(*text >= 'A' && *text <= 'Z') *text = (char)(*text + 'a' - 'A');
      if(capitalize && *text >= 'a' && *text <= 'z')
         *text = (char)(*text - 'a' + 'A');
      capitalize = *text == ' ' || *text == '\t' || *text == ','
         || *text == '.' || *text == '-';
      ++text;
   }
}

BYTE ODCALL utm_od_color_config(char *description)
{
   (void)description;
   ++ut_color_calls;
   return (BYTE)(0x20 + ut_color_calls);
}

char *utm_strchr(const char *text, int value)
{
   while(*text != '\0')
   {
      if(*text == value) return (char *)text;
      ++text;
   }
   return value == 0 ? (char *)text : NULL;
}

time_t utm_time(time_t *result)
{
   time_t value = (time_t)123;
   if(result != NULL) *result = value;
   return value;
}

struct tm *utm_localtime(const time_t *value)
{
   (void)value;
   ut_time_block.tm_wday = 1;
   return &ut_time_block;
}

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   UT_ASSERT_EQ_INT(TRUE, bCalledFromConfig);
   od_control.od_pagestartmin = 9;
   od_control.od_pageendmin = 9;
   od_control.od_inactivity = 9;
   od_control.od_page_len = 9;
   ut_copy(od_control.sysop_name, "Initialized Sysop");
   ut_copy(od_control.system_name, "Initialized System");
   if(ut_clear_original_during_init) szOriginalDir = NULL;
}

void utm_ODInitError(char *message)
{
   UT_ASSERT(message != NULL);
   ++ut_error_calls;
}

void utm_exit(int status)
{
   ++ut_exit_calls;
   ut_exit_status = status;
   longjmp(ut_exit_target, 1);
}

#ifdef ODPLAT_DOS32
static void ODCALL ut_custom_line(char *keyword, char *options)
#else
static void ut_custom_line(char *keyword, char *options)
#endif
{
   UT_ASSERT(keyword != NULL);
   UT_ASSERT(options != NULL);
   ++ut_callback_calls;
}

static FILE *ut_create_file(const char *name)
{
   FILE *file = fopen(name, "wt");
   UT_ASSERT(file != NULL);
   return file;
}

static void ut_write_drop_file(void)
{
   FILE *file = ut_create_file("UTDROP.DAT");
   unsigned index;
   for(index = 0; index < 300; ++index) fputc('X', file);
   fputc('\n', file);
   fputs("1\n2\n38400\nYES\nNO\nJOHN PUBLIC\nJOHN\nPUBLIC\nHANDLE\n1\n", file);
   fputs("2\n1:03\n120\n2:120\n1:2:180\nYES\nNO\nYES\nNO\n", file);
   fputs("YES\nNO\n25\nYES\nNO\n42\nDETROIT\n7\n", file);
   fputs("JANE OPERATOR\nJANE\nOPERATOR\nTEST BBS\nYES\n", file);
   fclose(file);
}

static void ut_write_custom_line(FILE *file, const char *keyword)
{
   fprintf(file, "CUSTOMFILELINE %s\n", keyword);
}

static void ut_write_full_configuration(void)
{
   FILE *file = ut_create_file("UTCFG.CFG");
   fputs("; comment\n\t  \n", file);
   fputs("NODE 3\nBBSDIR BBS-PATH\nDOORDIR\n", file);
   fprintf(file, "DOORDIR %c\n", DIRSEP);
   fprintf(file, "DOORDIR C:%c\n", DIRSEP);
   fprintf(file, "DOORDIR WORK%c\n", DIRSEP);
   fputs("LOGFILENAME OPENDOORS.LOG\nDISABLELOGGING\n", file);
   fputs("SUNDAYPAGINGHOURS 1:02 3:04\n", file);
   fputs("MONDAYPAGINGHOURS 5:06 7:08\n", file);
   fputs("TUESDAYPAGINGHOURS 9:10 11:12\n", file);
   fputs("WEDNESDAYPAGINGHOURS 13:14 15:16\n", file);
   fputs("THURSDAYPAGINGHOURS 17:18 19:20\n", file);
   fputs("FRIDAYPAGINGHOURS 21:22 23:24\n", file);
   fputs("SATURDAYPAGINGHOURS 2:03 4:05\n", file);
   fputs("MAXIMUMDOORTIME 77\nSYSOPNAME CONFIG SYSOP\n", file);
   fputs("SYSTEMNAME CONFIG SYSTEM\nSWAPPINGDISABLE\n", file);
   fputs("SWAPPINGDIR SWAP-PATH\nSWAPPINGNOEMS\nLOCKEDBPS 57600\n", file);
   fputs("SERIALPORT 4\nCUSTOMFILELINE IGNORE\n", file);
   fputs("CUSTOMFILENAME UTDROP.DAT\n", file);
   ut_write_custom_line(file, "IGNORE");
   ut_write_custom_line(file, "COMPORT");
   ut_write_custom_line(file, "FOSSILPORT");
   ut_write_custom_line(file, "MODEMBPS");
   ut_write_custom_line(file, "LOCALMODE");
   ut_write_custom_line(file, "LOCALMODE");
   ut_write_custom_line(file, "USERNAME");
   ut_write_custom_line(file, "USERFIRSTNAME");
   ut_write_custom_line(file, "USERLASTNAME");
   ut_write_custom_line(file, "ALIAS");
   ut_write_custom_line(file, "HOURSLEFT");
   ut_write_custom_line(file, "MINUTESLEFT");
   ut_write_custom_line(file, "MINUTESLEFT");
   ut_write_custom_line(file, "SECONDSLEFT");
   ut_write_custom_line(file, "SECONDSLEFT");
   ut_write_custom_line(file, "SECONDSLEFT");
   ut_write_custom_line(file, "ANSI");
   ut_write_custom_line(file, "ANSI");
   ut_write_custom_line(file, "AVATAR");
   ut_write_custom_line(file, "AVATAR");
   ut_write_custom_line(file, "PAGEPAUSING");
   ut_write_custom_line(file, "PAGEPAUSING");
   ut_write_custom_line(file, "SCREENLENGTH");
   ut_write_custom_line(file, "SCREENCLEARING");
   ut_write_custom_line(file, "SCREENCLEARING");
   ut_write_custom_line(file, "SECURITY");
   ut_write_custom_line(file, "CITY");
   ut_write_custom_line(file, "NODE");
   ut_write_custom_line(file, "SYSOPNAME");
   ut_write_custom_line(file, "SYSOPFIRSTNAME");
   ut_write_custom_line(file, "SYSOPLASTNAME");
   ut_write_custom_line(file, "SYSTEMNAME");
   ut_write_custom_line(file, "RIP");
   fputs("CUSTOMFILENAME UTDROP.DAT\n", file);
   fputs("INACTIVITYTIMEOUT 65535\nINACTIVITYTIMEOUT 20\nPAGEDURATION 13\n", file);
   fputs("CHATUSERCOLOUR C\nCHATSYSOPCOLOUR C\n", file);
   fputs("FILELISTTITLECOLOUR C\nFILELISTNAMECOLOUR C\n", file);
   fputs("FILELISTSIZECOLOUR C\nFILELISTDESCRIPTIONCOLOUR C\n", file);
   fputs("FILELISTOFFLINECOLOUR C\nPERSONALITY STANDARD\nNOFOSSIL\n", file);
   fputs("PORTADDRESS XYZ\nPORTADDRESS 2F8\nPORTIRQ 5\n", file);
   fputs("RECEIVEBUFFER 512\nTRANSMITBUFFER 768\nPAGEPROMPTCOLOUR C\n", file);
   fputs("LOCALMODE\nPOPUPMENUTITLECOLOUR C\n", file);
   fputs("POPUPMENUBORDERCOLOUR C\nPOPUPMENUTEXTCOLOUR C\n", file);
   fputs("POPUPMENUKEYCOLOUR C\nPOPUPMENUHIGHLIGHTCOLOUR C\n", file);
   fputs("POPUPMENUHIGHKEYCOLOUR C\nNOFIFO\nFIFOTRIGGERSIZE 8\n", file);
   fputs("DISABLEDTR DROP DTR\nNODTRDISABLE\n", file);
   fputs("APPLICATIONOPTION value with spaces   \n", file);
   fclose(file);
}

static void reset_fixture(void)
{
   unsigned index;
   memset(&od_control, 0, sizeof(od_control));
   memset(&ut_time_block, 0, sizeof(ut_time_block));
   bIsCallbackActive = FALSE;
   bCalledFromConfig = FALSE;
   ut_init_calls = 0;
   ut_error_calls = 0;
   ut_exit_calls = 0;
   ut_exit_status = -1;
   ut_open_enabled = FALSE;
   ut_fail_first_open = FALSE;
   ut_inject_cr_line = FALSE;
   ut_fopen_calls = 0;
   ut_allow_allocation = TRUE;
   ut_current_directory[0] = '\0';
   ut_changed_directory[0] = '\0';
   ut_search_result = -1;
   ut_search_path[0] = '\0';
   ut_callback_calls = 0;
   ut_color_calls = 0;
   ut_clear_original_during_init = FALSE;
   ut_public_call_allowed = TRUE;
   szOriginalDir = NULL;
   wODNodeNumber = 0;
   dwForcedBPS = 0;
   nForcedPort = 0;
   bSysopNameSet = FALSE;
   bSystemNameSet = FALSE;
   szForcedSysopName[0] = '\0';
   szForcedSystemName[0] = '\0';
   szDesiredPersonality[0] = '\0';
   for(index = 0; index < TEXT_SIZE; ++index)
      utm_ODStringCopy(od_config_text[index], ut_text_keywords[index], 33);
   for(index = 0; index < LINES_SIZE; ++index)
      utm_ODStringCopy(od_config_lines[index], ut_line_keywords[index], 33);
   od_control.od_errorlevel[1] = 17;
}

static void missing_default_configuration_is_optional(void)
{
   reset_fixture();
   utt_ODConfigInit();
   UT_ASSERT(od_control.od_config_filename != NULL);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(0, ut_error_calls);
   UT_ASSERT_EQ_UINT(0, ut_exit_calls);
   UT_ASSERT_EQ_INT(FALSE, bCalledFromConfig);
   UT_ASSERT_EQ_INT(FALSE, bIsCallbackActive);
}

static void rejects_a_terminal_session(void)
{
   reset_fixture();
   ut_public_call_allowed = FALSE;
   utt_ODConfigInit();
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_init_calls);
}

static void missing_explicit_configuration_is_fatal(void)
{
   reset_fixture();
   od_control.od_config_filename = "missing.cfg";
   if(setjmp(ut_exit_target) == 0)
   {
      utt_ODConfigInit();
      UT_ASSERT(FALSE);
   }
   UT_ASSERT_EQ_INT(ERR_FILEOPEN, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_error_calls);
   UT_ASSERT_EQ_UINT(1, ut_exit_calls);
   UT_ASSERT_EQ_INT(17, ut_exit_status);
   UT_ASSERT_EQ_UINT(0, ut_init_calls);
}

static void parses_every_builtin_and_custom_drop_mapping(void)
{
   reset_fixture();
   ut_write_drop_file();
   ut_write_full_configuration();
   ut_open_enabled = TRUE;
   ut_time_block.tm_wday = 1;
   ut_copy(ut_current_directory, "ORIGINAL");
   ut_search_result = 0;
   ut_copy(ut_search_path, "UTDROP.DAT");
   od_control.od_config_filename = "UTCFG.CFG";
   od_control.config_function = ut_custom_line;

   utt_ODConfigInit();

   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_INT(FALSE, bIsCallbackActive);
   UT_ASSERT_EQ_INT(FALSE, bCalledFromConfig);
   UT_ASSERT(ut_callback_calls > TEXT_SIZE);
   UT_ASSERT_EQ_UINT(7, wODNodeNumber);
   UT_ASSERT(utm_strcmp("BBS-PATH", od_control.info_path) == 0);
   UT_ASSERT(utm_strcmp("WORK", ut_changed_directory) == 0);
   UT_ASSERT(utm_strcmp("OPENDOORS.LOG", od_control.od_logfile_name) == 0);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_logfile_disable);
   UT_ASSERT_EQ_INT(5 * 60 + 6, od_control.od_pagestartmin);
   UT_ASSERT_EQ_INT(7 * 60 + 8, od_control.od_pageendmin);
   UT_ASSERT_EQ_INT(20, od_control.od_inactivity);
   UT_ASSERT_EQ_INT(13, od_control.od_page_len);
   UT_ASSERT(utm_strcmp("CONFIG SYSOP", od_control.sysop_name) == 0);
   UT_ASSERT(utm_strcmp("CONFIG SYSTEM", od_control.system_name) == 0);
   UT_ASSERT_EQ_UINT(57600UL, dwForcedBPS);
   UT_ASSERT_EQ_INT(4, nForcedPort);
   UT_ASSERT_EQ_INT(CUSTOM, od_control.od_info_type);
   UT_ASSERT(utm_strcmp("John Public", od_control.user_name) == 0);
   UT_ASSERT(utm_strcmp("Handle", od_control.user_handle) == 0);
   UT_ASSERT(utm_strcmp("Detroit", od_control.user_location) == 0);
   UT_ASSERT_EQ_INT(196, od_control.user_timelimit);
   UT_ASSERT_EQ_INT(FALSE, od_control.user_ansi);
   UT_ASSERT_EQ_INT(FALSE, od_control.user_avatar);
   UT_ASSERT_EQ_INT(FALSE, od_control.od_page_pausing);
   UT_ASSERT_EQ_INT(25, od_control.user_screen_length);
   UT_ASSERT_EQ_INT(42, od_control.user_security);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_rip);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_no_fossil);
   UT_ASSERT_EQ_UINT(0x2f8, (WORD)od_control.od_com_address);
   UT_ASSERT_EQ_INT(5, od_control.od_com_irq);
   UT_ASSERT_EQ_UINT(512, od_control.od_com_rx_buf);
   UT_ASSERT_EQ_UINT(768, od_control.od_com_tx_buf);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_force_local);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_com_no_fifo);
   UT_ASSERT_EQ_UINT(8, od_control.od_com_fifo_trigger);
   UT_ASSERT(utm_strcmp("DROP DTR", od_control.od_disable_dtr) == 0);
   UT_ASSERT((od_control.od_disable & DIS_DTR_DISABLE) != 0);
   UT_ASSERT_EQ_UINT(14, ut_color_calls);
   UT_ASSERT(utm_strcmp("STANDARD", szDesiredPersonality) == 0);

   remove("UTCFG.CFG");
   remove("UTDROP.DAT");
}

static void ut_write_config_text(const char *text)
{
   FILE *file = ut_create_file("UTCFG.CFG");
   fputs(text, file);
   fclose(file);
}

static void retries_a_path_or_drive_qualified_filename_by_basename(void)
{
   char path_name[40];
   reset_fixture();
   ut_write_config_text("NODE 11\n");
   ut_open_enabled = TRUE;
   ut_fail_first_open = TRUE;
   sprintf(path_name, "missing%cUTCFG.CFG", DIRSEP);
   od_control.od_config_filename = path_name;
   utt_ODConfigInit();
   UT_ASSERT_EQ_UINT(2, ut_fopen_calls);
   UT_ASSERT_EQ_UINT(11, wODNodeNumber);

   reset_fixture();
   ut_open_enabled = TRUE;
   ut_fail_first_open = TRUE;
   od_control.od_config_filename = "Z:UTCFG.CFG";
   utt_ODConfigInit();
   UT_ASSERT_EQ_UINT(2, ut_fopen_calls);
   UT_ASSERT_EQ_UINT(11, wODNodeNumber);
   remove("UTCFG.CFG");
}

static void rejects_an_overlong_fallback_basename(void)
{
   char name[280];
   unsigned index;
   reset_fixture();
   name[0] = DIRSEP;
   for(index = 1; index + 1 < sizeof(name); ++index) name[index] = 'A';
   name[sizeof(name) - 1] = '\0';
   od_control.od_config_filename = name;
   if(setjmp(ut_exit_target) == 0)
   {
      utt_ODConfigInit();
      UT_ASSERT(FALSE);
   }
   UT_ASSERT_EQ_UINT(1, ut_fopen_calls);
}

static void parses_empty_cr_tab_and_overlong_tokens(void)
{
   FILE *file;
   unsigned index;
   reset_fixture();
   file = ut_create_file("UTCFG.CFG");
   fputc(0, file);
   fputc('\n', file);
   fputs("NODE 8\n", file);
   for(index = 0; index < 40; ++index) fputc('A', file);
   fputs("\tvalue\t\t\nUNKNOWN\tvalue\t\n", file);
   fclose(file);
   ut_open_enabled = TRUE;
   ut_inject_cr_line = TRUE;
   od_control.od_config_filename = "UTCFG.CFG";
   utt_ODConfigInit();
   UT_ASSERT_EQ_UINT(8, wODNodeNumber);
   remove("UTCFG.CFG");
}

static void handles_directory_setup_failures_and_post_init_loss(void)
{
   reset_fixture();
   ut_write_config_text("DOORDIR WORK\n");
   ut_open_enabled = TRUE;
   ut_allow_allocation = FALSE;
   od_control.od_config_filename = "UTCFG.CFG";
   utt_ODConfigInit();
   UT_ASSERT(ut_changed_directory[0] == '\0');

   reset_fixture();
   ut_open_enabled = TRUE;
   ut_current_directory[0] = '\0';
   od_control.od_config_filename = "UTCFG.CFG";
   utt_ODConfigInit();
   UT_ASSERT(ut_changed_directory[0] == '\0');

   reset_fixture();
   ut_open_enabled = TRUE;
   ut_copy(ut_current_directory, "ORIGINAL");
   ut_clear_original_during_init = TRUE;
   od_control.od_config_filename = "UTCFG.CFG";
   utt_ODConfigInit();
   UT_ASSERT(ut_changed_directory[0] == '\0');
   remove("UTCFG.CFG");
}

static void handles_custom_drop_file_selection_failures_and_eof(void)
{
   FILE *file;
   unsigned index;
   reset_fixture();
   ut_write_config_text("LOCALMODE\nCUSTOMFILENAME UTDROP.DAT\n"
      "CUSTOMFILELINE IGNORE\n");
   ut_open_enabled = TRUE;
   od_control.od_config_filename = "UTCFG.CFG";
   utt_ODConfigInit();
   UT_ASSERT_EQ_INT(0, od_control.od_info_type);

   ut_write_config_text("CUSTOMFILENAME UTDROP.DAT\n"
      "CUSTOMFILELINE IGNORE\n");
   reset_fixture();
   ut_open_enabled = TRUE;
   od_control.od_config_filename = "UTCFG.CFG";
   ut_search_result = -1;
   utt_ODConfigInit();
   UT_ASSERT_EQ_INT(0, od_control.od_info_type);

   reset_fixture();
   ut_open_enabled = TRUE;
   od_control.od_config_filename = "UTCFG.CFG";
   ut_search_result = 0;
   ut_copy(ut_search_path, "MISSING.DAT");
   utt_ODConfigInit();
   UT_ASSERT_EQ_INT(0, od_control.od_info_type);

   file = ut_create_file("UTDROP.DAT");
   fclose(file);
   reset_fixture();
   ut_open_enabled = TRUE;
   od_control.od_config_filename = "UTCFG.CFG";
   ut_search_result = 0;
   ut_copy(ut_search_path, "UTDROP.DAT");
   utt_ODConfigInit();
   UT_ASSERT_EQ_INT(CUSTOM, od_control.od_info_type);

   file = ut_create_file("UTDROP.DAT");
   for(index = 0; index < 300; ++index) fputc('X', file);
   fclose(file);
   reset_fixture();
   ut_open_enabled = TRUE;
   od_control.od_config_filename = "UTCFG.CFG";
   ut_search_result = 0;
   ut_copy(ut_search_path, "UTDROP.DAT");
   utt_ODConfigInit();
   UT_ASSERT_EQ_INT(CUSTOM, od_control.od_info_type);
   remove("UTCFG.CFG");
   remove("UTDROP.DAT");
}

static void retains_initialized_inactivity_for_explicit_zero(void)
{
   reset_fixture();
   ut_write_config_text("INACTIVITYTIMEOUT 0\n");
   ut_open_enabled = TRUE;
   szOriginalDir = ut_allocation;
   od_control.od_config_filename = "UTCFG.CFG";
   utt_ODConfigInit();
   UT_ASSERT_EQ_INT(9, od_control.od_inactivity);
   UT_ASSERT(ut_changed_directory[0] == '\0');
   remove("UTCFG.CFG");
}

static const UTTestCase ut_cases[] = {
   {"terminal session", rejects_a_terminal_session},
   {"optional default", missing_default_configuration_is_optional},
   {"required file", missing_explicit_configuration_is_fatal},
   {"all configuration mappings", parses_every_builtin_and_custom_drop_mapping},
   {"fallback basename", retries_a_path_or_drive_qualified_filename_by_basename},
   {"overlong fallback", rejects_an_overlong_fallback_basename},
   {"line parser boundaries", parses_empty_cr_tab_and_overlong_tokens},
   {"directory failures", handles_directory_setup_failures_and_post_init_loss},
   {"custom drop failures", handles_custom_drop_file_selection_failures_and_eof},
   {"zero inactivity", retains_initialized_inactivity_for_explicit_zero}
};
