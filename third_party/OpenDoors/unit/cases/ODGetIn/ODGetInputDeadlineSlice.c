#define UT_CUSTOM_MOCK_ODSessionTimeGet

#define UT_CLOCK_READS 8
static DWORD ut_seconds[UT_CLOCK_READS];
static WORD ut_milliseconds[UT_CLOCK_READS];
static unsigned ut_read_count;
static unsigned ut_read_index;

static void add_time(DWORD seconds, WORD milliseconds)
{
   UT_ASSERT(ut_read_count < UT_CLOCK_READS);
   ut_seconds[ut_read_count] = seconds;
   ut_milliseconds[ut_read_count++] = milliseconds;
}

void utm_ODSessionTimeGet(DWORD *seconds, WORD *milliseconds)
{
   UT_ASSERT(ut_read_index < ut_read_count);
   *seconds = ut_seconds[ut_read_index];
   *milliseconds = ut_milliseconds[ut_read_index++];
}

static void calculates_bounded_deadline_slices(void)
{
   tODInputDeadline deadline;
   deadline.dwSeconds = 10;
   deadline.wMilliseconds = 20;
   ut_read_count = ut_read_index = 0;

   add_time(11, 0);
   add_time(10, 20);
   add_time(10, 21);
   add_time(8, 999);
   add_time(9, 0);
   add_time(9, 990);
   add_time(10, 0);
   add_time(10, 19);

   UT_ASSERT_EQ_UINT(0, utt_ODGetInputDeadlineSlice(&deadline));
   UT_ASSERT_EQ_UINT(0, utt_ODGetInputDeadlineSlice(&deadline));
   UT_ASSERT_EQ_UINT(0, utt_ODGetInputDeadlineSlice(&deadline));
   UT_ASSERT_EQ_UINT(50, utt_ODGetInputDeadlineSlice(&deadline));
   UT_ASSERT_EQ_UINT(50, utt_ODGetInputDeadlineSlice(&deadline));
   UT_ASSERT_EQ_UINT(30, utt_ODGetInputDeadlineSlice(&deadline));
   UT_ASSERT_EQ_UINT(20, utt_ODGetInputDeadlineSlice(&deadline));
   UT_ASSERT_EQ_UINT(1, utt_ODGetInputDeadlineSlice(&deadline));
}

static const UTTestCase ut_cases[] = {
   {"slices", calculates_bounded_deadline_slices}
};
