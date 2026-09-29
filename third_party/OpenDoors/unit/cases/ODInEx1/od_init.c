#include <setjmp.h>

#ifndef UT_TURBO_SHARD
#define UT_TURBO_SHARD 0
#endif

#define UT_CUSTOM_MOCK_ODFileAccessMode
#define UT_CUSTOM_MOCK_ODFramingIsEightBit
#define UT_CUSTOM_MOCK_ODInQueueAlloc
#define UT_CUSTOM_MOCK_ODInitError
#define UT_CUSTOM_MOCK_ODInitPartTwo
#define UT_CUSTOM_MOCK_ODInitReadExitInfo
#define UT_CUSTOM_MOCK_ODInitReadSFDoorsDAT
#define UT_CUSTOM_MOCK_ODMakeFilename
#define UT_CUSTOM_MOCK_ODPlatInit
#define UT_CUSTOM_MOCK_ODSessionTimeInitialize
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#endif
#define UT_CUSTOM_MOCK_ODSearchForDropFile
#define UT_CUSTOM_MOCK_ODStringCopy
#define UT_CUSTOM_MOCK_ODStringHasTail
#define UT_CUSTOM_MOCK_ODStringToName
#define UT_CUSTOM_MOCK_ODSyncSessionInitialize
#define UT_CUSTOM_MOCK_atoi
#define UT_CUSTOM_MOCK_atol
#define UT_CUSTOM_MOCK_exit
#define UT_CUSTOM_MOCK_fclose
#define UT_CUSTOM_MOCK_fgetc
#define UT_CUSTOM_MOCK_fgets
#define UT_CUSTOM_MOCK_fopen
#define UT_CUSTOM_MOCK_getenv
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_od_strupr
#define UT_CUSTOM_MOCK_od_exit
#define UT_CUSTOM_MOCK_od_set_user_8bit
#define UT_CUSTOM_MOCK_safe_strcat
#define UT_CUSTOM_MOCK_safe_strcpy
#define UT_CUSTOM_MOCK_setlocale
#define UT_CUSTOM_MOCK_sprintf
#define UT_CUSTOM_MOCK_strcat
#define UT_CUSTOM_MOCK_strchr
#define UT_CUSTOM_MOCK_strcmp
#define UT_CUSTOM_MOCK_strcpy
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_strncasecmp
#define UT_CUSTOM_MOCK_strncmp
#define UT_CUSTOM_MOCK_strncpy
#define UT_CUSTOM_MOCK_strnicmp
#define UT_CUSTOM_MOCK_strstr
#define UT_CUSTOM_MOCK_strupr
#define UT_CUSTOM_MOCK_time

#define UT_INIT_MAX_LINES 64
#define UT_INIT_LINE_SIZE 128

static int ut_queue_token;
static int ut_file_token;
static char ut_allocations[25][81];
static char ut_lines[UT_INIT_MAX_LINES][UT_INIT_LINE_SIZE];
static int ut_characters[4];
static unsigned ut_line_count;
static unsigned ut_read_count;
static unsigned ut_character_count;
static unsigned ut_character_index;
static unsigned ut_close_count;
static unsigned ut_malloc_count;
static int ut_malloc_failure;
static BOOL ut_open_fails;
static BOOL ut_file_is_accessible;
static INT ut_found;
static tODResult ut_sync_result;
static tODResult ut_queue_result;
static INT ut_queue_size;
static unsigned ut_sync_calls;
static unsigned ut_platform_calls;
static unsigned ut_session_time_calls;
static unsigned ut_part_two_calls;
static unsigned ut_error_calls;
static unsigned ut_exit_calls;
static unsigned ut_config_calls;
static unsigned ut_mps_calls;
static unsigned ut_no_file_calls;
static unsigned ut_exitinfo_calls;
static unsigned ut_sfdoors_calls;
static unsigned ut_od_exit_calls;
static INT ut_od_exit_error;
static BOOL ut_od_exit_term;
static BOOL ut_config_requests_exit;
static BOOL ut_config_fails;
static BOOL ut_part_two_succeeds;
static BOOL ut_sfdoors_result;
static const char *ut_error_text;
static const char *ut_task_value;
static const char *ut_sbbs_node_value;
static const char *ut_locale_value;
static BOOL ut_exitinfo_succeeds;
static unsigned ut_set_user_8bit_calls;
static BOOL ut_set_user_8bit_value;
static BOOL ut_no_file_forces_local;
static BOOL ut_no_file_supplies_info;
static jmp_buf ut_exit_target;
static BOOL ut_exit_expected;
#ifdef ODPLAT_WIN32
static tODWindowsSubsystem ut_subsystem;
#endif

BOOL utm_ODFramingIsEightBit(const char *framing)
{
   const char *scan;
   while(*framing == ' ' || *framing == '\t' || *framing == '\r' ||
      *framing == '\n')
   {
      ++framing;
   }
   if(framing[0] == '8')
      return(TRUE);
   for(scan = framing; scan[0] != '\0'; ++scan)
      if(scan[0] == ',' && scan[1] == '8' && scan[2] == ',')
         return(TRUE);
   return(FALSE);
}

#ifdef ODPLAT_WIN32
tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{
   return(ut_subsystem);
}
#endif

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0')
      ++length;
   return length;
}

static char *ut_copy(char *destination, const char *source, size_t maximum)
{
   size_t index = 0;
   if(maximum == 0)
      return destination;
   while(index + 1 < maximum && source[index] != '\0')
   {
      destination[index] = source[index];
      ++index;
   }
   destination[index] = '\0';
   return destination;
}

char *utm_strcpy(char *destination, const char *source)
{
   return ut_copy(destination, source, utm_strlen(source) + 1);
}

char *utm_strncpy(char *destination, const char *source, size_t count)
{
   size_t index = 0;
   while(index < count && source[index] != '\0')
   {
      destination[index] = source[index];
      ++index;
   }
   while(index < count)
      destination[index++] = '\0';
   return destination;
}

char *utm_strcat(char *destination, const char *source)
{
   size_t length = utm_strlen(destination);
   utm_strcpy(destination + length, source);
   return destination;
}

char *utm_safe_strcpy(char *destination, const char *source, size_t size)
{
   return ut_copy(destination, source, size);
}

char *utm_safe_strcat(char *destination, const char *source, size_t size)
{
   size_t length = utm_strlen(destination);
   if(length < size)
      ut_copy(destination + length, source, size - length);
   return destination;
}

int utm_strcmp(const char *left, const char *right)
{
   while(*left != '\0' && *left == *right)
   {
      ++left;
      ++right;
   }
   return (unsigned char)*left - (unsigned char)*right;
}

int utm_strncmp(const char *left, const char *right, size_t count)
{
   while(count != 0 && *left != '\0' && *left == *right)
   {
      ++left;
      ++right;
      --count;
   }
   if(count == 0)
      return 0;
   return (unsigned char)*left - (unsigned char)*right;
}

static int ut_lower(int value)
{
   if(value >= 'A' && value <= 'Z')
      return value - 'A' + 'a';
   return value;
}

static int ut_compare_case_n(const char *left, const char *right, size_t count)
{
   while(count != 0 && *left != '\0' && ut_lower(*left) == ut_lower(*right))
   {
      ++left;
      ++right;
      --count;
   }
   if(count == 0)
      return 0;
   return ut_lower((unsigned char)*left) - ut_lower((unsigned char)*right);
}

int utm_strncasecmp(const char *left, const char *right, size_t count)
{
   return ut_compare_case_n(left, right, count);
}

int utm_strnicmp(const char *left, const char *right, size_t count)
{
   return ut_compare_case_n(left, right, count);
}

char *utm_strchr(const char *text, int character)
{
   while(*text != '\0')
   {
      if(*text == character)
         return (char *)text;
      ++text;
   }
   return character == 0 ? (char *)text : NULL;
}

char *utm_strstr(const char *text, const char *needle)
{
   size_t length = utm_strlen(needle);
   do
   {
      if(utm_strncmp(text, needle, length) == 0)
         return (char *)text;
   } while(*text++ != '\0');
   return NULL;
}

char *utm_od_strupr(char *text)
{
   char *current = text;
   while(*current != '\0')
   {
      if(*current >= 'a' && *current <= 'z')
         *current = (char)(*current - 'a' + 'A');
      ++current;
   }
   return text;
}

char *utm_strupr(char *text)
{
   return utm_od_strupr(text);
}

long utm_atol(const char *text)
{
   long value = 0;
   int sign = 1;
   if(*text == '-')
   {
      sign = -1;
      ++text;
   }
   while(*text >= '0' && *text <= '9')
   {
      value = value * 10 + (*text - '0');
      ++text;
   }
   return value * sign;
}

int utm_atoi(const char *text)
{
   return (int)utm_atol(text);
}

void utm_ODStringToName(char *text)
{
   size_t length = utm_strlen(text);
   while(length != 0 && (text[length - 1] == '\r' ||
      text[length - 1] == '\n' || text[length - 1] == ' '))
      --length;
   text[length] = '\0';
}

void utm_ODStringCopy(char *destination, const char *source, INT size)
{
   ut_copy(destination, source, (size_t)size);
}

BOOL utm_ODStringHasTail(char *text, char *tail)
{
   size_t text_length = utm_strlen(text);
   size_t tail_length = utm_strlen(tail);
   if(text_length < tail_length)
      return FALSE;
   return ut_compare_case_n(text + text_length - tail_length, tail,
      tail_length) == 0;
}

int utm_sprintf(char *output, const char *format, ...)
{
   va_list arguments;
   int node;
   const char *prefix = "dorinfo";
   unsigned index = 0;
   va_start(arguments, format);
   while(*prefix != '\0')
      output[index++] = *prefix++;
   if(utm_strcmp(format, "dorinfo%c.def") == 0)
      output[index++] = (char)va_arg(arguments, int);
   else
   {
      node = va_arg(arguments, int);
      output[index++] = (char)('0' + node);
   }
   va_end(arguments);
   output[index++] = '.';
   output[index++] = 'd';
   output[index++] = 'e';
   output[index++] = 'f';
   output[index] = '\0';
   return (int)index;
}

tODResult utm_ODSyncSessionInitialize(void)
{
   ++ut_sync_calls;
   return ut_sync_result;
}

void utm_ODPlatInit(void)
{
   ++ut_platform_calls;
}

void utm_ODSessionTimeInitialize(void)
{
   ++ut_session_time_calls;
}

time_t utm_time(time_t *destination)
{
   *destination = (time_t)123456;
   return *destination;
}

void *utm_malloc(size_t size)
{
   unsigned index = ut_malloc_count++;
   UT_ASSERT_EQ_UINT(81, size);
   if((int)index == ut_malloc_failure)
      return NULL;
   UT_ASSERT(index < DIM(ut_allocations));
   return ut_allocations[index];
}

char *utm_getenv(const char *name)
{
   if(utm_strcmp(name, "TASK") == 0)
      return (char *)ut_task_value;
   UT_ASSERT(utm_strcmp(name, "SBBSNNUM") == 0);
   return (char *)ut_sbbs_node_value;
}

tODResult utm_ODInQueueAlloc(tODInQueueHandle *queue, INT size)
{
   ut_queue_size = size;
   if(ut_queue_result == kODRCSuccess)
      *queue = (tODInQueueHandle)&ut_queue_token;
   return ut_queue_result;
}

BOOL utm_ODFileAccessMode(const char *path, int mode)
{
   UT_ASSERT_EQ_PTR(od_control.info_path, path);
   UT_ASSERT_EQ_INT(4, mode);
   return ut_file_is_accessible;
}

INT utm_ODSearchForDropFile(char **names, INT count, char *found,
   char *exitinfo_path)
{
   (void)names;
   UT_ASSERT_EQ_UINT(DIM(apszDropFileNames), count);
   utm_strcpy(found, "selected.drop");
   utm_strcpy(exitinfo_path, "selected-dir");
   return ut_found;
}

tODResult utm_ODMakeFilename(char *output, const char *path,
   const char *name, INT size)
{
   (void)path;
   ut_copy(output, name, (size_t)size);
   return kODRCSuccess;
}

FILE *utm_fopen(const char *path, const char *mode)
{
   (void)path;
   UT_ASSERT(utm_strcmp(mode, "r") == 0);
   return ut_open_fails ? NULL : (FILE *)&ut_file_token;
}

char *utm_fgets(char *destination, int size, FILE *stream)
{
   int index = 0;
   const char *source;
   UT_ASSERT_EQ_PTR(&ut_file_token, stream);
   if(ut_read_count >= ut_line_count)
   {
      ++ut_read_count;
      return NULL;
   }
   source = ut_lines[ut_read_count++];
   while(index < size - 1 && source[index] != '\0')
   {
      destination[index] = source[index];
      ++index;
   }
   destination[index] = '\0';
   return destination;
}

int utm_fgetc(FILE *stream)
{
   UT_ASSERT_EQ_PTR(&ut_file_token, stream);
   if(ut_character_index < ut_character_count)
      return ut_characters[ut_character_index++];
   return '\n';
}

int utm_fclose(FILE *stream)
{
   UT_ASSERT_EQ_PTR(&ut_file_token, stream);
   ++ut_close_count;
   return 0;
}

void utm_ODInitReadExitInfo(void)
{
   ++ut_exitinfo_calls;
   if(ut_exitinfo_succeeds)
      od_control.od_info_type = EXITINFO;
}

BOOL utm_ODInitReadSFDoorsDAT(void)
{
   ++ut_sfdoors_calls;
   return ut_sfdoors_result;
}

void utm_ODInitPartTwo(void)
{
   ++ut_part_two_calls;
   if(!ut_part_two_succeeds)
      bODInitialized = FALSE;
}

void utm_ODInitError(char *text)
{
   ++ut_error_calls;
   ut_error_text = text;
}

void utm_exit(int status)
{
   ++ut_exit_calls;
   UT_ASSERT_EQ_INT(od_control.od_errorlevel[1], status);
   if(!ut_exit_expected)
      UT_ASSERT(FALSE);
   longjmp(ut_exit_target, 1);
}

void ODCALL utm_od_exit(INT error_level, BOOL term_call)
{
   ++ut_od_exit_calls;
   ut_od_exit_error = error_level;
   ut_od_exit_term = term_call;
}

BOOL ODCALL utm_od_set_user_8bit(BOOL value)
{
   ++ut_set_user_8bit_calls;
   ut_set_user_8bit_value = value;
   return TRUE;
}

#ifdef ODPLAT_NIX
char *utm_setlocale(int category, const char *locale)
{
   UT_ASSERT_EQ_INT(LC_ALL, category);
   UT_ASSERT(utm_strcmp("", locale) == 0);
   return (char *)ut_locale_value;
}
#endif

#ifdef ODPLAT_DOS32
static void ODCALL ut_config_callback(void)
#else
static void ut_config_callback(void)
#endif
{
   ++ut_config_calls;
   if(ut_config_fails)
      bODInitialized = FALSE;
   if(ut_config_requests_exit)
   {
      bODExitRequestedDuringInitialization = TRUE;
      nODPendingExitErrorLevel = 31;
      bODPendingExitTermCall = TRUE;
   }
}

#ifdef ODPLAT_DOS32
static void ODCALL ut_mps_callback(void)
#else
static void ut_mps_callback(void)
#endif
{
   ++ut_mps_calls;
}

static void accepts_public_mps_callback_type(void)
{
   OD_COMPONENT_CALLBACK *pCallback = ut_mps_callback;

   UT_ASSERT_EQ_PTR(ut_mps_callback, pCallback);
}

#ifdef ODPLAT_DOS32
static void ODCALL ut_no_file_callback(void)
#else
static void ut_no_file_callback(void)
#endif
{
   ++ut_no_file_calls;
   if(ut_no_file_forces_local)
      od_control.od_force_local = TRUE;
   if(ut_no_file_supplies_info)
      od_control.od_info_type = CUSTOM;
}

static void ut_set_line(unsigned line, const char *text)
{
   UT_ASSERT(line < UT_INIT_MAX_LINES);
   ut_copy(ut_lines[line], text, UT_INIT_LINE_SIZE);
   if(ut_line_count <= line)
      ut_line_count = line + 1;
}

static void reset_init_fixture(void)
{
   unsigned index;
   memset(&od_control, 0, sizeof(od_control));
   memset(ut_allocations, 0, sizeof(ut_allocations));
   memset(ut_lines, 0, sizeof(ut_lines));
   memset(apszDropFileInfo, 0, sizeof(apszDropFileInfo));
   od_control.od_errorlevel[1] = 77;
   od_control.od_force_local = TRUE;
   ut_line_count = 0;
   ut_read_count = 0;
   ut_character_count = 0;
   ut_character_index = 0;
   ut_close_count = 0;
   ut_malloc_count = 0;
   ut_malloc_failure = -1;
   ut_open_fails = FALSE;
   ut_file_is_accessible = TRUE;
   ut_found = FOUND_NONE;
   ut_sync_result = kODRCSuccess;
   ut_queue_result = kODRCSuccess;
   ut_queue_size = 0;
   ut_sync_calls = 0;
   ut_platform_calls = 0;
   ut_session_time_calls = 0;
   ut_part_two_calls = 0;
   ut_error_calls = 0;
   ut_exit_calls = 0;
   ut_config_calls = 0;
   ut_mps_calls = 0;
   ut_no_file_calls = 0;
   ut_exitinfo_calls = 0;
   ut_sfdoors_calls = 0;
   ut_od_exit_calls = 0;
   ut_config_requests_exit = FALSE;
   ut_config_fails = FALSE;
   ut_part_two_succeeds = TRUE;
   ut_sfdoors_result = TRUE;
   ut_error_text = NULL;
   ut_task_value = NULL;
   ut_sbbs_node_value = NULL;
   ut_locale_value = "C";
   ut_exitinfo_succeeds = TRUE;
   ut_set_user_8bit_calls = 0;
   ut_set_user_8bit_value = FALSE;
   ut_no_file_forces_local = FALSE;
   ut_no_file_supplies_info = FALSE;
   ut_exit_expected = FALSE;
#ifdef ODPLAT_WIN32
   ut_subsystem = kODWindowsSubsystemConsole;
#endif
   bODInitialized = FALSE;
   eODLifecycleState = kODLifecycleNeverStarted;
   bODExitRequestedDuringInitialization = FALSE;
   bIsCallbackActive = FALSE;
   bCalledFromConfig = FALSE;
   bTelnetSocket = FALSE;
   bParsedCmdLine = FALSE;
   bSystemNameSet = FALSE;
   bSysopNameSet = FALSE;
   bUserFull = FALSE;
   bPreOrExit = FALSE;
   bPromptForUserName = FALSE;
   btDoorSYSLock = 0;
   szForcedSystemName[0] = '\0';
   szForcedSysopName[0] = '\0';
   wODNodeNumber = 65535U;
   wPreSetInfo = 0;
   dwForcedBPS = 1;
   nForcedPort = -1;
   hODInputQueue = NULL;
   for(index = 0; index < DIM(apszColorNames); ++index)
      od_control.od_color_names[index][0] = '\0';
}

static void run_expecting_fatal_exit(void)
{
   ut_exit_expected = TRUE;
   if(setjmp(ut_exit_target) == 0)
   {
      utt_od_init();
      UT_ASSERT(FALSE);
   }
   ut_exit_expected = FALSE;
   UT_ASSERT_EQ_UINT(1, ut_exit_calls);
   UT_ASSERT_EQ_UINT(1, ut_error_calls);
   UT_ASSERT_NOT_NULL(ut_error_text);
}

#if UT_TURBO_SHARD == 0 || UT_TURBO_SHARD == 1
static void honors_entry_guards_and_sync_failure(void)
{
   reset_init_fixture();
   bIsCallbackActive = TRUE;
   utt_od_init();
   UT_ASSERT_EQ_UINT(0, ut_sync_calls);

   reset_init_fixture();
   bODInitialized = TRUE;
   utt_od_init();
   UT_ASSERT_EQ_UINT(0, ut_sync_calls);

   reset_init_fixture();
   ut_sync_result = kODRCGeneralFailure;
   utt_od_init();
   UT_ASSERT_EQ_UINT(1, ut_sync_calls);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_error_calls);
   UT_ASSERT_EQ_UINT(0, ut_platform_calls);
}

static void completes_initialization_before_processing_an_exit_request(void)
{
   reset_init_fixture();
   bODExitRequestedDuringInitialization = TRUE;
   nODPendingExitErrorLevel = 23;
   bODPendingExitTermCall = TRUE;
   utt_od_init();
   UT_ASSERT_EQ_INT(kODLifecycleActive, eODLifecycleState);
   UT_ASSERT_EQ_UINT(1, ut_od_exit_calls);
   UT_ASSERT_EQ_INT(23, ut_od_exit_error);
   UT_ASSERT_EQ_INT(TRUE, ut_od_exit_term);
   UT_ASSERT(!bODExitRequestedDuringInitialization);

   reset_init_fixture();
   eODLifecycleState = kODLifecycleTerminal;
   utt_od_init();
   UT_ASSERT_EQ_INT(FALSE, bODInitialized);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_sync_calls);
}

static void does_not_publish_a_failed_initialization(void)
{
   reset_init_fixture();
   ut_part_two_succeeds = FALSE;
   utt_od_init();
   UT_ASSERT_EQ_INT(FALSE, bODInitialized);
   UT_ASSERT_EQ_INT(kODLifecycleInitializing, eODLifecycleState);
   UT_ASSERT_EQ_UINT(1, ut_part_two_calls);

   reset_init_fixture();
   od_control.config_file = ut_config_callback;
   ut_config_fails = TRUE;
   utt_od_init();
   UT_ASSERT_EQ_INT(FALSE, bODInitialized);
   UT_ASSERT_EQ_INT(kODLifecycleInitializing, eODLifecycleState);
   UT_ASSERT_EQ_UINT(1, ut_config_calls);
}

static void initializes_tables_allocations_and_local_defaults(void)
{
   reset_init_fixture();
   utt_od_init();
   UT_ASSERT_EQ_INT(TRUE, bODInitialized);
   UT_ASSERT(utm_strcmp(OD_VER_SHORTNAME, od_control.od_prog_name) == 0);
   UT_ASSERT(utm_strcmp("BLACK", od_control.od_color_names[0]) == 0);
   UT_ASSERT(utm_strcmp("FLASHING", od_control.od_color_names[11]) == 0);
   UT_ASSERT(od_control.od_cfg_lines[0][0] != '\0');
   UT_ASSERT(od_control.od_cfg_text[0][0] != '\0');
   UT_ASSERT_EQ_UINT(25, ut_malloc_count);
   UT_ASSERT_EQ_INT(1, od_control.od_node);
   UT_ASSERT_EQ_INT(256, DEFAULT_EVENT_QUEUE_SIZE);
   UT_ASSERT_EQ_INT(DEFAULT_EVENT_QUEUE_SIZE, ut_queue_size);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_user_keyboard_on);
   UT_ASSERT_EQ_INT(NO_DOOR_FILE, od_control.od_info_type);
   UT_ASSERT_EQ_INT(80, od_control.user_screenwidth);
   UT_ASSERT_EQ_INT(23, od_control.user_screen_length);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_ansi);
   UT_ASSERT_EQ_INT(60, od_control.user_timelimit);
#ifdef ODPLAT_NIX
   UT_ASSERT_EQ_INT(1, od_control.baud);
#else
   UT_ASSERT_EQ_INT(0, od_control.baud);
#endif
   UT_ASSERT(utm_strcmp("Unknown Location", od_control.user_location) == 0);
   UT_ASSERT_EQ_UINT(1, ut_platform_calls);
   UT_ASSERT_EQ_UINT(1, ut_session_time_calls);
   UT_ASSERT_EQ_UINT(1, ut_part_two_calls);
}

static void preserves_configured_screen_dimensions(void)
{
   reset_init_fixture();
   od_control.user_screenwidth = 132;
   od_control.user_screen_length = 50;
   utt_od_init();
   UT_ASSERT_EQ_INT(132, od_control.user_screenwidth);
   UT_ASSERT_EQ_INT(50, od_control.user_screen_length);

   reset_init_fixture();
   od_control.user_screenwidth = 132;
   utt_od_init();
   UT_ASSERT_EQ_INT(132, od_control.user_screenwidth);
   UT_ASSERT_EQ_INT(23, od_control.user_screen_length);

   reset_init_fixture();
   od_control.user_screen_length = 50;
   utt_od_init();
   UT_ASSERT_EQ_INT(80, od_control.user_screenwidth);
   UT_ASSERT_EQ_INT(50, od_control.user_screen_length);
}

static void preserves_tables_and_delegates_to_configuration(void)
{
   reset_init_fixture();
   utm_strcpy(od_control.od_prog_name, "Configured Door");
   utm_strcpy(od_control.od_color_names[0], "INK");
   utm_strcpy(od_control.od_cfg_lines[0], "CustomLine");
   utm_strcpy(od_control.od_cfg_text[0], "CustomText");
   od_control.baud = 38400;
   od_control.port = 2;
   od_control.config_file = ut_config_callback;
#ifdef OD_PERSONALITY_SUPPORT
   od_control.od_mps = ut_mps_callback;
#endif
   utt_od_init();
   UT_ASSERT(utm_strcmp("Configured Door", od_control.od_prog_name) == 0);
   UT_ASSERT(utm_strcmp("INK", od_control.od_color_names[0]) == 0);
   UT_ASSERT(utm_strcmp("CustomLine", od_control.od_cfg_lines[0]) == 0);
   UT_ASSERT(utm_strcmp("CustomText", od_control.od_cfg_text[0]) == 0);
   UT_ASSERT_EQ_INT(38400, dwForcedBPS);
   UT_ASSERT_EQ_INT(2, nForcedPort);
   UT_ASSERT_EQ_UINT(1, ut_config_calls);
#ifdef OD_PERSONALITY_SUPPORT
   UT_ASSERT_EQ_UINT(1, ut_mps_calls);
#endif
   UT_ASSERT_EQ_UINT(0, ut_platform_calls);

#ifdef ODPLAT_WIN32
   reset_init_fixture();
   ut_subsystem = kODWindowsSubsystemGUI;
   od_control.config_file = ut_config_callback;
   od_control.od_mps = ut_mps_callback;
   utt_od_init();
   UT_ASSERT_EQ_UINT(0, ut_mps_calls);
#endif

   reset_init_fixture();
   od_control.config_file = ut_config_callback;
   ut_config_requests_exit = TRUE;
   utt_od_init();
   UT_ASSERT_EQ_UINT(1, ut_config_calls);
   UT_ASSERT_EQ_UINT(1, ut_od_exit_calls);
   UT_ASSERT_EQ_INT(31, ut_od_exit_error);
   UT_ASSERT_EQ_INT(TRUE, ut_od_exit_term);
   UT_ASSERT(!bODExitRequestedDuringInitialization);
}

static void handles_allocation_and_queue_failures(void)
{
   reset_init_fixture();
   ut_malloc_failure = 7;
   run_expecting_fatal_exit();
   UT_ASSERT_EQ_UINT(8, ut_malloc_count);

   reset_init_fixture();
   ut_queue_result = kODRCNoMemory;
   run_expecting_fatal_exit();
   UT_ASSERT_EQ_UINT(25, ut_malloc_count);

   reset_init_fixture();
   od_control.od_in_buf_size = 321;
   utt_od_init();
   UT_ASSERT_EQ_INT(321, ut_queue_size);
}

static void applies_node_precedence(void)
{
   reset_init_fixture();
   ut_task_value = "3";
   ut_sbbs_node_value = "4";
   utt_od_init();
   UT_ASSERT_EQ_INT(3, od_control.od_node);

   reset_init_fixture();
   ut_sbbs_node_value = "4";
   utt_od_init();
   UT_ASSERT_EQ_INT(4, od_control.od_node);

   reset_init_fixture();
   wODNodeNumber = 5;
   utt_od_init();
   UT_ASSERT_EQ_INT(5, od_control.od_node);

   reset_init_fixture();
   od_control.od_node = 6;
   utt_od_init();
   UT_ASSERT_EQ_INT(6, od_control.od_node);
}

static void handles_custom_and_forced_local_inputs(void)
{
   reset_init_fixture();
   od_control.od_force_local = FALSE;
   od_control.od_info_type = CUSTOM;
   od_control.user_timelimit = 0;
   od_control.port = -1;
   utt_od_init();
   UT_ASSERT_EQ_INT(60, od_control.user_timelimit);
#ifdef ODPLAT_NIX
   UT_ASSERT_EQ_INT(1, od_control.baud);
#else
   UT_ASSERT_EQ_INT(0, od_control.baud);
#endif

   reset_init_fixture();
   bParsedCmdLine = TRUE;
   bSystemNameSet = TRUE;
   utm_strcpy(szForcedSystemName, "Forced BBS");
   od_control.user_ansi = FALSE;
   od_control.user_timelimit = 12;
   utt_od_init();
   UT_ASSERT_EQ_INT(FALSE, od_control.user_ansi);
   UT_ASSERT_EQ_INT(12, od_control.user_timelimit);
   UT_ASSERT(utm_strcmp("Forced BBS", od_control.user_location) == 0);

   reset_init_fixture();
   utm_strcpy(od_control.system_name, "Named BBS");
   utt_od_init();
   UT_ASSERT(utm_strcmp("Named BBS", od_control.user_location) == 0);

#ifdef ODPLAT_NIX
   reset_init_fixture();
   ut_locale_value = "en_US.UTF-8";
   utt_od_init();
   UT_ASSERT_EQ_INT(TRUE, od_control.od_cp437_to_utf8_out);
#endif

   reset_init_fixture();
   od_control.od_force_local = FALSE;
   od_control.od_info_type = CUSTOM;
   od_control.user_timelimit = 10;
   od_control.port = 0;
   utt_od_init();
   UT_ASSERT_EQ_INT(10, od_control.user_timelimit);
}

static void resumes_after_configuration_callback(void)
{
   reset_init_fixture();
   bCalledFromConfig = TRUE;
   bODInitialized = TRUE;
   utt_od_init();
   UT_ASSERT_EQ_UINT(0, ut_sync_calls);
   UT_ASSERT_EQ_UINT(1, ut_platform_calls);
   UT_ASSERT_EQ_UINT(1, ut_part_two_calls);
}

static void handles_disabled_drop_files(void)
{
   reset_init_fixture();
   od_control.od_force_local = FALSE;
   od_control.od_disable = DIS_INFOFILE;
   utt_od_init();
   UT_ASSERT_EQ_INT(NO_DOOR_FILE, od_control.od_info_type);
   UT_ASSERT_EQ_UINT(1, ut_part_two_calls);
}
#endif

#if UT_TURBO_SHARD == 0 || UT_TURBO_SHARD == 2 || \
   UT_TURBO_SHARD == 3 || UT_TURBO_SHARD == 4
static void prepare_selected_format(INT found)
{
   reset_init_fixture();
   od_control.od_force_local = FALSE;
   ut_file_is_accessible = TRUE;
   ut_found = found;
   wPreSetInfo = PRESET_REQUIRED;
}

static void prepare_dorinfo(void)
{
   prepare_selected_format(FOUND_DORINFO1_DEF);
   ut_set_line(0, "Test BBS\r\n");
   ut_set_line(1, "Jane\n");
   ut_set_line(2, "Sysop\n");
   ut_set_line(3, "COM1\n");
   ut_set_line(4, "38400\n");
   ut_set_line(5, "8N1\n");
   ut_set_line(6, "Door\n");
   ut_set_line(7, "User\n");
   ut_set_line(8, "Somewhere\n");
   ut_set_line(9, "1\n");
   ut_set_line(10, "90\n");
   ut_set_line(11, "45\n");
}

static void prepare_chain(void)
{
   unsigned index;
   prepare_selected_format(FOUND_CHAIN_TXT);
   for(index = 0; index < 30; ++index)
      ut_set_line(index, "1\n");
   ut_set_line(1, "Handle\n");
   ut_set_line(2, "Chain User\n");
   ut_set_line(3, "CALL\n");
   ut_set_line(5, "M\n");
   ut_set_line(7, "01-01-99\n");
   ut_set_line(8, "80\n");
   ut_set_line(9, "24\n");
   ut_set_line(10, "70\n");
   ut_set_line(15, "3600\n");
   ut_set_line(19, "38400\n");
   ut_set_line(20, "2\n");
   ut_set_line(22, "secret\n");
   ut_set_line(29, "N,8,1\n");
}

static void prepare_doorway(void)
{
   prepare_selected_format(FOUND_DOOR_SYS);
   ut_set_line(0, "DoorWay User\n");
   ut_set_line(1, "2\n");
   ut_set_line(2, "19200\n");
   ut_set_line(3, "30\n");
   ut_set_line(4, "G\n");
}

static void prepare_callinfo(void)
{
   unsigned index;
   prepare_selected_format(FOUND_CALLINFO_BBS);
   for(index = 0; index < 31; ++index)
      ut_set_line(index, "unused\n");
   ut_set_line(0, "CallInfo User\n");
   ut_set_line(2, "Location\n");
   ut_set_line(3, "75\n");
   ut_set_line(4, "40\n");
   ut_set_line(5, "G\n");
   ut_set_line(6, "password\n");
   ut_set_line(16, "555-0100\n");
   ut_set_line(22, "24\n");
   ut_set_line(28, "COM2\n");
   ut_set_line(30, "38400\n");
}

static void prepare_tribbs(void)
{
   unsigned index;
   prepare_selected_format(FOUND_TRIBBS_SYS);
   for(index = 0; index < 20; ++index)
      ut_set_line(index, "1\n");
   ut_set_line(1, "Tri User\n");
   ut_set_line(2, "password\n");
   ut_set_line(4, "Y\n");
   ut_set_line(5, "Y\n");
   ut_set_line(6, "50\n");
   ut_set_line(7, "555-0100\n");
   ut_set_line(8, "Location\n");
   ut_set_line(9, "01/02/90\n");
   ut_set_line(10, "3\n");
   ut_set_line(11, "2\n");
   ut_set_line(12, "2400\n");
   ut_set_line(13, "38400\n");
   ut_set_line(14, "Y\n");
   ut_set_line(15, "Y\n");
   ut_set_line(16, "TriBBS\n");
   ut_set_line(17, "Sysop\n");
   ut_set_line(18, "Alias\n");
   ut_set_line(19, "Y\n");
}

static void prepare_door32(void)
{
   prepare_selected_format(FOUND_DOOR32_SYS);
   ut_set_line(0, "1\n");
   ut_set_line(1, "123\n");
   ut_set_line(2, "38400\n");
   ut_set_line(3, "BBS 1.0\n");
   ut_set_line(4, "42\n");
   ut_set_line(5, "Real Name\n");
   ut_set_line(6, "Alias\n");
   ut_set_line(7, "80\n");
   ut_set_line(8, "60\n");
   ut_set_line(9, "1\n");
   ut_set_line(10, "4\n");
}

typedef void (*UTInitPrepare)(void);

static void exercise_required_line_failures(UTInitPrepare prepare,
   unsigned required)
{
   unsigned available;
   for(available = 0; available < required; ++available)
   {
      (*prepare)();
      ut_line_count = available;
      utt_od_init();
      UT_ASSERT_EQ_UINT(1, ut_part_two_calls);
      UT_ASSERT_EQ_INT(NO_DOOR_FILE, od_control.od_info_type);
   }
}

static void exercise_open_failure(UTInitPrepare prepare)
{
   (*prepare)();
   ut_open_fails = TRUE;
   utt_od_init();
   UT_ASSERT_EQ_INT(NO_DOOR_FILE, od_control.od_info_type);
   UT_ASSERT_EQ_UINT(1, ut_part_two_calls);
}
#endif

#if UT_TURBO_SHARD == 0 || UT_TURBO_SHARD == 2
static void reads_dorinfo_and_each_required_line_failure(void)
{
   exercise_open_failure(prepare_dorinfo);
   exercise_required_line_failures(prepare_dorinfo, 12);

   prepare_dorinfo();
   utt_od_init();
   UT_ASSERT_EQ_INT(DORINFO1, od_control.od_info_type);
   UT_ASSERT(utm_strcmp("Test BBS", od_control.system_name) == 0);
   UT_ASSERT(utm_strcmp("Jane Sysop", od_control.sysop_name) == 0);
   UT_ASSERT(utm_strcmp("Door User", od_control.user_name) == 0);
   UT_ASSERT_EQ_INT(0, od_control.port);
   UT_ASSERT_EQ_INT(38400, od_control.baud);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_ansi);
   UT_ASSERT_EQ_INT(90, od_control.user_security);
   UT_ASSERT_EQ_INT(45, od_control.user_timelimit);
   UT_ASSERT_EQ_UINT(1, ut_set_user_8bit_calls);
   UT_ASSERT_EQ_INT(TRUE, ut_set_user_8bit_value);

   prepare_dorinfo();
   ut_set_line(5, "7E1\n");
   utt_od_init();
   UT_ASSERT_EQ_UINT(1, ut_set_user_8bit_calls);
   UT_ASSERT_EQ_INT(FALSE, ut_set_user_8bit_value);

   prepare_dorinfo();
   ut_set_line(0, "Test BBS");
   utt_od_init();
   UT_ASSERT(utm_strcmp("Test BBS", od_control.system_name) == 0);

   prepare_dorinfo();
   ut_set_line(0, "Test BBS\r");
   utt_od_init();
   UT_ASSERT(utm_strcmp("Test BBS", od_control.system_name) == 0);

   prepare_dorinfo();
   ut_set_line(2, "\n");
   ut_set_line(7, "\n");
   ut_set_line(9, "0\n");
   ut_set_line(3, "COM0\n");
   utt_od_init();
   UT_ASSERT(utm_strcmp("Jane", od_control.sysop_name) == 0);
   UT_ASSERT(utm_strcmp("Door", od_control.user_name) == 0);
   UT_ASSERT_EQ_INT(FALSE, od_control.user_ansi);
   UT_ASSERT_EQ_INT(-1, od_control.port);
#ifdef ODPLAT_NIX
   UT_ASSERT_EQ_INT(1, od_control.baud);
#else
   UT_ASSERT_EQ_INT(0, od_control.baud);
#endif
}

static void reads_chain_and_each_required_line_failure(void)
{
   exercise_open_failure(prepare_chain);
   exercise_required_line_failures(prepare_chain, 30);

   prepare_chain();
   utt_od_init();
   UT_ASSERT_EQ_INT(CHAINTXT, od_control.od_info_type);
   UT_ASSERT(utm_strcmp("Handle", od_control.user_handle) == 0);
   UT_ASSERT_EQ_INT(60, od_control.user_timelimit);
   UT_ASSERT_EQ_INT(38400, od_control.baud);
   UT_ASSERT_EQ_INT(1, od_control.port);
   UT_ASSERT_EQ_UINT(1, ut_set_user_8bit_calls);
   UT_ASSERT_EQ_INT(TRUE, ut_set_user_8bit_value);

   prepare_chain();
   ut_set_line(29, "N,7,1\n");
   utt_od_init();
   UT_ASSERT_EQ_UINT(1, ut_set_user_8bit_calls);
   UT_ASSERT_EQ_INT(FALSE, ut_set_user_8bit_value);

   prepare_chain();
   ut_set_line(19, "KB");
   utt_od_init();
#ifdef ODPLAT_NIX
   UT_ASSERT_EQ_INT(1, od_control.baud);
#else
   UT_ASSERT_EQ_INT(0, od_control.baud);
#endif
}

static void reads_doorway_and_each_required_line_failure(void)
{
   exercise_open_failure(prepare_doorway);
   exercise_required_line_failures(prepare_doorway, 5);

   prepare_doorway();
   utt_od_init();
   UT_ASSERT_EQ_INT(DOORSYS_DRWY, od_control.od_info_type);
   UT_ASSERT(utm_strcmp("DoorWay User", od_control.user_name) == 0);
   UT_ASSERT_EQ_INT(1, od_control.port);
   UT_ASSERT_EQ_INT(19200, od_control.baud);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_ansi);

   prepare_doorway();
   ut_set_line(1, "0\n");
   ut_set_line(4, "T\n");
   utt_od_init();
   UT_ASSERT_EQ_INT(-1, od_control.port);
   UT_ASSERT_EQ_INT(FALSE, od_control.user_ansi);
#ifdef ODPLAT_NIX
   UT_ASSERT_EQ_INT(1, od_control.baud);
#else
   UT_ASSERT_EQ_INT(0, od_control.baud);
#endif
}

static void reads_callinfo_and_each_required_line_failure(void)
{
   exercise_open_failure(prepare_callinfo);
   exercise_required_line_failures(prepare_callinfo, 31);

   prepare_callinfo();
   utt_od_init();
   UT_ASSERT_EQ_INT(CALLINFO, od_control.od_info_type);
   UT_ASSERT(utm_strcmp("CallInfo User", od_control.user_name) == 0);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_ansi);
   UT_ASSERT_EQ_INT(1, od_control.port);
   UT_ASSERT_EQ_INT(38400, od_control.baud);

   prepare_callinfo();
   ut_set_line(5, "M\n");
   utt_od_init();
   UT_ASSERT_EQ_INT(FALSE, od_control.user_ansi);
}

static void reads_tribbs_and_each_required_line_failure(void)
{
   exercise_open_failure(prepare_tribbs);
   exercise_required_line_failures(prepare_tribbs, 20);

   prepare_tribbs();
   utt_od_init();
   UT_ASSERT_EQ_INT(TRIBBSSYS, od_control.od_info_type);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_expert);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_ansi);
   UT_ASSERT_EQ_INT(COM_RTSCTS_FLOW, od_control.od_com_flow_control);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_error_free);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_rip);
   UT_ASSERT_EQ_INT(38400, od_control.baud);

   prepare_tribbs();
   ut_set_line(4, "N\n");
   ut_set_line(5, "N\n");
   ut_set_line(11, "0\n");
   ut_set_line(13, "0\n");
   ut_set_line(14, "N\n");
   ut_set_line(15, "N\n");
   ut_set_line(19, "N\n");
   utt_od_init();
   UT_ASSERT_EQ_INT(FALSE, od_control.user_expert);
   UT_ASSERT_EQ_INT(FALSE, od_control.user_ansi);
   UT_ASSERT_EQ_INT(-1, od_control.port);
   UT_ASSERT_EQ_INT(COM_NO_FLOW, od_control.od_com_flow_control);
   UT_ASSERT_EQ_INT(FALSE, od_control.user_error_free);
   UT_ASSERT_EQ_INT(FALSE, od_control.user_rip);
}

static void reads_door32_and_switch_variants(void)
{
   static const INT communication[] = {0, 1, 2, 9};
   static const INT terminals[] = {0, 1, 2, 3, 9};
   unsigned index;
   exercise_open_failure(prepare_door32);
   exercise_required_line_failures(prepare_door32, 11);

   for(index = 0; index < DIM(communication); ++index)
   {
      prepare_door32();
      ut_set_line(0, communication[index] == 0 ? "0\n" :
         communication[index] == 1 ? "1\n" :
         communication[index] == 2 ? "2\n" : "9\n");
      utt_od_init();
      UT_ASSERT_EQ_INT(DOOR32SYS, od_control.od_info_type);
      if(communication[index] == 0)
         UT_ASSERT_EQ_INT(TRUE, od_control.od_force_local);
      if(communication[index] == 2)
         UT_ASSERT_EQ_INT(TRUE, od_control.od_use_socket);
      UT_ASSERT_EQ_INT(communication[index] == 2, bTelnetSocket);
   }

   for(index = 0; index < DIM(terminals); ++index)
   {
      prepare_door32();
      ut_set_line(9, terminals[index] == 0 ? "0\n" :
         terminals[index] == 1 ? "1\n" :
         terminals[index] == 2 ? "2\n" :
         terminals[index] == 3 ? "3\n" : "9\n");
      utt_od_init();
      if(terminals[index] == 0)
         UT_ASSERT_EQ_INT(FALSE, od_control.user_ansi);
      if(terminals[index] == 2)
         UT_ASSERT_EQ_INT(TRUE, od_control.user_avatar);
      if(terminals[index] == 3)
         UT_ASSERT_EQ_INT(TRUE, od_control.user_rip);
   }

   prepare_door32();
   od_control.port = -1;
   utt_od_init();
#ifdef ODPLAT_NIX
   UT_ASSERT_EQ_INT(1, od_control.baud);
#else
   UT_ASSERT_EQ_INT(0, od_control.baud);
#endif
}
#endif

#if UT_TURBO_SHARD == 0 || UT_TURBO_SHARD == 3
static void selects_node_names_and_explicit_paths(void)
{
   prepare_dorinfo();
   od_control.od_node = 36;
   utt_od_init();
   UT_ASSERT_EQ_INT(DORINFO1, od_control.od_info_type);

   prepare_dorinfo();
   od_control.od_node = 10;
   utt_od_init();
   UT_ASSERT_EQ_INT(DORINFO1, od_control.od_info_type);

   prepare_dorinfo();
   od_control.od_node = 1;
   utt_od_init();
   UT_ASSERT_EQ_INT(DORINFO1, od_control.od_info_type);

   prepare_dorinfo();
   ut_file_is_accessible = FALSE;
   utm_strcpy(od_control.info_path, "path/dorinfo1.def");
   utt_od_init();
   UT_ASSERT_EQ_INT(DORINFO1, od_control.od_info_type);

   prepare_dorinfo();
   ut_file_is_accessible = FALSE;
   utm_strcpy(od_control.info_path, "path/notdorinfo.def");
   utt_od_init();
   UT_ASSERT_EQ_INT(DORINFO1, od_control.od_info_type);

   prepare_dorinfo();
   ut_file_is_accessible = FALSE;
   utm_strcpy(od_control.info_path, "tiny.def");
   utt_od_init();
   UT_ASSERT_EQ_INT(DORINFO1, od_control.od_info_type);

   prepare_door32();
   ut_file_is_accessible = FALSE;
   utm_strcpy(od_control.info_path, "path/door32.sys");
   ut_found = FOUND_NONE;
   utt_od_init();
   UT_ASSERT_EQ_INT(DOOR32SYS, od_control.od_info_type);

   prepare_dorinfo();
   ut_file_is_accessible = FALSE;
   utm_strcpy(od_control.info_path, "path/unknown.dat");
   utt_od_init();
   UT_ASSERT_EQ_INT(DORINFO1, od_control.od_info_type);
}

static void reads_exitinfo_and_sf_variants(void)
{
   static const INT sf_formats[] = {
      FOUND_SFDOORS_DAT, FOUND_SFMAIN_DAT, FOUND_SFFILE_DAT,
      FOUND_SFMESS_DAT, FOUND_SFSYSOP_DAT
   };
   unsigned index;

   prepare_dorinfo();
   ut_found = FOUND_EXITINFO_BBS;
   ut_exitinfo_succeeds = FALSE;
   utt_od_init();
   UT_ASSERT_EQ_UINT(1, ut_exitinfo_calls);
   UT_ASSERT_EQ_INT(NO_DOOR_FILE, od_control.od_info_type);

   prepare_dorinfo();
   ut_found = FOUND_EXITINFO_BBS;
   utt_od_init();
   UT_ASSERT_EQ_UINT(1, ut_exitinfo_calls);
   UT_ASSERT_EQ_INT(EXITINFO, od_control.od_info_type);

   prepare_dorinfo();
   ut_found = FOUND_EXITINFO_BBS;
   ut_open_fails = TRUE;
   utt_od_init();
   UT_ASSERT_EQ_INT(NO_DOOR_FILE, od_control.od_info_type);

   for(index = 0; index < DIM(sf_formats); ++index)
   {
      prepare_selected_format(sf_formats[index]);
      ut_sfdoors_result = TRUE;
      utt_od_init();
      UT_ASSERT_EQ_UINT(1, ut_sfdoors_calls);
      UT_ASSERT_EQ_INT(SFDOORSDAT, od_control.od_info_type);
   }

   prepare_selected_format(FOUND_SFDOORS_DAT);
   ut_sfdoors_result = FALSE;
   utt_od_init();
   UT_ASSERT_EQ_UINT(1, ut_sfdoors_calls);
   UT_ASSERT_EQ_INT(NO_DOOR_FILE, od_control.od_info_type);
}
#endif

#if UT_TURBO_SHARD == 0 || UT_TURBO_SHARD == 4
static void prepare_gap(void)
{
   unsigned index;
   prepare_selected_format(FOUND_DOOR_SYS);
   for(index = 0; index < 55; ++index)
      ut_set_line(index, "1\n");
   ut_set_line(0, "COM1:\n");
   ut_set_line(1, "38400\n");
   ut_set_line(2, "8N1\n");
   ut_set_line(3, "2\n");
   ut_set_line(4, "N\n");
   ut_set_line(5, "display\n");
   ut_set_line(6, "printer\n");
   ut_set_line(7, "page\n");
   ut_set_line(8, "seconds\n");
   ut_set_line(9, "Gap User\n");
   ut_set_line(10, "Gap Location\r\n");
   ut_set_line(11, "555-0100\n");
   ut_set_line(12, "555-0101\n");
   ut_set_line(13, "secret\r\n");
   ut_set_line(14, "80\n");
   ut_set_line(15, "10\n");
   ut_set_line(16, "01/02/90\n");
   ut_set_line(17, "unused\n");
   ut_set_line(18, "45\n");
   ut_set_line(19, "RIP");
   ut_set_line(20, "24\n");
   ut_set_line(21, "expert\n");
   ut_set_line(22, "conferences\n");
   ut_set_line(23, "daily\n");
   ut_set_line(24, "01/01/30\n");
   ut_set_line(25, "42\n");
   ut_set_line(26, "protocol\n");
   ut_set_line(27, "3\n");
   ut_set_line(28, "4\n");
   ut_set_line(29, "5\n");
   ut_set_line(30, "download-limit\n");
   ut_set_line(31, "01/02/90\n");
   ut_set_line(32, "file-area\n");
   ut_set_line(33, "conference\n");
   ut_set_line(34, "Gap Sysop\n");
   ut_set_line(35, "Gap Handle\n");
   ut_set_line(36, "12:34\n");
   ut_set_line(37, "Y\n");
   ut_set_line(38, "mail\n");
   ut_set_line(39, "color\n");
   ut_set_line(40, "credits\n");
   ut_set_line(41, "minutes\n");
   ut_set_line(42, "login\n");
   ut_set_line(43, "12:00\n");
   ut_set_line(44, "11:00\n");
   ut_set_line(45, "max-files\n");
   ut_set_line(46, "downloads-k\n");
   ut_set_line(47, "100\n");
   ut_set_line(48, "200\n");
   ut_set_line(49, "A comment\r\n");
   ut_set_line(50, "doors-opened\n");
   ut_set_line(51, "12\n");
}

static void reads_gap_required_and_optional_lines(void)
{
   unsigned available;

   exercise_open_failure(prepare_gap);
   exercise_required_line_failures(prepare_gap, 31);

   for(available = 31; available < 52; ++available)
   {
      prepare_gap();
      ut_line_count = available;
      utt_od_init();
      UT_ASSERT_EQ_INT(DOORSYS_GAP, od_control.od_info_type);
   }

   prepare_gap();
   utt_od_init();
   UT_ASSERT_EQ_INT(DOORSYS_WILDCAT, od_control.od_info_type);
   UT_ASSERT(utm_strcmp("Gap User", od_control.user_name) == 0);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_rip);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_ansi);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_error_free);
   UT_ASSERT_EQ_INT(12, od_control.user_messages);
   UT_ASSERT_EQ_UINT(1, ut_set_user_8bit_calls);
   UT_ASSERT_EQ_INT(TRUE, ut_set_user_8bit_value);

   prepare_gap();
   ut_set_line(2, "7E1\n");
   utt_od_init();
   UT_ASSERT_EQ_UINT(1, ut_set_user_8bit_calls);
   UT_ASSERT_EQ_INT(FALSE, ut_set_user_8bit_value);

   prepare_gap();
   ut_set_line(4, "Y\n");
   ut_set_line(19, "G\n");
   ut_set_line(37, "y\n");
   utt_od_init();
   UT_ASSERT_EQ_INT(19200, od_control.baud);
   UT_ASSERT_EQ_INT(FALSE, od_control.user_rip);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_ansi);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_error_free);

   prepare_gap();
   ut_set_line(4, "57600\n");
   ut_set_line(19, "N\n");
   ut_set_line(37, "N\n");
   utt_od_init();
   UT_ASSERT_EQ_INT(57600, od_control.baud);
   UT_ASSERT_EQ_INT(FALSE, od_control.user_ansi);
   UT_ASSERT_EQ_INT(FALSE, od_control.user_error_free);

   prepare_gap();
   ut_set_line(10, "Gap Location");
   ut_set_line(13, "secret");
   ut_set_line(49, "A comment");
   utt_od_init();
   UT_ASSERT(utm_strcmp("Gap Location", od_control.user_location) == 0);
   UT_ASSERT(utm_strcmp("secret", od_control.user_password) == 0);
   UT_ASSERT(utm_strcmp("A comment", od_control.user_comment) == 0);

   prepare_gap();
   ut_set_line(10, "Gap Location\r");
   ut_set_line(13, "secret\r");
   ut_set_line(49, "A comment\r");
   utt_od_init();
   UT_ASSERT(utm_strcmp("Gap Location", od_control.user_location) == 0);
   UT_ASSERT(utm_strcmp("secret", od_control.user_password) == 0);
   UT_ASSERT(utm_strcmp("A comment", od_control.user_comment) == 0);
}

static void reads_gap_communication_modes(void)
{
   prepare_gap();
   ut_set_line(0, "COM0:STDIO\n");
   utt_od_init();
   UT_ASSERT_EQ_INT(COM_STDIO, od_control.od_com_method);

   prepare_gap();
   ut_set_line(0, "COM0:SOCKET123\n");
   utt_od_init();
   UT_ASSERT_EQ_INT(COM_SOCKET, od_control.od_com_method);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_use_socket);
   UT_ASSERT_EQ_INT(123, od_control.od_open_handle);

   prepare_gap();
   ut_set_line(0, "COM0:HANDLE456\n");
   utt_od_init();
   UT_ASSERT_EQ_INT(COM_WIN32, od_control.od_com_method);
   UT_ASSERT_EQ_INT(456, od_control.od_open_handle);
}

static void distinguishes_gap_from_doorway(void)
{
   prepare_gap();
   ut_set_line(0, "XOM1:\n");
   utt_od_init();
   UT_ASSERT_EQ_INT(DOORSYS_DRWY, od_control.od_info_type);

   prepare_gap();
   ut_set_line(0, "CXM1:\n");
   utt_od_init();
   UT_ASSERT_EQ_INT(DOORSYS_DRWY, od_control.od_info_type);

   prepare_gap();
   ut_set_line(0, "COX1:\n");
   utt_od_init();
   UT_ASSERT_EQ_INT(DOORSYS_DRWY, od_control.od_info_type);

   prepare_gap();
   ut_set_line(0, "COM1X\n");
   utt_od_init();
   UT_ASSERT_EQ_INT(DOORSYS_DRWY, od_control.od_info_type);
}

static void handles_gap_long_and_repeated_lines(void)
{
   prepare_gap();
   ut_set_line(22, "no-newline");
   ut_characters[0] = 'x';
   ut_characters[1] = '\n';
   ut_character_count = 2;
   utt_od_init();
   UT_ASSERT_EQ_UINT(2, ut_character_index);

   prepare_gap();
   ut_set_line(22, "no-newline");
   ut_characters[0] = EOF;
   ut_character_count = 1;
   utt_od_init();
   UT_ASSERT_EQ_UINT(1, ut_character_index);

   prepare_gap();
   ut_set_line(23, "skip,this\n");
   utt_od_init();
   UT_ASSERT_EQ_INT(DOORSYS_WILDCAT, od_control.od_info_type);
}
#endif

#if UT_TURBO_SHARD == 0 || UT_TURBO_SHARD == 3
static void handles_no_file_callback_outcomes(void)
{
   reset_init_fixture();
   od_control.od_force_local = FALSE;
   od_control.od_no_file_func = ut_no_file_callback;
   ut_no_file_forces_local = TRUE;
   utt_od_init();
   UT_ASSERT_EQ_UINT(1, ut_no_file_calls);
   UT_ASSERT_EQ_INT(NO_DOOR_FILE, od_control.od_info_type);

   reset_init_fixture();
   od_control.od_force_local = FALSE;
   od_control.od_no_file_func = ut_no_file_callback;
   ut_no_file_supplies_info = TRUE;
   utt_od_init();
   UT_ASSERT_EQ_UINT(1, ut_no_file_calls);
   UT_ASSERT_EQ_INT(CUSTOM, od_control.od_info_type);

   reset_init_fixture();
   od_control.od_force_local = FALSE;
   run_expecting_fatal_exit();

   reset_init_fixture();
   od_control.od_force_local = FALSE;
   wPreSetInfo = PRESET_REQUIRED;
   utt_od_init();
   UT_ASSERT_EQ_INT(NO_DOOR_FILE, od_control.od_info_type);
}
#endif

static const UTTestCase ut_cases[] = {
#if UT_TURBO_SHARD == 0 || UT_TURBO_SHARD == 1
   {"MPS callback type", accepts_public_mps_callback_type},
   {"entry guards", honors_entry_guards_and_sync_failure},
   {"initialization exit", completes_initialization_before_processing_an_exit_request},
   {"failed initialization", does_not_publish_a_failed_initialization},
   {"initial defaults", initializes_tables_allocations_and_local_defaults},
   {"configured screen dimensions", preserves_configured_screen_dimensions},
   {"configuration delegation", preserves_tables_and_delegates_to_configuration},
   {"allocation failures", handles_allocation_and_queue_failures},
   {"node precedence", applies_node_precedence},
   {"custom and forced local", handles_custom_and_forced_local_inputs},
   {"configuration resume", resumes_after_configuration_callback},
   {"disabled drop files", handles_disabled_drop_files},
#endif
#if UT_TURBO_SHARD == 0 || UT_TURBO_SHARD == 2
   {"DORINFO", reads_dorinfo_and_each_required_line_failure},
   {"CHAIN", reads_chain_and_each_required_line_failure},
   {"DoorWay", reads_doorway_and_each_required_line_failure},
   {"CALLINFO", reads_callinfo_and_each_required_line_failure},
   {"TriBBS", reads_tribbs_and_each_required_line_failure},
   {"DOOR32", reads_door32_and_switch_variants},
#endif
#if UT_TURBO_SHARD == 0 || UT_TURBO_SHARD == 3
   {"drop selection", selects_node_names_and_explicit_paths},
   {"EXITINFO and SF", reads_exitinfo_and_sf_variants},
#endif
#if UT_TURBO_SHARD == 0 || UT_TURBO_SHARD == 4
   {"GAP lines", reads_gap_required_and_optional_lines},
   {"GAP communication", reads_gap_communication_modes},
   {"GAP discriminator", distinguishes_gap_from_doorway},
   {"GAP long lines", handles_gap_long_and_repeated_lines},
#endif
#if UT_TURBO_SHARD == 0 || UT_TURBO_SHARD == 3
   {"no-file callback", handles_no_file_callback_outcomes}
#endif
};
