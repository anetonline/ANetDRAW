#ifndef OD_UNIT_TEXT_DROP_FILE_FIXTURE_H
#define OD_UNIT_TEXT_DROP_FILE_FIXTURE_H

#include <stdarg.h>

#define UT_TEXT_MAX_LINES 64
#define UT_TEXT_LINE_SIZE 96

static int ut_text_file_token;
static FILE *ut_text_file = (FILE *)&ut_text_file_token;
static char ut_text_lines[UT_TEXT_MAX_LINES][UT_TEXT_LINE_SIZE];
static unsigned ut_text_line_count;
static unsigned ut_text_read_count;
static unsigned ut_text_open_count;
static unsigned ut_text_close_count;
static BOOL ut_text_open_fails;
static const char *ut_text_expected_path;
static const char *ut_text_expected_mode;

static void ut_text_reset(void)
{
   memset(ut_text_lines, 0, sizeof(ut_text_lines));
   ut_text_line_count = 0;
   ut_text_read_count = 0;
   ut_text_open_count = 0;
   ut_text_close_count = 0;
   ut_text_open_fails = FALSE;
   ut_text_expected_path = NULL;
   ut_text_expected_mode = NULL;
}

static void ut_text_set_line(unsigned line, const char *text)
{
   unsigned index = 0;
   UT_ASSERT(line < UT_TEXT_MAX_LINES);
   while(index + 1 < UT_TEXT_LINE_SIZE && text[index] != '\0')
   {
      ut_text_lines[line][index] = text[index];
      ++index;
   }
   ut_text_lines[line][index] = '\0';
   if(ut_text_line_count <= line)
      ut_text_line_count = line + 1;
}

FILE *utm_fopen(const char *path, const char *mode)
{
   ++ut_text_open_count;
   if(ut_text_expected_path != NULL)
      UT_ASSERT(strcmp(ut_text_expected_path, path) == 0);
   if(ut_text_expected_mode != NULL)
      UT_ASSERT(strcmp(ut_text_expected_mode, mode) == 0);
   return ut_text_open_fails ? NULL : ut_text_file;
}

char *utm_fgets(char *destination, int size, FILE *stream)
{
   const char *source;
   int index = 0;
   UT_ASSERT_EQ_PTR(ut_text_file, stream);
   if(ut_text_read_count >= ut_text_line_count)
   {
      ++ut_text_read_count;
      return NULL;
   }
   source = ut_text_lines[ut_text_read_count++];
   while(index < size - 1 && source[index] != '\0')
   {
      destination[index] = source[index];
      ++index;
   }
   destination[index] = '\0';
   return destination;
}

int utm_fclose(FILE *stream)
{
   UT_ASSERT_EQ_PTR(ut_text_file, stream);
   ++ut_text_close_count;
   return 0;
}

int utm_atoi(const char *text)
{
   return (int)utm_atol(text);
}

long utm_atol(const char *text)
{
   long value = 0;
   int sign = 1;
   if(*text == '-')
   {
      sign = -1;
      ++text;
   }
   while(*text >= '0' && *text <= '9')
   {
      value = value * 10 + (*text - '0');
      ++text;
   }
   return value * sign;
}

char *utm_strcpy(char *destination, const char *source)
{
   char *result = destination;
   while((*destination++ = *source++) != '\0')
      ;
   return result;
}

char *utm_od_strupr(char *text)
{
   char *current = text;
   while(*current != '\0')
   {
      if(*current >= 'a' && *current <= 'z')
         *current = (char)(*current - 'a' + 'A');
      ++current;
   }
   return text;
}

char *utm_strupr(char *text)
{
   return utm_od_strupr(text);
}

void utm_ODStringToName(char *text)
{
   unsigned length = 0;
   while(text[length] != '\0')
      ++length;
   while(length != 0 &&
      (text[length - 1] == '\r' || text[length - 1] == '\n' ||
       text[length - 1] == ' '))
      --length;
   text[length] = '\0';
}

int utm_sprintf(char *destination, const char *format, ...)
{
   va_list arguments;
   unsigned hour;
   unsigned minute;
   UT_ASSERT(strcmp("%02u:%02u", format) == 0);
   va_start(arguments, format);
   hour = va_arg(arguments, unsigned);
   minute = va_arg(arguments, unsigned);
   va_end(arguments);
   destination[0] = (char)('0' + (hour / 10) % 10);
   destination[1] = (char)('0' + hour % 10);
   destination[2] = ':';
   destination[3] = (char)('0' + (minute / 10) % 10);
   destination[4] = (char)('0' + minute % 10);
   destination[5] = '\0';
   return 5;
}

#endif
