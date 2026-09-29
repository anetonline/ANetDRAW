#define UT_CUSTOM_MOCK_ODWindowsQuoteArgument
#define UT_CUSTOM_MOCK_malloc

typedef union
{
   void *alignment;
   char bytes[256];
} tAlignedBlock;

static tAlignedBlock ut_block;
static BOOL ut_malloc_fails;
static size_t ut_forced_length;

size_t utm_ODWindowsQuoteArgument(char *destination, const char *argument)
{
   size_t length;
   if(ut_forced_length != 0)
      return(ut_forced_length);
   length = strlen(argument) + 2;
   if(destination != NULL)
   {
      destination[0] = '<';
      strcpy(destination + 1, argument);
      destination[length - 1] = '>';
      destination[length] = '\0';
   }
   return(length);
}

void *utm_malloc(size_t size)
{
   UT_ASSERT(size <= sizeof(ut_block.bytes));
   return(ut_malloc_fails ? NULL : ut_block.bytes);
}

static void reset_fixture(void)
{
   memset(&ut_block, 0, sizeof(ut_block));
   ut_malloc_fails = FALSE;
   ut_forced_length = 0;
}

static void builds_one_contiguous_quoted_vector(void)
{
   static const char *arguments[] = {"a", "bc", NULL};
   char **result;
   reset_fixture();
   result = utt_ODWindowsQuoteArguments(arguments);
   UT_ASSERT_EQ_PTR(ut_block.bytes, result);
   UT_ASSERT_EQ_INT(0, strcmp("<a>", result[0]));
   UT_ASSERT_EQ_INT(0, strcmp("<bc>", result[1]));
   UT_ASSERT_NULL(result[2]);
}

static void reports_allocation_and_size_failures(void)
{
   static const char *arguments[] = {"a", NULL};
   static const char *two_arguments[] = {"a", "b", NULL};
   reset_fixture();
   ut_malloc_fails = TRUE;
   UT_ASSERT_NULL(utt_ODWindowsQuoteArguments(arguments));

   reset_fixture();
   ut_forced_length = (size_t)-1;
   UT_ASSERT_NULL(utt_ODWindowsQuoteArguments(arguments));

   reset_fixture();
   ut_forced_length = (size_t)-1 - 2 * sizeof(char *);
   UT_ASSERT_NULL(utt_ODWindowsQuoteArguments(arguments));

   reset_fixture();
   ut_forced_length = (size_t)-1 / 2 + 1;
   UT_ASSERT_NULL(utt_ODWindowsQuoteArguments(two_arguments));
}

static const UTTestCase ut_cases[] = {
   {"quoted vector", builds_one_contiguous_quoted_vector},
   {"failures", reports_allocation_and_size_failures}
};
