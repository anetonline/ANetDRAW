#define UT_CUSTOM_MOCK_ODVsnprintf

#include <stdarg.h>
#include <string.h>

static int ut_format_result;
static unsigned ut_format_calls;

int utm_ODVsnprintf(char *buffer, size_t size, const char *format,
   va_list arguments)
{
   UT_ASSERT_EQ_PTR(szODWorkString, buffer);
   UT_ASSERT_EQ_UINT(sizeof(szODWorkString), size);
   UT_ASSERT_EQ_INT(0, strcmp("value %d", format));
   UT_ASSERT_EQ_INT(42, va_arg(arguments, int));
   strcpy(buffer, "formatted");
   ++ut_format_calls;
   return ut_format_result;
}

static void reset_format(void)
{
   memset(&od_control, 0, sizeof(od_control));
   strcpy(szODWorkString, "old");
   ut_format_result = 9;
   ut_format_calls = 0;
}

static void accepts_fitting_output(void)
{
   reset_format();
   UT_ASSERT(utt_ODLogFormatWorkString("value %d", 42));
   UT_ASSERT_EQ_UINT(1, ut_format_calls);
   UT_ASSERT_EQ_INT(0, strcmp("formatted", szODWorkString));
}

static void rejects_formatter_failure(void)
{
   reset_format();
   ut_format_result = -1;
   UT_ASSERT(!utt_ODLogFormatWorkString("value %d", 42));
   UT_ASSERT_EQ_INT('\0', szODWorkString[0]);
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);
}

static void rejects_truncated_output_at_and_beyond_capacity(void)
{
   reset_format();
   ut_format_result = (int)sizeof(szODWorkString);
   UT_ASSERT(!utt_ODLogFormatWorkString("value %d", 42));
   reset_format();
   ut_format_result = (int)sizeof(szODWorkString) + 1;
   UT_ASSERT(!utt_ODLogFormatWorkString("value %d", 42));
}

static const UTTestCase ut_cases[] = {
   {"fitting output", accepts_fitting_output},
   {"formatter failure", rejects_formatter_failure},
   {"truncation", rejects_truncated_output_at_and_beyond_capacity}
};
