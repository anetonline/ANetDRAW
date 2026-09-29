#define UT_CUSTOM_MOCK_ODReserveAccess
#define UT_CUSTOM_MOCK_time
#define UT_CUSTOM_MOCK_memset
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_memcpy
#if defined(ODPLAT_NIX)
#define UT_CUSTOM_MOCK_getpid
pid_t utm_getpid(void) { return((pid_t)7); }
#elif defined(ODPLAT_WIN32)
#define UT_CUSTOM_MOCK_GetCurrentProcessId
DWORD WINAPI utm_GetCurrentProcessId(void) { return(7UL); }
#endif

static tODReserveLockResult ut_access_result;
static BYTE ut_payload[OD_RESERVE_RECORD_SIZE - 1];

time_t utm_time(time_t *result) { (void)result; return((time_t)11); }
void *utm_memset(void *destination, int value, size_t size)
{
   BYTE *bytes = (BYTE *)destination;
   size_t index;
   for(index = 0; index < size; ++index) bytes[index] = (BYTE)value;
   return(destination);
}
size_t utm_strlen(const char *text)
{
   const char *end = text;
   while(*end != '\0') ++end;
   return((size_t)(end - text));
}
void *utm_memcpy(void *destination, const void *source, size_t size)
{
   BYTE *out = (BYTE *)destination;
   const BYTE *in = (const BYTE *)source;
   size_t index;
   for(index = 0; index < size; ++index) out[index] = in[index];
   return(destination);
}
tODReserveLockResult utm_ODReserveAccess(long offset, BYTE *buffer,
   size_t size, BOOL write, BOOL exclusive, tODMilliSec timeout)
{
   size_t index;
   UT_ASSERT_EQ_INT(OD_RESERVE_HEADER_SIZE + 3 * OD_RESERVE_RECORD_SIZE + 1,
      (int)offset);
   UT_ASSERT_EQ_UINT(OD_RESERVE_RECORD_SIZE - 1, size);
   UT_ASSERT(write && exclusive);
   UT_ASSERT_EQ_UINT(OD_RESERVE_RETRY_TIME, timeout);
   for(index = 0; index < size; ++index) ut_payload[index] = buffer[index];
   return(ut_access_result);
}

static void writes_the_owned_payload(void)
{
   dwODReserveRecord = 3;
   btODReserveKind = OD_RESERVE_KIND_NODE;
   wODReserveNode = 9;
   ut_access_result = kODReserveLockAcquired;
   UT_ASSERT(utt_ODReserveWriteOwn(OD_RESERVE_FLAG_PRESENT,
      NULL, 0, 0));
   UT_ASSERT_EQ_INT(OD_RESERVE_COMMIT,
      ut_payload[OD_RESERVE_SLOT_COMMIT - 1]);
   UT_ASSERT_EQ_INT(OD_RESERVE_KIND_NODE,
      ut_payload[OD_RESERVE_SLOT_KIND - 1]);
   UT_ASSERT_EQ_INT(9, OD_RESERVE_GET_WORD(
      ut_payload + OD_RESERVE_SLOT_NODE - 1));
   UT_ASSERT_EQ_INT(0, ut_payload[OD_RESERVE_SLOT_NAME_LENGTH - 1]);

   UT_ASSERT(utt_ODReserveWriteOwn(OD_RESERVE_FLAG_PRESENT,
      "Shared", 0x12345678UL, 0x9abcdef0UL));
   UT_ASSERT_EQ_INT(6, ut_payload[OD_RESERVE_SLOT_NAME_LENGTH - 1]);
   UT_ASSERT_EQ_INT(0, memcmp(ut_payload + OD_RESERVE_SLOT_NAME - 1,
      "Shared", 6));
   UT_ASSERT_EQ_UINT(0x12345678UL, OD_RESERVE_GET_DWORD(
      ut_payload + OD_RESERVE_SLOT_TICKET_LOW - 1));
   UT_ASSERT_EQ_UINT(0x9abcdef0UL, OD_RESERVE_GET_DWORD(
      ut_payload + OD_RESERVE_SLOT_TICKET_HIGH - 1));
   UT_ASSERT_EQ_UINT(11, OD_RESERVE_GET_DWORD(
      ut_payload + OD_RESERVE_SLOT_STARTED - 1));
}

static void reports_a_write_failure(void)
{
   dwODReserveRecord = 3;
   btODReserveKind = OD_RESERVE_KIND_LOCAL;
   wODReserveNode = 0;
   ut_access_result = kODReserveLockError;
   UT_ASSERT(!utt_ODReserveWriteOwn(OD_RESERVE_FLAG_PRESENT,
      NULL, 0, 0));
}

static const UTTestCase ut_cases[] = {
   {"payload", writes_the_owned_payload},
   {"failure", reports_a_write_failure}
};
