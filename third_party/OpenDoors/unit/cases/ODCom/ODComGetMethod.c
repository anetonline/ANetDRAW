static tPortInfo ut_port;

static void returns_the_recorded_method(void)
{
   tComMethod method = kComMethodUnspecified;
   memset(&ut_port, 0, sizeof(ut_port));
   ut_port.Method = kComMethodSocket;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODComGetMethod(
      ODPTR2HANDLE(&ut_port, tPortInfo), &method));
   UT_ASSERT_EQ_INT(kComMethodSocket, method);
}

static const UTTestCase ut_cases[] = {
   {"method", returns_the_recorded_method}
};
