#define UT_CUSTOM_MOCK_free

static void *ut_freed[2];
static unsigned ut_free_calls;

void utm_free(void *allocation)
{
   UT_ASSERT(ut_free_calls < 2);
   ut_freed[ut_free_calls++] = allocation;
}

static void releases_internal_storage_and_resets_index_state(void)
{
   tEditInstance instance;
   char remembered;
   char *lines[1];

   instance.pRememberBuffer = &remembered;
   instance.papchStartOfLine = lines;
   instance.unLineArraySize = 7;
   instance.unLinesInBuffer = 5;
   ut_free_calls = 0;
   utt_ODEditCleanupInstance(&instance);
   UT_ASSERT_EQ_UINT(2, ut_free_calls);
   UT_ASSERT_EQ_PTR(&remembered, ut_freed[0]);
   UT_ASSERT_EQ_PTR(lines, ut_freed[1]);
   UT_ASSERT_NULL(instance.pRememberBuffer);
   UT_ASSERT_NULL(instance.papchStartOfLine);
   UT_ASSERT_EQ_UINT(0, instance.unLineArraySize);
   UT_ASSERT_EQ_UINT(0, instance.unLinesInBuffer);
}

static void passes_null_storage_to_the_platform_free_function(void)
{
   tEditInstance instance;

   instance.pRememberBuffer = NULL;
   instance.papchStartOfLine = NULL;
   instance.unLineArraySize = 0;
   instance.unLinesInBuffer = 0;
   ut_free_calls = 0;
   utt_ODEditCleanupInstance(&instance);
   UT_ASSERT_EQ_UINT(2, ut_free_calls);
   UT_ASSERT_NULL(ut_freed[0]);
   UT_ASSERT_NULL(ut_freed[1]);
}

static const UTTestCase ut_cases[] = {
   {"owned storage", releases_internal_storage_and_resets_index_state},
   {"null storage", passes_null_storage_to_the_platform_free_function}
};
