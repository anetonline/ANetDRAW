#define UT_CUSTOM_MOCK_ODEditRememberArea
#define UT_CUSTOM_MOCK_ODEditBufferGetCharacter
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_memmove
#define UT_CUSTOM_MOCK_ODEditBufferFormatAndIndex
#define UT_CUSTOM_MOCK_ODEditRedrawChanged

static unsigned ut_remember_calls;
static unsigned ut_format_calls;
static unsigned ut_redraw_calls;

void utm_ODEditRememberArea(tEditInstance *instance, void *area)
{
   ++ut_remember_calls;
   UT_ASSERT_EQ_PTR(instance->pRememberBuffer, area);
}

char *utm_ODEditBufferGetCharacter(tEditInstance *instance, UINT line,
   UINT column)
{
   UT_ASSERT_EQ_UINT(instance->unCurrentLine, line);
   UT_ASSERT_EQ_UINT(instance->unCurrentColumn, column);
   return(instance->pszEditBuffer + column);
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

static void removes_the_character_and_refreshes_editor_state(void)
{
   char buffer[8] = "abcd";
   char remembered;
   tEditInstance instance;

   instance.pszEditBuffer = buffer;
   instance.unCurrentLine = 0;
   instance.unCurrentColumn = 1;
   instance.pRememberBuffer = &remembered;
   ut_remember_calls = 0;
   ut_format_calls = 0;
   ut_redraw_calls = 0;
   utt_ODEditDeleteCurrentChar(&instance);
   UT_ASSERT(strcmp(buffer, "acd") == 0);
   UT_ASSERT_EQ_UINT(1, instance.unCurrentColumn);
   UT_ASSERT_EQ_UINT(1, ut_remember_calls);
   UT_ASSERT_EQ_UINT(1, ut_format_calls);
   UT_ASSERT_EQ_UINT(1, ut_redraw_calls);
}

static const UTTestCase ut_cases[] = {
   {"delete character", removes_the_character_and_refreshes_editor_state}
};
