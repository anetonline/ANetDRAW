#define UT_CUSTOM_MOCK_ODEditRememberArea
#define UT_CUSTOM_MOCK_ODEditBufferGetCharacter
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_memmove
#define UT_CUSTOM_MOCK_ODEditBufferFormatAndIndex
#define UT_CUSTOM_MOCK_ODEditRedrawChanged

static char *ut_line_starts[3];
static unsigned ut_character_calls;
static unsigned ut_format_calls;
static unsigned ut_redraw_calls;

void utm_ODEditRememberArea(tEditInstance *instance, void *area)
{
   UT_ASSERT_EQ_PTR(instance->pRememberBuffer, area);
}

char *utm_ODEditBufferGetCharacter(tEditInstance *instance, UINT line,
   UINT column)
{
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT(line < 3);
   UT_ASSERT_EQ_UINT(0, column);
   ++ut_character_calls;
   return(ut_line_starts[line]);
}

size_t utm_strlen(const char *text)
{
   size_t length;
   length = 0;
   while(text[length] != '\0')
      ++length;
   return(length);
}

void *utm_memmove(void *destination, const void *source, size_t count)
{
   char *dest;
   const char *src;
   size_t index;
   dest = (char *)destination;
   src = (const char *)source;
   if(dest > src && dest < src + count)
   {
      index = count;
      while(index-- > 0)
         dest[index] = src[index];
   }
   else
   {
      for(index = 0; index < count; ++index)
         dest[index] = src[index];
   }
   return(destination);
}

BOOL utm_ODEditBufferFormatAndIndex(tEditInstance *instance)
{
   ++ut_format_calls;
   UT_ASSERT_NOT_NULL(instance);
   return(TRUE);
}

void utm_ODEditRedrawChanged(tEditInstance *instance, void *area,
   UINT upper, UINT lower)
{
   ++ut_redraw_calls;
   UT_ASSERT_EQ_PTR(instance->pRememberBuffer, area);
   UT_ASSERT_EQ_UINT(REDRAW_NO_BOUNDARY, upper);
   UT_ASSERT_EQ_UINT(REDRAW_NO_BOUNDARY, lower);
}

static void reset_delete(tEditInstance *instance, char *buffer,
   UINT current, UINT lines, void *remembered)
{
   instance->pszEditBuffer = buffer;
   instance->unCurrentLine = current;
   instance->unCurrentColumn = 4;
   instance->unLinesInBuffer = lines;
   instance->pRememberBuffer = remembered;
   ut_character_calls = 0;
   ut_format_calls = 0;
   ut_redraw_calls = 0;
}

static void empties_the_final_line_without_removing_it(void)
{
   char buffer[16] = "zero\rone";
   char remembered;
   tEditInstance instance;

   ut_line_starts[0] = buffer;
   ut_line_starts[1] = buffer + 5;
   reset_delete(&instance, buffer, 1, 2, &remembered);
   utt_ODEditDeleteCurrentLine(&instance);
   UT_ASSERT(buffer[5] == '\0');
   UT_ASSERT(strcmp(buffer, "zero\r") == 0);
   UT_ASSERT_EQ_UINT(0, instance.unCurrentColumn);
   UT_ASSERT_EQ_UINT(1, ut_character_calls);
   UT_ASSERT_EQ_UINT(1, ut_format_calls);
   UT_ASSERT_EQ_UINT(1, ut_redraw_calls);
}

static void removes_a_nonfinal_line_from_the_buffer(void)
{
   char buffer[24] = "zero\rone\rtwo";
   char remembered;
   tEditInstance instance;

   ut_line_starts[0] = buffer;
   ut_line_starts[1] = buffer + 5;
   ut_line_starts[2] = buffer + 9;
   reset_delete(&instance, buffer, 1, 3, &remembered);
   utt_ODEditDeleteCurrentLine(&instance);
   UT_ASSERT(strcmp(buffer, "zero\rtwo") == 0);
   UT_ASSERT_EQ_UINT(0, instance.unCurrentColumn);
   UT_ASSERT_EQ_UINT(2, ut_character_calls);
   UT_ASSERT_EQ_UINT(1, ut_format_calls);
   UT_ASSERT_EQ_UINT(1, ut_redraw_calls);
}

static const UTTestCase ut_cases[] = {
   {"final line", empties_the_final_line_without_removing_it},
   {"nonfinal line", removes_a_nonfinal_line_from_the_buffer}
};
