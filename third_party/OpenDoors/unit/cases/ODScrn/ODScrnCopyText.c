#define UT_CUSTOM_MOCK_ODSessionScreenIsEmulating
#define UT_CUSTOM_MOCK_ODSessionScreenCopyText
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_ODScrnGetText
#define UT_CUSTOM_MOCK_ODScrnPutText

static BOOL ut_emulating;
static BOOL ut_session_result;
static BOOL ut_malloc_fails;
static BYTE ut_allocation[8];
static unsigned ut_session_calls;
static unsigned ut_malloc_calls;
static unsigned ut_free_calls;
static unsigned ut_get_calls;
static unsigned ut_put_calls;

BOOL utm_ODSessionScreenIsEmulating(void) { return ut_emulating; }

BOOL utm_ODSessionScreenCopyText(INT left, INT top, INT right, INT bottom,
   INT dest_column, INT dest_row)
{
   ++ut_session_calls;
   UT_ASSERT_EQ_INT(1, left); UT_ASSERT_EQ_INT(2, top);
   UT_ASSERT_EQ_INT(3, right); UT_ASSERT_EQ_INT(4, bottom);
   UT_ASSERT_EQ_INT(5, dest_column); UT_ASSERT_EQ_INT(6, dest_row);
   return ut_session_result;
}

void *utm_malloc(size_t count)
{
   ++ut_malloc_calls;
   UT_ASSERT_EQ_UINT(sizeof(ut_allocation), count);
   return ut_malloc_fails ? NULL : ut_allocation;
}

void utm_free(void *allocation)
{
   ++ut_free_calls;
   UT_ASSERT(allocation == ut_allocation);
}

BOOL ODCALL utm_ODScrnGetText(BYTE left, BYTE top, BYTE right, BYTE bottom,
   void *buffer)
{
   ++ut_get_calls;
   UT_ASSERT_EQ_UINT(1, left); UT_ASSERT_EQ_UINT(1, top);
   UT_ASSERT_EQ_UINT(2, right); UT_ASSERT_EQ_UINT(2, bottom);
   UT_ASSERT(buffer == ut_allocation);
   return TRUE;
}

BOOL ODCALL utm_ODScrnPutText(BYTE left, BYTE top, BYTE right, BYTE bottom,
   void *buffer)
{
   ++ut_put_calls;
   UT_ASSERT_EQ_UINT(2, left); UT_ASSERT_EQ_UINT(2, top);
   UT_ASSERT_EQ_UINT(3, right); UT_ASSERT_EQ_UINT(3, bottom);
   UT_ASSERT(buffer == ut_allocation);
   return TRUE;
}

static void reset_copy(void)
{
   btLeftBoundary = 1; btTopBoundary = 1;
   btRightBoundary = 5; btBottomBoundary = 5;
   ut_emulating = FALSE; ut_session_result = FALSE; ut_malloc_fails = FALSE;
   ut_session_calls = ut_malloc_calls = ut_free_calls = 0;
   ut_get_calls = ut_put_calls = 0;
}

static void forwards_to_the_session_screen(void)
{
   reset_copy();
   ut_emulating = TRUE; ut_session_result = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODScrnCopyText(1, 2, 3, 4, 5, 6));
   ut_session_result = TRUE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODScrnCopyText(1, 2, 3, 4, 5, 6));
   UT_ASSERT_EQ_UINT(2, ut_session_calls);
   UT_ASSERT_EQ_UINT(0, ut_malloc_calls);
}

static void rejects_each_out_of_window_coordinate(void)
{
   reset_copy();
   UT_ASSERT_EQ_INT(FALSE, utt_ODScrnCopyText(5, 1, 2, 2, 2, 2));
   UT_ASSERT_EQ_INT(FALSE, utt_ODScrnCopyText(1, 5, 2, 2, 2, 2));
   UT_ASSERT_EQ_INT(FALSE, utt_ODScrnCopyText(1, 1, 5, 2, 2, 2));
   UT_ASSERT_EQ_INT(FALSE, utt_ODScrnCopyText(1, 1, 2, 5, 2, 2));
   UT_ASSERT_EQ_INT(FALSE, utt_ODScrnCopyText(1, 1, 2, 2, 5, 2));
   UT_ASSERT_EQ_INT(FALSE, utt_ODScrnCopyText(1, 1, 2, 2, 2, 5));
   UT_ASSERT_EQ_UINT(0, ut_malloc_calls);
}

static void rejects_reversed_and_overflowing_rectangles(void)
{
   reset_copy();
   UT_ASSERT_EQ_INT(FALSE, utt_ODScrnCopyText(3, 1, 2, 2, 1, 1));
   UT_ASSERT_EQ_INT(FALSE, utt_ODScrnCopyText(1, 3, 2, 2, 1, 1));
   UT_ASSERT_EQ_INT(FALSE, utt_ODScrnCopyText(1, 1, 3, 2, 3, 1));
   UT_ASSERT_EQ_INT(FALSE, utt_ODScrnCopyText(1, 1, 2, 3, 1, 3));
   UT_ASSERT_EQ_UINT(0, ut_malloc_calls);
}

static void reports_allocation_failure(void)
{
   reset_copy(); ut_malloc_fails = TRUE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODScrnCopyText(1, 1, 2, 2, 2, 2));
   UT_ASSERT_EQ_UINT(1, ut_malloc_calls);
   UT_ASSERT_EQ_UINT(0, ut_get_calls); UT_ASSERT_EQ_UINT(0, ut_free_calls);
}

static void copies_through_a_temporary_buffer(void)
{
   reset_copy();
   UT_ASSERT_EQ_INT(TRUE, utt_ODScrnCopyText(1, 1, 2, 2, 2, 2));
   UT_ASSERT_EQ_UINT(1, ut_malloc_calls); UT_ASSERT_EQ_UINT(1, ut_get_calls);
   UT_ASSERT_EQ_UINT(1, ut_put_calls); UT_ASSERT_EQ_UINT(1, ut_free_calls);
}

static const UTTestCase ut_cases[] = {
   {"session screen", forwards_to_the_session_screen},
   {"invalid coordinates", rejects_each_out_of_window_coordinate},
   {"invalid rectangles", rejects_reversed_and_overflowing_rectangles},
   {"allocation failure", reports_allocation_failure},
   {"copy through buffer", copies_through_a_temporary_buffer}
};
