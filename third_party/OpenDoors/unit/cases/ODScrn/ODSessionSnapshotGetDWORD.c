static void reads_little_endian_bytes(void)
{
   BYTE bytes[] = {0xff, 0x12, 0x34, 0x56, 0x78, 0xee};
   UT_ASSERT(utt_ODSessionSnapshotGetDWORD(bytes + 1) == 0x78563412UL);
}

static const UTTestCase ut_cases[] = {
   {"little endian read", reads_little_endian_bytes}
};
