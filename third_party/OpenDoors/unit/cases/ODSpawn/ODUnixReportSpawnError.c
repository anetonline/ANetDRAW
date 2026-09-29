#include <setjmp.h>

static int ut_errno_value;
#define UT_ERRNO_STORAGE ut_errno_value
#include "../unix_errno_mock.h"
#define UT_CUSTOM_MOCK_write
#define UT_CUSTOM_MOCK__exit
static ssize_t ut_write_results[4];
static int ut_write_errors[4];
static unsigned ut_write_count, ut_exit_count;
static jmp_buf ut_exit_target;
ssize_t utm_write(int file, const void *buffer, size_t size)
{
   unsigned call = ut_write_count++;
   UT_ASSERT_EQ_INT(12, file); UT_ASSERT_NOT_NULL(buffer);
   UT_ASSERT(size > 0 && size <= sizeof(int));
   errno = ut_write_errors[call]; return(ut_write_results[call]);
}
void utm__exit(int status)
{ UT_ASSERT_EQ_INT(127, status); ++ut_exit_count; longjmp(ut_exit_target, 1); }
static void invoke_report(int error)
{
   if(setjmp(ut_exit_target) == 0)
   {
      utt_ODUnixReportSpawnError(12, error);
      UT_ASSERT(FALSE);
   }
}
static void reset_report(void)
{ memset(ut_write_results, 0, sizeof(ut_write_results));
  memset(ut_write_errors, 0, sizeof(ut_write_errors));
  ut_write_count = ut_exit_count = 0; }
static void retries_interrupts_and_completes_partial_writes(void)
{
   reset_report(); ut_write_results[0] = -1; ut_write_errors[0] = EINTR;
   ut_write_results[1] = 1; ut_write_results[2] = sizeof(int) - 1;
   invoke_report(EACCES);
   UT_ASSERT_EQ_UINT(3, ut_write_count); UT_ASSERT_EQ_UINT(1, ut_exit_count);
}
static void stops_on_zero_or_noninterrupt_failure(void)
{
   reset_report(); ut_write_results[0] = 0;
   invoke_report(EIO);
   UT_ASSERT_EQ_UINT(1, ut_write_count); UT_ASSERT_EQ_UINT(1, ut_exit_count);
   reset_report(); ut_write_results[0] = -1; ut_write_errors[0] = EIO;
   invoke_report(EIO);
   UT_ASSERT_EQ_UINT(1, ut_write_count); UT_ASSERT_EQ_UINT(1, ut_exit_count);
}
static const UTTestCase ut_cases[] = {
   {"partial/interrupted writes", retries_interrupts_and_completes_partial_writes},
   {"terminal write failures", stops_on_zero_or_noninterrupt_failure}
};
