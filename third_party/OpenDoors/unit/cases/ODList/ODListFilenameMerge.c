#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_strlen

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

void *utm_memcpy(void *destination, const void *source, size_t count)
{
   size_t index;
   unsigned char *out = destination;
   const unsigned char *in = source;
   for(index = 0; index < count; ++index) out[index] = in[index];
   return destination;
}

static void rejects_invalid_destinations(void)
{
   char output[4] = "old";
   UT_ASSERT(!utt_ODListFilenameMerge(NULL, sizeof(output), "", "", "a", ""));
   UT_ASSERT(!utt_ODListFilenameMerge(output, 0, "", "", "a", ""));
   UT_ASSERT_EQ_INT('o', output[0]);
}

static void joins_all_nonnull_components(void)
{
   char output[32];
   UT_ASSERT(utt_ODListFilenameMerge(output, sizeof(output),
      "C:", "DIR/", "FILE", ".EXT"));
   UT_ASSERT(strcmp(output, "C:DIR/FILE.EXT") == 0);
}

static void skips_null_and_empty_components(void)
{
   char output[8];
   UT_ASSERT(utt_ODListFilenameMerge(output, sizeof(output),
      NULL, "", "X", NULL));
   UT_ASSERT(strcmp(output, "X") == 0);
}

static void clears_output_when_a_component_does_not_fit(void)
{
   char output[5];
   UT_ASSERT(!utt_ODListFilenameMerge(output, sizeof(output),
      "A", "B", "CDE", ""));
   UT_ASSERT_EQ_INT('\0', output[0]);

   UT_ASSERT(utt_ODListFilenameMerge(output, sizeof(output),
      "A", "B", "CD", ""));
   UT_ASSERT(strcmp(output, "ABCD") == 0);
}

static const UTTestCase ut_cases[] = {
   {"invalid destination", rejects_invalid_destinations},
   {"all components", joins_all_nonnull_components},
   {"optional components", skips_null_and_empty_components},
   {"capacity", clears_output_when_a_component_does_not_fit}
};
