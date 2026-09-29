#define UT_CUSTOM_MOCK_ODSessionScreenError
#define UT_CUSTOM_MOCK_ODSessionScreenSnapshotSize
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_init

#define MOCK_ENTRY 200
#define MOCK_ERROR 201
#define MOCK_EXIT 202
#define MOCK_INIT 203
#define MOCK_SIZE 204

static DWORD ut_snapshot_size;
static INT ut_screen_error;
static BOOL ut_init_succeeds;

static void reset_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
   bODInitialized = TRUE;
   ut_snapshot_size = 1;
   ut_screen_error = ERR_NONE;
   ut_init_succeeds = TRUE;
}

INT utm_ODSessionScreenError(void)
{
   ut_mock_called(MOCK_ERROR);
   return ut_screen_error;
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
   reset_fixture(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT_EQ_UINT(0, utt_od_save_screen_size());
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_ENTRY));
}

static void available_snapshot_returns_size(void)
{
   reset_fixture();
   bODInitialized = FALSE;
   ut_snapshot_size = 12345;

   UT_ASSERT_EQ_UINT(12345, utt_od_save_screen_size());
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_INIT));
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_ERROR));
   UT_ASSERT_EQ_UINT(MOCK_EXIT, ut_mock_calls[ut_mock_call_count - 1]);
}

static void unavailable_snapshot_preserves_specific_error(void)
{
   reset_fixture();
   ut_snapshot_size = 0;
   ut_screen_error = ERR_MEMORY;

   UT_ASSERT_EQ_UINT(0, utt_od_save_screen_size());
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_ERROR));
}

static void unavailable_snapshot_substitutes_limit_for_no_error(void)
{
   reset_fixture();
   ut_snapshot_size = 0;

   UT_ASSERT_EQ_UINT(0, utt_od_save_screen_size());
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);
}

static const UTTestCase ut_cases[] = {
   {"available snapshot", available_snapshot_returns_size},
   {"specific snapshot error", unavailable_snapshot_preserves_specific_error},
   {"missing snapshot error", unavailable_snapshot_substitutes_limit_for_no_error},
   {"terminal session", terminal_session_is_rejected}
};
