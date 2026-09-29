static int ut_errno_value;
#define UT_ERRNO_STORAGE ut_errno_value
#include "../unix_errno_mock.h"
#define UT_CUSTOM_MOCK_read
static ssize_t ut_read_results[4];
static int ut_read_errors[4];
static int ut_reported_error;
static unsigned ut_read_count;
static size_t ut_bytes_delivered;
ssize_t utm_read(int file, void *buffer, size_t size)
{
   unsigned call = ut_read_count++; ssize_t result = ut_read_results[call];
   UT_ASSERT_EQ_INT(13, file); UT_ASSERT_NOT_NULL(buffer);
   UT_ASSERT(size > 0 && size <= sizeof(int)); errno = ut_read_errors[call];
   if(result > 0)
   {
      size_t index; const char *source = (const char *)&ut_reported_error;
      for(index = 0; index < (size_t)result; ++index)
         ((char *)buffer)[index] = source[ut_bytes_delivered + index];
      ut_bytes_delivered += (size_t)result;
   }
   return(result);
}
static void reset_read(void)
{ memset(ut_read_results, 0, sizeof(ut_read_results));
  memset(ut_read_errors, 0, sizeof(ut_read_errors));
  ut_reported_error = EACCES; ut_read_count = 0; ut_bytes_delivered = 0; }
static void reads_complete_error_across_interrupts_and_fragments(void)
{
   int error = 0; reset_read(); ut_read_results[0] = -1; ut_read_errors[0] = EINTR;
   ut_read_results[1] = 1; ut_read_results[2] = sizeof(int) - 1;
   UT_ASSERT_EQ_INT(1, utt_ODUnixReadSpawnError(13, &error));
   UT_ASSERT_EQ_INT(EACCES, error); UT_ASSERT_EQ_UINT(3, ut_read_count);
}
static void distinguishes_clean_and_truncated_end_of_file(void)
{
   int error = 0; reset_read(); ut_read_results[0] = 0;
   UT_ASSERT_EQ_INT(0, utt_ODUnixReadSpawnError(13, &error));
   reset_read(); ut_read_results[0] = 1; ut_read_results[1] = 0;
   UT_ASSERT_EQ_INT(-1, utt_ODUnixReadSpawnError(13, &error));
   UT_ASSERT_EQ_INT(EIO, errno);
}
static void reports_noninterrupt_read_failure(void)
{
   int error = 0; reset_read(); ut_read_results[0] = -1; ut_read_errors[0] = EIO;
   UT_ASSERT_EQ_INT(-1, utt_ODUnixReadSpawnError(13, &error));
   UT_ASSERT_EQ_INT(EIO, errno);
}
static const UTTestCase ut_cases[] = {
   {"complete error", reads_complete_error_across_interrupts_and_fragments},
   {"pipe EOF", distinguishes_clean_and_truncated_end_of_file},
   {"read error", reports_noninterrupt_read_failure}
};
