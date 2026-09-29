#define UT_CUSTOM_MOCK_ODSessionScreenError
#define UT_CUSTOM_MOCK_ODSessionScreenSave
#define UT_CUSTOM_MOCK_ODSessionScreenSnapshotSize
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_init

#define MOCK_ENTRY 220
#define MOCK_ERROR 221
#define MOCK_EXIT 222
#define MOCK_INIT 223
#define MOCK_SAVE 224
#define MOCK_SIZE 225

static BOOL ut_save_result;
static DWORD ut_snapshot_size;
static INT ut_screen_error;
static void *ut_expected_buffer;
static BOOL ut_init_succeeds;

static void reset_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
   bODInitialized = TRUE;
   ut_save_result = TRUE;
   ut_snapshot_size = 64;
   ut_screen_error = ERR_NONE;
   ut_init_succeeds = TRUE;
}

INT utm_ODSessionScreenError(void)
{
   ut_mock_called(MOCK_ERROR);
   return ut_screen_error;
}

BOOL utm_ODSessionScreenSave(void *buffer, DWORD size)
{
   UT_ASSERT_EQ_PTR(ut_expected_buffer, buffer);
   UT_ASSERT_EQ_UINT(64, size);
   ut_mock_called(MOCK_SAVE);
   return ut_save_result;
}

DWORD utm_ODSessionScreenSnapshotSize(void)
{
   ut_mock_called(MOCK_SIZE);
   return ut_snapshot_size;
}

void utm_ODSyncAPIEntry(void) { ut_mock_called(MOCK_ENTRY); }
void utm_ODSyncAPIExit(void) { ut_mock_called(MOCK_EXIT); }
void ODCALL utm_od_init(void)
{
   ut_mock_called(MOCK_INIT);
   if(ut_init_succeeds) bODInitialized = TRUE;
}

static void terminal_session_is_rejected(void)
{
   char buffer[64];
   reset_fixture(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_save_screen_ex(buffer, sizeof(buffer)));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_ENTRY));
}

static void successful_save_forwards_buffer_and_size(void)
{
   char buffer[64];
   reset_fixture();
   bODInitialized = FALSE;
   ut_expected_buffer = buffer;

   UT_ASSERT_EQ_INT(TRUE, utt_od_save_screen_ex(buffer, sizeof(buffer)));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_INIT));
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_SIZE));
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_ERROR));
}

static void ordinary_save_failure_reports_parameter_error(void)
{
   char buffer[64];
   reset_fixture();
   ut_expected_buffer = buffer;
   ut_save_result = FALSE;

   UT_ASSERT_EQ_INT(FALSE, utt_od_save_screen_ex(buffer, sizeof(buffer)));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_ERROR));
}

static void unavailable_snapshot_preserves_specific_error(void)
{
   char buffer[64];
   reset_fixture();
   ut_expected_buffer = buffer;
   ut_save_result = FALSE;
   ut_snapshot_size = 0;
   ut_screen_error = ERR_MEMORY;

   UT_ASSERT_EQ_INT(FALSE, utt_od_save_screen_ex(buffer, sizeof(buffer)));
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);
}

static void unavailable_snapshot_substitutes_limit_for_no_error(void)
{
   char buffer[64];
   reset_fixture();
   ut_expected_buffer = buffer;
   ut_save_result = FALSE;
   ut_snapshot_size = 0;

   UT_ASSERT_EQ_INT(FALSE, utt_od_save_screen_ex(buffer, sizeof(buffer)));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);
}

static const UTTestCase ut_cases[] = {
   {"successful save", successful_save_forwards_buffer_and_size},
   {"parameter failure", ordinary_save_failure_reports_parameter_error},
   {"specific snapshot error", unavailable_snapshot_preserves_specific_error},
   {"missing snapshot error", unavailable_snapshot_substitutes_limit_for_no_error},
   {"terminal session", terminal_session_is_rejected}
};
