#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK__close
#else
#define UT_CUSTOM_MOCK_close
#endif
#define UT_CUSTOM_MOCK_ODReserveWriteOwn
static int ut_close_calls;
static int ut_write_calls;
#ifdef ODPLAT_WIN32
int utm__close(int file)
#else
int utm_close(int file)
#endif
{ (void)file; ++ut_close_calls; return(0); }
BOOL utm_ODReserveWriteOwn(BYTE flags, const char *name,
   DWORD low, DWORD high)
{
   UT_ASSERT_EQ_INT(0, flags);
   UT_ASSERT_NULL(name);
   UT_ASSERT_EQ_UINT(0, low);
   UT_ASSERT_EQ_UINT(0, high);
   ++ut_write_calls;
   return(TRUE);
}
static void closes_and_clears_session_state(void)
{
   hODReserveFile = 7;
   bODReserveActive = FALSE;
   bODReserveRequested = TRUE;
   bODReserveAcquired = TRUE;
   strcpy(szODReserveName, "x");
   ut_close_calls = 0;
   ut_write_calls = 0;
   utt_ODReserveSessionShutdown();
   UT_ASSERT_EQ_INT(1, ut_close_calls);
   UT_ASSERT_EQ_INT(0, ut_write_calls);
   UT_ASSERT_EQ_INT(-1, hODReserveFile);
   UT_ASSERT(!bODReserveRequested && !bODReserveAcquired);

   hODReserveFile = -1;
   bODReserveActive = TRUE;
   utt_ODReserveSessionShutdown();
   UT_ASSERT_EQ_INT(1, ut_write_calls);
   UT_ASSERT_EQ_INT(1, ut_close_calls);
}
static const UTTestCase ut_cases[] = {
   {"shutdown", closes_and_clears_session_state}
};
