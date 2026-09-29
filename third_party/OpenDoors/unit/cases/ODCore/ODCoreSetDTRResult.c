static void success_preserves_the_existing_error(void)
{
   od_control.od_error = 77;
   utt_ODCoreSetDTRResult(kODRCSuccess);
   UT_ASSERT_EQ_INT(77, od_control.od_error);
}

static void failure_sets_the_general_error(void)
{
   od_control.od_error = 0;
   utt_ODCoreSetDTRResult(kODRCGeneralFailure);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
}

static const UTTestCase ut_cases[] = {
   {"successful DTR", success_preserves_the_existing_error},
   {"failed DTR", failure_sets_the_general_error}
};
