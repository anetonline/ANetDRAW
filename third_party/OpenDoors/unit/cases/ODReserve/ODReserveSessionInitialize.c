#define UT_CUSTOM_MOCK_ODReserveOpenRegistry
#define UT_CUSTOM_MOCK_ODReservePrepareHeader
#define UT_CUSTOM_MOCK_ODReserveSetIdentity
#define UT_CUSTOM_MOCK_ODReserveCheckIdentities
#define UT_CUSTOM_MOCK_ODReserveAppendParticipant
#define UT_CUSTOM_MOCK_ODReserveWriteOwn
#define UT_CUSTOM_MOCK_ODReserveRangeUnlock
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK__close
#else
#define UT_CUSTOM_MOCK_close
#endif

static BOOL ut_open;
static BOOL ut_prepare;
static BOOL ut_check;
static BOOL ut_append;
static BOOL ut_write;
static BOOL ut_unlock;
static BOOL ut_created;
static int ut_close_calls;

BOOL utm_ODReserveOpenRegistry(BOOL *created)
{
   *created = ut_created;
   if(ut_open) hODReserveFile = 7;
   return(ut_open);
}
BOOL utm_ODReservePrepareHeader(BOOL created)
{ UT_ASSERT_EQ_INT(ut_created, created); return(ut_prepare); }
void utm_ODReserveSetIdentity(void)
{ btODReserveKind = OD_RESERVE_KIND_NODE; wODReserveNode = 3; }
BOOL utm_ODReserveCheckIdentities(void)
{ return(ut_check); }
BOOL utm_ODReserveAppendParticipant(void)
{
   if(ut_append)
   {
      dwODReserveRecord = 1;
      dwODReserveRecordCount = 2;
   }
   return(ut_append);
}
BOOL utm_ODReserveWriteOwn(BYTE flags, const char *name,
   DWORD low, DWORD high)
{ (void)flags; (void)name; (void)low; (void)high; return(ut_write); }
BOOL utm_ODReserveRangeUnlock(int file, long offset, long length)
{ (void)file; (void)offset; (void)length; return(ut_unlock); }
#ifdef ODPLAT_WIN32
int utm__close(int file)
#else
int utm_close(int file)
#endif
{ (void)file; ++ut_close_calls; return(0); }

static void reset_initialize(void)
{
   memset(&od_control, 0, sizeof(od_control));
   pszODReservePath = (char *)"VOTE.SYN";
   hODReserveFile = -1;
   bODReserveActive = FALSE;
   ut_open = TRUE;
   ut_prepare = TRUE;
   ut_check = TRUE;
   ut_append = TRUE;
   ut_write = TRUE;
   ut_unlock = TRUE;
   ut_created = FALSE;
   ut_close_calls = 0;
}

static void ignores_unconfigured_and_active_sessions(void)
{
   reset_initialize();
   pszODReservePath = NULL;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODReserveSessionInitialize());
   reset_initialize();
   bODReserveActive = TRUE;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODReserveSessionInitialize());
}

static void initializes_an_appended_participant(void)
{
   reset_initialize();
   ut_created = TRUE;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODReserveSessionInitialize());
   UT_ASSERT(bODReserveActive && !bODReserveDetached);
   UT_ASSERT_EQ_UINT(1, dwODReserveRecord);
   UT_ASSERT_EQ_UINT(2, dwODReserveRecordCount);
}

static void cleans_up_each_initialization_failure(void)
{
   reset_initialize();
   ut_open = FALSE;
   UT_ASSERT_EQ_INT(kODRCFileAccessError, utt_ODReserveSessionInitialize());
   UT_ASSERT_EQ_INT(ERR_FILEOPEN, od_control.od_error);
   reset_initialize();
   ut_prepare = FALSE;
   UT_ASSERT_EQ_INT(kODRCFileAccessError, utt_ODReserveSessionInitialize());
   reset_initialize();
   ut_check = FALSE;
   od_control.od_error = ERR_FILEREAD;
   UT_ASSERT_EQ_INT(kODRCFileAccessError, utt_ODReserveSessionInitialize());
   UT_ASSERT_EQ_INT(ERR_FILEREAD, od_control.od_error);
   reset_initialize();
   ut_append = FALSE;
   UT_ASSERT_EQ_INT(kODRCFileAccessError, utt_ODReserveSessionInitialize());
   reset_initialize();
   ut_write = FALSE;
   UT_ASSERT_EQ_INT(kODRCFileAccessError, utt_ODReserveSessionInitialize());
   reset_initialize();
   ut_unlock = FALSE;
   UT_ASSERT_EQ_INT(kODRCFileAccessError, utt_ODReserveSessionInitialize());
}

static const UTTestCase ut_cases[] = {
   {"ignore", ignores_unconfigured_and_active_sessions},
   {"initialize", initializes_an_appended_participant},
   {"failures", cleans_up_each_initialization_failure}
};
