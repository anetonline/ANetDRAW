static void converts_the_ra1_extension_both_ways(void)
{
   static tExtendedExitInfo record;
   BYTE *bytes = (BYTE *)&record.deducted_time;

   UT_ASSERT_EQ_UINT(1017, sizeof(record));
   UT_ASSERT_EQ_UINT(2, (BYTE *)&record.menustack - (BYTE *)&record);
   memset(&record, 0, sizeof(record));
   bytes[0] = 0x34;
   bytes[1] = 0x12;
   utt_ODExitInfoExtendedEndian(&record, TRUE);
   UT_ASSERT_EQ_UINT(0x1234, (WORD)record.deducted_time);
   utt_ODExitInfoExtendedEndian(&record, FALSE);
   UT_ASSERT_EQ_UINT(0x34, bytes[0]);
   UT_ASSERT_EQ_UINT(0x12, bytes[1]);
}

static const UTTestCase ut_cases[] = {
   {"little-endian disk conversion", converts_the_ra1_extension_both_ways}
};
