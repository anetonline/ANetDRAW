#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK__lseek
#else
#define UT_CUSTOM_MOCK_lseek
#endif
static long ut_length;
static int ut_seek_result;
#if defined(ODPLAT_WIN32)
long utm__lseek(int file, long offset, int whence)
#elif defined(ODPLAT_NIX)
off_t utm_lseek(int file, off_t offset, int whence)
#else
long utm_lseek(int file, long offset, int whence)
#endif
{
   UT_ASSERT_EQ_INT(hODReserveFile, file);
   UT_ASSERT_EQ_INT(0, (int)offset);
   UT_ASSERT_EQ_INT(SEEK_END, whence);
   return(ut_seek_result == 0 ? ut_length : -1L);
}
static void accepts_an_empty_registry(void)
{
   DWORD count = 99;
   ut_seek_result = 0;
   ut_length = OD_RESERVE_HEADER_SIZE;
   UT_ASSERT(utt_ODReserveCountRecords(&count));
   UT_ASSERT_EQ_UINT(0, count);
}
static void reports_invalid_sizes_and_access_failures(void)
{
   DWORD count = 99;
   ut_seek_result = -1;
   UT_ASSERT(!utt_ODReserveCountRecords(&count));
   ut_seek_result = 0;
   ut_length = OD_RESERVE_HEADER_SIZE - 1;
   UT_ASSERT(!utt_ODReserveCountRecords(&count));
}
static void counts_every_physical_record(void)
{
   DWORD count = 99;
   ut_length = OD_RESERVE_HEADER_SIZE + 2 * OD_RESERVE_RECORD_SIZE;
   ut_seek_result = 0;
   UT_ASSERT(utt_ODReserveCountRecords(&count));
   UT_ASSERT_EQ_UINT(2, count);

   /* An incomplete trailing slot is never reused either. */
   ut_length += 17;
   UT_ASSERT(utt_ODReserveCountRecords(&count));
   UT_ASSERT_EQ_UINT(3, count);
}
static const UTTestCase ut_cases[] = {
   {"empty", accepts_an_empty_registry},
   {"failures", reports_invalid_sizes_and_access_failures},
   {"physical records", counts_every_physical_record}
};
