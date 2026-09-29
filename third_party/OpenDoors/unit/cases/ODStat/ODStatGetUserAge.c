#define UT_CUSTOM_MOCK_atoi
#define UT_CUSTOM_MOCK_localtime
#define UT_CUSTOM_MOCK_sprintf
#define UT_CUSTOM_MOCK_strcpy
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_time

#include <stdarg.h>

static struct tm ut_now;

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

int utm_atoi(const char *text)
{
   int value = 0;
   while(*text >= '0' && *text <= '9')
   {
      value = value * 10 + (*text - '0');
      ++text;
   }
   return value;
}

time_t utm_time(time_t *result)
{
   if(result != NULL) *result = (time_t)1;
   return (time_t)1;
}

struct tm *utm_localtime(const time_t *value)
{
   UT_ASSERT(value != NULL);
   UT_ASSERT_EQ_INT(1, *value);
   return &ut_now;
}

int utm_sprintf(char *output, const char *format, ...)
{
   va_list arguments;
   int value;
   char reversed[8];
   int count = 0;
   int position = 0;
   UT_ASSERT_EQ_INT(0, strcmp("%d", format));
   va_start(arguments, format);
   value = va_arg(arguments, int);
   va_end(arguments);
   if(value < 0)
   {
      output[position++] = '-';
      value = -value;
   }
   do
   {
      reversed[count++] = (char)('0' + value % 10);
      value /= 10;
   } while(value != 0);
   while(count != 0) output[position++] = reversed[--count];
   output[position] = '\0';
   return position;
}

char *utm_strcpy(char *output, const char *input)
{
   char *result = output;
   while((*output++ = *input++) != '\0') { }
   return result;
}

static void set_date(int year, int month, int day)
{
   memset(&ut_now, 0, sizeof(ut_now));
   ut_now.tm_year = year - 1900;
   ut_now.tm_mon = month - 1;
   ut_now.tm_mday = day;
}

static void expect_age(INT info_type, const char *birthday,
   int year, int month, int day, const char *expected)
{
   char output[16] = "untouched";
   od_control.od_info_type = info_type;
   utm_strcpy(od_control.user_birthday, birthday);
   set_date(year, month, day);
   utt_ODStatGetUserAge(output);
   UT_ASSERT_EQ_INT(0, strcmp(expected, output));
}

static void accepts_supported_drop_file_types(void)
{
   expect_age(RA2EXITINFO, "01-01-80", 2025, 1, 1, "45");
   expect_age(DOORSYS_WILDCAT, "01-01-80", 2025, 1, 1, "45");
   expect_age(NO_DOOR_FILE, "01-01-80", 2025, 1, 1, "?");
}

static void rejects_invalid_length_and_month(void)
{
   expect_age(RA2EXITINFO, "1-01-80", 2025, 1, 1, "?");
   expect_age(RA2EXITINFO, "13-01-80", 2025, 1, 1, "?");
}

static void rejects_each_invalid_year_digit(void)
{
   expect_age(RA2EXITINFO, "01-01-/0", 2025, 1, 1, "?");
   expect_age(RA2EXITINFO, "01-01-:0", 2025, 1, 1, "?");
   expect_age(RA2EXITINFO, "01-01-8/", 2025, 1, 1, "?");
   expect_age(RA2EXITINFO, "01-01-8:", 2025, 1, 1, "?");
}

static void rejects_each_invalid_day_digit(void)
{
   expect_age(RA2EXITINFO, "01-/1-80", 2025, 1, 1, "?");
   expect_age(RA2EXITINFO, "01-41-80", 2025, 1, 1, "?");
   expect_age(RA2EXITINFO, "01-0/-80", 2025, 1, 1, "?");
   expect_age(RA2EXITINFO, "01-0:-80", 2025, 1, 1, "?");
}

static void applies_century_and_birthday_adjustments(void)
{
   expect_age(RA2EXITINFO, "12-31-80", 2025, 6, 1, "44");
   expect_age(RA2EXITINFO, "01-31-80", 2025, 1, 30, "44");
   expect_age(RA2EXITINFO, "01-31-80", 2025, 1, 31, "45");
   expect_age(RA2EXITINFO, "01-01-80", 2025, 2, 1, "45");
   expect_age(RA2EXITINFO, "01-01-30", 2025, 2, 1, "95");
   expect_age(RA2EXITINFO, "01-01-20", 2025, 2, 1, "5");
}

static const UTTestCase ut_cases[] = {
   {"supported information types", accepts_supported_drop_file_types},
   {"invalid length and month", rejects_invalid_length_and_month},
   {"invalid year digits", rejects_each_invalid_year_digit},
   {"invalid day digits", rejects_each_invalid_day_digit},
   {"age date arithmetic", applies_century_and_birthday_adjustments}
};
