static void successful_queries_preserve_the_reported_state(void)
{
   od_control.od_error = 77;
   UT_ASSERT_EQ_INT(FALSE,
      utt_ODCoreCarrierResult(kODRCSuccess, FALSE));
   UT_ASSERT_EQ_INT(77, od_control.od_error);
   UT_ASSERT_EQ_INT(TRUE,
      utt_ODCoreCarrierResult(kODRCSuccess, TRUE));
   UT_ASSERT_EQ_INT(77, od_control.od_error);
}

static void failed_queries_report_no_carrier_and_set_the_error(void)
{
   od_control.od_error = 0;
   UT_ASSERT_EQ_INT(FALSE,
      utt_ODCoreCarrierResult(kODRCGeneralFailure, TRUE));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
}

static const UTTestCase ut_cases[] = {
   {"successful state", successful_queries_preserve_the_reported_state},
   {"failed query", failed_queries_report_no_carrier_and_set_the_error}
};
