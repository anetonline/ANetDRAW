#include <stdarg.h>

#define UT_CUSTOM_MOCK_time
#define UT_CUSTOM_MOCK_ODInQueueGetLastActivity
#define UT_CUSTOM_MOCK_ODInQueueResetLastActivity
#define UT_CUSTOM_MOCK_ODKrnlDeliverTimeMessage
#define UT_CUSTOM_MOCK_sprintf

static time_t ut_times[8];
static unsigned ut_time_index;
static unsigned ut_time_count;
static time_t ut_last_activity;
static unsigned ut_reset_calls;
static unsigned ut_delivery_calls;
static BOOL ut_delivery_result;
static BYTE ut_delivery_reason;
static BOOL ut_delivery_allows_callbacks;
static char ut_delivery_text[80];

time_t utm_time(time_t *result)
{
   time_t value;
   UT_ASSERT(ut_time_index < ut_time_count);
   value = ut_times[ut_time_index++];
   if(result != NULL)
      *result = value;
   return(value);
}
time_t utm_ODInQueueGetLastActivity(tODInQueueHandle queue)
{
   UT_ASSERT(queue == hODInputQueue);
   return(ut_last_activity);
}
void utm_ODInQueueResetLastActivity(tODInQueueHandle queue)
{
   ++ut_reset_calls;
   UT_ASSERT(queue == hODInputQueue);
}
BOOL utm_ODKrnlDeliverTimeMessage(char *message, BYTE reason,
   BOOL allow_callbacks)
{
   ++ut_delivery_calls;
   strncpy(ut_delivery_text, message, sizeof(ut_delivery_text) - 1);
   ut_delivery_text[sizeof(ut_delivery_text) - 1] = '\0';
   ut_delivery_reason = reason;
   ut_delivery_allows_callbacks = allow_callbacks;
   return(ut_delivery_result);
}
int utm_sprintf(char *buffer, const char *format, ...)
{
   va_list arguments;
   int minutes;
   va_start(arguments, format);
   minutes = va_arg(arguments, int);
   va_end(arguments);
   UT_ASSERT(strcmp("%d minutes", format) == 0);
   UT_ASSERT_EQ_INT(3, minutes);
   strcpy(buffer, "3 minutes");
   return(9);
}

static void reset_time_update(void)
{
   memset(&od_control, 0, sizeof(od_control));
   hODInputQueue = (tODInQueueHandle)1;
   ut_times[0] = 100;
   ut_time_index = 0;
   ut_time_count = 1;
   ut_last_activity = 100;
   ut_reset_calls = 0;
   ut_delivery_calls = 0;
   ut_delivery_result = FALSE;
   ut_delivery_reason = 0;
   ut_delivery_allows_callbacks = FALSE;
   ut_delivery_text[0] = '\0';
   nLastInactivitySetting = 0;
   bWarnedAboutInactivity = FALSE;
   nNextTimeDeductTime = 200;
   bForceStatusUpdate = FALSE;
   bODInitialized = TRUE;
   od_control.user_timelimit = 10;
   od_control.od_inactivity_timeout = (char *)"inactive";
   od_control.od_inactivity_warning = (char *)"warning";
   od_control.od_time_warning = (char *)"%d minutes";
   od_control.od_no_time = (char *)"no time";
}

static void tracks_inactivity_setting_changes(void)
{
   reset_time_update();
   od_control.od_inactivity = 10;
   ut_last_activity = 100;
   UT_ASSERT(!utt_ODKrnlTimeUpdate(TRUE));
   UT_ASSERT_EQ_UINT(1, ut_reset_calls);
   UT_ASSERT_EQ_INT(10, nLastInactivitySetting);

   reset_time_update();
   nLastInactivitySetting = 5;
   od_control.od_inactivity = 10;
   UT_ASSERT(!utt_ODKrnlTimeUpdate(TRUE));
   UT_ASSERT_EQ_UINT(0, ut_reset_calls);

   reset_time_update();
   nLastInactivitySetting = 10;
   od_control.od_inactivity = 10;
   UT_ASSERT(!utt_ODKrnlTimeUpdate(TRUE));
   UT_ASSERT_EQ_UINT(0, ut_reset_calls);
}

static void enforces_or_suppresses_inactivity_timeout(void)
{
   reset_time_update();
   nLastInactivitySetting = od_control.od_inactivity = 10;
   ut_last_activity = 0;
   UT_ASSERT(utt_ODKrnlTimeUpdate(TRUE));
   UT_ASSERT_EQ_UINT(1, ut_delivery_calls);
   UT_ASSERT_EQ_UINT(ERRORLEVEL_INACTIVITY, ut_delivery_reason);
   UT_ASSERT(ut_delivery_allows_callbacks);

   reset_time_update();
   ut_last_activity = 0;
   od_control.od_inactivity = 0;
   UT_ASSERT(!utt_ODKrnlTimeUpdate(FALSE));
   UT_ASSERT_EQ_UINT(0, ut_delivery_calls);

   reset_time_update();
   nLastInactivitySetting = od_control.od_inactivity = 10;
   od_control.od_disable_inactivity = TRUE;
   ut_last_activity = 0;
   UT_ASSERT(!utt_ODKrnlTimeUpdate(FALSE));
   UT_ASSERT_EQ_UINT(0, ut_delivery_calls);
}

static void warns_once_and_propagates_a_callback_stop(void)
{
   reset_time_update();
   nLastInactivitySetting = od_control.od_inactivity = 10;
   od_control.od_inactive_warning = 5;
   ut_last_activity = 92;
   UT_ASSERT(!utt_ODKrnlTimeUpdate(FALSE));
   UT_ASSERT_EQ_UINT(1, ut_delivery_calls);
   UT_ASSERT(strcmp("warning", ut_delivery_text) == 0);
   UT_ASSERT(bWarnedAboutInactivity);

   ut_time_index = 0;
   ut_delivery_calls = 0;
   UT_ASSERT(!utt_ODKrnlTimeUpdate(FALSE));
   UT_ASSERT_EQ_UINT(0, ut_delivery_calls);

   reset_time_update();
   od_control.od_inactivity = 0;
   od_control.od_inactive_warning = 5;
   ut_last_activity = 100;
   UT_ASSERT(!utt_ODKrnlTimeUpdate(FALSE));
   UT_ASSERT_EQ_UINT(0, ut_delivery_calls);

   reset_time_update();
   nLastInactivitySetting = od_control.od_inactivity = 10;
   od_control.od_inactive_warning = 5;
   od_control.od_disable_inactivity = TRUE;
   ut_last_activity = 92;
   UT_ASSERT(!utt_ODKrnlTimeUpdate(FALSE));
   UT_ASSERT_EQ_UINT(0, ut_delivery_calls);

   reset_time_update();
   nLastInactivitySetting = od_control.od_inactivity = 10;
   od_control.od_inactive_warning = 5;
   ut_last_activity = 92;
   ut_delivery_result = TRUE;
   UT_ASSERT(utt_ODKrnlTimeUpdate(TRUE));

   reset_time_update();
   bWarnedAboutInactivity = TRUE;
   ut_last_activity = 100;
   UT_ASSERT(!utt_ODKrnlTimeUpdate(FALSE));
   UT_ASSERT(!bWarnedAboutInactivity);
}

static void preserves_time_during_chat(void)
{
   reset_time_update();
   od_control.od_chat_active = TRUE;
   ut_times[1] = 125;
   ut_time_count = 2;
   UT_ASSERT(!utt_ODKrnlTimeUpdate(FALSE));
   UT_ASSERT_EQ_INT((time_t)185, nNextTimeDeductTime);

   reset_time_update();
   od_control.od_chat_active = FALSE;
   UT_ASSERT(!utt_ODKrnlTimeUpdate(FALSE));
   UT_ASSERT_EQ_INT((time_t)200, nNextTimeDeductTime);
}

static void deducts_elapsed_minutes_and_covers_warning_policy(void)
{
   reset_time_update();
   nNextTimeDeductTime = 100;
   od_control.user_timelimit = 5;
   UT_ASSERT(!utt_ODKrnlTimeUpdate(FALSE));
   UT_ASSERT_EQ_INT(4, od_control.user_timelimit);
   UT_ASSERT_EQ_UINT(0, ut_delivery_calls);
   UT_ASSERT(bForceStatusUpdate);

   reset_time_update();
   nNextTimeDeductTime = 100;
   od_control.user_timelimit = 4;
   UT_ASSERT(!utt_ODKrnlTimeUpdate(FALSE));
   UT_ASSERT_EQ_INT(3, od_control.user_timelimit);
   UT_ASSERT_EQ_UINT(1, ut_delivery_calls);
   UT_ASSERT(strcmp("3 minutes", ut_delivery_text) == 0);

   reset_time_update();
   nNextTimeDeductTime = 100;
   od_control.user_timelimit = 1;
   UT_ASSERT(utt_ODKrnlTimeUpdate(FALSE));
   UT_ASSERT_EQ_UINT(ERRORLEVEL_TIMEOUT, ut_delivery_reason);

   reset_time_update();
   nNextTimeDeductTime = 100;
   od_control.user_timelimit = 4;
   od_control.od_disable = DIS_TIMEOUT;
   UT_ASSERT(!utt_ODKrnlTimeUpdate(FALSE));
   UT_ASSERT_EQ_UINT(0, ut_delivery_calls);

   reset_time_update();
   nNextTimeDeductTime = 100;
   od_control.user_timelimit = 4;
   ut_delivery_result = TRUE;
   UT_ASSERT(utt_ODKrnlTimeUpdate(TRUE));
   UT_ASSERT_EQ_UINT(1, ut_delivery_calls);
}

static void stops_deduction_when_the_library_is_no_longer_initialized(void)
{
   reset_time_update();
   nNextTimeDeductTime = 100;
   bODInitialized = FALSE;
   UT_ASSERT(!utt_ODKrnlTimeUpdate(FALSE));
   UT_ASSERT_EQ_INT(10, od_control.user_timelimit);
}

static void applies_final_time_limit_policy(void)
{
   reset_time_update();
   od_control.user_timelimit = 1;
   UT_ASSERT(!utt_ODKrnlTimeUpdate(FALSE));

   reset_time_update();
   od_control.user_timelimit = 0;
   od_control.od_disable = DIS_TIMEOUT;
   UT_ASSERT(!utt_ODKrnlTimeUpdate(FALSE));

   reset_time_update();
   od_control.user_timelimit = 0;
   UT_ASSERT(utt_ODKrnlTimeUpdate(TRUE));
   UT_ASSERT_EQ_UINT(1, ut_delivery_calls);
   UT_ASSERT_EQ_UINT(ERRORLEVEL_TIMEOUT, ut_delivery_reason);
   UT_ASSERT(strcmp("no time", ut_delivery_text) == 0);
}

static const UTTestCase ut_cases[] = {
   {"setting", tracks_inactivity_setting_changes},
   {"inactivity timeout", enforces_or_suppresses_inactivity_timeout},
   {"inactivity warning", warns_once_and_propagates_a_callback_stop},
   {"chat", preserves_time_during_chat},
   {"deduction", deducts_elapsed_minutes_and_covers_warning_policy},
   {"not initialized", stops_deduction_when_the_library_is_no_longer_initialized},
   {"final timeout", applies_final_time_limit_policy}
};
