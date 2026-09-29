#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed
#define UT_CUSTOM_MOCK_ODSyncAPIIsNested
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_ODReserveNameValid
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_strcpy
#define UT_CUSTOM_MOCK_memcmp
#define UT_CUSTOM_MOCK_ODReserveWriteOwn
#define UT_CUSTOM_MOCK_ODReserveSnapshotRecords
#define UT_CUSTOM_MOCK_ODReserveReadLiveRecord
#define UT_CUSTOM_MOCK_ODReserveTicketGreater
#define UT_CUSTOM_MOCK_ODReserveDetach

static BOOL ut_allowed;
static BOOL ut_name_valid;
static BOOL ut_init_succeeds;
static int ut_entry_calls;
static int ut_exit_calls;
static int ut_write_calls;
static int ut_write_failure_call;
static int ut_detach_calls;
static tODReserveLockResult ut_snapshot;
static DWORD ut_count;
static tODReserveRecordResult ut_record_result;
static const char *ut_peer_name;
static DWORD ut_peer_low;
static DWORD ut_peer_high;
static BOOL ut_ticket_greater;

BOOL utm_ODSyncPublicCallAllowed(void) { return(ut_allowed); }
BOOL utm_ODSyncAPIIsNested(void) { return(FALSE); }
void utm_ODSyncAPIEntry(void) { ++ut_entry_calls; }
void utm_ODSyncAPIExit(void) { ++ut_exit_calls; }
void ODCALL utm_od_init(void)
{ if(ut_init_succeeds) bODInitialized = TRUE; }
BOOL utm_ODReserveNameValid(const char *name)
{ (void)name; return(ut_name_valid); }
size_t utm_strlen(const char *text)
{
   const char *end = text;
   while(*end != '\0') ++end;
   return((size_t)(end - text));
}
char *utm_strcpy(char *destination, const char *source)
{
   char *result = destination;
   while((*destination++ = *source++) != '\0') { }
   return(result);
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
BOOL utm_ODReserveWriteOwn(BYTE flags, const char *name,
   DWORD low, DWORD high)
{
   (void)flags;
   (void)name;
   (void)low;
   (void)high;
   ++ut_write_calls;
   return(ut_write_failure_call == 0
      || ut_write_calls != ut_write_failure_call);
}
tODReserveLockResult utm_ODReserveSnapshotRecords(DWORD *count,
   tODMilliSec timeout)
{
   UT_ASSERT_EQ_UINT(OD_RESERVE_RETRY_TIME, timeout);
   *count = ut_count;
   return(ut_snapshot);
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
   OD_RESERVE_PUT_DWORD(buffer + OD_RESERVE_SLOT_TICKET_LOW, ut_peer_low);
   OD_RESERVE_PUT_DWORD(buffer + OD_RESERVE_SLOT_TICKET_HIGH, ut_peer_high);
   return(ut_record_result);
}
BOOL utm_ODReserveTicketGreater(DWORD left_high, DWORD left_low,
   DWORD right_high, DWORD right_low)
{
   (void)left_high;
   (void)left_low;
   (void)right_high;
   (void)right_low;
   return(ut_ticket_greater);
}
void utm_ODReserveDetach(void) { ++ut_detach_calls; }

static void reset_request(void)
{
   memset(&od_control, 0, sizeof(od_control));
   pszODReservePath = (char *)"NODE.SYN";
   bODInitialized = TRUE;
   eODLifecycleState = kODLifecycleActive;
   bODReserveActive = TRUE;
   bODReserveDetached = FALSE;
   bODReserveRequested = FALSE;
   dwODReserveRecord = 0;
   ut_allowed = TRUE;
   ut_name_valid = TRUE;
   ut_init_succeeds = TRUE;
   ut_entry_calls = 0;
   ut_exit_calls = 0;
   ut_write_calls = 0;
   ut_write_failure_call = 0;
   ut_detach_calls = 0;
   ut_snapshot = kODReserveLockAcquired;
   ut_count = 1;
   ut_record_result = kODReserveRecordStale;
   ut_peer_name = "Shared";
   ut_peer_low = 0;
   ut_peer_high = 0;
   ut_ticket_greater = FALSE;
}

static void rejects_invalid_lifecycle_and_state(void)
{
   reset_request();
   ut_allowed = FALSE;
   UT_ASSERT(!utt_od_reserve_request("Shared"));
   reset_request();
   pszODReservePath = NULL;
   UT_ASSERT(!utt_od_reserve_request("Shared"));
   reset_request();
   ut_name_valid = FALSE;
   UT_ASSERT(!utt_od_reserve_request("Shared"));
   reset_request();
   bODInitialized = FALSE;
   eODLifecycleState = kODLifecycleTerminal;
   ut_init_succeeds = FALSE;
   UT_ASSERT(!utt_od_reserve_request("Shared"));
   reset_request();
   bODReserveActive = FALSE;
   UT_ASSERT(!utt_od_reserve_request("Shared"));
   reset_request();
   bODReserveDetached = TRUE;
   UT_ASSERT(!utt_od_reserve_request("Shared"));
   reset_request();
   bODReserveRequested = TRUE;
   UT_ASSERT(!utt_od_reserve_request("Shared"));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
}

static void publishes_a_new_ticket(void)
{
   reset_request();
   bODInitialized = FALSE;
   UT_ASSERT(utt_od_reserve_request("Shared"));
   UT_ASSERT(bODReserveRequested);
   UT_ASSERT_EQ_UINT(1, dwODReserveTicketLow);
   UT_ASSERT_EQ_UINT(0, dwODReserveTicketHigh);
   UT_ASSERT_EQ_INT(2, ut_write_calls);

   reset_request();
   ut_count = 2;
   ut_record_result = kODReserveRecordStale;
   UT_ASSERT(utt_od_reserve_request("Shared"));

   reset_request();
   ut_count = 2;
   ut_record_result = kODReserveRecordLive;
   ut_peer_name = "Other";
   UT_ASSERT(utt_od_reserve_request("Shared"));

   reset_request();
   ut_count = 2;
   ut_record_result = kODReserveRecordLive;
   ut_peer_name = "Sharex";
   UT_ASSERT(utt_od_reserve_request("Shared"));

   reset_request();
   ut_count = 2;
   ut_record_result = kODReserveRecordLive;
   ut_peer_name = "Shared";
   ut_ticket_greater = FALSE;
   UT_ASSERT(utt_od_reserve_request("Shared"));

   reset_request();
   ut_count = 2;
   ut_record_result = kODReserveRecordLive;
   ut_peer_name = "Shared";
   ut_peer_low = 0xffffffffUL;
   ut_peer_high = 5;
   ut_ticket_greater = TRUE;
   UT_ASSERT(utt_od_reserve_request("Shared"));
   UT_ASSERT_EQ_UINT(0, dwODReserveTicketLow);
   UT_ASSERT_EQ_UINT(6, dwODReserveTicketHigh);
}

static void detaches_after_registry_failures(void)
{
   reset_request();
   ut_write_failure_call = 1;
   UT_ASSERT(!utt_od_reserve_request("Shared"));
   UT_ASSERT_EQ_INT(1, ut_detach_calls);
   reset_request();
   ut_snapshot = kODReserveLockError;
   UT_ASSERT(!utt_od_reserve_request("Shared"));
   UT_ASSERT_EQ_INT(1, ut_detach_calls);
   reset_request();
   ut_count = 2;
   ut_record_result = kODReserveRecordError;
   UT_ASSERT(!utt_od_reserve_request("Shared"));
   reset_request();
   ut_write_failure_call = 2;
   UT_ASSERT(!utt_od_reserve_request("Shared"));
}

static void rejects_ticket_space_exhaustion(void)
{
   reset_request();
   ut_count = 2;
   ut_record_result = kODReserveRecordLive;
   ut_peer_low = 0xffffffffUL;
   ut_peer_high = 0xffffffffUL;
   ut_ticket_greater = TRUE;
   UT_ASSERT(!utt_od_reserve_request("Shared"));
   UT_ASSERT_EQ_INT(1, ut_detach_calls);
}

static const UTTestCase ut_cases[] = {
   {"reject", rejects_invalid_lifecycle_and_state},
   {"publish", publishes_a_new_ticket},
   {"failures", detaches_after_registry_failures},
   {"exhaustion", rejects_ticket_space_exhaustion}
};
