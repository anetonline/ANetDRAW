#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_strlen

void *utm_memcpy(void *destination, const void *source, size_t size)
{
   unsigned char *out = (unsigned char *)destination;
   const unsigned char *in = (const unsigned char *)source;
   size_t index;
   for(index = 0; index < size; ++index) out[index] = in[index];
   return destination;
}

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

static void converts_short_and_truncated_strings(void)
{
   char output[8];
   memset(output, 0, sizeof(output));
   UT_ASSERT_EQ_PTR(output, utt_ODStringCToPascal(output, 6, "abc"));
   UT_ASSERT_EQ_UINT(3, (BYTE)output[0]);
   UT_ASSERT(memcmp(output + 1, "abc", 3) == 0);
   UT_ASSERT_EQ_PTR(output, utt_ODStringCToPascal(output, 3, "abcdef"));
   UT_ASSERT_EQ_UINT(3, (BYTE)output[0]);
   UT_ASSERT(memcmp(output + 1, "abc", 3) == 0);
}

static const UTTestCase ut_cases[] = {
   {"C to Pascal", converts_short_and_truncated_strings}
};
