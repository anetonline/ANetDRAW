#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_puttext

static unsigned char ut_window_data[32];
static BOOL ut_put_result;
static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static unsigned ut_frees;
static BOOL ut_init_succeeds;

static void reset_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
   memset(ut_window_data, 0, sizeof(ut_window_data));
   bODInitialized = TRUE;
   ut_put_result = TRUE;
   ut_init_calls = ut_entries = ut_exits = ut_frees = 0;
   ut_init_succeeds = TRUE;
}

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_fixture(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT(!utt_od_window_remove(ut_window_data));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

BOOL ODCALL utm_od_puttext(INT left, INT top, INT right, INT bottom,
   void *contents)
{
   UT_ASSERT_EQ_INT(2, left);
   UT_ASSERT_EQ_INT(3, top);
   UT_ASSERT_EQ_INT(8, right);
   UT_ASSERT_EQ_INT(9, bottom);
   UT_ASSERT_EQ_PTR(ut_window_data + 4, contents);
   return ut_put_result;
}

void utm_free(void *memory)
{
   UT_ASSERT_EQ_PTR(ut_window_data, memory);
   ++ut_frees;
}

static void rejects_null_window_after_initializing(void)
{
   reset_fixture();
   bODInitialized = FALSE;
   UT_ASSERT(!utt_od_window_remove(NULL));
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(0, ut_frees);
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
}

static void frees_window_when_restore_fails(void)
{
   reset_fixture();
   ut_window_data[0] = 2;
   ut_window_data[1] = 3;
   ut_window_data[2] = 8;
   ut_window_data[3] = 9;
   ut_put_result = FALSE;
   UT_ASSERT(!utt_od_window_remove(ut_window_data));
   UT_ASSERT_EQ_UINT(1, ut_frees);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static void restores_and_frees_window(void)
{
   reset_fixture();
   ut_window_data[0] = 2;
   ut_window_data[1] = 3;
   ut_window_data[2] = 8;
   ut_window_data[3] = 9;
   UT_ASSERT(utt_od_window_remove(ut_window_data));
   UT_ASSERT_EQ_UINT(1, ut_frees);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static const UTTestCase ut_cases[] = {
   {"null window", rejects_null_window_after_initializing},
   {"restore failure", frees_window_when_restore_fails},
   {"successful restore", restores_and_frees_window},
   {"terminal session", terminal_session_is_rejected}
};
