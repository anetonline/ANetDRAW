static tPortInfo ut_port;

static void set_port(BYTE number)
{
   memset(&ut_port, 0xa5, sizeof(ut_port));
   ut_port.bIsOpen = FALSE;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComSetPort(ODPTR2HANDLE(&ut_port, tPortInfo), number));
   UT_ASSERT_EQ_INT(number, ut_port.btPort);
}

static void stores_the_port_and_platform_defaults(void)
{
#ifdef INCLUDE_UART_COM
   set_port(0);
   UT_ASSERT(ut_port.nPortAddress != 0);
   UT_ASSERT_EQ_INT(4, ut_port.btIRQLevel);

   set_port(2);
   UT_ASSERT(ut_port.nPortAddress != 0);
   UT_ASSERT_EQ_INT(4, ut_port.btIRQLevel);

   set_port(1);
   UT_ASSERT(ut_port.nPortAddress != 0);
   UT_ASSERT_EQ_INT(3, ut_port.btIRQLevel);

   set_port(4);
   UT_ASSERT_EQ_INT(anDefaultPortAddr[4], ut_port.nPortAddress);
   UT_ASSERT_EQ_INT(3, ut_port.btIRQLevel);

   set_port((BYTE)DIM(anDefaultPortAddr));
   UT_ASSERT_EQ_INT(0, ut_port.nPortAddress);
   UT_ASSERT_EQ_INT(3, ut_port.btIRQLevel);
#else
   set_port(7);
#endif
}

static const UTTestCase ut_cases[] = {
   {"port defaults", stores_the_port_and_platform_defaults}
};
