#ifdef OPENDOORS_HAVE_VSNPRINTF
#define UT_CUSTOM_MOCK_vsnprintf
#else
#define UT_CUSTOM_MOCK_ODFallbackVsnprintf
#endif

#include <stdarg.h>
#include <string.h>

static unsigned ut_backend_calls;
static char *ut_seen_buffer;
static size_t ut_seen_size;

static int check_arguments_and_format(char *buffer, size_t size,
   const char *format, va_list args)
{
   const char *text;
   int number;
   UT_ASSERT_EQ_INT(0, strcmp("%s:%d", format));
   text = va_arg(args, const char *);
   number = va_arg(args, int);
   UT_ASSERT_EQ_INT(0, strcmp("value", text));
   UT_ASSERT_EQ_INT(42, number);
   ut_seen_buffer = buffer;
   ut_seen_size = size;
   if(size >= 3)
   {
      buffer[0] = 'o';
      buffer[1] = 'k';
      buffer[2] = '\0';
   }
   ++ut_backend_calls;
   return 8;
}

#ifdef OPENDOORS_HAVE_VSNPRINTF
int utm_vsnprintf(char *buffer, size_t size, const char *format,
   va_list args)
{
   return check_arguments_and_format(buffer, size, format, args);
}
#else
int utm_ODFallbackVsnprintf(char *buffer, size_t size, const char *format,
   va_list args)
{
   return check_arguments_and_format(buffer, size, format, args);
}
#endif

static int call_target(char *buffer, size_t size, const char *format, ...)
{
   int result;
   va_list args;
   va_start(args, format);
   result = utt_ODVsnprintf(buffer, size, format, args);
   va_end(args);
   return result;
}

static void forwards_the_complete_variadic_call(void)
{
   char buffer[16];
   ut_backend_calls = 0;
   ut_seen_buffer = NULL;
   ut_seen_size = 0;
   memset(buffer, 0, sizeof(buffer));
   UT_ASSERT_EQ_INT(8,
      call_target(buffer, sizeof(buffer), "%s:%d", "value", 42));
   UT_ASSERT_EQ_UINT(1, ut_backend_calls);
   UT_ASSERT_EQ_PTR(buffer, ut_seen_buffer);
   UT_ASSERT_EQ_UINT(sizeof(buffer), ut_seen_size);
   UT_ASSERT_EQ_INT(0, strcmp("ok", buffer));
}

static const UTTestCase ut_cases[] = {
   {"variadic forwarding", forwards_the_complete_variadic_call}
};
