#define UT_CUSTOM_MOCK_ODScrnDisplayString
#define UT_CUSTOM_MOCK_ODScrnSetAttribute
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos
#define UT_CUSTOM_MOCK_localtime
#define UT_CUSTOM_MOCK_time

static struct tm ut_time_record;
static char ut_text[16];
static unsigned ut_text_calls;
static unsigned ut_cursor_calls;
static BYTE ut_attributes[4];
static unsigned ut_attribute_calls;

time_t utm_time(time_t *storage)
{
   UT_ASSERT(storage == NULL);
   return (time_t)123;
}

struct tm *utm_localtime(const time_t *value)
{
   UT_ASSERT(value != NULL);
   UT_ASSERT_EQ_INT(123, *value);
   return &ut_time_record;
}

void ODCALL utm_ODScrnSetCursorPos(BYTE column, BYTE row)
{
   ++ut_cursor_calls;
   UT_ASSERT_EQ_UINT(60, column);
   UT_ASSERT_EQ_UINT(24, row);
}

void ODCALL utm_ODScrnSetAttribute(BYTE attribute)
{
   UT_ASSERT(ut_attribute_calls < sizeof(ut_attributes));
   ut_attributes[ut_attribute_calls++] = attribute;
}

void ODCALL utm_ODScrnDisplayString(const char *text)
{
   size_t length = strlen(text);
   UT_ASSERT(length < sizeof(ut_text));
   memcpy(ut_text, text, length + 1);
   ++ut_text_calls;
}

static void reset_page(INT mode, int minute)
{
   memset(&ut_time_record, 0, sizeof(ut_time_record));
   ut_time_record.tm_hour = minute / 60;
   ut_time_record.tm_min = minute % 60;
   od_control.od_okaytopage = mode;
   ut_text[0] = '\0';
   ut_text_calls = 0;
   ut_cursor_calls = 0;
   ut_attribute_calls = 0;
}

static void expect_page(INT mode, int minute, const char *expected)
{
   reset_page(mode, minute);
   utt_ODRADisplayPageInfo();
   UT_ASSERT_EQ_UINT(1, ut_cursor_calls);
   UT_ASSERT_EQ_UINT(1, ut_text_calls);
   UT_ASSERT(strcmp(expected, ut_text) == 0);
}

static void displays_explicit_page_states(void)
{
   expect_page(PAGE_ENABLE, 0, "(PAGE ON) ");
   UT_ASSERT_EQ_UINT(2, ut_attribute_calls);
   UT_ASSERT_EQ_UINT(0x19, ut_attributes[0]);
   UT_ASSERT_EQ_UINT(0x70, ut_attributes[1]);

   expect_page(PAGE_DISABLE, 0, "(PAGE OFF)");
   UT_ASSERT_EQ_UINT(2, ut_attribute_calls);
}

static void applies_a_nonwrapping_page_window(void)
{
   od_control.od_pagestartmin = 100;
   od_control.od_pageendmin = 200;
   expect_page(PAGE_USE_HOURS, 99, "(PAGE OFF)");
   expect_page(PAGE_USE_HOURS, 200, "(PAGE OFF)");
   expect_page(PAGE_USE_HOURS, 150, "(PAGE ON) ");
}

static void applies_a_midnight_wrapping_page_window(void)
{
   od_control.od_pagestartmin = 1200;
   od_control.od_pageendmin = 400;
   expect_page(PAGE_USE_HOURS, 500, "(PAGE OFF)");
   expect_page(PAGE_USE_HOURS, 300, "(PAGE ON) ");
   expect_page(PAGE_USE_HOURS, 1300, "(PAGE ON) ");

   od_control.od_pagestartmin = 500;
   od_control.od_pageendmin = 500;
   expect_page(PAGE_USE_HOURS, 500, "(PAGE ON) ");
}

static void ignores_an_unknown_page_mode(void)
{
   reset_page(99, 0);
   utt_ODRADisplayPageInfo();
   UT_ASSERT_EQ_UINT(1, ut_cursor_calls);
   UT_ASSERT_EQ_UINT(0, ut_text_calls);
   UT_ASSERT_EQ_UINT(0, ut_attribute_calls);
}

static const UTTestCase ut_cases[] = {
   {"explicit states", displays_explicit_page_states},
   {"ordinary hours", applies_a_nonwrapping_page_window},
   {"midnight hours", applies_a_midnight_wrapping_page_window},
   {"unknown mode", ignores_an_unknown_page_mode}
};
