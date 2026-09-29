#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_memset

static unsigned ut_free_calls;
static unsigned ut_clear_calls;
static void *ut_expected_cells;

void utm_free(void *memory)
{
   ++ut_free_calls;
   UT_ASSERT_EQ_PTR(ut_expected_cells, memory);
}

void *utm_memset(void *destination, int value, size_t count)
{
   ++ut_clear_calls;
   UT_ASSERT_EQ_PTR(&SessionScreen, destination);
   UT_ASSERT_EQ_INT(0, value);
   UT_ASSERT_EQ_UINT(sizeof(SessionScreen), count);
   SessionScreen.pCells = NULL;
   SessionScreen.nWidth = 0;
   return destination;
}

static void clears_screen_without_freeing_null(void)
{
   SessionScreen.pCells = NULL;
   SessionScreen.nWidth = 80;
   bSessionScreenAvailable = TRUE;
   bSessionScreenEmulating = TRUE;
   ut_free_calls = ut_clear_calls = 0;
   utt_ODSessionScreenShutdown();
   UT_ASSERT_EQ_UINT(0, ut_free_calls);
   UT_ASSERT_EQ_UINT(1, ut_clear_calls);
   UT_ASSERT_EQ_INT(FALSE, bSessionScreenAvailable);
   UT_ASSERT_EQ_INT(FALSE, bSessionScreenEmulating);
}

static void frees_allocated_cells_before_clearing(void)
{
   static BYTE cells[2];
   ut_expected_cells = cells;
   SessionScreen.pCells = cells;
   ut_free_calls = ut_clear_calls = 0;
   utt_ODSessionScreenShutdown();
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
   UT_ASSERT_EQ_UINT(1, ut_clear_calls);
}

static const UTTestCase ut_cases[] = {
   {"null shutdown", clears_screen_without_freeing_null},
   {"allocated shutdown", frees_allocated_cells_before_clearing}
};
