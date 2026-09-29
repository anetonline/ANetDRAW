static void returns_indexed_line_count(void)
{
   tEditInstance instance;

   instance.unLinesInBuffer = 4;
   instance.unLineArraySize = 8;
   UT_ASSERT_EQ_UINT(4, utt_ODEditBufferGetTotalLines(&instance));
}

static const UTTestCase ut_cases[] = {
   {"line count", returns_indexed_line_count}
};
