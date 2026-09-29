#define UT_CUSTOM_MOCK_GetCommTimeouts
#define UT_CUSTOM_MOCK_SetCommTimeouts

static BOOL ut_get_succeeds;
static BOOL ut_set_succeeds;
static unsigned ut_get_calls;
static unsigned ut_set_calls;
static COMMTIMEOUTS ut_written;

BOOL WINAPI utm_GetCommTimeouts(HANDLE device, LPCOMMTIMEOUTS timeouts)
{
   ++ut_get_calls;
   UT_ASSERT_EQ_PTR((HANDLE)123, device);
   memset(timeouts, 0x5a, sizeof(*timeouts));
   return ut_get_succeeds;
}

BOOL WINAPI utm_SetCommTimeouts(HANDLE device, LPCOMMTIMEOUTS timeouts)
{
   ++ut_set_calls;
   UT_ASSERT_EQ_PTR((HANDLE)123, device);
   ut_written = *timeouts;
   return ut_set_succeeds;
}

static void reset_timeouts(tPortInfo *port)
{
   memset(port, 0, sizeof(*port));
   memset(&ut_written, 0, sizeof(ut_written));
   port->hCommDev = (HANDLE)123;
   port->ReadTimeoutState = kNotSet;
   ut_get_succeeds = TRUE;
   ut_set_succeeds = TRUE;
   ut_get_calls = 0;
   ut_set_calls = 0;
}

static void leaves_an_existing_state_untouched(void)
{
   tPortInfo port;
   reset_timeouts(&port);
   port.ReadTimeoutState = kBlocking;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComWin32SetReadTimeouts(&port, kBlocking));
   UT_ASSERT_EQ_UINT(0, ut_get_calls);
   UT_ASSERT_EQ_UINT(0, ut_set_calls);
}

static void reports_read_and_write_failures_without_recording_the_state(void)
{
   tPortInfo port;
   reset_timeouts(&port);
   ut_get_succeeds = FALSE;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComWin32SetReadTimeouts(&port, kBlocking));
   UT_ASSERT_EQ_INT(kNotSet, port.ReadTimeoutState);
   UT_ASSERT_EQ_UINT(0, ut_set_calls);

   reset_timeouts(&port);
   ut_set_succeeds = FALSE;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODComWin32SetReadTimeouts(&port, kNonBlocking));
   UT_ASSERT_EQ_INT(kNotSet, port.ReadTimeoutState);
   UT_ASSERT_EQ_UINT(1, ut_set_calls);
}

static void writes_blocking_nonblocking_and_defensive_default_settings(void)
{
   tPortInfo port;
   reset_timeouts(&port);
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComWin32SetReadTimeouts(&port, kBlocking));
   UT_ASSERT_EQ_INT(0, ut_written.ReadIntervalTimeout);
   UT_ASSERT_EQ_INT(0, ut_written.ReadTotalTimeoutMultiplier);
   UT_ASSERT_EQ_INT(0, ut_written.ReadTotalTimeoutConstant);
   UT_ASSERT_EQ_INT(kBlocking, port.ReadTimeoutState);

   reset_timeouts(&port);
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComWin32SetReadTimeouts(&port, kNonBlocking));
   UT_ASSERT((DWORD)INFINITE == ut_written.ReadIntervalTimeout);
   UT_ASSERT_EQ_INT(0, ut_written.ReadTotalTimeoutMultiplier);
   UT_ASSERT_EQ_INT(0, ut_written.ReadTotalTimeoutConstant);
   UT_ASSERT_EQ_INT(kNonBlocking, port.ReadTimeoutState);

   reset_timeouts(&port);
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComWin32SetReadTimeouts(
      &port, (tReadTimeoutState)99));
   UT_ASSERT_EQ_INT(99, port.ReadTimeoutState);
}

static const UTTestCase ut_cases[] = {
   {"unchanged", leaves_an_existing_state_untouched},
   {"failures", reports_read_and_write_failures_without_recording_the_state},
   {"settings", writes_blocking_nonblocking_and_defensive_default_settings}
};
