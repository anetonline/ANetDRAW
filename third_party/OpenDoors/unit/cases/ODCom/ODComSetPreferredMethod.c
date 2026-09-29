static tPortInfo ut_port;

static void stores_a_raw_method_on_a_closed_port(void)
{
   memset(&ut_port, 0, sizeof(ut_port));
#ifdef INCLUDE_SOCKET_COM
   ut_port.bTelnetSocket = TRUE;
   ut_port.TelnetInputState = kTelnetInputSubnegotiation;
   ut_port.bTelnetInputReplay = TRUE;
#endif
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComSetPreferredMethod(
      ODPTR2HANDLE(&ut_port, tPortInfo), kComMethodSocket));
   UT_ASSERT_EQ_INT(kComMethodSocket, ut_port.Method);
#ifdef INCLUDE_SOCKET_COM
   UT_ASSERT_EQ_INT(FALSE, ut_port.bTelnetSocket);
   UT_ASSERT_EQ_INT(kTelnetInputData, ut_port.TelnetInputState);
   UT_ASSERT_EQ_INT(FALSE, ut_port.bTelnetInputReplay);
#endif
}

#ifdef INCLUDE_SOCKET_COM
static void maps_the_telnet_selector_to_a_framed_socket(void)
{
   memset(&ut_port, 0, sizeof(ut_port));
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComSetPreferredMethod(
      ODPTR2HANDLE(&ut_port, tPortInfo), kComMethodTelnetSocket));
   UT_ASSERT_EQ_INT(kComMethodSocket, ut_port.Method);
   UT_ASSERT_EQ_INT(TRUE, ut_port.bTelnetSocket);
   UT_ASSERT_EQ_INT(kTelnetInputData, ut_port.TelnetInputState);
   UT_ASSERT_EQ_INT(FALSE, ut_port.bTelnetInputReplay);
}
#endif

static const UTTestCase ut_cases[] = {
   {"raw method", stores_a_raw_method_on_a_closed_port},
#ifdef INCLUDE_SOCKET_COM
   {"Telnet socket", maps_the_telnet_selector_to_a_framed_socket},
#endif
};
