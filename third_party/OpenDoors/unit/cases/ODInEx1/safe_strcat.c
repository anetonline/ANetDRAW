#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_strlen

static unsigned ut_length_calls;
static unsigned ut_copy_calls;
static size_t ut_last_copy_size;

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   ++ut_length_calls;
   while(text[length] != '\0') ++length;
   return length;
}

void *utm_memcpy(void *destination, const void *source, size_t size)
{
   unsigned char *output = (unsigned char *)destination;
   const unsigned char *input = (const unsigned char *)source;
   size_t index;
   ++ut_copy_calls;
   ut_last_copy_size = size;
   for(index = 0; index < size; ++index) output[index] = input[index];
   return destination;
}

static void appends_a_fitting_or_empty_suffix(void)
{
   char output[8] = "ab";
   ut_length_calls = ut_copy_calls = 0;
   UT_ASSERT_EQ_PTR(output, utt_safe_strcat(output, "cd", sizeof(output)));
   UT_ASSERT(strcmp(output, "abcd") == 0);
   UT_ASSERT_EQ_UINT(2, ut_length_calls);
   UT_ASSERT_EQ_UINT(1, ut_copy_calls);
   UT_ASSERT_EQ_UINT(2, ut_last_copy_size);

   UT_ASSERT_EQ_PTR(output, utt_safe_strcat(output, "", sizeof(output)));
   UT_ASSERT(strcmp(output, "abcd") == 0);
   UT_ASSERT_EQ_UINT(2, ut_copy_calls);
   UT_ASSERT_EQ_UINT(0, ut_last_copy_size);
}

static void truncates_the_suffix_to_the_remaining_capacity(void)
{
   char output[8] = "abc";
   ut_length_calls = ut_copy_calls = 0;
   UT_ASSERT_EQ_PTR(output, utt_safe_strcat(output, "defghi", 6));
   UT_ASSERT(strcmp(output, "abcde") == 0);
   UT_ASSERT_EQ_UINT(1, ut_copy_calls);
   UT_ASSERT_EQ_UINT(2, ut_last_copy_size);
}

static void leaves_an_already_full_destination_untouched(void)
{
   char output[8] = "abcdef";
   ut_length_calls = ut_copy_calls = 0;
   UT_ASSERT_EQ_PTR(output, utt_safe_strcat(output, "x", 6));
   UT_ASSERT(strcmp(output, "abcdef") == 0);
   UT_ASSERT_EQ_UINT(1, ut_length_calls);
   UT_ASSERT_EQ_UINT(0, ut_copy_calls);

   UT_ASSERT_EQ_PTR(output, utt_safe_strcat(output, "x", 0));
   UT_ASSERT(strcmp(output, "abcdef") == 0);
   UT_ASSERT_EQ_UINT(2, ut_length_calls);
   UT_ASSERT_EQ_UINT(0, ut_copy_calls);
}

static const UTTestCase ut_cases[] = {
   {"fitting append", appends_a_fitting_or_empty_suffix},
   {"truncated append", truncates_the_suffix_to_the_remaining_capacity},
   {"full destination", leaves_an_already_full_destination_untouched}
};
