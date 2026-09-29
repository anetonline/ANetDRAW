static void writes_little_endian_bytes(void)
{
   BYTE bytes[6] = {0, 0, 0, 0, 0, 0};
   utt_ODSessionSnapshotPutDWORD(bytes + 1, 0x78563412UL);
   UT_ASSERT_EQ_UINT(0, bytes[0]);
   UT_ASSERT_EQ_UINT(0x12, bytes[1]);
   UT_ASSERT_EQ_UINT(0x34, bytes[2]);
   UT_ASSERT_EQ_UINT(0x56, bytes[3]);
   UT_ASSERT_EQ_UINT(0x78, bytes[4]);
   UT_ASSERT_EQ_UINT(0, bytes[5]);
}

static const UTTestCase ut_cases[] = {
   {"little endian write", writes_little_endian_bytes}
};
