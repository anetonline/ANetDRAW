#include <setjmp.h>
#include <stdarg.h>

#define UT_CUSTOM_MOCK_exit
#define UT_CUSTOM_MOCK_printf

#define MOCK_EXIT 310
#define MOCK_PRINTF 311

static jmp_buf ut_exit_target;
static int ut_exit_status;

void utm_exit(int status)
{
   ut_exit_status = status;
   ut_mock_called(MOCK_EXIT);
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
   UT_ASSERT_EQ_INT(0, strcmp("-P", option));
   ut_mock_called(MOCK_PRINTF);
   return 0;
}

static void advances_when_an_argument_remains(void)
{
   INT current = 1;
   char option[] = "-P";
   utt_ODAdvanceToNextArg(&current, 3, option);
   UT_ASSERT_EQ_INT(2, current);
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_EXIT));
}

static void reports_and_exits_when_argument_is_missing(void)
{
   INT current = 1;
   char option[] = "-P";
   ut_exit_status = -1;
   if(setjmp(ut_exit_target) == 0)
      utt_ODAdvanceToNextArg(&current, 2, option);
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_PRINTF));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_EXIT));
   UT_ASSERT_EQ_INT(1, ut_exit_status);
}

static const UTTestCase ut_cases[] = {
   {"argument remains", advances_when_an_argument_remains},
   {"argument missing", reports_and_exits_when_argument_is_missing}
};
