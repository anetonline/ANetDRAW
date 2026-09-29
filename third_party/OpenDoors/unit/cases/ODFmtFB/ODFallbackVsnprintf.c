#define UT_CUSTOM_MOCK_trio_vsnprintf

#include <stdarg.h>
#include <string.h>

static unsigned ut_trio_calls;

int utm_trio_vsnprintf(char *buffer, size_t size, const char *format,
   va_list args)
{
   const char *text;
   UT_ASSERT_EQ_INT(0, strcmp("[%s]", format));
   text = va_arg(args, const char *);
   UT_ASSERT_EQ_INT(0, strcmp("trio", text));
   UT_ASSERT_EQ_UINT(8, size);
   buffer[0] = 'T';
   buffer[1] = '\0';
   ++ut_trio_calls;
   return 6;
}

static int call_target(char *buffer, size_t size, const char *format, ...)
{
   int result;
   va_list args;
   va_start(args, format);
   result = utt_ODFallbackVsnprintf(buffer, size, format, args);
   va_end(args);
   return result;
}

static void delegates_to_embedded_trio(void)
{
   char buffer[8];
   ut_trio_calls = 0;
   memset(buffer, 0, sizeof(buffer));
   UT_ASSERT_EQ_INT(6,
      call_target(buffer, sizeof(buffer), "[%s]", "trio"));
   UT_ASSERT_EQ_UINT(1, ut_trio_calls);
   UT_ASSERT_EQ_INT('T', buffer[0]);
}

static const UTTestCase ut_cases[] = {
   {"trio delegation", delegates_to_embedded_trio}
};
