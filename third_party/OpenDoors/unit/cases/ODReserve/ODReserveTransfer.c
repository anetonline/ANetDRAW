#if defined(ODPLAT_WIN32)
#define UT_CUSTOM_MOCK__lseek
#define UT_CUSTOM_MOCK__read
#define UT_CUSTOM_MOCK__write
#else
#define UT_CUSTOM_MOCK_lseek
#define UT_CUSTOM_MOCK_read
#define UT_CUSTOM_MOCK_write
#endif

static int ut_seek_result;
static int ut_results[3];
static int ut_result_index;
static long ut_expected_offset;

#if defined(ODPLAT_WIN32)
long utm__lseek(int file, long offset, int whence)
#elif defined(ODPLAT_NIX)
off_t utm_lseek(int file, off_t offset, int whence)
#else
long utm_lseek(int file, long offset, int whence)
#endif
{
   UT_ASSERT_EQ_INT(7, file);
   UT_ASSERT_EQ_INT((int)ut_expected_offset, (int)offset);
   UT_ASSERT_EQ_INT(SEEK_SET, whence);
   return(ut_seek_result);
}

#if defined(ODPLAT_NIX)
ssize_t utm_read(int file, void *buffer, size_t count)
{
   (void)buffer;
   UT_ASSERT_EQ_INT(7, file);
   UT_ASSERT(count <= 5);
   return((ssize_t)ut_results[ut_result_index++]);
}
ssize_t utm_write(int file, const void *buffer, size_t count)
{
   (void)buffer;
   UT_ASSERT_EQ_INT(7, file);
   UT_ASSERT(count <= 5);
   return((ssize_t)ut_results[ut_result_index++]);
}
#else
#ifdef ODPLAT_WIN32
int utm__read(int file, void *buffer, unsigned int count)
#else
int utm_read(int file, void *buffer, unsigned int count)
#endif
{
   (void)buffer;
   UT_ASSERT_EQ_INT(7, file);
   UT_ASSERT(count <= 5U);
   return(ut_results[ut_result_index++]);
}
#ifdef ODPLAT_WIN32
int utm__write(int file, const void *buffer, unsigned int count)
#else
#ifdef __TURBOC__
int utm_write(int file, void *buffer, unsigned int count)
#else
int utm_write(int file, const void *buffer, unsigned int count)
#endif
#endif
{
   (void)buffer;
   UT_ASSERT_EQ_INT(7, file);
   UT_ASSERT(count <= 5U);
   return(ut_results[ut_result_index++]);
}
#endif

static void reset_transfer(void)
{
   ut_seek_result = 9;
   ut_expected_offset = 9;
   ut_results[0] = 5;
   ut_results[1] = 0;
   ut_results[2] = 0;
   ut_result_index = 0;
}

static void handles_complete_and_partial_transfers(void)
{
   BYTE buffer[5] = {0, 0, 0, 0, 0};

   reset_transfer();
   UT_ASSERT(utt_ODReserveTransfer(7, 9, buffer, sizeof(buffer), TRUE));
   UT_ASSERT_EQ_INT(1, ut_result_index);

   reset_transfer();
   ut_results[0] = 2;
   ut_results[1] = 3;
   UT_ASSERT(utt_ODReserveTransfer(7, 9, buffer, sizeof(buffer), FALSE));
   UT_ASSERT_EQ_INT(2, ut_result_index);
}

static void reports_seek_and_transfer_failures(void)
{
   BYTE buffer[5] = {0, 0, 0, 0, 0};

   reset_transfer();
   ut_seek_result = -1;
   UT_ASSERT(!utt_ODReserveTransfer(7, 9, buffer, sizeof(buffer), TRUE));
   UT_ASSERT_EQ_INT(0, ut_result_index);

   reset_transfer();
   ut_results[0] = 0;
   UT_ASSERT(!utt_ODReserveTransfer(7, 9, buffer, sizeof(buffer), FALSE));

   reset_transfer();
   ut_results[0] = -1;
   UT_ASSERT(!utt_ODReserveTransfer(7, 9, buffer, sizeof(buffer), TRUE));

   reset_transfer();
   ut_results[0] = 6;
   UT_ASSERT(!utt_ODReserveTransfer(7, 9, buffer, sizeof(buffer), FALSE));
}

static const UTTestCase ut_cases[] = {
   {"complete and partial", handles_complete_and_partial_transfers},
   {"failures", reports_seek_and_transfer_failures}
};
