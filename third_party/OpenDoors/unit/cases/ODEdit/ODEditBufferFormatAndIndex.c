#define UT_CUSTOM_MOCK_ODEditIsEOLForMode
#define UT_CUSTOM_MOCK_ODEditCalculateLineArrayGrowth
#define UT_CUSTOM_MOCK_realloc
#define UT_CUSTOM_MOCK_memmove
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_ODEditSetBreakSequence

static BOOL ut_growth_result;
static UINT ut_growth_size;
static size_t ut_growth_bytes;
static unsigned ut_growth_calls;
static void *ut_realloc_result;
static unsigned ut_realloc_calls;
static char ut_break_first[8];
static char ut_break_second[8];
static unsigned ut_break_calls;

BOOL utm_ODEditIsEOLForMode(tEditInstance *instance, char character)
{
   if(instance->pUserOptions->TextFormat == FORMAT_FTSC_MESSAGE)
      return(character == '\r' || character == '\0');
   return(character == '\r' || character == '\n' || character == '\0');
}

BOOL utm_ODEditCalculateLineArrayGrowth(UINT current, UINT *new_size,
   size_t *new_bytes)
{
   ++ut_growth_calls;
   UT_ASSERT_EQ_UINT(0, current);
   UT_ASSERT_NOT_NULL(new_size);
   UT_ASSERT_NOT_NULL(new_bytes);
   if(!ut_growth_result) return(FALSE);
   *new_size = ut_growth_size;
   *new_bytes = ut_growth_bytes;
   return(TRUE);
}

void *utm_realloc(void *allocation, size_t size)
{
   ++ut_realloc_calls;
   UT_ASSERT_NOT_NULL(allocation);
   UT_ASSERT(size == ut_growth_bytes);
   return(ut_realloc_result);
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

size_t utm_strlen(const char *text)
{
   size_t length;
   length = 0;
   while(text[length] != '\0')
      ++length;
   return(length);
}

void utm_ODEditSetBreakSequence(tEditInstance *instance, char first,
   char second)
{
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT(ut_break_calls < 8);
   ut_break_first[ut_break_calls] = first;
   ut_break_second[ut_break_calls] = second;
   ++ut_break_calls;
}

static void reset_format(tEditInstance *instance, tODEditOptions *options,
   char *buffer, char **lines, UINT line_capacity, UINT width)
{
   instance->pszEditBuffer = buffer;
   instance->papchStartOfLine = lines;
   instance->unLineArraySize = line_capacity;
   instance->unLinesInBuffer = 0;
   instance->unAreaWidth = width;
   instance->unCurrentLine = 0;
   instance->unCurrentColumn = 0;
   instance->bWordWrapLongLines = FALSE;
   instance->pUserOptions = options;
   options->TextFormat = FORMAT_LINE_BREAKS;
   ut_growth_result = TRUE;
   ut_growth_size = LINE_ARRAY_GROW_SIZE;
   ut_growth_bytes = LINE_ARRAY_GROW_SIZE * sizeof(char *);
   ut_growth_calls = 0;
   ut_realloc_result = lines;
   ut_realloc_calls = 0;
   ut_break_calls = 0;
   od_control.od_error = 0;
}

static void indexes_an_empty_buffer(void)
{
   char buffer[] = "";
   char *lines[2];
   tEditInstance instance;
   tODEditOptions options;
   reset_format(&instance, &options, buffer, lines, 2, 10);
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditBufferFormatAndIndex(&instance));
   UT_ASSERT_EQ_UINT(1, instance.unLinesInBuffer);
   UT_ASSERT_EQ_PTR(buffer, lines[0]);
   UT_ASSERT_EQ_UINT(0, ut_growth_calls);
   UT_ASSERT_EQ_UINT(0, ut_break_calls);
}

static void reports_line_array_growth_failures_and_success(void)
{
   char buffer[] = "";
   char *old_lines[1];
   char *new_lines[LINE_ARRAY_GROW_SIZE];
   tEditInstance instance;
   tODEditOptions options;

   reset_format(&instance, &options, buffer, old_lines, 0, 10);
   ut_growth_result = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODEditBufferFormatAndIndex(&instance));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_realloc_calls);

   reset_format(&instance, &options, buffer, old_lines, 0, 10);
   ut_realloc_result = NULL;
   UT_ASSERT_EQ_INT(FALSE, utt_ODEditBufferFormatAndIndex(&instance));
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_realloc_calls);

   reset_format(&instance, &options, buffer, old_lines, 0, 10);
   ut_realloc_result = new_lines;
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditBufferFormatAndIndex(&instance));
   UT_ASSERT_EQ_PTR(new_lines, instance.papchStartOfLine);
   UT_ASSERT_EQ_UINT(LINE_ARRAY_GROW_SIZE, instance.unLineArraySize);
   UT_ASSERT_EQ_UINT(1, instance.unLinesInBuffer);
   UT_ASSERT_EQ_PTR(buffer, new_lines[0]);
}

static void recognizes_single_and_paired_line_endings(void)
{
   char cr[] = "\r";
   char cr_text[] = "\rX";
   char repeated[] = "\r\r";
   char paired[] = "\r\n";
   char *lines[8];
   tEditInstance instance;
   tODEditOptions options;

   reset_format(&instance, &options, cr, lines, 8, 10);
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditBufferFormatAndIndex(&instance));
   UT_ASSERT_EQ_UINT(1, ut_break_calls);
   UT_ASSERT_EQ_INT('\r', ut_break_first[0]);
   UT_ASSERT_EQ_INT('\0', ut_break_second[0]);

   reset_format(&instance, &options, cr_text, lines, 8, 10);
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditBufferFormatAndIndex(&instance));
   UT_ASSERT_EQ_UINT(1, ut_break_calls);
   UT_ASSERT_EQ_INT('\0', ut_break_second[0]);

   reset_format(&instance, &options, repeated, lines, 8, 10);
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditBufferFormatAndIndex(&instance));
   UT_ASSERT_EQ_INT('\0', ut_break_second[0]);

   reset_format(&instance, &options, paired, lines, 8, 10);
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditBufferFormatAndIndex(&instance));
   UT_ASSERT_EQ_UINT(1, ut_break_calls);
   UT_ASSERT_EQ_INT('\r', ut_break_first[0]);
   UT_ASSERT_EQ_INT('\n', ut_break_second[0]);
}

static void wraps_with_each_word_wrap_boundary_condition(void)
{
   char no_space[] = "abcd";
   char final_space[] = "ab ";
   char prior_space[] = "a bc";
   char *lines[8];
   tEditInstance instance;
   tODEditOptions options;

   reset_format(&instance, &options, no_space, lines, 8, 4);
   instance.unCurrentLine = 1;
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditBufferFormatAndIndex(&instance));
   UT_ASSERT_EQ_PTR(no_space + 2, lines[1]);

   reset_format(&instance, &options, no_space, lines, 8, 4);
   instance.bWordWrapLongLines = TRUE;
   instance.unCurrentColumn = 2;
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditBufferFormatAndIndex(&instance));
   UT_ASSERT_EQ_UINT(0, instance.unCurrentLine);
   UT_ASSERT_EQ_UINT(2, instance.unCurrentColumn);

   reset_format(&instance, &options, final_space, lines, 8, 4);
   instance.bWordWrapLongLines = TRUE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditBufferFormatAndIndex(&instance));
   UT_ASSERT_EQ_PTR(final_space + 3, lines[1]);

   reset_format(&instance, &options, prior_space, lines, 8, 5);
   instance.bWordWrapLongLines = TRUE;
   instance.unCurrentColumn = 5;
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditBufferFormatAndIndex(&instance));
   UT_ASSERT_EQ_UINT(1, instance.unCurrentLine);
   UT_ASSERT_EQ_UINT(3, instance.unCurrentColumn);
   UT_ASSERT_EQ_PTR(prior_space + 2, lines[1]);
}

static void skips_kludges_and_filters_ftsc_body_characters(void)
{
   char message[] = {1, 'K', '\r', 'A', '\n', (char)0x8d, 'B', '\0'};
   char kludge_only[] = {1, 'K', '\0'};
   char *lines[8];
   tEditInstance instance;
   tODEditOptions options;

   reset_format(&instance, &options, message, lines, 8, 10);
   options.TextFormat = FORMAT_FTSC_MESSAGE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditBufferFormatAndIndex(&instance));
   UT_ASSERT_EQ_UINT(1, instance.unLinesInBuffer);
   UT_ASSERT_EQ_PTR(message + 3, lines[0]);
   UT_ASSERT(strcmp(message + 3, "AB") == 0);

   reset_format(&instance, &options, kludge_only, lines, 8, 10);
   options.TextFormat = FORMAT_FTSC_MESSAGE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODEditBufferFormatAndIndex(&instance));
   UT_ASSERT_EQ_UINT(1, instance.unLinesInBuffer);
   UT_ASSERT_EQ_PTR(kludge_only + 2, lines[0]);
}

static const UTTestCase ut_cases[] = {
   {"empty buffer", indexes_an_empty_buffer},
   {"line array growth", reports_line_array_growth_failures_and_success},
   {"line endings", recognizes_single_and_paired_line_endings},
   {"word wrapping", wraps_with_each_word_wrap_boundary_condition},
   {"FTSC formatting", skips_kludges_and_filters_ftsc_body_characters}
};
