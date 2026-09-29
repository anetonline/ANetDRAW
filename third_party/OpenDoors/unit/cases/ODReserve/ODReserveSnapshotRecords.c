#define UT_CUSTOM_MOCK_ODReserveAcquire
#define UT_CUSTOM_MOCK_ODReserveCountRecords
#define UT_CUSTOM_MOCK_ODReserveRangeUnlock

static tODReserveLockResult ut_lock_result;
static BOOL ut_count_result;
static BOOL ut_unlock_result;
static tODMilliSec ut_expected_timeout;

tODReserveLockResult utm_ODReserveAcquire(long offset, long length,
   BOOL write, tODMilliSec timeout)
{
   UT_ASSERT_EQ_INT(0, (int)offset);
   UT_ASSERT_EQ_INT(1, (int)length);
   UT_ASSERT(!write);
   UT_ASSERT_EQ_UINT(ut_expected_timeout, timeout);
   return(ut_lock_result);
}

BOOL utm_ODReserveCountRecords(DWORD *count)
{
   *count = 4UL;
   return(ut_count_result);
}

BOOL utm_ODReserveRangeUnlock(int file, long offset, long length)
{
   (void)file;
   (void)offset;
   (void)length;
   return(ut_unlock_result);
}

static void snapshots_under_the_registry_lock(void)
{
   DWORD count = 0;

   ut_lock_result = kODReserveLockAcquired;
   ut_count_result = TRUE;
   ut_unlock_result = TRUE;
   ut_expected_timeout = 37UL;
   UT_ASSERT_EQ_INT(kODReserveLockAcquired,
      utt_ODReserveSnapshotRecords(&count, ut_expected_timeout));
   UT_ASSERT_EQ_UINT(4, count);

   ut_lock_result = kODReserveLockBusy;
   ut_expected_timeout = 0;
   UT_ASSERT_EQ_INT(kODReserveLockBusy,
      utt_ODReserveSnapshotRecords(&count, ut_expected_timeout));

   ut_lock_result = kODReserveLockError;
   UT_ASSERT_EQ_INT(kODReserveLockError,
      utt_ODReserveSnapshotRecords(&count, ut_expected_timeout));

   ut_lock_result = kODReserveLockAcquired;
   ut_count_result = FALSE;
   UT_ASSERT_EQ_INT(kODReserveLockError,
      utt_ODReserveSnapshotRecords(&count, ut_expected_timeout));

   ut_count_result = TRUE;
   ut_unlock_result = FALSE;
   UT_ASSERT_EQ_INT(kODReserveLockError,
      utt_ODReserveSnapshotRecords(&count, ut_expected_timeout));
}

static const UTTestCase ut_cases[] = {
   {"snapshot", snapshots_under_the_registry_lock}
};
