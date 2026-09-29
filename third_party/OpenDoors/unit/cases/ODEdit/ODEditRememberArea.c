#define UT_CUSTOM_MOCK_ODEditBufferGetLineLength
#define UT_CUSTOM_MOCK_ODEditBufferGetCharacter
#define UT_CUSTOM_MOCK_memcpy

static char ut_lines[2][8];
static UINT ut_lengths[2];
static unsigned ut_length_calls;
static unsigned ut_character_calls;

UINT utm_ODEditBufferGetLineLength(tEditInstance *instance, UINT line)
{
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT(line < 2);
   ++ut_length_calls;
   return(ut_lengths[line]);
}

char *utm_ODEditBufferGetCharacter(tEditInstance *instance, UINT line,
   UINT column)
{
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT(line < 2);
   UT_ASSERT_EQ_UINT(0, column);
   ++ut_character_calls;
   return(ut_lines[line]);
}

void *utm_memcpy(void *destination, const void *source, size_t count)
{
   char *dest;
   const char *src;
   size_t index;
   dest = (char *)destination;
   src = (const char *)source;
   for(index = 0; index < count; ++index)
      dest[index] = src[index];
   return(destination);
}

static void remembers_visible_and_missing_lines(void)
{
   tEditInstance instance;
   char remembered[18];
   unsigned index;

   for(index = 0; index < sizeof(remembered); ++index)
      remembered[index] = '?';
   strcpy(ut_lines[0], "zero");
   strcpy(ut_lines[1], "one");
   ut_lengths[0] = 4;
   ut_lengths[1] = 3;
   instance.unAreaWidth = 5;
   instance.unAreaHeight = 3;
   instance.unLineScrolledToTop = 1;
   instance.unLinesInBuffer = 2;
   ut_length_calls = 0;
   ut_character_calls = 0;
   utt_ODEditRememberArea(&instance, remembered);
   UT_ASSERT_EQ_UINT(1, ut_length_calls);
   UT_ASSERT_EQ_UINT(1, ut_character_calls);
   UT_ASSERT(strcmp(remembered, "one") == 0);
   UT_ASSERT(remembered[6] == '\0');
   UT_ASSERT(remembered[12] == '\0');
}

static void accepts_an_empty_display_area(void)
{
   tEditInstance instance;
   char remembered = 'x';

   instance.unAreaWidth = 5;
   instance.unAreaHeight = 0;
   instance.unLineScrolledToTop = 0;
   instance.unLinesInBuffer = 0;
   ut_length_calls = 0;
   ut_character_calls = 0;
   utt_ODEditRememberArea(&instance, &remembered);
   UT_ASSERT_EQ_UINT(0, ut_length_calls);
   UT_ASSERT_EQ_UINT(0, ut_character_calls);
   UT_ASSERT(remembered == 'x');
}

static const UTTestCase ut_cases[] = {
   {"visible area", remembers_visible_and_missing_lines},
   {"empty area", accepts_an_empty_display_area}
};
