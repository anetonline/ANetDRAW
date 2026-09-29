#define UT_CUSTOM_MOCK_ODDirClose
#define UT_CUSTOM_MOCK_ODDirOpen
#define UT_CUSTOM_MOCK_ODDirRead
#define UT_CUSTOM_MOCK_ODListFilenameMerge
#define UT_CUSTOM_MOCK_ODListFilenameSplit
#define UT_CUSTOM_MOCK_ODListGetFirstWord
#define UT_CUSTOM_MOCK_ODListGetRemainingWords
#define UT_CUSTOM_MOCK_ODMakeFilename
#define UT_CUSTOM_MOCK_ODPagePrompt
#define UT_CUSTOM_MOCK_ODStringCopy
#define UT_CUSTOM_MOCK_ODStringNormalizeLine
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_ODInQueueExchangeLastControlKey
#define UT_CUSTOM_MOCK_fclose
#define UT_CUSTOM_MOCK_fgets
#define UT_CUSTOM_MOCK_fopen
#define UT_CUSTOM_MOCK_od_clear_keybuffer
#define UT_CUSTOM_MOCK_od_disp_str
#define UT_CUSTOM_MOCK_od_get_key
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_printf
#define UT_CUSTOM_MOCK_od_set_attrib
#define UT_CUSTOM_MOCK_strcpy
#define UT_CUSTOM_MOCK_strlen

#include <stdarg.h>

#define UT_SCRIPT_MAX 32
#define UT_FAILURE ((tODResult)1)

typedef struct {
   const char *text;
   BOOL complete;
   char control;
} UTFileLine;

static int ut_fake_file;
static int ut_handles[UT_SCRIPT_MAX];
static UTFileLine ut_lines[UT_SCRIPT_MAX];
static unsigned ut_line_count;
static unsigned ut_line_index;
static unsigned ut_normalize_index;
static tODResult ut_open_results[UT_SCRIPT_MAX];
static unsigned ut_open_calls;
static char ut_open_paths[UT_SCRIPT_MAX][128];
static tODResult ut_read_results[UT_SCRIPT_MAX];
static tODDirEntry ut_read_entries[UT_SCRIPT_MAX];
static unsigned ut_read_calls;
static unsigned ut_dir_close_calls;
static BOOL ut_fopen_succeeds;
static unsigned ut_fopen_calls;
static char ut_fopen_path[128];
static unsigned ut_fclose_calls;
static tODResult ut_make_results[UT_SCRIPT_MAX];
static unsigned ut_make_calls;
static BOOL ut_first_word_result;
static INT ut_split_results[UT_SCRIPT_MAX];
static unsigned ut_split_calls;
static BOOL ut_merge_results[UT_SCRIPT_MAX];
static unsigned ut_merge_calls;
static const char *ut_remaining_words;
static BOOL ut_page_result;
static unsigned ut_page_calls;
static unsigned ut_clear_calls;
static unsigned ut_get_key_calls;
static BOOL ut_end_session_on_key;
static unsigned ut_display_calls;
static unsigned ut_printf_calls;
static unsigned ut_attrib_calls;
static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static char ut_last_control_key;
static BOOL ut_init_succeeds;

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

char *utm_strcpy(char *destination, const char *source)
{
   char *result = destination;
   while((*destination++ = *source++) != '\0') { }
   return result;
}

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

FILE *utm_fopen(const char *path, const char *mode)
{
   (void)mode;
   ++ut_fopen_calls;
   utm_ODStringCopy(ut_fopen_path, path, sizeof(ut_fopen_path));
   return ut_fopen_succeeds ? (FILE *)&ut_fake_file : NULL;
}

char *utm_fgets(char *buffer, int size, FILE *file)
{
   UTFileLine *line;
   (void)file;
   if(ut_line_index >= ut_line_count) return NULL;
   line = &ut_lines[ut_line_index++];
   utm_ODStringCopy(buffer, line->text, size);
   ut_last_control_key = line->control;
   return buffer;
}

char utm_ODInQueueExchangeLastControlKey(tODInQueueHandle queue,
   char replacement)
{
   char previous;
   UT_ASSERT_EQ_PTR(hODInputQueue, queue);
   previous = ut_last_control_key;
   ut_last_control_key = replacement;
   return previous;
}

int utm_fclose(FILE *file)
{
   UT_ASSERT_EQ_PTR((FILE *)&ut_fake_file, file);
   ++ut_fclose_calls;
   return 0;
}

size_t utm_ODStringNormalizeLine(char *line, BOOL *complete)
{
   size_t length = utm_strlen(line);
   UT_ASSERT(ut_normalize_index < ut_line_count);
   *complete = ut_lines[ut_normalize_index++].complete;
   return length;
}

tODResult utm_ODDirOpen(const char *path, WORD attributes,
   tODDirHandle *handle)
{
   unsigned call = ut_open_calls++;
   UT_ASSERT(call < UT_SCRIPT_MAX);
   UT_ASSERT_EQ_UINT(DIR_ATTRIB_ARCH | DIR_ATTRIB_RDONLY |
      (call == 0 ? DIR_ATTRIB_DIREC : 0), attributes);
   utm_ODStringCopy(ut_open_paths[call], path, sizeof(ut_open_paths[call]));
   *handle = &ut_handles[call];
   return ut_open_results[call];
}

tODResult utm_ODDirRead(tODDirHandle handle, tODDirEntry *entry)
{
   unsigned call = ut_read_calls++;
   (void)handle;
   UT_ASSERT(call < UT_SCRIPT_MAX);
   if(ut_read_results[call] == kODRCSuccess)
      *entry = ut_read_entries[call];
   return ut_read_results[call];
}

void utm_ODDirClose(tODDirHandle handle)
{
   (void)handle;
   ++ut_dir_close_calls;
}

tODResult utm_ODMakeFilename(char *output, const char *path,
   const char *filename, INT maximum)
{
   unsigned call = ut_make_calls++;
   size_t path_length;
   UT_ASSERT(call < UT_SCRIPT_MAX);
   if(ut_make_results[call] != kODRCSuccess)
      return ut_make_results[call];
   utm_ODStringCopy(output, path, maximum);
   path_length = utm_strlen(output);
   utm_ODStringCopy(output + path_length, filename,
      maximum - (INT)path_length);
   return kODRCSuccess;
}

BOOL utm_ODListGetFirstWord(char *input, char *output, size_t size)
{
   size_t index = 0;
   if(!ut_first_word_result) return FALSE;
   while(input[index] != '\0' && input[index] != ' ' && index + 1 < size)
   {
      output[index] = input[index];
      ++index;
   }
   output[index] = '\0';
   return TRUE;
}

INT utm_ODListFilenameSplit(const char *path, char *drive, size_t drive_size,
   char *dir, size_t dir_size, char *name, size_t name_size,
   char *extension, size_t extension_size)
{
   unsigned call = ut_split_calls++;
   (void)path;
   utm_ODStringCopy(drive, "", (INT)drive_size);
   utm_ODStringCopy(dir, "DIR" DIRSEP_STR, (INT)dir_size);
   utm_ODStringCopy(name, "FILE", (INT)name_size);
   utm_ODStringCopy(extension, ".ZIP", (INT)extension_size);
   return ut_split_results[call];
}

BOOL utm_ODListFilenameMerge(char *output, size_t size, const char *drive,
   const char *dir, const char *name, const char *extension)
{
   unsigned call = ut_merge_calls++;
   size_t length;
   if(!ut_merge_results[call]) return FALSE;
   utm_ODStringCopy(output, drive, (INT)size);
   length = utm_strlen(output);
   utm_ODStringCopy(output + length, dir, (INT)(size - length));
   length = utm_strlen(output);
   utm_ODStringCopy(output + length, name, (INT)(size - length));
   length = utm_strlen(output);
   utm_ODStringCopy(output + length, extension, (INT)(size - length));
   return TRUE;
}

char *utm_ODListGetRemainingWords(char *input)
{
   (void)input;
   return (char *)ut_remaining_words;
}

BOOL utm_ODPagePrompt(BOOL *pausing)
{
   ++ut_page_calls;
   if(!ut_page_result) *pausing = FALSE;
   return ut_page_result;
}

void ODCALL utm_od_clear_keybuffer(void) { ++ut_clear_calls; }
char ODCALL utm_od_get_key(BOOL wait)
{
   UT_ASSERT(wait);
   ++ut_get_key_calls;
   if(ut_end_session_on_key) bODInitialized = FALSE;
   return 'x';
}
void ODCALL utm_od_disp_str(const char *text)
{
   if((text[0] == '\r' || text[0] == '\n')
      && (text[1] == '\r' || text[1] == '\n') && text[2] == '\0')
   {
      UT_ASSERT(text[0] == '\r' && text[1] == '\n');
   }
   ++ut_display_calls;
}
void ODCALL utm_od_set_attrib(INT attribute)
{
   (void)attribute;
   ++ut_attrib_calls;
}
void ODVCALL utm_od_printf(const char *format, ...)
{
   (void)format;
   ++ut_printf_calls;
}
void ODCALL utm_od_init(void)
{
   if(ut_init_succeeds) bODInitialized = TRUE;
   ++ut_init_calls;
}
void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

static void reset_list(void)
{
   unsigned index;
   memset(&od_control, 0, sizeof(od_control));
   memset(ut_lines, 0, sizeof(ut_lines));
   memset(ut_open_paths, 0, sizeof(ut_open_paths));
   memset(ut_read_entries, 0, sizeof(ut_read_entries));
   for(index = 0; index < UT_SCRIPT_MAX; ++index)
   {
      ut_open_results[index] = UT_FAILURE;
      ut_read_results[index] = UT_FAILURE;
      ut_make_results[index] = kODRCSuccess;
      ut_split_results[index] = FILENAME;
      ut_merge_results[index] = TRUE;
   }
   bODInitialized = TRUE;
   od_control.od_page_pausing = FALSE;
   od_control.user_screen_length = 24;
   od_control.od_list_stop = TRUE;
   od_control.od_list_pause = TRUE;
   od_control.od_offline = "Offline";
   ut_line_count = 0;
   ut_line_index = 0;
   ut_normalize_index = 0;
   ut_open_calls = 0;
   ut_read_calls = 0;
   ut_dir_close_calls = 0;
   ut_fopen_succeeds = TRUE;
   ut_fopen_calls = 0;
   ut_fclose_calls = 0;
   ut_make_calls = 0;
   ut_first_word_result = TRUE;
   ut_split_calls = 0;
   ut_merge_calls = 0;
   ut_remaining_words = "description";
   ut_page_result = FALSE;
   ut_page_calls = 0;
   ut_clear_calls = 0;
   ut_get_key_calls = 0;
   ut_end_session_on_key = FALSE;
   ut_display_calls = 0;
   ut_printf_calls = 0;
   ut_attrib_calls = 0;
   ut_init_calls = 0;
   ut_entries = 0;
   ut_exits = 0;
   ut_last_control_key = 0;
   ut_init_succeeds = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_list(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT(!utt_od_list_files("index.bbs"));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void add_line(const char *text, BOOL complete, char control)
{
   UT_ASSERT(ut_line_count < UT_SCRIPT_MAX);
   ut_lines[ut_line_count].text = text;
   ut_lines[ut_line_count].complete = complete;
   ut_lines[ut_line_count].control = control;
   ++ut_line_count;
}

static void make_initial_directory(BOOL directory)
{
   ut_open_results[0] = kODRCSuccess;
   ut_read_results[0] = kODRCSuccess;
   ut_read_entries[0].wAttributes = directory ? DIR_ATTRIB_DIREC : 0;
}

static void rejects_long_and_unopenable_specs(void)
{
   char long_spec[101];
   reset_list();
   memset(long_spec, 'x', sizeof(long_spec));
   long_spec[sizeof(long_spec) - 1] = '\0';
   bODInitialized = FALSE;
   UT_ASSERT(!utt_od_list_files(long_spec));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);

   reset_list();
   UT_ASSERT(!utt_od_list_files(NULL));
   UT_ASSERT(strcmp(ut_open_paths[0], ".") == 0);
   UT_ASSERT_EQ_INT(ERR_FILEOPEN, od_control.od_error);

   reset_list();
   UT_ASSERT(!utt_od_list_files(""));
   UT_ASSERT(strcmp(ut_open_paths[0], ".") == 0);
}

static void handles_directory_probe_failures(void)
{
   reset_list();
   ut_open_results[0] = kODRCSuccess;
   UT_ASSERT(!utt_od_list_files("path" DIRSEP_STR));
   UT_ASSERT_EQ_UINT(1, ut_dir_close_calls);
   UT_ASSERT_EQ_INT(ERR_FILEOPEN, od_control.od_error);
}

static void handles_index_path_and_open_failures(void)
{
   reset_list();
   make_initial_directory(TRUE);
   ut_make_results[0] = UT_FAILURE;
   UT_ASSERT(!utt_od_list_files("dir"));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);

   reset_list();
   make_initial_directory(TRUE);
   ut_fopen_succeeds = FALSE;
   UT_ASSERT(!utt_od_list_files("dir"));
   UT_ASSERT_EQ_INT(ERR_FILEOPEN, od_control.od_error);

   reset_list();
   make_initial_directory(FALSE);
   ut_fopen_succeeds = FALSE;
   UT_ASSERT(!utt_od_list_files("index.bbs"));
   UT_ASSERT_EQ_INT(ERR_FILEOPEN, od_control.od_error);
}

static void accepts_empty_indexes_and_both_pause_sources(void)
{
   reset_list();
   make_initial_directory(FALSE);
   od_control.od_page_pausing = TRUE;
   UT_ASSERT(utt_od_list_files("index.bbs"));
   UT_ASSERT_EQ_UINT(1, ut_fclose_calls);

   reset_list();
   make_initial_directory(FALSE);
   od_control.od_extended_info = TRUE;
   od_control.user_attribute = 0;
   UT_ASSERT(utt_od_list_files("index.bbs"));

   reset_list();
   make_initial_directory(FALSE);
   od_control.od_extended_info = TRUE;
   od_control.user_attribute = 4;
   UT_ASSERT(utt_od_list_files("index.bbs"));
}

static void skips_all_fragments_until_a_complete_line(void)
{
   reset_list();
   make_initial_directory(FALSE);
   add_line("fragment", FALSE, 0);
   add_line("still fragment", FALSE, 0);
   add_line("end", TRUE, 0);
   add_line(" comment", TRUE, 0);
   UT_ASSERT(utt_od_list_files("index.bbs"));
   UT_ASSERT_EQ_UINT(2, ut_display_calls);
   UT_ASSERT_EQ_UINT(0, ut_split_calls);
}

static void handles_stop_and_pause_control_keys(void)
{
   reset_list();
   make_initial_directory(FALSE);
   od_control.baud = 1;
   add_line(" comment", TRUE, 's');
   UT_ASSERT(utt_od_list_files("index.bbs"));
   UT_ASSERT_EQ_UINT(1, ut_clear_calls);

   reset_list();
   make_initial_directory(FALSE);
   od_control.baud = 0;
   add_line(" comment", TRUE, 's');
   UT_ASSERT(utt_od_list_files("index.bbs"));

   reset_list();
   make_initial_directory(FALSE);
   od_control.od_list_stop = FALSE;
   add_line(" comment", TRUE, 's');
   add_line(" comment", TRUE, 'x');
   add_line("", TRUE, 0);
   UT_ASSERT(utt_od_list_files("index.bbs"));

   reset_list();
   make_initial_directory(FALSE);
   add_line(" comment", TRUE, 'p');
   UT_ASSERT(utt_od_list_files("index.bbs"));
   UT_ASSERT_EQ_UINT(1, ut_get_key_calls);

   reset_list();
   make_initial_directory(FALSE);
   add_line(" comment", TRUE, 'p');
   ut_end_session_on_key = TRUE;
   UT_ASSERT(utt_od_list_files("index.bbs"));
   UT_ASSERT_EQ_UINT(1, ut_fclose_calls);

   reset_list();
   make_initial_directory(FALSE);
   add_line(" comment", TRUE, 'p');
   ut_lines[0].control = 'p';
   bODInitialized = TRUE;
   od_control.od_list_pause = FALSE;
   UT_ASSERT(utt_od_list_files("index.bbs"));
}

static void reports_entry_parsing_and_resolution_failures(void)
{
   reset_list();
   make_initial_directory(TRUE);
   add_line("FILE.ZIP description", TRUE, 0);
   ut_first_word_result = FALSE;
   UT_ASSERT(!utt_od_list_files("dir"));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);

   reset_list();
   make_initial_directory(TRUE);
   add_line("FILE.ZIP description", TRUE, 0);
   ut_split_results[0] = -1;
   UT_ASSERT(!utt_od_list_files("dir"));

   reset_list();
   make_initial_directory(TRUE);
   add_line("FILE.ZIP description", TRUE, 0);
   ut_make_results[1] = UT_FAILURE;
   UT_ASSERT(!utt_od_list_files("dir"));

   reset_list();
   make_initial_directory(FALSE);
   add_line("FILE.ZIP description", TRUE, 0);
   ut_split_results[1] = -1;
   UT_ASSERT(!utt_od_list_files("index.bbs"));

   reset_list();
   make_initial_directory(FALSE);
   add_line("FILE.ZIP description", TRUE, 0);
   ut_merge_results[0] = FALSE;
   UT_ASSERT(!utt_od_list_files("index.bbs"));
}

static void displays_online_and_offline_entries(void)
{
   reset_list();
   make_initial_directory(FALSE);
   add_line("FILE.ZIP offline", TRUE, 0);
   UT_ASSERT(utt_od_list_files("index.bbs"));

   reset_list();
   make_initial_directory(TRUE);
   add_line("FILE.ZIP short", TRUE, 0);
   ut_open_results[1] = kODRCSuccess;
   ut_read_results[1] = kODRCSuccess;
   utm_ODStringCopy(ut_read_entries[1].szFileName, "FILE.ZIP",
      sizeof(ut_read_entries[1].szFileName));
   ut_read_entries[1].dwFileSize = 123;
   ut_read_results[2] = UT_FAILURE;
   UT_ASSERT(utt_od_list_files("dir"));
   UT_ASSERT_EQ_UINT(2, ut_printf_calls);
   UT_ASSERT_EQ_UINT(2, ut_display_calls);

   reset_list();
   make_initial_directory(TRUE);
   add_line("FILE.ZIP long", TRUE, 0);
   ut_remaining_words = "123456789012345678901234567890123456789012345678901234567";
   ut_open_results[1] = kODRCSuccess;
   ut_read_results[1] = kODRCSuccess;
   utm_ODStringCopy(ut_read_entries[1].szFileName, "FILE.ZIP",
      sizeof(ut_read_entries[1].szFileName));
   ut_read_results[2] = UT_FAILURE;
   UT_ASSERT(utt_od_list_files("dir"));
   UT_ASSERT_EQ_UINT(3, ut_printf_calls);

   reset_list();
   make_initial_directory(TRUE);
   add_line("FILE.ZIP offline", TRUE, 0);
   ut_merge_results[0] = TRUE;
   UT_ASSERT(utt_od_list_files("dir"));
   UT_ASSERT_EQ_UINT(2, ut_printf_calls);

   reset_list();
   make_initial_directory(TRUE);
   add_line("FILE.ZIP offline", TRUE, 0);
   ut_merge_results[0] = FALSE;
   UT_ASSERT(!utt_od_list_files("dir"));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);
}

static void resolves_explicit_entry_paths_without_a_base_directory(void)
{
   reset_list();
   make_initial_directory(TRUE);
   add_line("PATH" DIRSEP_STR "FILE.ZIP description", TRUE, 0);
   ut_split_results[0] = DIRECTORY | FILENAME | EXTENSION;
   UT_ASSERT(utt_od_list_files("dir"));
   UT_ASSERT(strcmp(ut_open_paths[1], "PATH" DIRSEP_STR "FILE.ZIP") == 0);

   reset_list();
   make_initial_directory(TRUE);
   add_line("DRIVE:FILE.ZIP description", TRUE, 0);
   ut_split_results[0] = DRIVE | FILENAME | EXTENSION;
   UT_ASSERT(utt_od_list_files("dir"));
}

static void performs_and_can_abort_page_pausing(void)
{
   reset_list();
   make_initial_directory(FALSE);
   od_control.od_page_pausing = TRUE;
   od_control.user_screen_length = 3;
   add_line(" comment", TRUE, 0);
   ut_page_result = FALSE;
   UT_ASSERT(utt_od_list_files("index.bbs"));
   UT_ASSERT_EQ_UINT(1, ut_page_calls);

   reset_list();
   make_initial_directory(FALSE);
   od_control.od_page_pausing = TRUE;
   od_control.user_screen_length = 3;
   add_line(" comment", TRUE, 0);
   ut_page_result = TRUE;
   UT_ASSERT(utt_od_list_files("index.bbs"));
   UT_ASSERT_EQ_UINT(1, ut_page_calls);

   reset_list();
   make_initial_directory(FALSE);
   od_control.od_page_pausing = FALSE;
   od_control.user_screen_length = 1;
   add_line(" comment", TRUE, 0);
   UT_ASSERT(utt_od_list_files("index.bbs"));
   UT_ASSERT_EQ_UINT(0, ut_page_calls);
}

static const UTTestCase ut_cases[] = {
   {"spec validation", rejects_long_and_unopenable_specs},
   {"directory probe", handles_directory_probe_failures},
   {"index opening", handles_index_path_and_open_failures},
   {"empty index", accepts_empty_indexes_and_both_pause_sources},
   {"line fragments", skips_all_fragments_until_a_complete_line},
   {"control keys", handles_stop_and_pause_control_keys},
   {"entry failures", reports_entry_parsing_and_resolution_failures},
   {"online and offline", displays_online_and_offline_entries},
   {"explicit entry path", resolves_explicit_entry_paths_without_a_base_directory},
   {"page prompt", performs_and_can_abort_page_pausing},
   {"terminal session", terminal_session_is_rejected}
};
