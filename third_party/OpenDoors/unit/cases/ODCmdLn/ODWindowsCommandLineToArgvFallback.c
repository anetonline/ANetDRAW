#define UT_CUSTOM_MOCK_LocalAlloc

typedef union
{
   void *alignment;
   unsigned char bytes[70000];
} tAlignedBlock;

static tAlignedBlock ut_block;
static BOOL ut_alloc_fails;

HLOCAL WINAPI utm_LocalAlloc(UINT flags, SIZE_T bytes)
{
   UT_ASSERT_EQ_UINT(LMEM_FIXED, flags);
   UT_ASSERT(bytes <= sizeof(ut_block.bytes));
   return(ut_alloc_fails ? NULL : (HLOCAL)ut_block.bytes);
}

static int wide_equal(LPCWSTR left, LPCWSTR right)
{
   while(*left != L'\0' && *left == *right)
   {
      ++left;
      ++right;
   }
   return(*left == *right);
}

static void parses_documented_quote_and_backslash_cases(void)
{
   LPWSTR *arguments;
   INT count;
   memset(&ut_block, 0, sizeof(ut_block));
   ut_alloc_fails = FALSE;
   arguments = utt_ODWindowsCommandLineToArgvFallback(
      L"test \"a b c\" d\\\\\\\"e \"C:\\Program Files\\\\\" \"\"",
      &count);
   UT_ASSERT_NOT_NULL(arguments);
   UT_ASSERT_EQ_INT(5, count);
   UT_ASSERT(wide_equal(L"test", arguments[0]));
   UT_ASSERT(wide_equal(L"a b c", arguments[1]));
   UT_ASSERT(wide_equal(L"d\\\"e", arguments[2]));
   UT_ASSERT(wide_equal(L"C:\\Program Files\\", arguments[3]));
   UT_ASSERT(wide_equal(L"", arguments[4]));
   UT_ASSERT_NULL(arguments[5]);

   arguments = utt_ODWindowsCommandLineToArgvFallback(
      L"test \"a\"\" b\" c", &count);
   UT_ASSERT_NOT_NULL(arguments);
   UT_ASSERT_EQ_INT(3, count);
   UT_ASSERT(wide_equal(L"test", arguments[0]));
   UT_ASSERT(wide_equal(L"a\"", arguments[1]));
   UT_ASSERT(wide_equal(L"b c", arguments[2]));

   arguments = utt_ODWindowsCommandLineToArgvFallback(
      L"test trailing\\", &count);
   UT_ASSERT_NOT_NULL(arguments);
   UT_ASSERT_EQ_INT(2, count);
   UT_ASSERT(wide_equal(L"trailing\\", arguments[1]));
}

static void handles_whitespace_limits_and_allocation_failure(void)
{
   static WCHAR long_command[9000];
   LPWSTR *arguments;
   INT count;
   size_t index;

   memset(&ut_block, 0, sizeof(ut_block));
   ut_alloc_fails = FALSE;
   arguments = utt_ODWindowsCommandLineToArgvFallback(L"  one\ttwo", &count);
   UT_ASSERT_NOT_NULL(arguments);
   UT_ASSERT_EQ_INT(3, count);
   UT_ASSERT(wide_equal(L"", arguments[0]));
   UT_ASSERT(wide_equal(L"one", arguments[1]));
   UT_ASSERT(wide_equal(L"two", arguments[2]));

   arguments = utt_ODWindowsCommandLineToArgvFallback(L"\tone", &count);
   UT_ASSERT_NOT_NULL(arguments);
   UT_ASSERT_EQ_INT(2, count);
   UT_ASSERT(wide_equal(L"", arguments[0]));
   UT_ASSERT(wide_equal(L"one", arguments[1]));

   arguments = utt_ODWindowsCommandLineToArgvFallback(L"   ", &count);
   UT_ASSERT_NOT_NULL(arguments);
   UT_ASSERT_EQ_INT(1, count);
   UT_ASSERT(wide_equal(L"", arguments[0]));

   for(index = 0; index + 1 < sizeof(long_command) / sizeof(long_command[0]);
      ++index)
   {
      long_command[index] = (index & 1) == 0 ? L'x' : L' ';
   }
   long_command[index] = L'\0';
   arguments = utt_ODWindowsCommandLineToArgvFallback(long_command, &count);
   UT_ASSERT_NOT_NULL(arguments);
   UT_ASSERT_EQ_INT(4096, count);

   ut_alloc_fails = TRUE;
   UT_ASSERT_NULL(utt_ODWindowsCommandLineToArgvFallback(L"test", &count));
}

static const UTTestCase ut_cases[] = {
   {"grammar", parses_documented_quote_and_backslash_cases},
   {"limits and failure", handles_whitespace_limits_and_allocation_failure}
};
