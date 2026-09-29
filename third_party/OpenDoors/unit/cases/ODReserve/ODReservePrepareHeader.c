#define UT_CUSTOM_MOCK_ODTimerStart
#define UT_CUSTOM_MOCK_ODTimerElapsed
#define UT_CUSTOM_MOCK_ODReserveAcquire
#define UT_CUSTOM_MOCK_ODReserveRangeUnlock
#define UT_CUSTOM_MOCK_ODReserveTransfer
#define UT_CUSTOM_MOCK_ODReservePause
#define UT_CUSTOM_MOCK_memset
#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_memcmp

static tODReserveLockResult ut_acquire;
static BOOL ut_unlock;
static BOOL ut_elapsed;
static int ut_transfer_calls;
static int ut_fail_transfer_call;
static BOOL ut_committed;
static BOOL ut_magic_valid;
static BOOL ut_size_valid;
static int ut_pause_calls;

void utm_ODTimerStart(tODTimer *timer, tODMilliSec duration)
{ (void)timer; (void)duration; }
BOOL utm_ODTimerElapsed(tODTimer *timer)
{ (void)timer; return(ut_elapsed); }
tODReserveLockResult utm_ODReserveAcquire(long offset, long length,
   BOOL write, tODMilliSec timeout)
{ (void)offset; (void)length; (void)write; (void)timeout; return(ut_acquire); }
BOOL utm_ODReserveRangeUnlock(int file, long offset, long length)
{ (void)file; (void)offset; (void)length; return(ut_unlock); }
void utm_ODReservePause(tODMilliSec duration)
{ (void)duration; ++ut_pause_calls; }
void *utm_memset(void *destination, int value, size_t size)
{
   BYTE *bytes = (BYTE *)destination;
   size_t index;
   for(index = 0; index < size; ++index) bytes[index] = (BYTE)value;
   return(destination);
}
void *utm_memcpy(void *destination, const void *source, size_t size)
{
   BYTE *out = (BYTE *)destination;
   const BYTE *in = (const BYTE *)source;
   size_t index;
   for(index = 0; index < size; ++index) out[index] = in[index];
   return(destination);
}
int utm_memcmp(const void *left, const void *right, size_t size)
{
   (void)left;
   (void)right;
   (void)size;
   return(ut_magic_valid ? 0 : 1);
}
BOOL utm_ODReserveTransfer(int file, long offset, BYTE *data,
   size_t size, BOOL write)
{
   static BYTE magic[8] = {'O','D','R','S','Y','N','C','1'};
   size_t index;

   UT_ASSERT_EQ_INT(hODReserveFile, file);
   ++ut_transfer_calls;
   if(ut_transfer_calls == ut_fail_transfer_call)
      return(FALSE);
   if(write)
   {
      UT_ASSERT(offset == 2L || offset == OD_RESERVE_HEADER_COMMIT);
      UT_ASSERT(size == OD_RESERVE_RECORD_SIZE - 2 || size == 1);
      if(offset == 2L)
      {
         UT_ASSERT(data[0] == 'O');
         UT_ASSERT(data[1] == 'D');
         UT_ASSERT(data[2] == 'R');
         UT_ASSERT(data[3] == 'S');
         UT_ASSERT(data[4] == 'Y');
         UT_ASSERT(data[5] == 'N');
         UT_ASSERT(data[6] == 'C');
         UT_ASSERT(data[7] == '1');
      }
      return(TRUE);
   }
   UT_ASSERT_EQ_INT(1, (int)offset);
   UT_ASSERT_EQ_UINT(OD_RESERVE_RECORD_SIZE - 1, size);
   for(index = 0; index < size; ++index) data[index] = 0;
   if(ut_committed)
      data[OD_RESERVE_HEADER_COMMIT - 1] = OD_RESERVE_COMMIT;
   if(ut_magic_valid)
      for(index = 0; index < sizeof(magic); ++index)
         data[OD_RESERVE_HEADER_MAGIC - 1 + index] = magic[index];
   if(ut_size_valid)
      data[OD_RESERVE_HEADER_RECORD_SIZE - 1] = OD_RESERVE_RECORD_SIZE;
   return(TRUE);
}

static void reset_header(void)
{
   utm_memset(&od_control, 0, sizeof(od_control));
   hODReserveFile = 7;
   ut_acquire = kODReserveLockAcquired;
   ut_unlock = TRUE;
   ut_elapsed = TRUE;
   ut_transfer_calls = 0;
   ut_fail_transfer_call = 0;
   ut_committed = TRUE;
   ut_magic_valid = TRUE;
   ut_size_valid = TRUE;
   ut_pause_calls = 0;
}

static void creates_and_reads_valid_headers(void)
{
   reset_header();
   UT_ASSERT(utt_ODReservePrepareHeader(TRUE));
   reset_header();
   UT_ASSERT(utt_ODReservePrepareHeader(FALSE));
}

static void reports_created_header_failures(void)
{
   reset_header();
   ut_acquire = kODReserveLockBusy;
   UT_ASSERT(!utt_ODReservePrepareHeader(TRUE));
   reset_header(); ut_fail_transfer_call = 1;
   UT_ASSERT(!utt_ODReservePrepareHeader(TRUE));
   reset_header(); ut_fail_transfer_call = 2;
   UT_ASSERT(!utt_ODReservePrepareHeader(TRUE));
}

static void retries_incomplete_existing_headers(void)
{
   reset_header();
   ut_fail_transfer_call = 1;
   ut_elapsed = FALSE;
   UT_ASSERT(utt_ODReservePrepareHeader(FALSE));
   UT_ASSERT_EQ_INT(1, ut_pause_calls);

   reset_header(); ut_committed = FALSE;
   UT_ASSERT(!utt_ODReservePrepareHeader(FALSE));
   UT_ASSERT_EQ_INT(ERR_FILEREAD, od_control.od_error);
   reset_header(); ut_committed = FALSE; ut_unlock = FALSE;
   UT_ASSERT(!utt_ODReservePrepareHeader(FALSE));
}

static void rejects_incompatible_existing_headers(void)
{
   reset_header(); ut_magic_valid = FALSE;
   UT_ASSERT(!utt_ODReservePrepareHeader(FALSE));
   UT_ASSERT_EQ_INT(ERR_FILEREAD, od_control.od_error);
   reset_header(); ut_size_valid = FALSE;
   UT_ASSERT(!utt_ODReservePrepareHeader(FALSE));
}

static const UTTestCase ut_cases[] = {
   {"valid", creates_and_reads_valid_headers},
   {"create failures", reports_created_header_failures},
   {"retry", retries_incomplete_existing_headers},
   {"incompatible", rejects_incompatible_existing_headers}
};
