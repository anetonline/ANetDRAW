#define UT_CUSTOM_MOCK_ODLogTimeRecordSucceeded
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_fflush
#define UT_CUSTOM_MOCK_fprintf
#define UT_CUSTOM_MOCK_localtime
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_log_open
#define UT_CUSTOM_MOCK_time

#include "errno_mock.h"

#include <stdarg.h>
#include <string.h>

static FILE *ut_stream = (FILE *)1;
static struct tm ut_time_record;
static time_t ut_time_value;
static BOOL ut_localtime_available;
static BOOL ut_time_valid;
static BOOL ut_open_result;
static int ut_fprintf_result;
static int ut_fflush_result;
static int ut_mock_errno;
static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static unsigned ut_open_calls;
static unsigned ut_time_calls;
static unsigned ut_localtime_calls;
static unsigned ut_validate_calls;
static unsigned ut_fprintf_calls;
static unsigned ut_fflush_calls;
static BOOL ut_init_succeeds;

void ODCALL utm_od_init(void)
{
   if(ut_init_succeeds) bODInitialized = TRUE;
   ++ut_init_calls;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

BOOL ODCALL utm_od_log_open(void)
{
   ++ut_open_calls;
   if(ut_open_result) logfile_pointer = ut_stream;
   return ut_open_result;
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
   return ut_time_valid;
}

int utm_fprintf(FILE *stream, const char *format, ...)
{
   va_list arguments;
   va_start(arguments, format);
   UT_ASSERT_EQ_PTR(ut_stream, stream);
   if(ut_time_record.tm_hour < 10)
      UT_ASSERT_EQ_INT(0, strcmp(">  %1.1d:%02d:%02d  %s\n", format));
   else
      UT_ASSERT_EQ_INT(0, strcmp("> %2.2d:%02d:%02d  %s\n", format));
   UT_ASSERT_EQ_INT(ut_time_record.tm_hour, va_arg(arguments, int));
   UT_ASSERT_EQ_INT(ut_time_record.tm_min, va_arg(arguments, int));
   UT_ASSERT_EQ_INT(ut_time_record.tm_sec, va_arg(arguments, int));
   UT_ASSERT_EQ_INT(0, strcmp("message", va_arg(arguments, char *)));
   va_end(arguments);
   ++ut_fprintf_calls;
   errno = ut_mock_errno;
   return ut_fprintf_result;
}

int utm_fflush(FILE *stream)
{
   UT_ASSERT_EQ_PTR(ut_stream, stream);
   ++ut_fflush_calls;
   errno = ut_mock_errno;
   return ut_fflush_result;
}

static void reset_write(void)
{
   memset(&od_control, 0, sizeof(od_control));
   memset(&ut_time_record, 0, sizeof(ut_time_record));
   bODInitialized = TRUE;
   logfile_pointer = ut_stream;
   ut_time_record.tm_hour = 9;
   ut_time_record.tm_min = 8;
   ut_time_record.tm_sec = 7;
   ut_time_value = (time_t)123;
   ut_localtime_available = TRUE;
   ut_time_valid = TRUE;
   ut_open_result = TRUE;
   ut_fprintf_result = 1;
   ut_fflush_result = 0;
   ut_mock_errno = 77;
   ut_init_calls = 0;
   ut_entries = 0;
   ut_exits = 0;
   ut_open_calls = 0;
   ut_time_calls = 0;
   ut_localtime_calls = 0;
   ut_validate_calls = 0;
   ut_fprintf_calls = 0;
   ut_fflush_calls = 0;
   errno = 0;
   ut_init_succeeds = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_write(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT(!utt_od_log_write("message"));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void initializes_and_honors_disabled_logging(void)
{
   reset_write();
   bODInitialized = FALSE;
   od_control.od_logfile_disable = TRUE;
   UT_ASSERT(utt_od_log_write("message"));
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(0, ut_time_calls);
}

static void reports_lazy_open_failure(void)
{
   reset_write();
   logfile_pointer = NULL;
   ut_open_result = FALSE;
   UT_ASSERT(!utt_od_log_write("message"));
   UT_ASSERT_EQ_UINT(1, ut_open_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(0, ut_time_calls);
}

static void rejects_both_invalid_timestamp_forms(void)
{
   reset_write();
   ut_time_value = (time_t)-1;
   ut_localtime_available = FALSE;
   ut_time_valid = FALSE;
   errno = 41;
   UT_ASSERT(!utt_od_log_write("message"));
   UT_ASSERT_EQ_UINT(0, ut_localtime_calls);
   UT_ASSERT_EQ_UINT(1, ut_validate_calls);
   UT_ASSERT_EQ_INT(41, errno);
   UT_ASSERT_EQ_UINT(1, ut_exits);

   reset_write();
   ut_localtime_available = FALSE;
   ut_time_valid = FALSE;
   errno = 42;
   UT_ASSERT(!utt_od_log_write("message"));
   UT_ASSERT_EQ_UINT(1, ut_localtime_calls);
   UT_ASSERT_EQ_INT(42, errno);
}

static void writes_both_hour_formats_after_lazy_open(void)
{
   reset_write();
   logfile_pointer = NULL;
   UT_ASSERT(utt_od_log_write("message"));
   UT_ASSERT_EQ_UINT(1, ut_open_calls);
   UT_ASSERT_EQ_UINT(1, ut_fprintf_calls);
   UT_ASSERT_EQ_UINT(1, ut_fflush_calls);
   UT_ASSERT_EQ_UINT(1, ut_exits);

   reset_write();
   ut_time_record.tm_hour = 10;
   UT_ASSERT(utt_od_log_write("message"));
   UT_ASSERT_EQ_UINT(1, ut_fprintf_calls);
}

static void reports_each_output_failure_and_preserves_errno(void)
{
   reset_write();
   ut_fprintf_result = -1;
   UT_ASSERT(!utt_od_log_write("message"));
   UT_ASSERT_EQ_UINT(0, ut_fflush_calls);
   UT_ASSERT_EQ_INT(77, errno);
   UT_ASSERT_EQ_UINT(1, ut_exits);

   reset_write();
   ut_fflush_result = EOF;
   UT_ASSERT(!utt_od_log_write("message"));
   UT_ASSERT_EQ_UINT(1, ut_fflush_calls);
   UT_ASSERT_EQ_INT(77, errno);
}

static const UTTestCase ut_cases[] = {
   {"disabled", initializes_and_honors_disabled_logging},
   {"lazy open failure", reports_lazy_open_failure},
   {"timestamp failures", rejects_both_invalid_timestamp_forms},
   {"hour formats", writes_both_hour_formats_after_lazy_open},
   {"output failures", reports_each_output_failure_and_preserves_errno},
   {"terminal session", terminal_session_is_rejected}
};
