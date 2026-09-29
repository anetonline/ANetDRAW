#define UT_CUSTOM_MOCK_ODEditBufferGetLineLength
#define UT_CUSTOM_MOCK_ODSizeAdd
#define UT_CUSTOM_MOCK_ODEditTryToGrow
#define UT_CUSTOM_MOCK_ODEditBufferGetCharacter
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_memmove
#define UT_CUSTOM_MOCK_strcmp

static UINT ut_line_length;
static size_t ut_add_left[2];
static size_t ut_add_right[2];
static size_t ut_add_value[2];
static BOOL ut_add_result[2];
static unsigned ut_add_calls;
static tODResult ut_grow_result;
static UINT ut_grow_size;
static unsigned ut_grow_calls;
static unsigned ut_character_calls;
static UINT ut_character_line;
static UINT ut_character_column;

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

int utm_strcmp(const char *left, const char *right)
{
   while(*left != '\0' && *left == *right)
   {
      ++left;
      ++right;
   }
   return((unsigned char)*left - (unsigned char)*right);
}

UINT utm_ODEditBufferGetLineLength(tEditInstance *instance, UINT line)
{
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT(line < instance->unLinesInBuffer);
   return(ut_line_length);
}

int utm_ODSizeAdd(size_t left, size_t right, size_t *result)
{
   unsigned call;
   call = ut_add_calls++;
   UT_ASSERT(call < 2);
   UT_ASSERT(left == ut_add_left[call]);
   UT_ASSERT(right == ut_add_right[call]);
   UT_ASSERT_NOT_NULL(result);
   *result = ut_add_value[call];
   return(ut_add_result[call]);
}

tODResult utm_ODEditTryToGrow(tEditInstance *instance, UINT size)
{
   ++ut_grow_calls;
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT_EQ_UINT(ut_grow_size, size);
   if(ut_grow_result == kODRCSuccess)
      instance->unBufferSize = size;
   return(ut_grow_result);
}

char *utm_ODEditBufferGetCharacter(tEditInstance *instance, UINT line,
   UINT column)
{
   ++ut_character_calls;
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT_EQ_UINT(ut_character_line, line);
   UT_ASSERT_EQ_UINT(ut_character_column, column);
   return(instance->pszEditBuffer + column);
}

static void reset_space(tEditInstance *instance, char *buffer, UINT size,
   UINT line_length)
{
   instance->pszEditBuffer = buffer;
   instance->unBufferSize = size;
   instance->unLinesInBuffer = 1;
   ut_line_length = line_length;
   ut_add_calls = 0;
   ut_add_result[0] = TRUE;
   ut_add_result[1] = TRUE;
   ut_add_value[0] = 0;
   ut_add_value[1] = 0;
   ut_grow_result = kODRCSuccess;
   ut_grow_size = 0;
   ut_grow_calls = 0;
   ut_character_calls = 0;
   ut_character_line = 0;
   ut_character_column = 0;
   od_control.od_error = 0;
}

static void extends_a_short_line_and_moves_the_tail(void)
{
   char buffer[32] = "abcXYZ";
   tEditInstance instance;

   reset_space(&instance, buffer, sizeof(buffer), 3);
   ut_add_left[0] = 2;
   ut_add_right[0] = 2;
   ut_add_value[0] = 4;
   ut_character_column = 3;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODEditBufferMakeSpace(&instance, 0, 5, 2));
   UT_ASSERT_EQ_UINT(1, ut_add_calls);
   UT_ASSERT_EQ_UINT(0, ut_grow_calls);
   UT_ASSERT_EQ_UINT(1, ut_character_calls);
   UT_ASSERT(strcmp(buffer, "abc    XYZ") == 0);
}

static void rejects_extension_arithmetic_failures(void)
{
   char buffer[8] = "abc";
   tEditInstance instance;

   reset_space(&instance, buffer, sizeof(buffer), 3);
   ut_add_left[0] = 2;
   ut_add_right[0] = 2;
   ut_add_result[0] = FALSE;
   UT_ASSERT_EQ_INT(kODRCSafeFailure,
      utt_ODEditBufferMakeSpace(&instance, 0, 5, 2));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_character_calls);

#if !defined(ODPLAT_DOS) && !defined(ODPLAT_DOS32)
   if(sizeof(size_t) > sizeof(UINT))
   {
      reset_space(&instance, buffer, sizeof(buffer), 3);
      ut_add_left[0] = 2;
      ut_add_right[0] = 2;
      ut_add_result[0] = TRUE;
      ut_add_value[0] = (size_t)(UINT)-1;
      ++ut_add_value[0];
      UT_ASSERT_EQ_INT(kODRCSafeFailure,
         utt_ODEditBufferMakeSpace(&instance, 0, 5, 2));
      UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);
      UT_ASSERT_EQ_UINT(0, ut_character_calls);
   }
#endif
}

static void grows_when_the_existing_buffer_is_too_small(void)
{
   char buffer[16] = "abc";
   tEditInstance instance;

   reset_space(&instance, buffer, 5, 3);
   ut_add_left[0] = 4;
   ut_add_right[0] = 3;
   ut_add_value[0] = 7;
   ut_grow_size = 7;
   ut_character_column = 1;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODEditBufferMakeSpace(&instance, 0, 1, 3));
   UT_ASSERT_EQ_UINT(1, ut_add_calls);
   UT_ASSERT_EQ_UINT(1, ut_grow_calls);
   UT_ASSERT_EQ_UINT(1, ut_character_calls);
   UT_ASSERT(strcmp(buffer, "a   bc") == 0);
}

static void returns_growth_failures_before_changing_the_buffer(void)
{
   char buffer[16] = "abc";
   tEditInstance instance;

   reset_space(&instance, buffer, 5, 3);
   ut_add_left[0] = 4;
   ut_add_right[0] = 3;
   ut_add_value[0] = 7;
   ut_grow_size = 7;
   ut_grow_result = kODRCUnrecoverableFailure;
   UT_ASSERT_EQ_INT(kODRCUnrecoverableFailure,
      utt_ODEditBufferMakeSpace(&instance, 0, 1, 3));
   UT_ASSERT_EQ_UINT(1, ut_grow_calls);
   UT_ASSERT_EQ_UINT(0, ut_character_calls);
   UT_ASSERT(strcmp(buffer, "abc") == 0);
}

static void rejects_required_size_arithmetic_failures(void)
{
   char buffer[16] = "abc";
   tEditInstance instance;

   reset_space(&instance, buffer, 5, 3);
   ut_add_left[0] = 4;
   ut_add_right[0] = 3;
   ut_add_result[0] = FALSE;
   UT_ASSERT_EQ_INT(kODRCSafeFailure,
      utt_ODEditBufferMakeSpace(&instance, 0, 1, 3));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_grow_calls);

#if !defined(ODPLAT_DOS) && !defined(ODPLAT_DOS32)
   if(sizeof(size_t) > sizeof(UINT))
   {
      reset_space(&instance, buffer, 5, 3);
      ut_add_left[0] = 4;
      ut_add_right[0] = 3;
      ut_add_result[0] = TRUE;
      ut_add_value[0] = (size_t)(UINT)-1;
      ++ut_add_value[0];
      UT_ASSERT_EQ_INT(kODRCSafeFailure,
         utt_ODEditBufferMakeSpace(&instance, 0, 1, 3));
      UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);
      UT_ASSERT_EQ_UINT(0, ut_grow_calls);
   }
#endif
}

static void accepts_a_zero_length_insertion(void)
{
   char buffer[8] = "abc";
   tEditInstance instance;

   reset_space(&instance, buffer, sizeof(buffer), 3);
   ut_character_column = 1;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODEditBufferMakeSpace(&instance, 0, 1, 0));
   UT_ASSERT_EQ_UINT(0, ut_add_calls);
   UT_ASSERT_EQ_UINT(0, ut_grow_calls);
   UT_ASSERT_EQ_UINT(1, ut_character_calls);
   UT_ASSERT(strcmp(buffer, "abc") == 0);
}

static const UTTestCase ut_cases[] = {
   {"line extension", extends_a_short_line_and_moves_the_tail},
   {"extension limits", rejects_extension_arithmetic_failures},
   {"buffer growth", grows_when_the_existing_buffer_is_too_small},
   {"growth failure", returns_growth_failures_before_changing_the_buffer},
   {"required size limits", rejects_required_size_arithmetic_failures},
   {"zero insertion", accepts_a_zero_length_insertion}
};
