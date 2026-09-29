#define UT_CUSTOM_MOCK_ODLogFormatWorkString
#define UT_CUSTOM_MOCK_ODLogTimeRecordSucceeded
#define UT_CUSTOM_MOCK_fclose
#define UT_CUSTOM_MOCK_fopen
#define UT_CUSTOM_MOCK_fprintf
#define UT_CUSTOM_MOCK_localtime
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_log_write
#define UT_CUSTOM_MOCK_time

#include "errno_mock.h"

#include <stdarg.h>
#include <string.h>

static FILE *ut_stream = (FILE *)1;
static struct tm ut_time_record;
static FILE *ut_fopen_result;
static time_t ut_time_value;
static BOOL ut_localtime_available;
static BOOL ut_time_valid;
static int ut_fprintf_result;
static BOOL ut_format_result;
static BOOL ut_write_result;
static int ut_failure_errno;
static unsigned ut_init_calls;
static unsigned ut_fopen_calls;
static unsigned ut_fclose_calls;
static unsigned ut_time_calls;
static unsigned ut_localtime_calls;
static unsigned ut_validate_calls;
static unsigned ut_fprintf_calls;
static unsigned ut_format_calls;
static unsigned ut_write_calls;
static BOOL ut_init_succeeds;

void ODCALL utm_od_init(void)
{
   if(ut_init_succeeds) bODInitialized = TRUE;
   ++ut_init_calls;
}

FILE *utm_fopen(const char *name, const char *mode)
{
   UT_ASSERT_EQ_PTR(od_control.od_logfile_name, name);
   UT_ASSERT_EQ_INT(0, strcmp("a", mode));
   ++ut_fopen_calls;
   errno = ut_failure_errno;
   return ut_fopen_result;
}

int utm_fclose(FILE *stream)
{
   UT_ASSERT_EQ_PTR(ut_stream, stream);
   ++ut_fclose_calls;
   errno = 99;
   return 0;
}

time_t utm_time(time_t *storage)
{
   UT_ASSERT_NULL(storage);
   ++ut_time_calls;
   return ut_time_value;
}

struct tm *utm_localtime(const time_t *value)
{
   UT_ASSERT_NOT_NULL(value);
   UT_ASSERT_EQ_INT(ut_time_value, *value);
   ++ut_localtime_calls;
   return ut_localtime_available ? &ut_time_record : NULL;
}

BOOL ODCALL utm_ODLogTimeRecordSucceeded(time_t value,
   const struct tm *record)
{
   UT_ASSERT_EQ_INT(ut_time_value, value);
   UT_ASSERT_EQ_PTR(ut_localtime_available ? &ut_time_record : NULL, record);
   ++ut_validate_calls;
   errno = ut_failure_errno;
   return ut_time_valid;
}

int utm_fprintf(FILE *stream, const char *format, ...)
{
   va_list arguments;
   va_start(arguments, format);
   UT_ASSERT_EQ_PTR(ut_stream, stream);
   UT_ASSERT_EQ_INT(0, strcmp("\n----------  %s %02d %s %02d, %s\n",
      format));
   UT_ASSERT_EQ_PTR(od_control.od_day[ut_time_record.tm_wday],
      va_arg(arguments, char *));
   UT_ASSERT_EQ_INT(ut_time_record.tm_mday, va_arg(arguments, int));
   UT_ASSERT_EQ_PTR(od_control.od_month[ut_time_record.tm_mon],
      va_arg(arguments, char *));
   UT_ASSERT_EQ_INT(ut_time_record.tm_year % 100, va_arg(arguments, int));
   UT_ASSERT_EQ_PTR(od_program_name, va_arg(arguments, char *));
   va_end(arguments);
   ++ut_fprintf_calls;
   errno = ut_failure_errno;
   return ut_fprintf_result;
}

BOOL utm_ODLogFormatWorkString(const char *format, ...)
{
   va_list arguments;
   va_start(arguments, format);
   UT_ASSERT_EQ_PTR(od_control.od_logfile_messages[11], format);
   UT_ASSERT_EQ_PTR(od_control.user_name, va_arg(arguments, char *));
   va_end(arguments);
   strcpy(szODWorkString, "entered");
   ++ut_format_calls;
   errno = ut_failure_errno;
   return ut_format_result;
}

BOOL ODCALL utm_od_log_write(const char *message)
{
   UT_ASSERT_EQ_PTR(szODWorkString, message);
   ++ut_write_calls;
   errno = ut_failure_errno;
   return ut_write_result;
}

static void reset_open(void)
{
   memset(&od_control, 0, sizeof(od_control));
   memset(&ut_time_record, 0, sizeof(ut_time_record));
   strcpy(od_control.od_logfile_name, "door.log");
   strcpy(od_control.user_name, "User");
   strcpy(od_program_name, "Door");
   od_control.od_day[2] = "Tuesday";
   od_control.od_month[3] = "April";
   od_control.od_logfile_messages[11] = "entered: %s";
   bODInitialized = TRUE;
   logfile_pointer = NULL;
   pfLogWrite = NULL;
   pfLogClose = NULL;
   ut_time_record.tm_wday = 2;
   ut_time_record.tm_mday = 9;
   ut_time_record.tm_mon = 3;
   ut_time_record.tm_year = 126;
   ut_fopen_result = ut_stream;
   ut_time_value = (time_t)123;
   ut_localtime_available = TRUE;
   ut_time_valid = TRUE;
   ut_fprintf_result = 1;
   ut_format_result = TRUE;
   ut_write_result = TRUE;
   ut_failure_errno = 73;
   ut_init_calls = 0;
   ut_fopen_calls = 0;
   ut_fclose_calls = 0;
   ut_time_calls = 0;
   ut_localtime_calls = 0;
   ut_validate_calls = 0;
   ut_fprintf_calls = 0;
   ut_format_calls = 0;
   ut_write_calls = 0;
   errno = 0;
   ut_init_succeeds = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_open(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT(!utt_od_log_open());
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_fopen_calls);
}

static void initializes_but_reuses_or_honors_disabled_state(void)
{
   reset_open();
   bODInitialized = FALSE;
   logfile_pointer = ut_stream;
   UT_ASSERT(utt_od_log_open());
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(0, ut_fopen_calls);

   reset_open();
   od_control.od_logfile_disable = TRUE;
   UT_ASSERT(utt_od_log_open());
   UT_ASSERT_EQ_UINT(0, ut_fopen_calls);
}

static void reports_file_open_failure(void)
{
   reset_open();
   ut_fopen_result = NULL;
   UT_ASSERT(!utt_od_log_open());
   UT_ASSERT_NULL(logfile_pointer);
   UT_ASSERT_EQ_UINT(1, ut_fopen_calls);
   UT_ASSERT_EQ_UINT(0, ut_time_calls);
}

static void closes_and_preserves_errno_for_both_timestamp_failures(void)
{
   reset_open();
   ut_time_value = (time_t)-1;
   ut_localtime_available = FALSE;
   ut_time_valid = FALSE;
   UT_ASSERT(!utt_od_log_open());
   UT_ASSERT_EQ_UINT(0, ut_localtime_calls);
   UT_ASSERT_EQ_UINT(1, ut_fclose_calls);
   UT_ASSERT_EQ_INT(73, errno);
   UT_ASSERT_NULL(logfile_pointer);

   reset_open();
   ut_localtime_available = FALSE;
   ut_time_valid = FALSE;
   UT_ASSERT(!utt_od_log_open());
   UT_ASSERT_EQ_UINT(1, ut_localtime_calls);
   UT_ASSERT_EQ_INT(73, errno);
}

static void cleans_up_each_initial_output_failure(void)
{
   reset_open();
   ut_fprintf_result = -1;
   UT_ASSERT(!utt_od_log_open());
   UT_ASSERT_EQ_UINT(1, ut_fclose_calls);
   UT_ASSERT_EQ_UINT(0, ut_format_calls);
   UT_ASSERT_EQ_INT(73, errno);

   reset_open();
   ut_format_result = FALSE;
   UT_ASSERT(!utt_od_log_open());
   UT_ASSERT_EQ_UINT(0, ut_write_calls);
   UT_ASSERT_EQ_UINT(1, ut_fclose_calls);
   UT_ASSERT_EQ_INT(73, errno);

   reset_open();
   ut_write_result = FALSE;
   UT_ASSERT(!utt_od_log_open());
   UT_ASSERT_EQ_UINT(1, ut_write_calls);
   UT_ASSERT_EQ_UINT(1, ut_fclose_calls);
   UT_ASSERT_EQ_INT(73, errno);
}

static void opens_writes_and_installs_hooks(void)
{
   reset_open();
   UT_ASSERT(utt_od_log_open());
   UT_ASSERT_EQ_PTR(ut_stream, logfile_pointer);
   UT_ASSERT_EQ_UINT(1, ut_fprintf_calls);
   UT_ASSERT_EQ_UINT(1, ut_format_calls);
   UT_ASSERT_EQ_UINT(1, ut_write_calls);
   UT_ASSERT_NOT_NULL(pfLogWrite);
   UT_ASSERT_NOT_NULL(pfLogClose);
}

static const UTTestCase ut_cases[] = {
   {"early success", initializes_but_reuses_or_honors_disabled_state},
   {"open failure", reports_file_open_failure},
   {"timestamp failure", closes_and_preserves_errno_for_both_timestamp_failures},
   {"initial output failure", cleans_up_each_initial_output_failure},
   {"success", opens_writes_and_installs_hooks},
   {"terminal session", terminal_session_is_rejected}
};
