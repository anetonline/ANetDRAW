#define UT_CUSTOM_MOCK_ODScrnPrintf
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos
#define UT_CUSTOM_MOCK_localtime
#define UT_CUSTOM_MOCK_time

#include <stdarg.h>

static struct tm ut_time_record;
static unsigned ut_cursor_calls;
static unsigned ut_printf_calls;

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
   UT_ASSERT_EQ_UINT(74, column);
   UT_ASSERT_EQ_UINT(24, row);
}

INT ODVCALL utm_ODScrnPrintf(char *format, ...)
{
   va_list arguments;
   int hour;
   int minute;
   ++ut_printf_calls;
   UT_ASSERT(strcmp("%02.2d:%02.2d", format) == 0);
   va_start(arguments, format);
   hour = va_arg(arguments, int);
   minute = va_arg(arguments, int);
   va_end(arguments);
   UT_ASSERT_EQ_INT(7, hour);
   UT_ASSERT_EQ_INT(5, minute);
   return 5;
}

static void displays_the_current_local_time(void)
{
   memset(&ut_time_record, 0, sizeof(ut_time_record));
   ut_time_record.tm_hour = 7;
   ut_time_record.tm_min = 5;
   ut_cursor_calls = 0;
   ut_printf_calls = 0;
   utt_ODRADisplayTime();
   UT_ASSERT_EQ_UINT(1, ut_cursor_calls);
   UT_ASSERT_EQ_UINT(1, ut_printf_calls);
}

static const UTTestCase ut_cases[] = {
   {"local time", displays_the_current_local_time}
};
