#define UT_CUSTOM_MOCK_ODReserveRangeLock
#define UT_CUSTOM_MOCK_ODReserveRangeUnlock
#define UT_CUSTOM_MOCK_ODReserveAccess

static tODReserveLockResult ut_first_lock;
static tODReserveLockResult ut_second_lock;
static tODReserveLockResult ut_access;
static int ut_lock_calls;
static BYTE ut_commit;
static BYTE ut_kind;
static BYTE ut_name_length;
static BYTE ut_flags;

tODReserveLockResult utm_ODReserveRangeLock(int file, long offset,
   long length, BOOL write)
{
   (void)file;
   (void)offset;
   (void)length;
   (void)write;
   return(ut_lock_calls++ == 0 ? ut_first_lock : ut_second_lock);
}

BOOL utm_ODReserveRangeUnlock(int file, long offset, long length)
{ (void)file; (void)offset; (void)length; return(TRUE); }

tODReserveLockResult utm_ODReserveAccess(long offset, BYTE *buffer,
   size_t size, BOOL write, BOOL exclusive, tODMilliSec timeout)
{
   size_t index;
   (void)offset;
   (void)write;
   (void)exclusive;
   (void)timeout;
   for(index = 0; index < size; ++index) buffer[index] = 0;
   buffer[OD_RESERVE_SLOT_COMMIT - 1] = ut_commit;
   buffer[OD_RESERVE_SLOT_KIND - 1] = ut_kind;
   buffer[OD_RESERVE_SLOT_NAME_LENGTH - 1] = ut_name_length;
   buffer[OD_RESERVE_SLOT_FLAGS - 1] = ut_flags;
   return(ut_access);
}

static void reset_record(void)
{
   ut_first_lock = kODReserveLockBusy;
   ut_second_lock = kODReserveLockBusy;
   ut_access = kODReserveLockAcquired;
   ut_lock_calls = 0;
   ut_commit = OD_RESERVE_COMMIT;
   ut_kind = OD_RESERVE_KIND_NODE;
   ut_name_length = 0;
   ut_flags = OD_RESERVE_FLAG_PRESENT;
}

static void distinguishes_lifetime_and_payload_states(void)
{
   BYTE record[OD_RESERVE_RECORD_SIZE];
   reset_record();
   ut_first_lock = kODReserveLockAcquired;
   UT_ASSERT_EQ_INT(kODReserveRecordStale,
      utt_ODReserveReadLiveRecord(2, record, 0));
   reset_record();
   ut_first_lock = kODReserveLockError;
   UT_ASSERT_EQ_INT(kODReserveRecordError,
      utt_ODReserveReadLiveRecord(2, record, 0));
   reset_record();
   ut_access = kODReserveLockBusy;
   UT_ASSERT_EQ_INT(kODReserveRecordBusy,
      utt_ODReserveReadLiveRecord(2, record, 0));
   reset_record();
   ut_access = kODReserveLockError;
   UT_ASSERT_EQ_INT(kODReserveRecordError,
      utt_ODReserveReadLiveRecord(2, record, 0));
}

static void validates_live_payloads(void)
{
   BYTE record[OD_RESERVE_RECORD_SIZE];
   reset_record();
   UT_ASSERT_EQ_INT(kODReserveRecordLive,
      utt_ODReserveReadLiveRecord(2, record, 0));
   reset_record();
   ut_kind = OD_RESERVE_KIND_LOCAL;
   UT_ASSERT_EQ_INT(kODReserveRecordLive,
      utt_ODReserveReadLiveRecord(2, record, 0));

   reset_record();
   ut_commit = 0;
   ut_second_lock = kODReserveLockAcquired;
   UT_ASSERT_EQ_INT(kODReserveRecordStale,
      utt_ODReserveReadLiveRecord(2, record, 0));
   reset_record();
   ut_kind = 99;
   UT_ASSERT_EQ_INT(kODReserveRecordError,
      utt_ODReserveReadLiveRecord(2, record, 0));
   reset_record();
   ut_name_length = OD_RESERVE_NAME_MAX + 1;
   UT_ASSERT_EQ_INT(kODReserveRecordError,
      utt_ODReserveReadLiveRecord(2, record, 0));
   reset_record();
   ut_flags = 0x80;
   UT_ASSERT_EQ_INT(kODReserveRecordError,
      utt_ODReserveReadLiveRecord(2, record, 0));
}

static const UTTestCase ut_cases[] = {
   {"states", distinguishes_lifetime_and_payload_states},
   {"validate", validates_live_payloads}
};
