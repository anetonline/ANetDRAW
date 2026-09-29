#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_strcpy

static BOOL ut_call_allowed;
static BOOL ut_malloc_fails;
static char ut_allocation[64];
static void *ut_freed;

BOOL utm_ODSyncPublicCallAllowed(void)
{
   return(ut_call_allowed);
}

void *utm_malloc(size_t size)
{
   UT_ASSERT(size <= sizeof(ut_allocation));
   return(ut_malloc_fails ? NULL : ut_allocation);
}

void utm_free(void *memory)
{
   ut_freed = memory;
}

size_t utm_strlen(const char *text)
{
   const char *end = text;
   while(*end != '\0') ++end;
   return((size_t)(end - text));
}

char *utm_strcpy(char *destination, const char *source)
{
   char *result = destination;
   while((*destination++ = *source++) != '\0') { }
   return(result);
}

static void reset_state(void)
{
   memset(&od_control, 0, sizeof(od_control));
   bODInitialized = FALSE;
   pszODReservePath = NULL;
   bODReserveDetached = TRUE;
   ut_call_allowed = TRUE;
   ut_malloc_fails = FALSE;
   ut_freed = NULL;
   memset(ut_allocation, 0, sizeof(ut_allocation));
}

static void rejects_disallowed_initialized_and_empty_calls(void)
{
   reset_state();
   ut_call_allowed = FALSE;
   UT_ASSERT(!utt_od_reserve_configure("NODE.SYN"));

   reset_state();
   bODInitialized = TRUE;
   UT_ASSERT(!utt_od_reserve_configure("NODE.SYN"));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);

   reset_state();
   UT_ASSERT(!utt_od_reserve_configure(NULL));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);

   reset_state();
   UT_ASSERT(!utt_od_reserve_configure(""));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
}

static void reports_allocation_failure(void)
{
   reset_state();
   ut_malloc_fails = TRUE;
   UT_ASSERT(!utt_od_reserve_configure("NODE.SYN"));
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);
}

static void records_and_replaces_a_path(void)
{
   char old_path[8];

   reset_state();
   pszODReservePath = old_path;
   UT_ASSERT(utt_od_reserve_configure("NODE.SYN"));
   UT_ASSERT_EQ_INT(0, strcmp("NODE.SYN", pszODReservePath));
   UT_ASSERT_EQ_PTR(old_path, ut_freed);
   UT_ASSERT(!bODReserveDetached);
}

static const UTTestCase ut_cases[] = {
   {"invalid calls", rejects_disallowed_initialized_and_empty_calls},
   {"allocation failure", reports_allocation_failure},
   {"record path", records_and_replaces_a_path}
};
