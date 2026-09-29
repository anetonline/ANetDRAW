static int ut_errno_value;
#define UT_ERRNO_STORAGE ut_errno_value
#include "../unix_errno_mock.h"
#define UT_CUSTOM_MOCK_waitpid
static pid_t ut_wait_results[3];
static int ut_wait_errors[3];
static unsigned ut_wait_count;
pid_t utm_waitpid(pid_t child, int *status, int options)
{
   unsigned call = ut_wait_count++; UT_ASSERT(child == (pid_t)42);
   UT_ASSERT_NOT_NULL(status); UT_ASSERT_EQ_INT(0, options);
   errno = ut_wait_errors[call]; return(ut_wait_results[call]);
}
static void retries_interrupt_and_accepts_requested_child(void)
{
   int status = 0; ut_wait_count = 0;
   ut_wait_results[0] = -1; ut_wait_errors[0] = EINTR; ut_wait_results[1] = 42;
   UT_ASSERT_EQ_INT(0, utt_ODUnixWaitForChild((pid_t)42, &status));
   UT_ASSERT_EQ_UINT(2, ut_wait_count);
}
static void rejects_errors_and_unexpected_child(void)
{
   int status = 0; ut_wait_count = 0;
   ut_wait_results[0] = -1; ut_wait_errors[0] = EIO;
   UT_ASSERT_EQ_INT(-1, utt_ODUnixWaitForChild((pid_t)42, &status));
   UT_ASSERT_EQ_UINT(1, ut_wait_count);
   ut_wait_count = 0; ut_wait_results[0] = 41; ut_wait_errors[0] = 0;
   UT_ASSERT_EQ_INT(-1, utt_ODUnixWaitForChild((pid_t)42, &status));
}
static const UTTestCase ut_cases[] = {
   {"interrupted wait", retries_interrupt_and_accepts_requested_child},
   {"wait failure", rejects_errors_and_unexpected_child}
};
