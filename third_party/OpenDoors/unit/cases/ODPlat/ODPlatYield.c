static void yields_for_each_detected_multitasker(void)
{
   ODMultitasker = kMultitaskerDV;
   utt_ODPlatYield();
   ODMultitasker = kMultitaskerWin;
   utt_ODPlatYield();
   ODMultitasker = kMultitaskerOS2;
   utt_ODPlatYield();
   ODMultitasker = (tODMultitasker)99;
   utt_ODPlatYield();
}

static const UTTestCase ut_cases[] = {
   {"multitaskers", yields_for_each_detected_multitasker}
};
