#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_strlen

static unsigned ut_copy_calls;
static size_t ut_last_copy_size;

size_t utm_strlen(const char *text)
{
   size_t length = 0;
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

static void copies_a_fitting_string_and_its_empty_case(void)
{
   char output[6] = {'x', 'x', 'x', 'x', 'x', '\0'};
   ut_copy_calls = 0;
   UT_ASSERT_EQ_PTR(output, utt_safe_strcpy(output, "abc", sizeof(output)));
   UT_ASSERT(strcmp(output, "abc") == 0);
   UT_ASSERT_EQ_INT('x', output[4]);
   UT_ASSERT_EQ_UINT(1, ut_copy_calls);
   UT_ASSERT_EQ_UINT(3, ut_last_copy_size);

   UT_ASSERT_EQ_PTR(output, utt_safe_strcpy(output, "", sizeof(output)));
   UT_ASSERT(strcmp(output, "") == 0);
   UT_ASSERT_EQ_UINT(2, ut_copy_calls);
   UT_ASSERT_EQ_UINT(0, ut_last_copy_size);
}

static void truncates_to_leave_room_for_the_terminator(void)
{
   char output[6] = {'x', 'x', 'x', 'x', 'x', '\0'};
   ut_copy_calls = 0;
   UT_ASSERT_EQ_PTR(output, utt_safe_strcpy(output, "abcdef", 4));
   UT_ASSERT(strcmp(output, "abc") == 0);
   UT_ASSERT_EQ_INT('x', output[4]);
   UT_ASSERT_EQ_UINT(1, ut_copy_calls);
   UT_ASSERT_EQ_UINT(3, ut_last_copy_size);
}

static const UTTestCase ut_cases[] = {
   {"fitting copy", copies_a_fitting_string_and_its_empty_case},
   {"truncated copy", truncates_to_leave_room_for_the_terminator}
};
