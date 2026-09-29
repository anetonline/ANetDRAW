#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_ODScrnGetText
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos
#define UT_CUSTOM_MOCK_ODScrnSetAttribute
#define UT_CUSTOM_MOCK_ODScrnDisplayChar
#define UT_CUSTOM_MOCK_ODScrnDisplayString
#define UT_CUSTOM_MOCK_strlen

static BYTE ut_window[1028];
static BOOL ut_malloc_fails;
static BOOL ut_has_title;
static BYTE ut_left;
static BYTE ut_top;
static BYTE ut_right;
static BYTE ut_bottom;
static size_t ut_expected_malloc_size;
static unsigned ut_malloc_calls;
static unsigned ut_get_calls;
static unsigned ut_cursor_calls;
static unsigned ut_attribute_calls;
static unsigned ut_char_calls;
static unsigned ut_string_calls;
static unsigned char ut_chars[256];

size_t utm_strlen(const char *string)
{
   size_t length;
   length = 0;
   while(string[length] != '\0') ++length;
   return length;
}

void *utm_malloc(size_t count)
{
   ++ut_malloc_calls;
   UT_ASSERT_EQ_UINT(ut_expected_malloc_size, count);
   return ut_malloc_fails ? NULL : ut_window;
}

BOOL ODCALL utm_ODScrnGetText(BYTE left, BYTE top, BYTE right, BYTE bottom,
   void *buffer)
{
   ++ut_get_calls;
   UT_ASSERT_EQ_UINT(ut_left, left); UT_ASSERT_EQ_UINT(ut_top, top);
   UT_ASSERT_EQ_UINT(ut_right, right); UT_ASSERT_EQ_UINT(ut_bottom, bottom);
   UT_ASSERT(buffer == ut_window + 4);
   return TRUE;
}

void ODCALL utm_ODScrnSetCursorPos(BYTE column, BYTE row)
{
   UT_ASSERT_EQ_UINT(ut_left, column);
   UT_ASSERT_EQ_UINT((unsigned)ut_top + ut_cursor_calls, row);
   ++ut_cursor_calls;
}

void ODCALL utm_ODScrnSetAttribute(BYTE attribute)
{
   ++ut_attribute_calls;
   if(ut_has_title && ut_attribute_calls == 2)
      UT_ASSERT_EQ_UINT(0x3f, attribute);
   else
      UT_ASSERT_EQ_UINT(0x17, attribute);
}

void ODCALL utm_ODScrnDisplayChar(unsigned char character)
{
   if(ut_char_calls < DIM(ut_chars))
      ut_chars[ut_char_calls] = (unsigned char)character;
   ++ut_char_calls;
}

void ODCALL utm_ODScrnDisplayString(const char *string)
{
   ++ut_string_calls;
   UT_ASSERT(ut_has_title && ut_string_calls == 1);
   UT_ASSERT(strcmp(string, "HI") == 0);
}

static void reset_window(void)
{
   unsigned index;
   ut_malloc_fails = FALSE; ut_has_title = FALSE;
   ut_left = 2; ut_top = 3; ut_right = 8; ut_bottom = 6;
   ut_expected_malloc_size = 60;
   ut_malloc_calls = ut_get_calls = ut_cursor_calls = 0;
   ut_attribute_calls = ut_char_calls = ut_string_calls = 0;
   for(index = 0; index < DIM(ut_chars); ++index) ut_chars[index] = 0;
}

static void reports_allocation_failure_without_drawing(void)
{
   reset_window(); ut_malloc_fails = TRUE;
   UT_ASSERT(utt_ODScrnCreateWindow(2, 3, 8, 6, 0x17, "", 0x3f) == NULL);
   UT_ASSERT_EQ_UINT(1, ut_malloc_calls); UT_ASSERT_EQ_UINT(0, ut_get_calls);
   UT_ASSERT_EQ_UINT(0, ut_cursor_calls); UT_ASSERT_EQ_UINT(0, ut_char_calls);
}

static void draws_an_untitled_window_and_returns_the_saved_area(void)
{
   void *result;
   reset_window();
   result = utt_ODScrnCreateWindow(2, 3, 8, 6, 0x17, "", 0x3f);
   UT_ASSERT(result == ut_window);
   UT_ASSERT_EQ_UINT(2, ut_window[0]); UT_ASSERT_EQ_UINT(3, ut_window[1]);
   UT_ASSERT_EQ_UINT(8, ut_window[2]); UT_ASSERT_EQ_UINT(6, ut_window[3]);
   UT_ASSERT_EQ_UINT(1, ut_get_calls); UT_ASSERT_EQ_UINT(4, ut_cursor_calls);
   UT_ASSERT_EQ_UINT(1, ut_attribute_calls); UT_ASSERT_EQ_UINT(28, ut_char_calls);
   UT_ASSERT_EQ_UINT(0, ut_string_calls);
   UT_ASSERT_EQ_UINT(214, ut_chars[0]); UT_ASSERT_EQ_UINT(183, ut_chars[6]);
   UT_ASSERT_EQ_UINT(211, ut_chars[21]); UT_ASSERT_EQ_UINT(189, ut_chars[27]);
}

static void centers_a_title_with_its_own_attribute(void)
{
   reset_window(); ut_has_title = TRUE;
   UT_ASSERT(utt_ODScrnCreateWindow(2, 3, 8, 6, 0x17, "HI", 0x3f)
      == ut_window);
   UT_ASSERT_EQ_UINT(3, ut_attribute_calls);
   UT_ASSERT_EQ_UINT(26, ut_char_calls); UT_ASSERT_EQ_UINT(1, ut_string_calls);
   UT_ASSERT_EQ_UINT(214, ut_chars[0]); UT_ASSERT_EQ_UINT(' ', ut_chars[1]);
   UT_ASSERT_EQ_UINT(' ', ut_chars[2]); UT_ASSERT_EQ_UINT(196, ut_chars[3]);
   UT_ASSERT_EQ_UINT(183, ut_chars[4]);
}

static void draws_a_wide_window_without_a_fixed_line_buffer(void)
{
   reset_window();
   ut_left = 1; ut_top = 1; ut_right = 82; ut_bottom = 3;
   ut_expected_malloc_size = 496;
   UT_ASSERT(utt_ODScrnCreateWindow(1, 1, 82, 3, 0x17, "", 0x3f)
      == ut_window);
   UT_ASSERT_EQ_UINT(3, ut_cursor_calls);
   UT_ASSERT_EQ_UINT(246, ut_char_calls);
   UT_ASSERT_EQ_UINT(0, ut_string_calls);
}

static const UTTestCase ut_cases[] = {
   {"allocation failure", reports_allocation_failure_without_drawing},
   {"untitled window", draws_an_untitled_window_and_returns_the_saved_area},
   {"titled window", centers_a_title_with_its_own_attribute},
   {"wide window", draws_a_wide_window_without_a_fixed_line_buffer}
};
