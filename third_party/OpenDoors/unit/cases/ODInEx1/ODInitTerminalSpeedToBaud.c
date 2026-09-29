static void recognizes_standard_posix_speeds(void)
{
   UT_ASSERT_EQ_UINT(50L, utt_ODInitTerminalSpeedToBaud(B50));
   UT_ASSERT_EQ_UINT(75L, utt_ODInitTerminalSpeedToBaud(B75));
   UT_ASSERT_EQ_UINT(110L, utt_ODInitTerminalSpeedToBaud(B110));
   UT_ASSERT_EQ_UINT(134L, utt_ODInitTerminalSpeedToBaud(B134));
   UT_ASSERT_EQ_UINT(150L, utt_ODInitTerminalSpeedToBaud(B150));
   UT_ASSERT_EQ_UINT(200L, utt_ODInitTerminalSpeedToBaud(B200));
   UT_ASSERT_EQ_UINT(300L, utt_ODInitTerminalSpeedToBaud(B300));
   UT_ASSERT_EQ_UINT(600L, utt_ODInitTerminalSpeedToBaud(B600));
   UT_ASSERT_EQ_UINT(1200L, utt_ODInitTerminalSpeedToBaud(B1200));
   UT_ASSERT_EQ_UINT(1800L, utt_ODInitTerminalSpeedToBaud(B1800));
   UT_ASSERT_EQ_UINT(2400L, utt_ODInitTerminalSpeedToBaud(B2400));
   UT_ASSERT_EQ_UINT(4800L, utt_ODInitTerminalSpeedToBaud(B4800));
   UT_ASSERT_EQ_UINT(9600L, utt_ODInitTerminalSpeedToBaud(B9600));
   UT_ASSERT_EQ_UINT(19200L, utt_ODInitTerminalSpeedToBaud(B19200));
   UT_ASSERT_EQ_UINT(38400L, utt_ODInitTerminalSpeedToBaud(B38400));
}

static void recognizes_available_extended_speeds(void)
{
#ifdef B7200
   UT_ASSERT_EQ_UINT(7200L, utt_ODInitTerminalSpeedToBaud(B7200));
#endif
#ifdef B14400
   UT_ASSERT_EQ_UINT(14400L, utt_ODInitTerminalSpeedToBaud(B14400));
#endif
#ifdef B28800
   UT_ASSERT_EQ_UINT(28800L, utt_ODInitTerminalSpeedToBaud(B28800));
#endif
#ifdef B57600
   UT_ASSERT_EQ_UINT(57600L, utt_ODInitTerminalSpeedToBaud(B57600));
#endif
#ifdef B76800
   UT_ASSERT_EQ_UINT(76800L, utt_ODInitTerminalSpeedToBaud(B76800));
#endif
#ifdef B115200
   UT_ASSERT_EQ_UINT(115200L, utt_ODInitTerminalSpeedToBaud(B115200));
#endif
#ifdef B230400
   UT_ASSERT_EQ_UINT(230400L, utt_ODInitTerminalSpeedToBaud(B230400));
#endif
#ifdef B460800
   UT_ASSERT_EQ_UINT(460800L, utt_ODInitTerminalSpeedToBaud(B460800));
#endif
#ifdef B500000
   UT_ASSERT_EQ_UINT(500000L, utt_ODInitTerminalSpeedToBaud(B500000));
#endif
#ifdef B576000
   UT_ASSERT_EQ_UINT(576000L, utt_ODInitTerminalSpeedToBaud(B576000));
#endif
#ifdef B921600
   UT_ASSERT_EQ_UINT(921600L, utt_ODInitTerminalSpeedToBaud(B921600));
#endif
#ifdef B1000000
   UT_ASSERT_EQ_UINT(1000000L, utt_ODInitTerminalSpeedToBaud(B1000000));
#endif
#ifdef B1152000
   UT_ASSERT_EQ_UINT(1152000L, utt_ODInitTerminalSpeedToBaud(B1152000));
#endif
#ifdef B1500000
   UT_ASSERT_EQ_UINT(1500000L, utt_ODInitTerminalSpeedToBaud(B1500000));
#endif
#ifdef B2000000
   UT_ASSERT_EQ_UINT(2000000L, utt_ODInitTerminalSpeedToBaud(B2000000));
#endif
#ifdef B2500000
   UT_ASSERT_EQ_UINT(2500000L, utt_ODInitTerminalSpeedToBaud(B2500000));
#endif
#ifdef B3000000
   UT_ASSERT_EQ_UINT(3000000L, utt_ODInitTerminalSpeedToBaud(B3000000));
#endif
#ifdef B3500000
   UT_ASSERT_EQ_UINT(3500000L, utt_ODInitTerminalSpeedToBaud(B3500000));
#endif
#ifdef B4000000
   UT_ASSERT_EQ_UINT(4000000L, utt_ODInitTerminalSpeedToBaud(B4000000));
#endif
}

static void rejects_an_unknown_speed(void)
{
   UT_ASSERT_EQ_UINT(0, utt_ODInitTerminalSpeedToBaud((speed_t)-1));
}

static const UTTestCase ut_cases[] = {
   {"standard speeds", recognizes_standard_posix_speeds},
   {"extended speeds", recognizes_available_extended_speeds},
   {"unknown speed", rejects_an_unknown_speed}
};
