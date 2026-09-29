#define UT_CUSTOM_MOCK_ODReserveRangeLock
#define UT_CUSTOM_MOCK_ODReserveRangeUnlock
#define UT_CUSTOM_MOCK_ODReserveTransfer
#define UT_CUSTOM_MOCK_ODTimerStart
#define UT_CUSTOM_MOCK_ODTimerElapsed
#define UT_CUSTOM_MOCK_ODReservePause

static int ut_busy_calls;
static tODReserveLockResult ut_final_lock;
static BOOL ut_unlock;
static BOOL ut_elapsed;
static BOOL ut_transfer;
static int ut_pause_calls;

tODReserveLockResult utm_ODReserveRangeLock(int file, long offset,
   long length, BOOL write)
{
   UT_ASSERT_EQ_INT(7, file);
   UT_ASSERT_EQ_INT(9, (int)offset);
   UT_ASSERT_EQ_INT(4, (int)length);
   UT_ASSERT(write);
   if(ut_busy_calls > 0)
   {
      --ut_busy_calls;
      return(kODReserveLockBusy);
   }
   return(ut_final_lock);
}

BOOL utm_ODReserveRangeUnlock(int file, long offset, long length)
{ (void)file; (void)offset; (void)length; return(ut_unlock); }
BOOL utm_ODReserveTransfer(int file, long offset, BYTE *buffer,
   size_t size, BOOL write)
{
   (void)buffer;
   UT_ASSERT_EQ_INT(7, file);
   UT_ASSERT_EQ_INT(9, (int)offset);
   UT_ASSERT_EQ_UINT(4, size);
   (void)write;
   return(ut_transfer);
}
void utm_ODTimerStart(tODTimer *timer, tODMilliSec delay)
{ (void)timer; (void)delay; }
BOOL utm_ODTimerElapsed(tODTimer *timer)
{ (void)timer; return(ut_elapsed); }
void utm_ODReservePause(tODMilliSec delay)
{ (void)delay; ++ut_pause_calls; }
static void reset_access(void)
{
   hODReserveFile = 7;
   ut_busy_calls = 0;
   ut_final_lock = kODReserveLockAcquired;
   ut_unlock = TRUE;
   ut_elapsed = FALSE;
   ut_transfer = TRUE;
   ut_pause_calls = 0;
}

static void transfers_reads_and_writes(void)
{
   BYTE buffer[4] = {0, 0, 0, 0};
   reset_access();
   UT_ASSERT_EQ_INT(kODReserveLockAcquired,
      utt_ODReserveAccess(9, buffer, sizeof(buffer), TRUE, TRUE, 0));
   reset_access();
   UT_ASSERT_EQ_INT(kODReserveLockAcquired,
      utt_ODReserveAccess(9, buffer, sizeof(buffer), FALSE, TRUE, 0));
}

static void reports_each_transfer_failure(void)
{
   BYTE buffer[4] = {0, 0, 0, 0};
   reset_access();
   ut_transfer = FALSE;
   UT_ASSERT_EQ_INT(kODReserveLockError,
      utt_ODReserveAccess(9, buffer, sizeof(buffer), TRUE, TRUE, 0));
   reset_access(); ut_transfer = FALSE;
   UT_ASSERT_EQ_INT(kODReserveLockError,
      utt_ODReserveAccess(9, buffer, sizeof(buffer), FALSE, TRUE, 0));
   reset_access();
   ut_unlock = FALSE;
   UT_ASSERT_EQ_INT(kODReserveLockError,
      utt_ODReserveAccess(9, buffer, sizeof(buffer), FALSE, TRUE, 0));
}

static void observes_lock_timeout_and_retry(void)
{
   BYTE buffer[4] = {0, 0, 0, 0};
   reset_access();
   ut_final_lock = kODReserveLockBusy;
   UT_ASSERT_EQ_INT(kODReserveLockBusy,
      utt_ODReserveAccess(9, buffer, sizeof(buffer), TRUE, TRUE, 0));
   reset_access();
   ut_final_lock = kODReserveLockError;
   ut_elapsed = TRUE;
   UT_ASSERT_EQ_INT(kODReserveLockError,
      utt_ODReserveAccess(9, buffer, sizeof(buffer), TRUE, TRUE, 100));
   reset_access();
   ut_busy_calls = 1;
   UT_ASSERT_EQ_INT(kODReserveLockAcquired,
      utt_ODReserveAccess(9, buffer, sizeof(buffer), TRUE, TRUE, 100));
   UT_ASSERT_EQ_INT(1, ut_pause_calls);
}

static void caps_retry_backoff(void)
{
   BYTE buffer[4] = {0, 0, 0, 0};
   reset_access();
   ut_busy_calls = 7;
   UT_ASSERT_EQ_INT(kODReserveLockAcquired,
      utt_ODReserveAccess(9, buffer, sizeof(buffer), TRUE, TRUE, 1000));
   UT_ASSERT_EQ_INT(7, ut_pause_calls);
}

static const UTTestCase ut_cases[] = {
   {"transfer", transfers_reads_and_writes},
   {"transfer failures", reports_each_transfer_failure},
   {"lock retry", observes_lock_timeout_and_retry},
   {"backoff", caps_retry_backoff}
};
