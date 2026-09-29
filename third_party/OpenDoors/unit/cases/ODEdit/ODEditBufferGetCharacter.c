static void returns_the_requested_line_offset(void)
{
   static char first[] = "first";
   static char second[] = "second";
   static char *lines[] = {first, second};
   tEditInstance instance;

   instance.papchStartOfLine = lines;
   instance.unLinesInBuffer = 2;
   instance.unLineArraySize = 2;
   UT_ASSERT_EQ_PTR(second + 3,
      utt_ODEditBufferGetCharacter(&instance, 1, 3));
}

static const UTTestCase ut_cases[] = {
   {"character address", returns_the_requested_line_offset}
};
