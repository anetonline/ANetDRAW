#define UT_CUSTOM_MOCK_ODSizeAdd
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_ODVsnprintf
#define UT_CUSTOM_MOCK_od_color_config
#define UT_CUSTOM_MOCK_od_disp
#define UT_CUSTOM_MOCK_od_disp_str
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_set_attrib
#define UT_CUSTOM_MOCK_realloc

#include <stdarg.h>
#include <string.h>

#define UT_PRINTF_BUFFER_SIZE 1024
#define UT_PRINTF_DISPLAY_SIZE 1024

static char ut_printf_buffer[UT_PRINTF_BUFFER_SIZE];
static char ut_display[UT_PRINTF_DISPLAY_SIZE];
static char ut_large_output[601];
static const char *ut_formatted;
static int ut_probe_result;
static int ut_write_result;
static int ut_format_phase;
static BOOL ut_size_add_fails;
static BOOL ut_realloc_fails;
static unsigned ut_size_add_calls;
static unsigned ut_realloc_calls;
static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static unsigned ut_disp_calls;
static unsigned ut_disp_str_calls;
static unsigned ut_attribute_calls;
static INT ut_last_attribute;
static size_t ut_display_length;
static size_t ut_last_realloc_size;
static BOOL ut_init_succeeds;

static void reset_printf_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
   memset(szODWorkString, 0, sizeof(szODWorkString));
   memset(ut_printf_buffer, 0, sizeof(ut_printf_buffer));
   memset(ut_display, 0, sizeof(ut_display));
   bODInitialized = TRUE;
   chColorCheck = 0;
   pchColorEndPos = NULL;
   ut_formatted = "";
   ut_probe_result = 0;
   ut_write_result = 0;
   ut_format_phase = 0;
   ut_size_add_fails = FALSE;
   ut_realloc_fails = FALSE;
   ut_size_add_calls = 0;
   ut_realloc_calls = 0;
   ut_init_calls = 0;
   ut_entries = 0;
   ut_exits = 0;
   ut_disp_calls = 0;
   ut_disp_str_calls = 0;
   ut_attribute_calls = 0;
   ut_last_attribute = -1;
   ut_display_length = 0;
   ut_last_realloc_size = 0;
   ut_init_succeeds = TRUE;
}

static void select_output(const char *text)
{
   ut_formatted = text;
   ut_probe_result = (int)strlen(text);
   ut_write_result = ut_probe_result;
   ut_format_phase = 0;
   ut_disp_calls = 0;
   ut_disp_str_calls = 0;
   ut_attribute_calls = 0;
   ut_last_attribute = -1;
   ut_display_length = 0;
   ut_display[0] = '\0';
}

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_printf_fixture(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   utt_od_printf("%d", 42);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
   UT_ASSERT_EQ_UINT(0, ut_realloc_calls);
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

int utm_ODSizeAdd(size_t left, size_t right, size_t *result)
{
   ++ut_size_add_calls;
   UT_ASSERT_EQ_INT(ut_probe_result, left);
   UT_ASSERT_EQ_UINT(1, right);
   UT_ASSERT_NOT_NULL(result);
   if(ut_size_add_fails) return FALSE;
   *result = left + right;
   return TRUE;
}

void *utm_realloc(void *memory, size_t size)
{
   ++ut_realloc_calls;
   UT_ASSERT(memory == NULL || memory == ut_printf_buffer);
   UT_ASSERT_EQ_UINT(MAX((size_t)ut_probe_result + 1, (size_t)512), size);
   ut_last_realloc_size = size;
   return ut_realloc_fails ? NULL : ut_printf_buffer;
}

int utm_ODVsnprintf(char *buffer, size_t size, const char *format,
   va_list arguments)
{
   int value;
   UT_ASSERT_EQ_INT(0, strcmp("%d", format));
   value = va_arg(arguments, int);
   UT_ASSERT_EQ_INT(42, value);
   if(ut_format_phase == 0)
   {
      UT_ASSERT_EQ_PTR(szODWorkString, buffer);
      UT_ASSERT_EQ_UINT(0, size);
      ++ut_format_phase;
      return ut_probe_result;
   }
   UT_ASSERT_EQ_PTR(ut_printf_buffer, buffer);
   UT_ASSERT(size >= strlen(ut_formatted) + 1);
   strcpy(buffer, ut_formatted);
   ++ut_format_phase;
   return ut_write_result;
}

static void append_display(const char *text, size_t length)
{
   UT_ASSERT(ut_display_length + length < sizeof(ut_display));
   memcpy(ut_display + ut_display_length, text, length);
   ut_display_length += length;
   ut_display[ut_display_length] = '\0';
}

void ODCALL utm_od_disp(const char *buffer, INT size, BOOL local_echo)
{
   UT_ASSERT(size >= 0);
   UT_ASSERT(local_echo);
   append_display(buffer, (size_t)size);
   ++ut_disp_calls;
}

void ODCALL utm_od_disp_str(const char *text)
{
   append_display(text, strlen(text));
   ++ut_disp_str_calls;
}

BYTE ODCALL utm_od_color_config(char *description)
{
   char *end = description;
   while(*end != '\0' && *end != od_control.od_color_delimiter) ++end;
   pchColorEndPos = end;
   return 0x1e;
}

void ODCALL utm_od_set_attrib(INT attribute)
{
   ut_last_attribute = attribute;
   ++ut_attribute_calls;
}

static void formats_initializes_and_reuses_the_buffer(void)
{
   reset_printf_fixture();
   bODInitialized = FALSE;
   select_output("plain");
   utt_od_printf("%d", 42);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(1, ut_realloc_calls);
   UT_ASSERT_EQ_UINT(1, ut_disp_str_calls);
   UT_ASSERT_EQ_INT(0, strcmp("plain", ut_display));

   select_output("x");
   utt_od_printf("%d", 42);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(2, ut_entries);
   UT_ASSERT_EQ_UINT(2, ut_exits);
   UT_ASSERT_EQ_UINT(1, ut_realloc_calls);
   UT_ASSERT_EQ_UINT(1, ut_disp_str_calls);
   UT_ASSERT_EQ_INT(0, strcmp("x", ut_display));
}

static void grows_for_output_larger_than_the_initial_buffer(void)
{
   unsigned index;
   reset_printf_fixture();
   for(index = 0; index < sizeof(ut_large_output) - 1; ++index)
      ut_large_output[index] = 'L';
   ut_large_output[sizeof(ut_large_output) - 1] = '\0';
   select_output(ut_large_output);
   utt_od_printf("%d", 42);
   UT_ASSERT_EQ_UINT(sizeof(ut_large_output), ut_last_realloc_size);
   UT_ASSERT_EQ_UINT(sizeof(ut_large_output) - 1, ut_display_length);
   UT_ASSERT_EQ_INT(0, strcmp(ut_large_output, ut_display));
}

static void rejects_a_negative_required_size(void)
{
   reset_printf_fixture();
   ut_probe_result = -1;
   utt_od_printf("%d", 42);
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_size_add_calls);
   UT_ASSERT_EQ_UINT(0, ut_realloc_calls);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static void rejects_an_unrepresentable_required_size(void)
{
   reset_printf_fixture();
   ut_probe_result = 12;
   ut_size_add_fails = TRUE;
   utt_od_printf("%d", 42);
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_size_add_calls);
   UT_ASSERT_EQ_UINT(0, ut_realloc_calls);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static void reports_buffer_allocation_failure(void)
{
   reset_printf_fixture();
   select_output("allocate");
   ut_realloc_fails = TRUE;
   utt_od_printf("%d", 42);
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_realloc_calls);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static void reports_an_inconsistent_second_format(void)
{
   reset_printf_fixture();
   select_output("mismatch");
   ut_write_result = ut_probe_result - 1;
   utt_od_printf("%d", 42);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(2, ut_format_phase);
   UT_ASSERT_EQ_UINT(0, ut_disp_calls);
   UT_ASSERT_EQ_UINT(0, ut_disp_str_calls);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static void scans_plain_text_for_either_color_syntax(void)
{
   reset_printf_fixture();
   od_control.od_color_char = '^';
   select_output("character mode plain");
   utt_od_printf("%d", 42);
   UT_ASSERT_EQ_UINT(1, ut_disp_str_calls);
   UT_ASSERT_EQ_INT(0, strcmp("character mode plain", ut_display));

   od_control.od_color_char = 0;
   od_control.od_color_delimiter = '|';
   select_output("delimiter mode plain");
   utt_od_printf("%d", 42);
   UT_ASSERT_EQ_UINT(1, ut_disp_str_calls);
   UT_ASSERT_EQ_INT(0, strcmp("delimiter mode plain", ut_display));
}

static void handles_delimited_color_descriptions_at_each_boundary(void)
{
   reset_printf_fixture();
   od_control.od_color_delimiter = '|';

   select_output("|x|B");
   utt_od_printf("%d", 42);
   UT_ASSERT_EQ_UINT(1, ut_attribute_calls);
   UT_ASSERT_EQ_INT(0x1e, ut_last_attribute);
   UT_ASSERT_EQ_INT(0, strcmp("B", ut_display));

   select_output("A|");
   utt_od_printf("%d", 42);
   UT_ASSERT_EQ_UINT(0, ut_attribute_calls);
   UT_ASSERT_EQ_INT(0, strcmp("A", ut_display));

   select_output("A|x");
   utt_od_printf("%d", 42);
   UT_ASSERT_EQ_UINT(1, ut_attribute_calls);
   UT_ASSERT_EQ_INT(0, strcmp("A", ut_display));

   select_output("A|x|");
   utt_od_printf("%d", 42);
   UT_ASSERT_EQ_UINT(1, ut_attribute_calls);
   UT_ASSERT_EQ_INT(0, strcmp("A", ut_display));
   UT_ASSERT_EQ_INT(0, chColorCheck);
}

static void handles_single_character_colors_at_each_boundary(void)
{
   reset_printf_fixture();
   od_control.od_color_char = '^';

   select_output("^xB");
   utt_od_printf("%d", 42);
   UT_ASSERT_EQ_UINT(1, ut_attribute_calls);
   UT_ASSERT_EQ_INT('x', ut_last_attribute);
   UT_ASSERT_EQ_INT(0, strcmp("B", ut_display));

   select_output("A^");
   utt_od_printf("%d", 42);
   UT_ASSERT_EQ_UINT(0, ut_attribute_calls);
   UT_ASSERT_EQ_INT(0, strcmp("A", ut_display));

   select_output("A^x");
   utt_od_printf("%d", 42);
   UT_ASSERT_EQ_UINT(1, ut_attribute_calls);
   UT_ASSERT_EQ_INT('x', ut_last_attribute);
   UT_ASSERT_EQ_INT(0, strcmp("A", ut_display));
   UT_ASSERT_EQ_INT(0, chColorCheck);
}

static const UTTestCase ut_cases[] = {
   {"format, initialize, and reuse", formats_initializes_and_reuses_the_buffer},
   {"grow beyond initial buffer", grows_for_output_larger_than_the_initial_buffer},
   {"negative required size", rejects_a_negative_required_size},
   {"unrepresentable required size", rejects_an_unrepresentable_required_size},
   {"allocation failure", reports_buffer_allocation_failure},
   {"inconsistent second format", reports_an_inconsistent_second_format},
   {"plain color scanning", scans_plain_text_for_either_color_syntax},
   {"delimited colors", handles_delimited_color_descriptions_at_each_boundary},
   {"single-character colors", handles_single_character_colors_at_each_boundary},
   {"terminal session", terminal_session_is_rejected}
};
