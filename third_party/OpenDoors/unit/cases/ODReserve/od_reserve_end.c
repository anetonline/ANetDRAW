#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed
#define UT_CUSTOM_MOCK_ODSyncAPIIsNested
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_ODReserveWriteOwn
#define UT_CUSTOM_MOCK_ODReserveDetach

static BOOL ut_allowed;
static BOOL ut_init_succeeds;
static BOOL ut_write_result;
static int ut_detach_calls;

BOOL utm_ODSyncPublicCallAllowed(void) { return(ut_allowed); }
BOOL utm_ODSyncAPIIsNested(void) { return(FALSE); }
void utm_ODSyncAPIEntry(void) { }
void utm_ODSyncAPIExit(void) { }
void ODCALL utm_od_init(void)
{ if(ut_init_succeeds) bODInitialized = TRUE; }
BOOL utm_ODReserveWriteOwn(BYTE flags, const char *name,
   DWORD low, DWORD high)
{ (void)flags; (void)name; (void)low; (void)high; return(ut_write_result); }
void utm_ODReserveDetach(void) { ++ut_detach_calls; }

static void reset_end(void)
{
   memset(&od_control, 0, sizeof(od_control));
   bODInitialized = TRUE;
   eODLifecycleState = kODLifecycleActive;
   bODReserveActive = TRUE;
   bODReserveRequested = TRUE;
   bODReserveAcquired = TRUE;
   strcpy(szODReserveName, "Shared");
   dwODReserveTicketLow = 4;
   dwODReserveTicketHigh = 5;
   ut_allowed = TRUE;
   ut_init_succeeds = TRUE;
   ut_write_result = TRUE;
   ut_detach_calls = 0;
}

static void rejects_invalid_states(void)
{
   reset_end();
   ut_allowed = FALSE;
   UT_ASSERT(!utt_od_reserve_end());
   reset_end();
   bODInitialized = FALSE;
   eODLifecycleState = kODLifecycleTerminal;
   ut_init_succeeds = FALSE;
   UT_ASSERT(!utt_od_reserve_end());
   reset_end();
   bODReserveActive = FALSE;
   UT_ASSERT(!utt_od_reserve_end());
   reset_end();
   bODReserveRequested = FALSE;
   UT_ASSERT(!utt_od_reserve_end());
}

static void clears_or_detaches_the_request(void)
{
   reset_end();
   bODInitialized = FALSE;
   UT_ASSERT(utt_od_reserve_end());
   UT_ASSERT(!bODReserveRequested && !bODReserveAcquired);
   UT_ASSERT_EQ_INT(0, szODReserveName[0]);
   UT_ASSERT_EQ_UINT(0, dwODReserveTicketLow);
   UT_ASSERT_EQ_UINT(0, dwODReserveTicketHigh);
   reset_end();
   ut_write_result = FALSE;
   UT_ASSERT(!utt_od_reserve_end());
   UT_ASSERT_EQ_INT(1, ut_detach_calls);
}

static const UTTestCase ut_cases[] = {
   {"reject", rejects_invalid_states},
   {"end", clears_or_detaches_the_request}
};
