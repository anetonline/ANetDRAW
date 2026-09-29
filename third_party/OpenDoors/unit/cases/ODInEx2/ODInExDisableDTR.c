#define UT_CUSTOM_MOCK_ODComCarrier
#define UT_CUSTOM_MOCK_ODSendModemCommand
#ifdef OD_DIAGNOSTICS
#define UT_CUSTOM_MOCK_ODDiagnosticMessage
#endif
static BOOL ut_carrier;
static BOOL ut_send_result;
static unsigned ut_carrier_calls;
static unsigned ut_send_calls;
#ifdef OD_DIAGNOSTICS
static unsigned ut_diagnostic_calls;
#endif
tODResult utm_ODComCarrier(tPortHandle port, BOOL *carrier)
{
   UT_ASSERT_EQ_PTR((tPortHandle)(DWORD_PTR)41, port);
   UT_ASSERT_NOT_NULL(carrier); *carrier = ut_carrier; ++ut_carrier_calls;
   return(kODRCSuccess);
}
BOOL utm_ODSendModemCommand(char *command, int retries)
{
   UT_ASSERT_EQ_PTR(od_control.od_disable_dtr, command);
   UT_ASSERT_EQ_INT(2, retries); ++ut_send_calls; return(ut_send_result);
}
#ifdef OD_DIAGNOSTICS
void utm_ODDiagnosticMessage(const char *text, const char *title)
{
   UT_ASSERT_EQ_PTR(szDebugWorkString, text); UT_ASSERT_NOT_NULL(title);
   ++ut_diagnostic_calls;
}
#endif

static void reset_disable(void)
{
   memset(&od_control, 0, sizeof(od_control));
   strcpy(od_control.od_disable_dtr, "~~~+++~~~ATH0");
   hSerialPort = (tPortHandle)(DWORD_PTR)41;
   ut_carrier = TRUE; ut_send_result = TRUE;
   ut_carrier_calls = ut_send_calls = 0;
#ifdef OD_DIAGNOSTICS
   ut_diagnostic_calls = 0; strcpy(szDebugWorkString, "trace");
#endif
}

static void skips_network_methods_and_absent_carrier(void)
{
   reset_disable(); od_control.od_com_method = COM_DOOR32;
   utt_ODInExDisableDTR(); UT_ASSERT_EQ_UINT(0, ut_carrier_calls);
   reset_disable(); od_control.od_com_method = COM_SOCKET;
   utt_ODInExDisableDTR(); UT_ASSERT_EQ_UINT(0, ut_carrier_calls);
   reset_disable(); od_control.od_com_method = COM_INTERNAL; ut_carrier = FALSE;
   utt_ODInExDisableDTR(); UT_ASSERT_EQ_UINT(1, ut_carrier_calls);
   UT_ASSERT_EQ_UINT(0, ut_send_calls);
}

static void honors_handle_and_disable_flags(void)
{
   reset_disable(); od_control.od_com_method = COM_INTERNAL;
   od_control.od_open_handle = 1; utt_ODInExDisableDTR();
   UT_ASSERT_EQ_UINT(0, ut_send_calls);
   reset_disable(); od_control.od_com_method = COM_INTERNAL;
   od_control.od_disable = DIS_DTR_DISABLE; utt_ODInExDisableDTR();
   UT_ASSERT_EQ_UINT(0, ut_send_calls);
}

static void sends_command_and_reports_each_diagnostic_outcome(void)
{
   reset_disable(); od_control.od_com_method = COM_INTERNAL;
   od_control.od_internal_debug = FALSE; ut_send_result = FALSE;
   utt_ODInExDisableDTR(); UT_ASSERT_EQ_UINT(1, ut_send_calls);
#ifdef OD_DIAGNOSTICS
   UT_ASSERT_EQ_UINT(0, ut_diagnostic_calls);
#endif
   reset_disable(); od_control.od_com_method = COM_INTERNAL;
   od_control.od_internal_debug = TRUE; ut_send_result = FALSE;
   utt_ODInExDisableDTR();
#ifdef OD_DIAGNOSTICS
   UT_ASSERT_EQ_UINT(1, ut_diagnostic_calls);
   UT_ASSERT_EQ_INT(0, szDebugWorkString[0]);
#endif
   reset_disable(); od_control.od_com_method = COM_INTERNAL;
   od_control.od_internal_debug = TRUE; ut_send_result = TRUE;
   utt_ODInExDisableDTR();
#ifdef OD_DIAGNOSTICS
   UT_ASSERT_EQ_UINT(1, ut_diagnostic_calls);
   UT_ASSERT_EQ_INT(0, szDebugWorkString[0]);
#endif
   reset_disable(); od_control.od_com_method = COM_INTERNAL;
   od_control.od_internal_debug = FALSE; ut_send_result = TRUE;
   utt_ODInExDisableDTR();
#ifdef OD_DIAGNOSTICS
   UT_ASSERT_EQ_UINT(0, ut_diagnostic_calls);
#endif
}

static const UTTestCase ut_cases[] = {
   {"method/carrier", skips_network_methods_and_absent_carrier},
   {"flags", honors_handle_and_disable_flags},
   {"command", sends_command_and_reports_each_diagnostic_outcome}
};
