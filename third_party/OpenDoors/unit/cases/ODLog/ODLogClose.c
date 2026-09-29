#define UT_CUSTOM_MOCK_ODLogFormatWorkString
#define UT_CUSTOM_MOCK_fclose
#define UT_CUSTOM_MOCK_od_log_write

#include "errno_mock.h"

#include <stdarg.h>
#include <string.h>

static FILE *ut_stream = (FILE *)1;
static BOOL ut_write_result;
static BOOL ut_format_result;
static int ut_close_result;
static int ut_write_errno;
static int ut_close_errno;
static const char *ut_expected_message;
static INT ut_expected_reason;
static unsigned ut_write_calls;
static unsigned ut_format_calls;
static unsigned ut_close_calls;

static BOOL ODCALL ut_dummy_hook(INT value)
{
   (void)value;
   return TRUE;
}

BOOL ODCALL utm_od_log_write(const char *message)
{
   UT_ASSERT_EQ_PTR(ut_expected_message, message);
   ++ut_write_calls;
   errno = ut_write_errno;
   return ut_write_result;
}

BOOL utm_ODLogFormatWorkString(const char *format, ...)
{
   va_list arguments;
   va_start(arguments, format);
   UT_ASSERT_EQ_PTR(od_control.od_logfile_messages[5], format);
   UT_ASSERT_EQ_INT(ut_expected_reason, va_arg(arguments, int));
   va_end(arguments);
   strcpy(szODWorkString, "reason");
   ut_expected_message = szODWorkString;
   ++ut_format_calls;
   if(!ut_format_result) od_control.od_error = ERR_LIMIT;
   errno = ut_write_errno;
   return ut_format_result;
}

int utm_fclose(FILE *stream)
{
   UT_ASSERT_EQ_PTR(ut_stream, stream);
   ++ut_close_calls;
   errno = ut_close_errno;
   return ut_close_result;
}

static void reset_close(void)
{
   memset(&od_control, 0, sizeof(od_control));
   od_control.od_logfile_messages[0] = "exit one";
   od_control.od_logfile_messages[4] = "exit five";
   od_control.od_logfile_messages[5] = "exit reason %d";
   od_control.od_logfile_messages[13] = "pre-exit";
   logfile_pointer = ut_stream;
   pfLogWrite = ut_dummy_hook;
   pfLogClose = ut_dummy_hook;
   bPreOrExit = FALSE;
   btExitReason = 1;
   ut_write_result = TRUE;
   ut_format_result = TRUE;
   ut_close_result = 0;
   ut_write_errno = 61;
   ut_close_errno = 62;
   ut_expected_message = od_control.od_logfile_messages[0];
   ut_expected_reason = 99;
   ut_write_calls = 0;
   ut_format_calls = 0;
   ut_close_calls = 0;
   errno = 0;
}

static void returns_immediately_without_an_open_stream(void)
{
   reset_close();
   logfile_pointer = NULL;
   UT_ASSERT(utt_ODLogClose(99));
   UT_ASSERT_EQ_UINT(0, ut_close_calls);
   UT_ASSERT_NOT_NULL(pfLogWrite);
   UT_ASSERT_NOT_NULL(pfLogClose);
}

static void disabled_logging_still_closes_and_clears_hooks(void)
{
   reset_close();
   od_control.od_logfile_disable = TRUE;
   UT_ASSERT(utt_ODLogClose(99));
   UT_ASSERT_EQ_UINT(0, ut_write_calls);
   UT_ASSERT_EQ_UINT(1, ut_close_calls);
   UT_ASSERT_NULL(logfile_pointer);
   UT_ASSERT_NULL(pfLogWrite);
   UT_ASSERT_NULL(pfLogClose);
}

static void writes_preexit_and_each_standard_exit_boundary(void)
{
   reset_close();
   bPreOrExit = TRUE;
   ut_expected_message = od_control.od_logfile_messages[13];
   UT_ASSERT(utt_ODLogClose(99));
   UT_ASSERT_EQ_UINT(1, ut_write_calls);

   reset_close();
   btExitReason = 5;
   ut_expected_message = od_control.od_logfile_messages[4];
   UT_ASSERT(utt_ODLogClose(99));
   UT_ASSERT_EQ_UINT(1, ut_write_calls);
}

static void formats_nonstandard_exit_reasons_at_both_boundaries(void)
{
   reset_close();
   btExitReason = 0;
   ut_expected_message = szODWorkString;
   UT_ASSERT(utt_ODLogClose(99));
   UT_ASSERT_EQ_UINT(1, ut_format_calls);
   UT_ASSERT_EQ_UINT(1, ut_write_calls);

   reset_close();
   btExitReason = 6;
   ut_expected_message = szODWorkString;
   UT_ASSERT(utt_ODLogClose(99));
   UT_ASSERT_EQ_UINT(1, ut_format_calls);
}

static void preserves_first_write_error_when_close_also_fails(void)
{
   reset_close();
   bPreOrExit = TRUE;
   ut_expected_message = od_control.od_logfile_messages[13];
   ut_write_result = FALSE;
   ut_close_result = EOF;
   UT_ASSERT(!utt_ODLogClose(99));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_INT(61, errno);
   UT_ASSERT_EQ_UINT(1, ut_close_calls);

   reset_close();
   btExitReason = 1;
   ut_write_result = FALSE;
   UT_ASSERT(!utt_ODLogClose(99));
   UT_ASSERT_EQ_INT(61, errno);
}

static void reports_formatted_write_and_close_failures(void)
{
   reset_close();
   btExitReason = 0;
   ut_expected_message = szODWorkString;
   ut_write_result = FALSE;
   UT_ASSERT(!utt_ODLogClose(99));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_INT(61, errno);

   reset_close();
   od_control.od_logfile_disable = TRUE;
   ut_close_result = EOF;
   UT_ASSERT(!utt_ODLogClose(99));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_INT(62, errno);
}

static void retains_limit_error_when_formatting_fails(void)
{
   reset_close();
   btExitReason = 0;
   ut_format_result = FALSE;
   UT_ASSERT(!utt_ODLogClose(99));
   UT_ASSERT_EQ_UINT(0, ut_write_calls);
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);

   reset_close();
   btExitReason = 0;
   ut_format_result = FALSE;
   ut_close_result = EOF;
   UT_ASSERT(!utt_ODLogClose(99));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);
   UT_ASSERT_EQ_INT(62, errno);
}

static const UTTestCase ut_cases[] = {
   {"not open", returns_immediately_without_an_open_stream},
   {"disabled", disabled_logging_still_closes_and_clears_hooks},
   {"known reasons", writes_preexit_and_each_standard_exit_boundary},
   {"other reasons", formats_nonstandard_exit_reasons_at_both_boundaries},
   {"first runtime error", preserves_first_write_error_when_close_also_fails},
   {"write and close errors", reports_formatted_write_and_close_failures},
   {"format limit", retains_limit_error_when_formatting_fails}
};
