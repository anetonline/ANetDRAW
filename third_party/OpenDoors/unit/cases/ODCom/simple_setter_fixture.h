static tPortInfo ut_port;

static void stores_the_value_on_a_closed_port(void)
{
   memset(&ut_port, 0, sizeof(ut_port));
   UT_ASSERT_EQ_INT(kODRCSuccess,
      UT_CALL(ODPTR2HANDLE(&ut_port, tPortInfo), UT_VALUE));
   UT_ASSERT((long)UT_FIELD(&ut_port) == (long)UT_VALUE);
}

static const UTTestCase ut_cases[] = {
   {"closed port", stores_the_value_on_a_closed_port}
};
