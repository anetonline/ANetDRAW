#define UT_CUSTOM_MOCK_ODReserveRangeLock
#define UT_CUSTOM_MOCK_ODTimerStart
#define UT_CUSTOM_MOCK_ODTimerElapsed
#define UT_CUSTOM_MOCK_ODReservePause

static int ut_busy_calls;
static BOOL ut_elapsed;
static int ut_pause_calls;
static tODMilliSec ut_last_pause;

tODReserveLockResult utm_ODReserveRangeLock(int file, long offset,
   long length, BOOL write)
{
   UT_ASSERT_EQ_INT(hODReserveFile, file);
   (void)offset;
   (void)length;
   (void)write;
   if(ut_busy_calls > 0)
   {
      --ut_busy_calls;
      return(kODReserveLockBusy);
   }
   return(kODReserveLockAcquired);
}

void utm_ODTimerStart(tODTimer *timer, tODMilliSec delay)
{ (void)timer; (void)delay; }

BOOL utm_ODTimerElapsed(tODTimer *timer)
{ (void)timer; return(ut_elapsed); }

void utm_ODReservePause(tODMilliSec delay)
{
   ++ut_pause_calls;
   ut_last_pause = delay;
}

static void resets(void)
{
   ut_busy_calls = 0;
   ut_elapsed = FALSE;
   ut_pause_calls = 0;
   ut_last_pause = 0;
}

static void returns_for_each_terminal_condition(void)
{
   resets();
   UT_ASSERT_EQ_INT(kODReserveLockAcquired,
      utt_ODReserveAcquire(4, 8, TRUE, 100));

   resets();
   ut_busy_calls = 1;
   UT_ASSERT_EQ_INT(kODReserveLockBusy,
      utt_ODReserveAcquire(4, 8, TRUE, 0));

   resets();
   ut_busy_calls = 1;
   ut_elapsed = TRUE;
   UT_ASSERT_EQ_INT(kODReserveLockBusy,
      utt_ODReserveAcquire(4, 8, TRUE, 100));
}

static void backs_off_and_caps_the_delay(void)
{
   resets();
   ut_busy_calls = 7;
   UT_ASSERT_EQ_INT(kODReserveLockAcquired,
      utt_ODReserveAcquire(4, 8, TRUE, 1000));
   UT_ASSERT_EQ_INT(7, ut_pause_calls);
   UT_ASSERT_EQ_UINT(250, ut_last_pause);
}

static const UTTestCase ut_cases[] = {
   {"terminal", returns_for_each_terminal_condition},
   {"backoff", backs_off_and_caps_the_delay}
};
