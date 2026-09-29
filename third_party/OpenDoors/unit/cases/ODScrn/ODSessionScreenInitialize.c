#define UT_CUSTOM_MOCK_memset
#define UT_CUSTOM_MOCK_malloc

static BYTE ut_memory[32];
static BOOL ut_malloc_fails;
static unsigned ut_memset_calls;
static unsigned ut_malloc_calls;
static size_t ut_malloc_size;

static void fill_bytes(void *destination, BYTE value, size_t count)
{
   BYTE *bytes;
   size_t index;
   bytes = (BYTE *)destination;
   for(index = 0; index < count; ++index)
      bytes[index] = value;
}

void *utm_memset(void *destination, int value, size_t count)
{
   BYTE *bytes;
   size_t index;
   ++ut_memset_calls;
   UT_ASSERT_EQ_PTR(&SessionScreen, destination);
   UT_ASSERT_EQ_INT(0, value);
   UT_ASSERT_EQ_UINT(sizeof(SessionScreen), count);
   bytes = (BYTE *)destination;
   for(index = 0; index < count; ++index)
      bytes[index] = 0;
   return destination;
}

void *utm_malloc(size_t count)
{
   ++ut_malloc_calls;
   ut_malloc_size = count;
   if(ut_malloc_fails)
      return NULL;
   UT_ASSERT(count <= sizeof(ut_memory));
   return ut_memory;
}

static void reset_initialize(void)
{
   fill_bytes(&SessionScreen, 0x55, sizeof(SessionScreen));
   fill_bytes(ut_memory, 0x55, sizeof(ut_memory));
   bSessionScreenAvailable = TRUE;
   bSessionScreenEmulating = TRUE;
   nSessionScreenError = ERR_MEMORY;
   od_control.baud = 9600;
   od_control.user_screenwidth = 3;
   od_control.user_screen_length = 2;
   ut_malloc_fails = FALSE;
   ut_memset_calls = ut_malloc_calls = 0;
   ut_malloc_size = 0;
}

static void initializes_state_but_skips_a_local_session(void)
{
   reset_initialize();
   od_control.baud = 0;
   utt_ODSessionScreenInitialize(1, 1);
   UT_ASSERT_EQ_UINT(1, ut_memset_calls);
   UT_ASSERT_EQ_UINT(0, ut_malloc_calls);
   UT_ASSERT_EQ_INT(FALSE, bSessionScreenAvailable);
   UT_ASSERT_EQ_INT(FALSE, bSessionScreenEmulating);
   UT_ASSERT_EQ_INT(ERR_NONE, nSessionScreenError);
}

static void rejects_nonpositive_dimensions_independently(void)
{
   reset_initialize();
   od_control.user_screenwidth = 0;
   od_control.user_screen_length = 1;
   utt_ODSessionScreenInitialize(0, 0);
   UT_ASSERT_EQ_INT(ERR_LIMIT, nSessionScreenError);
   UT_ASSERT_EQ_UINT(0, ut_malloc_calls);

   reset_initialize();
   od_control.user_screenwidth = 1;
   od_control.user_screen_length = 0;
   utt_ODSessionScreenInitialize(0, 0);
   UT_ASSERT_EQ_INT(ERR_LIMIT, nSessionScreenError);
   UT_ASSERT_EQ_UINT(0, ut_malloc_calls);
}

#if !defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
static void rejects_a_cell_count_that_cannot_be_doubled(void)
{
   reset_initialize();
   utt_ODSessionScreenInitialize(50000, 50000);
   UT_ASSERT_EQ_INT(ERR_LIMIT, nSessionScreenError);
   UT_ASSERT_EQ_UINT(0, ut_malloc_calls);
}
#endif

#ifdef ODPLAT_DOS
static void rejects_a_dos_object_larger_than_the_snapshot_limit(void)
{
   reset_initialize();
   od_control.user_screenwidth = 200;
   od_control.user_screen_length = 200;
   utt_ODSessionScreenInitialize(1, 1);
   UT_ASSERT_EQ_INT(ERR_LIMIT, nSessionScreenError);
   UT_ASSERT_EQ_UINT(0, ut_malloc_calls);
}
#endif

static void reports_allocation_failure(void)
{
   reset_initialize();
   ut_malloc_fails = TRUE;
   utt_ODSessionScreenInitialize(1, 1);
   UT_ASSERT_EQ_UINT(1, ut_malloc_calls);
   UT_ASSERT_EQ_UINT(12, ut_malloc_size);
   UT_ASSERT_EQ_INT(ERR_MEMORY, nSessionScreenError);
   UT_ASSERT_EQ_INT(FALSE, bSessionScreenAvailable);
}

static void allocates_the_minimum_screen_and_initializes_each_cell(void)
{
   unsigned index;
   reset_initialize();
   od_control.user_screenwidth = 1;
   od_control.user_screen_length = 1;
   utt_ODSessionScreenInitialize(3, 2);
   UT_ASSERT_EQ_UINT(1, ut_malloc_calls);
   UT_ASSERT_EQ_UINT(12, ut_malloc_size);
   UT_ASSERT_EQ_PTR(ut_memory, SessionScreen.pCells);
   UT_ASSERT_EQ_INT(3, SessionScreen.nWidth);
   UT_ASSERT_EQ_INT(2, SessionScreen.nHeight);
   UT_ASSERT_EQ_INT(0, SessionScreen.nLeft);
   UT_ASSERT_EQ_INT(0, SessionScreen.nTop);
   UT_ASSERT_EQ_INT(2, SessionScreen.nRight);
   UT_ASSERT_EQ_INT(1, SessionScreen.nBottom);
   UT_ASSERT_EQ_UINT(0x07, SessionScreen.btAttribute);
   UT_ASSERT_EQ_INT(TRUE, SessionScreen.bScrolling);
   UT_ASSERT_EQ_INT(TRUE, bSessionScreenAvailable);
   for(index = 0; index < 12; index += 2)
   {
      UT_ASSERT_EQ_UINT(' ', ut_memory[index]);
      UT_ASSERT_EQ_UINT(0x07, ut_memory[index + 1]);
   }

   reset_initialize();
   utt_ODSessionScreenInitialize(1, 1);
   UT_ASSERT_EQ_INT(3, SessionScreen.nWidth);
   UT_ASSERT_EQ_INT(2, SessionScreen.nHeight);
}

static const UTTestCase ut_cases[] = {
   {"local session", initializes_state_but_skips_a_local_session},
   {"invalid dimensions", rejects_nonpositive_dimensions_independently},
#if !defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   {"cell count overflow", rejects_a_cell_count_that_cannot_be_doubled},
#endif
#ifdef ODPLAT_DOS
   {"DOS object limit", rejects_a_dos_object_larger_than_the_snapshot_limit},
#endif
   {"allocation failure", reports_allocation_failure},
   {"initialized cells", allocates_the_minimum_screen_and_initializes_each_cell}
};
