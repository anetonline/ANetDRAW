#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_memcmp
#define UT_CUSTOM_MOCK_ODReserveSnapshotRecords
#define UT_CUSTOM_MOCK_ODReserveReadLiveRecord
#define UT_CUSTOM_MOCK_ODReserveTicketPrecedes

static tODReserveRecordResult ut_record_result;
static DWORD ut_record_count;
static tODReserveLockResult ut_snapshot_result;
static BYTE ut_peer_flags;
static const char *ut_peer_name;
static DWORD ut_peer_low;
static DWORD ut_peer_high;
static BOOL ut_precedes;

size_t utm_strlen(const char *text)
{
   const char *end = text;
   while(*end != '\0') ++end;
   return((size_t)(end - text));
}
int utm_memcmp(const void *left, const void *right, size_t size)
{
   const BYTE *a = (const BYTE *)left;
   const BYTE *b = (const BYTE *)right;
   size_t index;
   for(index = 0; index < size; ++index)
      if(a[index] != b[index]) return((int)a[index] - (int)b[index]);
   return(0);
}
tODReserveLockResult utm_ODReserveSnapshotRecords(DWORD *count,
   tODMilliSec timeout)
{
   UT_ASSERT_EQ_UINT(0, timeout);
   *count = ut_record_count;
   return(ut_snapshot_result);
}
tODReserveRecordResult utm_ODReserveReadLiveRecord(DWORD record,
   BYTE *buffer, tODMilliSec timeout)
{
   size_t length = utm_strlen(ut_peer_name);
   (void)record;
   (void)timeout;
   memset(buffer, 0, OD_RESERVE_RECORD_SIZE);
   buffer[OD_RESERVE_SLOT_NAME_LENGTH] = (BYTE)length;
   memcpy(buffer + OD_RESERVE_SLOT_NAME, ut_peer_name, length);
   buffer[OD_RESERVE_SLOT_FLAGS] = ut_peer_flags;
   OD_RESERVE_PUT_DWORD(buffer + OD_RESERVE_SLOT_TICKET_LOW, ut_peer_low);
   OD_RESERVE_PUT_DWORD(buffer + OD_RESERVE_SLOT_TICKET_HIGH, ut_peer_high);
   return(ut_record_result);
}
BOOL utm_ODReserveTicketPrecedes(DWORD high, DWORD low, DWORD record)
{
   (void)high;
   (void)low;
   (void)record;
   return(ut_precedes);
}

static void reset_wait_core(void)
{
   bODReserveAcquired = FALSE;
   dwODReserveRecord = 1;
   dwODReserveTicketLow = 5;
   dwODReserveTicketHigh = 2;
   strcpy(szODReserveName, "Shared");
   ut_record_count = 2;
   ut_snapshot_result = kODReserveLockAcquired;
   ut_record_result = kODReserveRecordLive;
   ut_peer_flags = OD_RESERVE_FLAG_PRESENT;
   ut_peer_name = "Shared";
   ut_peer_low = 5;
   ut_peer_high = 2;
   ut_precedes = FALSE;
}

static void handles_terminal_record_states(void)
{
   reset_wait_core();
   bODReserveAcquired = TRUE;
   UT_ASSERT_EQ_INT(OD_RESERVE_ACQUIRED, utt_ODReserveWaitCore(0));
   reset_wait_core();
   ut_snapshot_result = kODReserveLockBusy;
   UT_ASSERT_EQ_INT(OD_RESERVE_PENDING, utt_ODReserveWaitCore(0));
   reset_wait_core();
   ut_snapshot_result = kODReserveLockError;
   UT_ASSERT_EQ_INT(OD_RESERVE_ERROR, utt_ODReserveWaitCore(0));
   reset_wait_core();
   ut_record_result = kODReserveRecordBusy;
   UT_ASSERT_EQ_INT(OD_RESERVE_PENDING, utt_ODReserveWaitCore(0));
   reset_wait_core();
   ut_record_result = kODReserveRecordError;
   UT_ASSERT_EQ_INT(OD_RESERVE_ERROR, utt_ODReserveWaitCore(0));
   reset_wait_core();
   ut_record_result = kODReserveRecordStale;
   UT_ASSERT_EQ_INT(OD_RESERVE_ACQUIRED, utt_ODReserveWaitCore(0));
}

static void ignores_self_and_different_names(void)
{
   reset_wait_core();
   dwODReserveRecord = 0;
   ut_record_count = 1;
   UT_ASSERT_EQ_INT(OD_RESERVE_ACQUIRED, utt_ODReserveWaitCore(0));
   reset_wait_core();
   ut_peer_name = "Other";
   UT_ASSERT_EQ_INT(OD_RESERVE_ACQUIRED, utt_ODReserveWaitCore(0));
   reset_wait_core();
   ut_peer_name = "Sharex";
   UT_ASSERT_EQ_INT(OD_RESERVE_ACQUIRED, utt_ODReserveWaitCore(0));
}

static void waits_for_choosing_and_prior_tickets(void)
{
   reset_wait_core();
   ut_peer_flags |= OD_RESERVE_FLAG_CHOOSING;
   UT_ASSERT_EQ_INT(OD_RESERVE_PENDING, utt_ODReserveWaitCore(0));
   reset_wait_core();
   ut_peer_high = 1;
   ut_precedes = TRUE;
   UT_ASSERT_EQ_INT(OD_RESERVE_PENDING, utt_ODReserveWaitCore(0));
   reset_wait_core();
   ut_peer_low = 4;
   ut_precedes = TRUE;
   UT_ASSERT_EQ_INT(OD_RESERVE_PENDING, utt_ODReserveWaitCore(0));
   reset_wait_core();
   dwODReserveRecord = 2;
   ut_precedes = TRUE;
   UT_ASSERT_EQ_INT(OD_RESERVE_PENDING, utt_ODReserveWaitCore(0));
}

static void acquires_after_later_tickets(void)
{
   reset_wait_core();
   ut_peer_high = 3;
   UT_ASSERT_EQ_INT(OD_RESERVE_ACQUIRED, utt_ODReserveWaitCore(0));
   reset_wait_core();
   ut_peer_low = 6;
   UT_ASSERT_EQ_INT(OD_RESERVE_ACQUIRED, utt_ODReserveWaitCore(0));
   reset_wait_core();
   dwODReserveRecord = 0;
   UT_ASSERT_EQ_INT(OD_RESERVE_ACQUIRED, utt_ODReserveWaitCore(0));
}

static const UTTestCase ut_cases[] = {
   {"states", handles_terminal_record_states},
   {"ignore", ignores_self_and_different_names},
   {"prior", waits_for_choosing_and_prior_tickets},
   {"later", acquires_after_later_tickets}
};
