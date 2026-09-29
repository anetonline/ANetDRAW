static void put_le16(void *field, WORD value)
{
   BYTE *bytes = (BYTE *)field;
   bytes[0] = (BYTE)value;
   bytes[1] = (BYTE)(value >> 8);
}

static void put_le32(void *field, DWORD value)
{
   BYTE *bytes = (BYTE *)field;
   bytes[0] = (BYTE)value;
   bytes[1] = (BYTE)(value >> 8);
   bytes[2] = (BYTE)(value >> 16);
   bytes[3] = (BYTE)(value >> 24);
}

static BOOL is_le16(const void *field, WORD value)
{
   const BYTE *bytes = (const BYTE *)field;
   return(bytes[0] == (BYTE)value && bytes[1] == (BYTE)(value >> 8));
}

static BOOL is_le32(const void *field, DWORD value)
{
   const BYTE *bytes = (const BYTE *)field;
   return(bytes[0] == (BYTE)value && bytes[1] == (BYTE)(value >> 8) &&
      bytes[2] == (BYTE)(value >> 16) && bytes[3] == (BYTE)(value >> 24));
}

#define UT_RA2_16_FIELDS(operation) \
   operation(record.baud, value16); \
   operation(record.posted, value16); \
   operation(record.sec, value16); \
   operation(record.elapsed, value16); \
   operation(record.screenlen, value16); \
   operation(record.group, value16); \
   operation(record.msgarea, value16); \
   operation(record.filearea, value16); \
   operation(record.file_group, value16); \
   operation(record.msg_group, value16); \
   operation(record.timelimit, value16); \
   operation(record.userrecord, value16); \
   operation(record.readthru, value16); \
   operation(record.numberpages, value16); \
   operation(record.downloadlimit, value16); \
   operation(record.deducted_time, value16); \
   operation(record.menu_cost_per_min, value16)

#define UT_RA2_32_FIELDS(operation) \
   operation(record.num_calls, value32); \
   operation(record.password_crc, value32); \
   operation(record.credit, value32); \
   operation(record.pending, value32); \
   operation(record.lastread, value32); \
   operation(record.nocalls, value32); \
   operation(record.ups, value32); \
   operation(record.downs, value32); \
   operation(record.upk, value32); \
   operation(record.downk, value32); \
   operation(record.todayk, value32); \
   operation(record.xirecord, value32); \
   operation(record.loginsec, value32); \
   operation(record.logonpasswordcrc, value32)

#define UT_PUT16(field, value) put_le16(&(field), (WORD)(value))
#define UT_PUT32(field, value) put_le32(&(field), (DWORD)(value))
#define UT_ASSERT_HOST16(field, value) UT_ASSERT_EQ_UINT((WORD)(value), (WORD)(field))
#define UT_ASSERT_HOST32(field, value) UT_ASSERT((DWORD)(field) == (DWORD)(value))
#define UT_ASSERT_LE16(field, value) UT_ASSERT(is_le16(&(field), (WORD)(value)))
#define UT_ASSERT_LE32(field, value) UT_ASSERT(is_le32(&(field), (DWORD)(value)))

static void converts_the_ra2_record_both_ways(void)
{
   static tRA2ExitInfoRecord record;
   unsigned index;
   WORD value16 = 0x1234;
   DWORD value32 = 0x89abcdefUL;

   UT_ASSERT_EQ_UINT(2363, sizeof(record));
   UT_ASSERT_EQ_UINT(2, (BYTE *)&record.num_calls - (BYTE *)&record);
   UT_ASSERT_EQ_UINT(728,
      (BYTE *)&record.combinedrecord - (BYTE *)&record);
   UT_ASSERT_EQ_UINT(1293,
      (BYTE *)&record.timelimit - (BYTE *)&record);
   memset(&record, 0, sizeof(record));
   UT_RA2_16_FIELDS(UT_PUT16);
   UT_RA2_32_FIELDS(UT_PUT32);
   for(index = 0; index < 24; ++index)
      put_le16(&record.busyperhour[index], (WORD)(0x2000 + index));
   for(index = 0; index < 7; ++index)
      put_le16(&record.busyperday[index], (WORD)(0x3000 + index));
   for(index = 0; index < 200; ++index)
      put_le16(&record.combinedrecord[index], (WORD)(0x4000 + index));

   utt_ODExitInfoRA2Endian(&record, TRUE);
   UT_RA2_16_FIELDS(UT_ASSERT_HOST16);
   UT_RA2_32_FIELDS(UT_ASSERT_HOST32);
   for(index = 0; index < 24; ++index)
      UT_ASSERT_EQ_UINT((WORD)(0x2000 + index), record.busyperhour[index]);
   for(index = 0; index < 7; ++index)
      UT_ASSERT_EQ_UINT((WORD)(0x3000 + index), record.busyperday[index]);
   for(index = 0; index < 200; ++index)
      UT_ASSERT_EQ_UINT((WORD)(0x4000 + index), record.combinedrecord[index]);

   utt_ODExitInfoRA2Endian(&record, FALSE);
   UT_RA2_16_FIELDS(UT_ASSERT_LE16);
   UT_RA2_32_FIELDS(UT_ASSERT_LE32);
   for(index = 0; index < 24; ++index)
      UT_ASSERT(is_le16(&record.busyperhour[index], (WORD)(0x2000 + index)));
   for(index = 0; index < 7; ++index)
      UT_ASSERT(is_le16(&record.busyperday[index], (WORD)(0x3000 + index)));
   for(index = 0; index < 200; ++index)
      UT_ASSERT(is_le16(&record.combinedrecord[index], (WORD)(0x4000 + index)));
}

static const UTTestCase ut_cases[] = {
   {"little-endian disk conversion", converts_the_ra2_record_both_ways}
};
