#define UT_CUSTOM_MOCK_ODReserveCountRecords
#define UT_CUSTOM_MOCK_ODReserveAccess
#define UT_CUSTOM_MOCK_ODReserveAcquire
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK__lseek
#else
#define UT_CUSTOM_MOCK_lseek
#endif

static BOOL ut_count_result;
static DWORD ut_count;
static tODReserveLockResult ut_access_result;
static tODReserveLockResult ut_acquire_result;
static BYTE ut_commit;
static BYTE ut_kind;
static WORD ut_node;
static BYTE ut_name_length;
static BYTE ut_flags;
static int ut_acquire_calls;
static int ut_access_calls;
static long ut_length;

BOOL utm_ODReserveCountRecords(DWORD *count)
{ *count = ut_count; return(ut_count_result); }

tODReserveLockResult utm_ODReserveAccess(long offset, BYTE *buffer,
   size_t size, BOOL write, BOOL exclusive, tODMilliSec timeout)
{
   size_t index;

   UT_ASSERT_EQ_INT(OD_RESERVE_HEADER_SIZE + 1L
      + (long)ut_access_calls * OD_RESERVE_RECORD_SIZE, offset);
   UT_ASSERT_EQ_UINT(OD_RESERVE_RECORD_SIZE - 1, size);
   UT_ASSERT(!write);
   UT_ASSERT(!exclusive);
   UT_ASSERT_EQ_UINT(OD_RESERVE_RETRY_TIME, timeout);
   for(index = 0; index < size; ++index) buffer[index] = 0;
   buffer[OD_RESERVE_SLOT_COMMIT - 1] = ut_commit;
   buffer[OD_RESERVE_SLOT_KIND - 1] = ut_kind;
   OD_RESERVE_PUT_WORD(buffer + OD_RESERVE_SLOT_NODE - 1, ut_node);
   buffer[OD_RESERVE_SLOT_NAME_LENGTH - 1] = ut_name_length;
   buffer[OD_RESERVE_SLOT_FLAGS - 1] = ut_flags;
   ++ut_access_calls;
   return(ut_access_result);
}

tODReserveLockResult utm_ODReserveAcquire(long offset, long length,
   BOOL write, tODMilliSec timeout)
{
   UT_ASSERT_EQ_INT(OD_RESERVE_HEADER_SIZE, offset);
   UT_ASSERT_EQ_INT(1, length);
   UT_ASSERT(write);
   UT_ASSERT_EQ_UINT(OD_RESERVE_RETRY_TIME, timeout);
   ++ut_acquire_calls;
   return(ut_acquire_result);
}

#if defined(ODPLAT_WIN32)
long utm__lseek(int file, long offset, int whence)
#elif defined(ODPLAT_NIX)
off_t utm_lseek(int file, off_t offset, int whence)
#else
long utm_lseek(int file, long offset, int whence)
#endif
{
   UT_ASSERT_EQ_INT(hODReserveFile, file);
   UT_ASSERT_EQ_INT(0, (int)offset);
   UT_ASSERT_EQ_INT(SEEK_END, whence);
   return(ut_length);
}

static void reset_identity_check(void)
{
   memset(&od_control, 0, sizeof(od_control));
   btODReserveKind = OD_RESERVE_KIND_NODE;
   wODReserveNode = 3;
   dwODReserveRecord = 99;
   ut_count_result = TRUE;
   ut_count = 1;
   ut_access_result = kODReserveLockAcquired;
   ut_acquire_result = kODReserveLockAcquired;
   ut_commit = OD_RESERVE_COMMIT;
   ut_kind = OD_RESERVE_KIND_NODE;
   ut_node = 4;
   ut_name_length = 0;
   ut_flags = OD_RESERVE_FLAG_PRESENT;
   ut_acquire_calls = 0;
   ut_access_calls = 0;
   ut_length = OD_RESERVE_HEADER_SIZE + OD_RESERVE_RECORD_SIZE;
}

static void selects_append_slot_for_new_identity(void)
{
   reset_identity_check();
   ut_count = 0;
   ut_length = OD_RESERVE_HEADER_SIZE;
   UT_ASSERT(utt_ODReserveCheckIdentities());
   UT_ASSERT_EQ_UINT(0, dwODReserveRecord);

   reset_identity_check();
   UT_ASSERT(utt_ODReserveCheckIdentities());
   UT_ASSERT_EQ_UINT(1, dwODReserveRecord);
   UT_ASSERT_EQ_INT(0, ut_acquire_calls);

   reset_identity_check();
   ut_commit = 0;
   ut_kind = OD_RESERVE_KIND_NODE;
   ut_node = 3;
   UT_ASSERT(utt_ODReserveCheckIdentities());
   UT_ASSERT_EQ_UINT(1, dwODReserveRecord);
   UT_ASSERT_EQ_INT(0, ut_acquire_calls);

   reset_identity_check();
   ut_length = OD_RESERVE_HEADER_SIZE + 17;
   UT_ASSERT(utt_ODReserveCheckIdentities());
   UT_ASSERT_EQ_UINT(1, dwODReserveRecord);
   UT_ASSERT_EQ_INT(0, ut_access_calls);
}

static void claims_stale_record_for_same_identity(void)
{
   reset_identity_check();
   ut_node = 3;
   UT_ASSERT(utt_ODReserveCheckIdentities());
   UT_ASSERT_EQ_UINT(0, dwODReserveRecord);
   UT_ASSERT_EQ_INT(1, ut_acquire_calls);

   reset_identity_check();
   btODReserveKind = OD_RESERVE_KIND_LOCAL;
   wODReserveNode = 0;
   ut_kind = OD_RESERVE_KIND_LOCAL;
   ut_node = 0;
   UT_ASSERT(utt_ODReserveCheckIdentities());
   UT_ASSERT_EQ_UINT(0, dwODReserveRecord);
   UT_ASSERT_EQ_INT(1, ut_acquire_calls);
}

static void rejects_read_and_matching_lock_failures(void)
{
   reset_identity_check();
   ut_count_result = FALSE;
   UT_ASSERT(!utt_ODReserveCheckIdentities());

   reset_identity_check();
   ut_access_result = kODReserveLockBusy;
   UT_ASSERT(!utt_ODReserveCheckIdentities());

   reset_identity_check();
   ut_length = -1L;
   UT_ASSERT(!utt_ODReserveCheckIdentities());

   reset_identity_check();
   ut_node = 3;
   ut_acquire_result = kODReserveLockBusy;
   UT_ASSERT(!utt_ODReserveCheckIdentities());
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);

   reset_identity_check();
   ut_node = 3;
   ut_acquire_result = kODReserveLockError;
   UT_ASSERT(!utt_ODReserveCheckIdentities());
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
}

static void rejects_corrupt_and_duplicate_records(void)
{
   reset_identity_check();
   ut_kind = 99;
   UT_ASSERT(!utt_ODReserveCheckIdentities());
   UT_ASSERT_EQ_INT(ERR_FILEREAD, od_control.od_error);

   reset_identity_check();
   ut_name_length = OD_RESERVE_NAME_MAX + 1;
   UT_ASSERT(!utt_ODReserveCheckIdentities());

   reset_identity_check();
   ut_flags = 0x80;
   UT_ASSERT(!utt_ODReserveCheckIdentities());

   reset_identity_check();
   ut_kind = OD_RESERVE_KIND_LOCAL;
   ut_node = 3;
   UT_ASSERT(utt_ODReserveCheckIdentities());

   reset_identity_check();
   ut_node = 3;
   ut_count = 2;
   ut_length = OD_RESERVE_HEADER_SIZE + 2L * OD_RESERVE_RECORD_SIZE;
   UT_ASSERT(!utt_ODReserveCheckIdentities());
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
}

static const UTTestCase ut_cases[] = {
   {"new", selects_append_slot_for_new_identity},
   {"reuse", claims_stale_record_for_same_identity},
   {"reject", rejects_read_and_matching_lock_failures},
   {"corrupt", rejects_corrupt_and_duplicate_records}
};
