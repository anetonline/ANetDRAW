#define UT_CUSTOM_MOCK_memcpy

void *utm_memcpy(void *destination, const void *source, size_t size)
{
   unsigned char *out = (unsigned char *)destination;
   const unsigned char *in = (const unsigned char *)source;
   size_t index;
   for(index = 0; index < size; ++index) out[index] = in[index];
   return destination;
}

static void converts_fitting_string_and_rejects_oversize(void)
{
   char pascal_string[] = {3, 'a', 'b', 'c'};
   char oversized[] = {4, 'a', 'b', 'c', 'd'};
   char output[8] = "keep";
   UT_ASSERT_EQ_PTR(output,
      utt_ODStringPascalToC(output, pascal_string, 3));
   UT_ASSERT_EQ_INT(0, strcmp("abc", output));
   UT_ASSERT_EQ_PTR(output, utt_ODStringPascalToC(output, oversized, 3));
   UT_ASSERT_EQ_INT(0, strcmp("", output));
}

static const UTTestCase ut_cases[] = {
   {"Pascal to C", converts_fitting_string_and_rejects_oversize}
};
