static void returns_line_relative_to_scroll_origin(void)
{
   tEditInstance instance;

   instance.unCurrentLine = 17;
   instance.unLineScrolledToTop = 5;
   UT_ASSERT_EQ_UINT(12, utt_ODEditGetCurrentLineInArea(&instance));
}

static const UTTestCase ut_cases[] = {
   {"relative line", returns_line_relative_to_scroll_origin}
};
