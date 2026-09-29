#include <setjmp.h>
#include <stdarg.h>

#define UT_CUSTOM_MOCK_ODGetCommandLineParameter
#define UT_CUSTOM_MOCK_exit
#define UT_CUSTOM_MOCK_printf

static jmp_buf ut_exit_target;

tCommandLineParameter utm_ODGetCommandLineParameter(char *argument)
{
   if(argument[0] == '-' || argument[0] == '/')
      return kParamUnknown;
   return kParamOption;
}

void utm_exit(int status)
{
   UT_ASSERT_EQ_INT(1, status);
   longjmp(ut_exit_target, 1);
}

int utm_printf(const char *format, ...)
{
   va_list arguments;
   const char *option;
   va_start(arguments, format);
   option = va_arg(arguments, const char *);
   va_end(arguments);
   UT_ASSERT_NOT_NULL(strstr(format, "Missing parameter"));
   UT_ASSERT_EQ_INT(0, strcmp("door", option));
   return 0;
}

static void joins_words_and_stops_before_next_option(void)
{
   char *arguments[] = {"door", "-NAME", "Ada", "Lovelace", "-P", "1"};
   char result[40];
   INT current = 1;
   utt_ODGetNextArgName(&current, 6, arguments, result, sizeof(result));
   UT_ASSERT_EQ_INT(0, strcmp("Ada Lovelace", result));
   UT_ASSERT_EQ_INT(3, current);
}

static void truncates_to_destination_and_accepts_final_word(void)
{
   char *arguments[] = {"door", "-NAME", "Long", "Name", "More"};
   char result[6];
   INT current = 1;
   utt_ODGetNextArgName(&current, 5, arguments, result, sizeof(result));
   UT_ASSERT_EQ_INT(0, strcmp("Long ", result));
   UT_ASSERT_EQ_INT(5, current);
}

static void reports_missing_value(void)
{
   char *arguments[] = {"door", "-NAME"};
   char result[8] = "keep";
   INT current = 1;
   if(setjmp(ut_exit_target) == 0)
      utt_ODGetNextArgName(&current, 2, arguments, result, sizeof(result));
   UT_ASSERT_EQ_INT(0, strcmp("keep", result));
}

static const UTTestCase ut_cases[] = {
   {"join name words", joins_words_and_stops_before_next_option},
   {"truncate name", truncates_to_destination_and_accepts_final_word},
   {"missing name", reports_missing_value}
};
