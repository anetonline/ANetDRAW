#define UT_CUSTOM_MOCK_ODSizeAdd
#define UT_CUSTOM_MOCK_ODEditBufferFormatAndIndex
static BOOL ut_size_add_result;
static size_t ut_size_add_value;
static unsigned ut_size_add_calls;
static BOOL ut_format_result;
static unsigned ut_format_calls;
static void *ut_realloc_result;
static UINT ut_expected_realloc_size;
static unsigned ut_realloc_calls;

int utm_ODSizeAdd(size_t left, size_t right, size_t *result)
{
   ++ut_size_add_calls;
   UT_ASSERT_NOT_NULL(result);
   UT_ASSERT_EQ_UINT(BUFFER_GROW_SIZE, right);
   *result = ut_size_add_value;
#if !defined(ODPLAT_DOS) && !defined(ODPLAT_DOS32)
   UT_ASSERT(left <= (size_t)(UINT)-1);
#endif
   return(ut_size_add_result);
}

BOOL utm_ODEditBufferFormatAndIndex(tEditInstance *instance)
{
   ++ut_format_calls;
   UT_ASSERT_NOT_NULL(instance);
   return(ut_format_result);
}

#ifdef ODPLAT_DOS32
static void *ODCALL ut_reallocate(void *original, UINT new_size)
#else
static void *ut_reallocate(void *original, UINT new_size)
#endif
{
   ++ut_realloc_calls;
   UT_ASSERT_NOT_NULL(original);
   UT_ASSERT_EQ_UINT(ut_expected_realloc_size, new_size);
   return(ut_realloc_result);
}

static void reset_growth(tEditInstance *instance, tODEditOptions *options,
   char *buffer, UINT size)
{
   instance->pszEditBuffer = buffer;
   instance->unBufferSize = size;
   instance->pUserOptions = options;
   options->pfBufferRealloc = ut_reallocate;
   ut_size_add_result = TRUE;
   ut_size_add_value = (size_t)size + BUFFER_GROW_SIZE;
   ut_size_add_calls = 0;
   ut_format_result = TRUE;
   ut_format_calls = 0;
   ut_realloc_result = buffer;
   ut_expected_realloc_size = (UINT)ut_size_add_value;
   ut_realloc_calls = 0;
}

static void rejects_an_ungrowable_buffer(void)
{
   char buffer[2];
   tEditInstance instance;
   tODEditOptions options;

   reset_growth(&instance, &options, buffer, 100);
   options.pfBufferRealloc = NULL;
   UT_ASSERT_EQ_INT(kODRCSafeFailure,
      utt_ODEditTryToGrow(&instance, 101));
   UT_ASSERT_EQ_UINT(0, ut_size_add_calls);
   UT_ASSERT_EQ_UINT(0, ut_realloc_calls);
}

static void handles_growth_arithmetic_and_callback_failure(void)
{
   char buffer[2];
   tEditInstance instance;
   tODEditOptions options;

   reset_growth(&instance, &options, buffer, 100);
   ut_size_add_result = FALSE;
   ut_expected_realloc_size = 200;
   ut_realloc_result = NULL;
   UT_ASSERT_EQ_INT(kODRCSafeFailure,
      utt_ODEditTryToGrow(&instance, 200));
   UT_ASSERT_EQ_UINT(1, ut_realloc_calls);

#if !defined(ODPLAT_DOS) && !defined(ODPLAT_DOS32)
   reset_growth(&instance, &options, buffer, (UINT)-101);
   ut_size_add_value = (size_t)(UINT)-1 + 100;
   ut_expected_realloc_size = (UINT)-50;
   ut_realloc_result = NULL;
   UT_ASSERT_EQ_INT(kODRCSafeFailure,
      utt_ODEditTryToGrow(&instance, (UINT)-50));
#endif
}

static void accepts_in_place_and_moved_growth(void)
{
   static char first[2];
   static char second[2];
   tEditInstance instance;
   tODEditOptions options;

   reset_growth(&instance, &options, first, 100);
   ut_size_add_value = 1124;
   ut_expected_realloc_size = 1124;
   ut_realloc_result = first;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODEditTryToGrow(&instance, 200));
   UT_ASSERT_EQ_PTR(first, instance.pszEditBuffer);
   UT_ASSERT_EQ_UINT(1124, instance.unBufferSize);
   UT_ASSERT_EQ_UINT(0, ut_format_calls);

   reset_growth(&instance, &options, first, 100);
   ut_size_add_value = 150;
   ut_expected_realloc_size = 200;
   ut_realloc_result = second;
   ut_format_result = TRUE;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODEditTryToGrow(&instance, 200));
   UT_ASSERT_EQ_PTR(second, instance.pszEditBuffer);
   UT_ASSERT_EQ_UINT(200, instance.unBufferSize);
   UT_ASSERT_EQ_UINT(1, ut_format_calls);

   reset_growth(&instance, &options, first, 100);
   ut_realloc_result = second;
   ut_format_result = FALSE;
   UT_ASSERT_EQ_INT(kODRCUnrecoverableFailure,
      utt_ODEditTryToGrow(&instance, 200));
   UT_ASSERT_EQ_UINT(1, ut_format_calls);
}

static const UTTestCase ut_cases[] = {
   {"ungrowable", rejects_an_ungrowable_buffer},
   {"growth failures", handles_growth_arithmetic_and_callback_failure},
   {"successful growth", accepts_in_place_and_moved_growth}
};
