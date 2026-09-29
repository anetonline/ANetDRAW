static void leaves_existing_sequences_unchanged(void)
{
   tEditInstance instance;
   char paragraph[] = "P";
   char line[] = "L";

   instance.pszParagraphBreak = paragraph;
   instance.pszLineBreak = line;
   utt_ODEditSetBreakSequence(&instance, '\r', '\n');
   UT_ASSERT_EQ_PTR(paragraph, instance.pszParagraphBreak);
   UT_ASSERT_EQ_PTR(line, instance.pszLineBreak);
}

static void recognizes_each_supported_sequence(void)
{
   tEditInstance instance;

   instance.pszParagraphBreak = NULL;
   instance.pszLineBreak = NULL;
   utt_ODEditSetBreakSequence(&instance, '\r', '\0');
   UT_ASSERT(strcmp(instance.pszParagraphBreak, "\r") == 0);
   UT_ASSERT(strcmp(instance.pszLineBreak, "\r") == 0);

   instance.pszParagraphBreak = NULL;
   instance.pszLineBreak = NULL;
   utt_ODEditSetBreakSequence(&instance, '\n', '\0');
   UT_ASSERT(strcmp(instance.pszParagraphBreak, "\n") == 0);
   UT_ASSERT(strcmp(instance.pszLineBreak, "\n") == 0);

   instance.pszParagraphBreak = NULL;
   instance.pszLineBreak = NULL;
   utt_ODEditSetBreakSequence(&instance, '\n', '\r');
   UT_ASSERT(strcmp(instance.pszParagraphBreak, "\n\r") == 0);
   UT_ASSERT(strcmp(instance.pszLineBreak, "\n\r") == 0);

   instance.pszParagraphBreak = NULL;
   instance.pszLineBreak = NULL;
   utt_ODEditSetBreakSequence(&instance, '\r', '\n');
   UT_ASSERT(strcmp(instance.pszParagraphBreak, "\r\n") == 0);
   UT_ASSERT(strcmp(instance.pszLineBreak, "\r\n") == 0);
}

static void fills_only_missing_sequence_fields(void)
{
   tEditInstance instance;
   char paragraph[] = "P";
   char line[] = "L";

   instance.pszParagraphBreak = paragraph;
   instance.pszLineBreak = NULL;
   utt_ODEditSetBreakSequence(&instance, '\r', '\0');
   UT_ASSERT_EQ_PTR(paragraph, instance.pszParagraphBreak);
   UT_ASSERT(strcmp(instance.pszLineBreak, "\r") == 0);

   instance.pszParagraphBreak = NULL;
   instance.pszLineBreak = line;
   utt_ODEditSetBreakSequence(&instance, '\n', '\0');
   UT_ASSERT(strcmp(instance.pszParagraphBreak, "\n") == 0);
   UT_ASSERT_EQ_PTR(line, instance.pszLineBreak);
}

static void rejects_an_invalid_sequence(void)
{
   tEditInstance instance;

   instance.pszParagraphBreak = NULL;
   instance.pszLineBreak = NULL;
   utt_ODEditSetBreakSequence(&instance, 'x', 'y');
   UT_ASSERT_NULL(instance.pszParagraphBreak);
   UT_ASSERT_NULL(instance.pszLineBreak);

   utt_ODEditSetBreakSequence(&instance, '\n', 'x');
   UT_ASSERT_NULL(instance.pszParagraphBreak);
   UT_ASSERT_NULL(instance.pszLineBreak);

   utt_ODEditSetBreakSequence(&instance, '\r', 'x');
   UT_ASSERT_NULL(instance.pszParagraphBreak);
   UT_ASSERT_NULL(instance.pszLineBreak);
}

static const UTTestCase ut_cases[] = {
   {"existing", leaves_existing_sequences_unchanged},
   {"valid sequences", recognizes_each_supported_sequence},
   {"individual fields", fills_only_missing_sequence_fields},
   {"invalid sequence", rejects_an_invalid_sequence}
};
