#define UT_CUSTOM_MOCK_sprintf
#define UT_CUSTOM_MOCK_strcat

#include <stdarg.h>

static int ut_write_parameter(char *output, char prefix, int value)
{
   int index = 0;
   output[index++] = prefix;
   if(prefix == 'x') output[index++] = '[';
   if(value >= 10) output[index++] = (char)('0' + value / 10);
   output[index++] = (char)('0' + value % 10);
   output[index] = '\0';
   return index;
}

int utm_sprintf(char *output, const char *format, ...)
{
   int value;
   va_list arguments;
   va_start(arguments, format);
   value = va_arg(arguments, int);
   va_end(arguments);
   UT_ASSERT(value >= 0 && value < 100);
   if(format[0] == ';')
   {
      UT_ASSERT(format[1] == '%' && format[2] == 'd');
      return ut_write_parameter(output, ';', value);
   }
   UT_ASSERT(format[0] == 'x' && format[1] == '[');
   UT_ASSERT(format[2] == '%' && format[3] == 'd');
   return ut_write_parameter(output, 'x', value);
}

char *utm_strcat(char *destination, const char *source)
{
   char *result = destination;
   while(*destination != '\0') ++destination;
   while((*destination++ = *source++) != '\0') { }
   return result;
}

static void starts_the_first_parameter_with_escape_and_csi(void)
{
   char sequence[16];
   bAnyColorChangeYet = FALSE;
   sequence[0] = '\0';
   utt_ODAddANSIParameter(sequence, 31);
   UT_ASSERT_EQ_INT(TRUE, bAnyColorChangeYet);
   UT_ASSERT_EQ_UINT(27, (unsigned char)sequence[0]);
   UT_ASSERT(sequence[1] == '[' && sequence[2] == '3'
      && sequence[3] == '1' && sequence[4] == '\0');
}

static void appends_later_parameters_with_a_semicolon(void)
{
   char sequence[16] = {27, '[', '3', '1', '\0'};
   bAnyColorChangeYet = TRUE;
   utt_ODAddANSIParameter(sequence, 44);
   UT_ASSERT_EQ_INT(TRUE, bAnyColorChangeYet);
   UT_ASSERT(sequence[4] == ';' && sequence[5] == '4'
      && sequence[6] == '4' && sequence[7] == '\0');
}

static const UTTestCase ut_cases[] = {
   {"first ANSI parameter", starts_the_first_parameter_with_escape_and_csi},
   {"later ANSI parameter", appends_later_parameters_with_a_semicolon}
};
