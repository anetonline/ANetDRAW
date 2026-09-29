#define UT_CUSTOM_MOCK_ODCoreSendRemoteBuffer
#define UT_CUSTOM_MOCK_ODScrnDisplayBuffer
#define UT_CUSTOM_MOCK_ODScrnDisplayString
#define UT_CUSTOM_MOCK_ODSessionScreenAvailable
#define UT_CUSTOM_MOCK_ODSessionScreenDisplayBuffer
#define UT_CUSTOM_MOCK_ODSessionScreenDisplayString
#define UT_CUSTOM_MOCK_ODSessionScreenPresent
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_ODTimerElapsed
#define UT_CUSTOM_MOCK_od_kernel

static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static unsigned ut_remote_calls;
static const char *ut_remote_buffer;
static INT ut_remote_size;
static BOOL ut_session_available;
static unsigned ut_session_buffer_calls;
static unsigned ut_session_string_calls;
static unsigned ut_present_calls;
static unsigned ut_local_buffer_calls;
static unsigned ut_local_string_calls;
static BOOL ut_timer_elapsed;
static unsigned ut_timer_calls;
static unsigned ut_kernel_calls;
static BOOL ut_init_succeeds;

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

tODResult utm_ODCoreSendRemoteBuffer(const void *buffer, INT size)
{
   ++ut_remote_calls;
   ut_remote_buffer = buffer;
   ut_remote_size = size;
   return kODRCSuccess;
}

BOOL utm_ODSessionScreenAvailable(void) { return ut_session_available; }

void utm_ODSessionScreenDisplayBuffer(const char *buffer, INT size)
{
   UT_ASSERT_EQ_PTR(ut_remote_buffer, buffer);
   UT_ASSERT_EQ_INT(ut_remote_size, size);
   ++ut_session_buffer_calls;
}

void utm_ODSessionScreenDisplayString(const char *text)
{
   UT_ASSERT_EQ_PTR(ut_remote_buffer, text);
   ++ut_session_string_calls;
}

void utm_ODSessionScreenPresent(void) { ++ut_present_calls; }

void ODCALL utm_ODScrnDisplayBuffer(const char *buffer, INT size)
{
   UT_ASSERT_EQ_PTR(ut_remote_buffer, buffer);
   UT_ASSERT_EQ_INT(ut_remote_size, size);
   ++ut_local_buffer_calls;
}

void ODCALL utm_ODScrnDisplayString(const char *text)
{
   UT_ASSERT_EQ_PTR(ut_remote_buffer, text);
   ++ut_local_string_calls;
}

BOOL utm_ODTimerElapsed(tODTimer *timer)
{
   (void)timer;
   ++ut_timer_calls;
   return ut_timer_elapsed;
}

void ODCALL utm_od_kernel(void) { ++ut_kernel_calls; }

static void ut_reset_display(void)
{
   bODInitialized = TRUE;
   od_control.baud = 0;
   ut_init_calls = 0;
   ut_entries = 0;
   ut_exits = 0;
   ut_remote_calls = 0;
   ut_remote_buffer = NULL;
   ut_remote_size = 0;
   ut_session_available = FALSE;
   ut_session_buffer_calls = 0;
   ut_session_string_calls = 0;
   ut_present_calls = 0;
   ut_local_buffer_calls = 0;
   ut_local_string_calls = 0;
   ut_timer_elapsed = FALSE;
   ut_timer_calls = 0;
   ut_kernel_calls = 0;
   ut_init_succeeds = TRUE;
}
