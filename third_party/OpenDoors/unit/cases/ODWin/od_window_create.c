#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_od_disp
#define UT_CUSTOM_MOCK_od_emulate
#define UT_CUSTOM_MOCK_od_gettext
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_putch
#define UT_CUSTOM_MOCK_od_repeat
#define UT_CUSTOM_MOCK_od_set_attrib
#define UT_CUSTOM_MOCK_od_set_cursor
#define UT_CUSTOM_MOCK_strlen

static unsigned char ut_window_buffer[5000];
static BOOL ut_malloc_fails;
static BOOL ut_gettext_result;
static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static unsigned ut_frees;
static unsigned ut_malloc_calls;
static unsigned ut_gettext_calls;
static unsigned ut_display_calls;
static INT ut_display_size;
static unsigned ut_emulate_calls;
static unsigned ut_cursor_calls;
static unsigned ut_attrib_calls;
static unsigned ut_putch_calls;
static unsigned ut_repeat_calls;
static BOOL ut_init_succeeds;

static void reset_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
   memset(ut_window_buffer, 0, sizeof(ut_window_buffer));
   bODInitialized = TRUE;
   od_control.user_ansi = TRUE;
   od_control.od_box_chars[BOX_TOP] = '-';
   od_control.od_box_chars[BOX_BOTTOM] = '=';
   od_control.od_box_chars[BOX_LEFT] = '|';
   od_control.od_box_chars[BOX_RIGHT] = '!';
   od_control.od_box_chars[BOX_UPPERLEFT] = '1';
   od_control.od_box_chars[BOX_UPPERRIGHT] = '2';
   od_control.od_box_chars[BOX_LOWERLEFT] = '3';
   od_control.od_box_chars[BOX_LOWERRIGHT] = '4';
   ut_malloc_fails = FALSE;
   ut_gettext_result = TRUE;
   ut_init_calls = ut_entries = ut_exits = 0;
   ut_frees = ut_malloc_calls = ut_gettext_calls = 0;
   ut_display_calls = ut_emulate_calls = 0;
   ut_cursor_calls = ut_attrib_calls = 0;
   ut_putch_calls = ut_repeat_calls = 0;
   ut_display_size = -1;
   ut_init_succeeds = TRUE;
}

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_fixture(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT_NULL(utt_od_window_create(2, 2, 9, 6, "A", 7, 7, 7, 0));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

void *utm_malloc(size_t size)
{
   ++ut_malloc_calls;
   UT_ASSERT(size <= sizeof(ut_window_buffer));
   return ut_malloc_fails ? NULL : ut_window_buffer;
}

void utm_free(void *memory)
{
   UT_ASSERT_EQ_PTR(ut_window_buffer, memory);
   ++ut_frees;
}

size_t utm_strlen(const char *text)
{
   const char *end = text;
   while(*end != '\0') ++end;
   return (size_t)(end - text);
}

BOOL ODCALL utm_od_gettext(INT left, INT top, INT right, INT bottom,
   void *contents)
{
   UT_ASSERT(left >= 1);
   UT_ASSERT(top >= 1);
   UT_ASSERT(right <= 80);
   UT_ASSERT(bottom <= 25);
   UT_ASSERT_EQ_PTR(ut_window_buffer + 4, contents);
   ++ut_gettext_calls;
   return ut_gettext_result;
}

void ODCALL utm_od_set_cursor(INT row, INT column)
{
   UT_ASSERT(row >= 1);
   UT_ASSERT(column >= 1);
   ++ut_cursor_calls;
}

void ODCALL utm_od_set_attrib(INT attribute)
{
   (void)attribute;
   ++ut_attrib_calls;
}

void ODCALL utm_od_putch(char character)
{
   (void)character;
   ++ut_putch_calls;
}

void ODCALL utm_od_repeat(char character, BYTE count)
{
   (void)character;
   (void)count;
   ++ut_repeat_calls;
}

void ODCALL utm_od_disp(const char *text, INT length, BOOL local_echo)
{
   UT_ASSERT_NOT_NULL(text);
   UT_ASSERT(local_echo);
   ++ut_display_calls;
   ut_display_size = length;
}

void ODCALL utm_od_emulate(char value)
{
   (void)value;
   ++ut_emulate_calls;
}

static void *create_window(INT left, INT top, INT right, INT bottom,
   char *title)
{
   return utt_od_window_create(left, top, right, bottom, title,
      1, 2, 3, 0);
}

static void rejects_display_without_graphics_after_initializing(void)
{
   reset_fixture();
   bODInitialized = FALSE;
   od_control.user_ansi = FALSE;
   od_control.user_avatar = FALSE;
   od_control.od_box_chars[BOX_BOTTOM] = 0;
   od_control.od_box_chars[BOX_RIGHT] = 0;
   UT_ASSERT_NULL(create_window(1, 1, 5, 3, NULL));
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_INT('-', od_control.od_box_chars[BOX_BOTTOM]);
   UT_ASSERT_EQ_INT('|', od_control.od_box_chars[BOX_RIGHT]);
   UT_ASSERT_EQ_INT(ERR_NOGRAPHICS, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static void rejects_each_invalid_coordinate_independently(void)
{
   static const INT coordinates[][4] = {
      {0, 1, 5, 4}, {1, 0, 5, 4}, {1, 1, 81, 4},
      {1, 1, 5, 26}, {1, 1, 2, 4}, {1, 1, 5, 2}
   };
   unsigned index;
   for(index = 0; index < sizeof(coordinates) / sizeof(coordinates[0]);
      ++index)
   {
      reset_fixture();
      UT_ASSERT_NULL(create_window(coordinates[index][0],
         coordinates[index][1], coordinates[index][2],
         coordinates[index][3], NULL));
      UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
      UT_ASSERT_EQ_UINT(0, ut_malloc_calls);
   }
}

static void reports_allocation_and_screen_capture_failures(void)
{
   reset_fixture();
   ut_malloc_fails = TRUE;
   UT_ASSERT_NULL(create_window(1, 1, 5, 4, NULL));
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_gettext_calls);

   reset_fixture();
   ut_gettext_result = FALSE;
   UT_ASSERT_NULL(create_window(1, 1, 5, 4, NULL));
   UT_ASSERT_EQ_UINT(1, ut_gettext_calls);
   UT_ASSERT_EQ_UINT(1, ut_frees);
}

static void draws_ansi_windows_with_each_title_form(void)
{
   reset_fixture();
   UT_ASSERT_EQ_PTR(ut_window_buffer,
      create_window(1, 1, 8, 4, NULL));
   UT_ASSERT_EQ_UINT(0, ut_display_calls);
   UT_ASSERT(ut_repeat_calls > 0);

   reset_fixture();
   UT_ASSERT_EQ_PTR(ut_window_buffer,
      create_window(1, 1, 4, 3, "x"));
   UT_ASSERT_EQ_UINT(0, ut_display_calls);

   reset_fixture();
   UT_ASSERT_EQ_PTR(ut_window_buffer,
      create_window(1, 1, 10, 4, "Hi"));
   UT_ASSERT_EQ_UINT(1, ut_display_calls);
   UT_ASSERT_EQ_INT(2, ut_display_size);

   reset_fixture();
   UT_ASSERT_EQ_PTR(ut_window_buffer,
      create_window(1, 1, 10, 4, "Long window title"));
   UT_ASSERT_EQ_UINT(1, ut_display_calls);
   UT_ASSERT_EQ_INT(4, ut_display_size);
}

static void draws_avatar_window_and_preserves_explicit_box_sides(void)
{
   reset_fixture();
   od_control.user_ansi = FALSE;
   od_control.user_avatar = TRUE;
   UT_ASSERT_EQ_PTR(ut_window_buffer,
      create_window(2, 2, 9, 6, "A"));
   UT_ASSERT_EQ_UINT(5, ut_emulate_calls);
   UT_ASSERT(ut_cursor_calls > 0);
   UT_ASSERT_EQ_INT('=', od_control.od_box_chars[BOX_BOTTOM]);
   UT_ASSERT_EQ_INT('!', od_control.od_box_chars[BOX_RIGHT]);
}

static const UTTestCase ut_cases[] = {
   {"graphics required", rejects_display_without_graphics_after_initializing},
   {"coordinate validation", rejects_each_invalid_coordinate_independently},
   {"allocation failures", reports_allocation_and_screen_capture_failures},
   {"ANSI drawing", draws_ansi_windows_with_each_title_form},
   {"AVATAR drawing", draws_avatar_window_and_preserves_explicit_box_sides},
   {"terminal session", terminal_session_is_rejected}
};
