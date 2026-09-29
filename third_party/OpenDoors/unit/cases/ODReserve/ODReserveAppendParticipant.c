#define UT_CUSTOM_MOCK_ODReserveAcquire
#define UT_CUSTOM_MOCK_ODReserveRangeUnlock
#define UT_CUSTOM_MOCK_ODReserveTransfer
#define UT_CUSTOM_MOCK_memset

static tODReserveLockResult ut_acquire;
static int ut_transfer_calls;
static int ut_fail_transfer_call;
static long ut_transfer_offsets[3];
static size_t ut_transfer_sizes[3];
static BYTE ut_transfer_first_byte[3];
static BYTE ut_payload[OD_RESERVE_RECORD_SIZE - 2];
static int ut_unlock_calls;

tODReserveLockResult utm_ODReserveAcquire(long offset, long length,
   BOOL write, tODMilliSec timeout)
{
   UT_ASSERT_EQ_INT(OD_RESERVE_HEADER_SIZE
      + (long)dwODReserveRecordCount * OD_RESERVE_RECORD_SIZE, offset);
   UT_ASSERT_EQ_INT(1, length);
   UT_ASSERT(write);
   UT_ASSERT_EQ_UINT(OD_RESERVE_RETRY_TIME, timeout);
   return(ut_acquire);
}

BOOL utm_ODReserveRangeUnlock(int file, long offset, long length)
{
   UT_ASSERT_EQ_INT(hODReserveFile, file);
   UT_ASSERT_EQ_INT(OD_RESERVE_HEADER_SIZE
      + (long)dwODReserveRecord * OD_RESERVE_RECORD_SIZE, offset);
   UT_ASSERT_EQ_INT(1, length);
   ++ut_unlock_calls;
   return(TRUE);
}

BOOL utm_ODReserveTransfer(int file, long offset, BYTE *data,
   size_t size, BOOL write)
{
   size_t index;
   UT_ASSERT_EQ_INT(hODReserveFile, file);
   UT_ASSERT(write);
   if(ut_transfer_calls < 3)
   {
      ut_transfer_offsets[ut_transfer_calls] = offset;
      ut_transfer_sizes[ut_transfer_calls] = size;
      ut_transfer_first_byte[ut_transfer_calls] = data[0];
   }
   if(ut_transfer_calls == 1)
   {
      for(index = 0; index < size; ++index) ut_payload[index] = data[index];
   }
   ++ut_transfer_calls;
   return(ut_transfer_calls != ut_fail_transfer_call);
}

void *utm_memset(void *destination, int value, size_t size)
{
   BYTE *bytes = (BYTE *)destination;
   size_t index;
   for(index = 0; index < size; ++index) bytes[index] = (BYTE)value;
   return(destination);
}

static void reset_append(void)
{
   int index;
   hODReserveFile = 7;
   dwODReserveRecord = 99;
   dwODReserveRecordCount = 2;
   btODReserveKind = OD_RESERVE_KIND_NODE;
   wODReserveNode = 7;
   ut_acquire = kODReserveLockAcquired;
   ut_transfer_calls = 0;
   ut_fail_transfer_call = 0;
   ut_unlock_calls = 0;
   for(index = 0; index < 3; ++index)
   {
      ut_transfer_offsets[index] = -1;
      ut_transfer_sizes[index] = 0;
      ut_transfer_first_byte[index] = 0xff;
   }
}

static void appends_only_to_a_new_owned_record(void)
{
   long base = OD_RESERVE_HEADER_SIZE + 2L * OD_RESERVE_RECORD_SIZE;

   reset_append();
   UT_ASSERT(utt_ODReserveAppendParticipant());
   UT_ASSERT_EQ_UINT(2, dwODReserveRecord);
   UT_ASSERT_EQ_UINT(3, dwODReserveRecordCount);
   UT_ASSERT_EQ_INT(base + OD_RESERVE_RECORD_SIZE - 1,
      ut_transfer_offsets[0]);
   UT_ASSERT_EQ_INT(base + 2, ut_transfer_offsets[1]);
   UT_ASSERT_EQ_INT(base + 1, ut_transfer_offsets[2]);
   UT_ASSERT_EQ_UINT(1, ut_transfer_sizes[0]);
   UT_ASSERT_EQ_UINT(OD_RESERVE_RECORD_SIZE - 2, ut_transfer_sizes[1]);
   UT_ASSERT_EQ_UINT(1, ut_transfer_sizes[2]);
   UT_ASSERT_EQ_INT(0, ut_transfer_first_byte[0]);
   UT_ASSERT_EQ_INT(OD_RESERVE_KIND_NODE,
      ut_payload[OD_RESERVE_SLOT_KIND - 2]);
   UT_ASSERT_EQ_INT(7, OD_RESERVE_GET_WORD(
      ut_payload + OD_RESERVE_SLOT_NODE - 2));
   UT_ASSERT_EQ_INT(OD_RESERVE_FLAG_PRESENT,
      ut_payload[OD_RESERVE_SLOT_FLAGS - 2]);
   UT_ASSERT_EQ_INT(OD_RESERVE_COMMIT, ut_transfer_first_byte[2]);
   UT_ASSERT_EQ_INT(0, ut_unlock_calls);
}

static void reuses_a_preclaimed_identity_record(void)
{
   reset_append();
   dwODReserveRecord = 1;
   UT_ASSERT(utt_ODReserveAppendParticipant());
   UT_ASSERT_EQ_UINT(1, dwODReserveRecord);
   UT_ASSERT_EQ_UINT(2, dwODReserveRecordCount);
   UT_ASSERT_EQ_INT(0, ut_transfer_calls);
}

static void rejects_extension_and_lock_failures(void)
{
   reset_append(); ut_fail_transfer_call = 1;
   UT_ASSERT(!utt_ODReserveAppendParticipant());
   reset_append(); ut_acquire = kODReserveLockBusy;
   UT_ASSERT(!utt_ODReserveAppendParticipant());
   UT_ASSERT_EQ_INT(0, ut_unlock_calls);
}

static void unlocks_after_payload_and_commit_failures(void)
{
   reset_append(); ut_fail_transfer_call = 2;
   UT_ASSERT(!utt_ODReserveAppendParticipant());
   UT_ASSERT_EQ_INT(1, ut_unlock_calls);
   reset_append(); ut_fail_transfer_call = 3;
   UT_ASSERT(!utt_ODReserveAppendParticipant());
   UT_ASSERT_EQ_INT(1, ut_unlock_calls);
}

static const UTTestCase ut_cases[] = {
   {"append", appends_only_to_a_new_owned_record},
   {"reuse", reuses_a_preclaimed_identity_record},
   {"extension", rejects_extension_and_lock_failures},
   {"payload", unlocks_after_payload_and_commit_failures}
};
