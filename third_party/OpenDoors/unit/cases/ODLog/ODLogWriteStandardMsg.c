#define UT_CUSTOM_MOCK_ODLogFormatWorkString
#define UT_CUSTOM_MOCK_od_log_write

#include <stdarg.h>
#include <string.h>

static BOOL ut_first_write_result;
static BOOL ut_second_write_result;
static BOOL ut_format_result;
static const char *ut_expected_first_message;
static unsigned ut_write_calls;
static unsigned ut_format_calls;

BOOL utm_ODLogFormatWorkString(const char *format, ...)
{
   va_list arguments;
   va_start(arguments, format);
   UT_ASSERT_EQ_PTR(od_control.od_logfile_messages[12], format);
   UT_ASSERT_EQ_PTR(od_control.user_reasonforchat,
      va_arg(arguments, char *));
   va_end(arguments);
   strcpy(szODWorkString,
      "123456789012345678901234567890123456789012345678901234567890123456789");
   ++ut_format_calls;
   return ut_format_result;
}

BOOL ODCALL utm_od_log_write(const char *message)
{
   ++ut_write_calls;
   if(ut_write_calls == 1)
   {
      UT_ASSERT_EQ_PTR(ut_expected_first_message, message);
      return ut_first_write_result;
   }
   UT_ASSERT_EQ_UINT(2, ut_write_calls);
   UT_ASSERT_EQ_PTR(szODWorkString, message);
   UT_ASSERT_EQ_INT('\0', szODWorkString[67]);
   return ut_second_write_result;
}

static void reset_standard_message(void)
{
   memset(&od_control, 0, sizeof(od_control));
   od_control.od_logfile_messages[8] = "chat";
   od_control.od_logfile_messages[12] = "reason: %s";
   strcpy(od_control.user_reasonforchat, "testing");
   ut_first_write_result = TRUE;
   ut_second_write_result = TRUE;
   ut_format_result = TRUE;
   ut_expected_first_message = od_control.od_logfile_messages[8];
   ut_write_calls = 0;
   ut_format_calls = 0;
}

static void rejects_indices_outside_message_table(void)
{
   reset_standard_message();
   UT_ASSERT(!utt_ODLogWriteStandardMsg(-1));
   UT_ASSERT(!utt_ODLogWriteStandardMsg(12));
   UT_ASSERT_EQ_UINT(0, ut_write_calls);
}

static void returns_first_write_result_for_ordinary_message(void)
{
   reset_standard_message();
   ut_first_write_result = FALSE;
   UT_ASSERT(!utt_ODLogWriteStandardMsg(8));
   UT_ASSERT_EQ_UINT(1, ut_write_calls);
   UT_ASSERT_EQ_UINT(0, ut_format_calls);

   reset_standard_message();
   od_control.od_logfile_messages[7] = "ordinary";
   ut_expected_first_message = od_control.od_logfile_messages[7];
   UT_ASSERT(utt_ODLogWriteStandardMsg(7));
   UT_ASSERT_EQ_UINT(1, ut_write_calls);
   UT_ASSERT_EQ_UINT(0, ut_format_calls);
}

static void writes_truncated_chat_reason(void)
{
   reset_standard_message();
   ut_format_result = FALSE;
   UT_ASSERT(!utt_ODLogWriteStandardMsg(8));
   UT_ASSERT_EQ_UINT(1, ut_write_calls);
   UT_ASSERT_EQ_UINT(1, ut_format_calls);

   reset_standard_message();
   ut_second_write_result = FALSE;
   UT_ASSERT(!utt_ODLogWriteStandardMsg(8));
   UT_ASSERT_EQ_UINT(2, ut_write_calls);

   reset_standard_message();
   UT_ASSERT(utt_ODLogWriteStandardMsg(8));
   UT_ASSERT_EQ_UINT(2, ut_write_calls);
}

static const UTTestCase ut_cases[] = {
   {"invalid index", rejects_indices_outside_message_table},
   {"ordinary write", returns_first_write_result_for_ordinary_message},
   {"chat reason", writes_truncated_chat_reason}
};
