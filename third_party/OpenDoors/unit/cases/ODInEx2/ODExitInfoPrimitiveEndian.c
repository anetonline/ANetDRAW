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

static void converts_the_primitive_record_both_ways(void)
{
   static tExitInfoRecord record;
   unsigned index;
   WORD value16 = 0x1234;
   DWORD value32 = 0x89abcdefUL;

   UT_ASSERT_EQ_UINT(902, sizeof(record));
   UT_ASSERT_EQ_UINT(2, (BYTE *)&record.num_calls - (BYTE *)&record);
   UT_ASSERT_EQ_UINT(179, (BYTE *)&record.busyperhour - (BYTE *)&record);
   UT_ASSERT_EQ_UINT(227, (BYTE *)&record.busyperday - (BYTE *)&record);
   UT_ASSERT_EQ_UINT(435, (BYTE *)&record.timelimit - (BYTE *)&record);
   UT_ASSERT_EQ_UINT(453, (BYTE *)&record.bbs - (BYTE *)&record);
   UT_ASSERT_EQ_UINT(648,
      (BYTE *)&record.bbs.qbbs.screenlength - (BYTE *)&record);
   memset(&record, 0, sizeof(record));
   put_le16(&record.baud, value16);
   put_le32(&record.num_calls, value32);
   for(index = 0; index < 24; ++index)
      put_le16(&record.busyperhour[index], (WORD)(0x2000 + index));
   for(index = 0; index < 7; ++index)
      put_le16(&record.busyperday[index], (WORD)(0x3000 + index));
   put_le16(&record.credit, value16);
   put_le16(&record.pending, value16);
   put_le16(&record.posted, value16);
   put_le16(&record.lastread, value16);
   put_le16(&record.sec, value16);
   put_le16(&record.nocalls, value16);
   put_le16(&record.ups, value16);
   put_le16(&record.downs, value16);
   put_le16(&record.upk, value16);
   put_le16(&record.downk, value16);
   put_le16(&record.todayk, value16);
   put_le16(&record.elapsed, value16);
   put_le16(&record.screenlen, value16);
   put_le16(&record.xirecord, value16);
   put_le16(&record.timelimit, value16);
   put_le32(&record.loginsec, value32);
   put_le32(&record.net_credit, value32);
   put_le16(&record.userrecord, value16);
   put_le16(&record.readthru, value16);
   put_le16(&record.numberpages, value16);
   put_le16(&record.downloadlimint, value16);
   put_le16(&record.bbs.qbbs.screenlength, value16);

   utt_ODExitInfoPrimitiveEndian(&record, TRUE);
   UT_ASSERT_EQ_UINT(value16, record.baud);
   UT_ASSERT(record.num_calls == value32);
   for(index = 0; index < 24; ++index)
      UT_ASSERT_EQ_UINT((WORD)(0x2000 + index), record.busyperhour[index]);
   for(index = 0; index < 7; ++index)
      UT_ASSERT_EQ_UINT((WORD)(0x3000 + index), record.busyperday[index]);
   UT_ASSERT_EQ_UINT(value16, record.credit);
   UT_ASSERT_EQ_UINT(value16, record.pending);
   UT_ASSERT_EQ_UINT(value16, record.posted);
   UT_ASSERT_EQ_UINT(value16, record.lastread);
   UT_ASSERT_EQ_UINT(value16, record.sec);
   UT_ASSERT_EQ_UINT(value16, record.nocalls);
   UT_ASSERT_EQ_UINT(value16, record.ups);
   UT_ASSERT_EQ_UINT(value16, record.downs);
   UT_ASSERT_EQ_UINT(value16, record.upk);
   UT_ASSERT_EQ_UINT(value16, record.downk);
   UT_ASSERT_EQ_UINT(value16, record.todayk);
   UT_ASSERT_EQ_UINT(value16, record.elapsed);
   UT_ASSERT_EQ_UINT(value16, record.screenlen);
   UT_ASSERT_EQ_UINT(value16, record.xirecord);
   UT_ASSERT_EQ_UINT(value16, record.timelimit);
   UT_ASSERT(record.loginsec == value32);
   UT_ASSERT(record.net_credit == value32);
   UT_ASSERT_EQ_UINT(value16, record.userrecord);
   UT_ASSERT_EQ_UINT(value16, record.readthru);
   UT_ASSERT_EQ_UINT(value16, record.numberpages);
   UT_ASSERT_EQ_UINT(value16, record.downloadlimint);
   UT_ASSERT_EQ_UINT(value16, (WORD)record.bbs.qbbs.screenlength);

   utt_ODExitInfoPrimitiveEndian(&record, FALSE);
   UT_ASSERT(is_le16(&record.baud, value16));
   UT_ASSERT(is_le32(&record.num_calls, value32));
   for(index = 0; index < 24; ++index)
      UT_ASSERT(is_le16(&record.busyperhour[index], (WORD)(0x2000 + index)));
   for(index = 0; index < 7; ++index)
      UT_ASSERT(is_le16(&record.busyperday[index], (WORD)(0x3000 + index)));
   UT_ASSERT(is_le16(&record.credit, value16));
   UT_ASSERT(is_le16(&record.pending, value16));
   UT_ASSERT(is_le16(&record.posted, value16));
   UT_ASSERT(is_le16(&record.lastread, value16));
   UT_ASSERT(is_le16(&record.sec, value16));
   UT_ASSERT(is_le16(&record.nocalls, value16));
   UT_ASSERT(is_le16(&record.ups, value16));
   UT_ASSERT(is_le16(&record.downs, value16));
   UT_ASSERT(is_le16(&record.upk, value16));
   UT_ASSERT(is_le16(&record.downk, value16));
   UT_ASSERT(is_le16(&record.todayk, value16));
   UT_ASSERT(is_le16(&record.elapsed, value16));
   UT_ASSERT(is_le16(&record.screenlen, value16));
   UT_ASSERT(is_le16(&record.xirecord, value16));
   UT_ASSERT(is_le16(&record.timelimit, value16));
   UT_ASSERT(is_le32(&record.loginsec, value32));
   UT_ASSERT(is_le32(&record.net_credit, value32));
   UT_ASSERT(is_le16(&record.userrecord, value16));
   UT_ASSERT(is_le16(&record.readthru, value16));
   UT_ASSERT(is_le16(&record.numberpages, value16));
   UT_ASSERT(is_le16(&record.downloadlimint, value16));
   UT_ASSERT(is_le16(&record.bbs.qbbs.screenlength, value16));
}

static const UTTestCase ut_cases[] = {
   {"little-endian disk conversion", converts_the_primitive_record_both_ways}
};
