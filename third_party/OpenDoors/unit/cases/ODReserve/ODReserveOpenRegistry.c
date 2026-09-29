#if defined(ODPLAT_WIN32)
#define UT_CUSTOM_MOCK__sopen
#define UT_CUSTOM_MOCK__close
#define UT_CUSTOM_MOCK__errno
#elif defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK___get_errno_ptr
#ifdef __TURBOC__
#define UT_CUSTOM_MOCK_open
#else
#define UT_CUSTOM_MOCK_sopen
#endif
#define UT_CUSTOM_MOCK_close
#else
#define UT_CUSTOM_MOCK_open
#define UT_CUSTOM_MOCK_close
#define UT_CUSTOM_MOCK___error
#define UT_CUSTOM_MOCK___errno_location
#endif
#define UT_CUSTOM_MOCK_ODReserveTransfer
#define UT_CUSTOM_MOCK_memset

static int ut_handles[4];
static int ut_errors[4];
static int ut_open_count;
static int ut_open_index;
static BOOL ut_transfer_result;
static int ut_transfer_calls;
static int ut_close_calls;
static int ut_errno_value;

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
int *utm___get_errno_ptr(void) { return(&ut_errno_value); }
#endif

#if defined(ODPLAT_WIN32)
int *utm__errno(void) { return(&ut_errno_value); }
int utm__sopen(const char *path, int flags, int sharing, ...)
#else
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
#ifdef __TURBOC__
int utm_open(const char *path, int flags, ...)
#else
int utm_sopen(const char *path, int flags, int sharing, ...)
#endif
#else
int *utm___error(void) { return(&ut_errno_value); }
int *utm___errno_location(void) { return(&ut_errno_value); }
int utm_open(const char *path, int flags, ...)
#endif
#endif
{
   int index = ut_open_index++;
   (void)path;
   (void)flags;
#if defined(ODPLAT_WIN32) \
   || ((defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)) \
      && !defined(__TURBOC__))
   (void)sharing;
#endif
   UT_ASSERT(index < ut_open_count);
#ifdef __TURBOC__
   errno = ut_errors[index];
#else
   ut_errno_value = ut_errors[index];
#endif
   return(ut_handles[index]);
}

#if defined(ODPLAT_WIN32)
int utm__close(int file)
#else
int utm_close(int file)
#endif
{ (void)file; ++ut_close_calls; return(0); }

BOOL utm_ODReserveTransfer(int file, long offset, BYTE *buffer,
   size_t size, BOOL write)
{
   size_t index;
   UT_ASSERT_EQ_INT(hODReserveFile, file);
   UT_ASSERT_EQ_INT(0, (int)offset);
   UT_ASSERT_EQ_UINT(OD_RESERVE_HEADER_SIZE, size);
   UT_ASSERT(write);
   for(index = 0; index < size; ++index)
      UT_ASSERT_EQ_INT(0, buffer[index]);
   ++ut_transfer_calls;
   return(ut_transfer_result);
}

void *utm_memset(void *destination, int value, size_t size)
{
   BYTE *bytes = (BYTE *)destination;
   size_t index;
   for(index = 0; index < size; ++index) bytes[index] = (BYTE)value;
   return(destination);
}

static void reset_open(void)
{
   int index;
   for(index = 0; index < 4; ++index)
   {
      ut_handles[index] = -1;
      ut_errors[index] = EIO;
   }
   ut_open_count = 1;
   ut_open_index = 0;
   ut_transfer_result = TRUE;
   ut_transfer_calls = 0;
   ut_close_calls = 0;
   hODReserveFile = -1;
   pszODReservePath = (char *)"NODE.SYN";
}

static void opens_an_existing_registry(void)
{
   BOOL created = TRUE;
   reset_open();
   ut_handles[0] = 7;
   UT_ASSERT(utt_ODReserveOpenRegistry(&created));
   UT_ASSERT(!created);
   UT_ASSERT_EQ_INT(7, hODReserveFile);
   UT_ASSERT_EQ_INT(0, ut_transfer_calls);
}

static void creates_a_missing_registry(void)
{
   BOOL created = FALSE;
   reset_open();
   ut_open_count = 2;
   ut_errors[0] = ENOENT;
   ut_handles[1] = 8;
   UT_ASSERT(utt_ODReserveOpenRegistry(&created));
   UT_ASSERT(created);
   UT_ASSERT_EQ_INT(1, ut_transfer_calls);
}

static void opens_after_a_create_race(void)
{
   BOOL created = TRUE;
   reset_open();
   ut_open_count = 3;
   ut_errors[0] = ENOENT;
   ut_errors[1] = EEXIST;
   ut_handles[2] = 9;
   UT_ASSERT(utt_ODReserveOpenRegistry(&created));
   UT_ASSERT(!created);
}

static void reports_open_and_extension_failures(void)
{
   BOOL created;
   reset_open();
   UT_ASSERT(!utt_ODReserveOpenRegistry(&created));

   reset_open();
   ut_errors[0] = ENOENT;
   ut_open_count = 2;
   UT_ASSERT(!utt_ODReserveOpenRegistry(&created));

   reset_open();
   ut_open_count = 2;
   ut_errors[0] = ENOENT;
   ut_handles[1] = 8;
   ut_transfer_result = FALSE;
   UT_ASSERT(!utt_ODReserveOpenRegistry(&created));
}

static const UTTestCase ut_cases[] = {
   {"existing", opens_an_existing_registry},
   {"create", creates_a_missing_registry},
   {"create race", opens_after_a_create_race},
   {"failures", reports_open_and_extension_failures}
};
