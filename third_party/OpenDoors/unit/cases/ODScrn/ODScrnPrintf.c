#define UT_CUSTOM_MOCK_ODVsnprintf
#define UT_CUSTOM_MOCK_ODSizeAdd
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_ODScrnDisplayString

static char ut_large[91];
static INT ut_probe_result;
static INT ut_write_result;
static BOOL ut_size_fails;
static BOOL ut_malloc_fails;
static unsigned ut_format_calls;
static unsigned ut_size_calls;
static unsigned ut_malloc_calls;
static unsigned ut_free_calls;
static unsigned ut_display_calls;

int utm_ODVsnprintf(char *buffer, size_t size, const char *format,
   va_list arguments)
{
   unsigned index;
   UT_ASSERT(strcmp(format, "%d") == 0);
   UT_ASSERT_EQ_INT(42, va_arg(arguments, int));
   if(ut_format_calls++ == 0)
   {
      UT_ASSERT(buffer == szBuffer); UT_ASSERT_EQ_UINT(0, size);
      return ut_probe_result;
   }
   if(ut_probe_result < (INT)sizeof(szBuffer))
   {
      UT_ASSERT(buffer == szBuffer); UT_ASSERT_EQ_UINT(sizeof(szBuffer), size);
      buffer[0] = '4'; buffer[1] = '2'; buffer[2] = '\0';
   }
   else
   {
      UT_ASSERT(buffer == ut_large); UT_ASSERT_EQ_UINT(sizeof(ut_large), size);
      for(index = 0; index < sizeof(ut_large) - 1; ++index) buffer[index] = 'X';
      buffer[sizeof(ut_large) - 1] = '\0';
   }
   return ut_write_result;
}

int utm_ODSizeAdd(size_t left, size_t right, size_t *result)
{
   ++ut_size_calls; UT_ASSERT_EQ_UINT((size_t)ut_probe_result, left);
   UT_ASSERT_EQ_UINT(1, right); UT_ASSERT(result != NULL);
   if(ut_size_fails) return FALSE;
   *result = left + right; return TRUE;
}

void *utm_malloc(size_t count)
{
   ++ut_malloc_calls; UT_ASSERT_EQ_UINT(sizeof(ut_large), count);
   return ut_malloc_fails ? NULL : ut_large;
}

void utm_free(void *allocation)
{
   ++ut_free_calls; UT_ASSERT(allocation == ut_large);
}

void ODCALL utm_ODScrnDisplayString(const char *string)
{
   ++ut_display_calls;
   if(ut_probe_result < (INT)sizeof(szBuffer))
      UT_ASSERT(strcmp(string, "42") == 0);
   else
      UT_ASSERT(string == ut_large && string[89] == 'X' && string[90] == '\0');
}

static void reset_printf(INT probe)
{
   ut_probe_result = probe; ut_write_result = probe;
   ut_size_fails = FALSE; ut_malloc_fails = FALSE;
   ut_format_calls = ut_size_calls = ut_malloc_calls = 0;
   ut_free_calls = ut_display_calls = 0;
}

static void rejects_probe_failure(void)
{
   reset_printf(-1);
   UT_ASSERT_EQ_INT(-1, utt_ODScrnPrintf("%d", 42));
   UT_ASSERT_EQ_UINT(1, ut_format_calls); UT_ASSERT_EQ_UINT(0, ut_size_calls);
}

static void rejects_size_overflow(void)
{
   reset_printf(2); ut_size_fails = TRUE;
   UT_ASSERT_EQ_INT(-1, utt_ODScrnPrintf("%d", 42));
   UT_ASSERT_EQ_UINT(1, ut_size_calls); UT_ASSERT_EQ_UINT(1, ut_format_calls);
}

static void formats_a_small_result_in_the_fixed_buffer(void)
{
   reset_printf(2);
   UT_ASSERT_EQ_INT(2, utt_ODScrnPrintf("%d", 42));
   UT_ASSERT_EQ_UINT(2, ut_format_calls); UT_ASSERT_EQ_UINT(1, ut_display_calls);
   UT_ASSERT_EQ_UINT(0, ut_malloc_calls); UT_ASSERT_EQ_UINT(0, ut_free_calls);
}

static void reports_large_allocation_failure(void)
{
   reset_printf(90); ut_malloc_fails = TRUE;
   UT_ASSERT_EQ_INT(-1, utt_ODScrnPrintf("%d", 42));
   UT_ASSERT_EQ_UINT(1, ut_malloc_calls); UT_ASSERT_EQ_UINT(1, ut_format_calls);
}

static void rejects_a_short_second_format_and_releases_storage(void)
{
   reset_printf(90); ut_write_result = 89;
   UT_ASSERT_EQ_INT(-1, utt_ODScrnPrintf("%d", 42));
   UT_ASSERT_EQ_UINT(2, ut_format_calls); UT_ASSERT_EQ_UINT(1, ut_free_calls);
   UT_ASSERT_EQ_UINT(0, ut_display_calls);
}

static void rejects_a_short_fixed_buffer_format_without_freeing(void)
{
   reset_printf(2); ut_write_result = 1;
   UT_ASSERT_EQ_INT(-1, utt_ODScrnPrintf("%d", 42));
   UT_ASSERT_EQ_UINT(0, ut_free_calls); UT_ASSERT_EQ_UINT(0, ut_display_calls);
}

static void formats_a_large_result_in_allocated_storage(void)
{
   reset_printf(90);
   UT_ASSERT_EQ_INT(90, utt_ODScrnPrintf("%d", 42));
   UT_ASSERT_EQ_UINT(1, ut_display_calls); UT_ASSERT_EQ_UINT(1, ut_free_calls);
}

static const UTTestCase ut_cases[] = {
   {"probe failure", rejects_probe_failure},
   {"size overflow", rejects_size_overflow},
   {"fixed buffer", formats_a_small_result_in_the_fixed_buffer},
   {"allocation failure", reports_large_allocation_failure},
   {"large write mismatch", rejects_a_short_second_format_and_releases_storage},
   {"small write mismatch", rejects_a_short_fixed_buffer_format_without_freeing},
   {"allocated buffer", formats_a_large_result_in_allocated_storage}
};
