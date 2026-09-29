#define UT_CUSTOM_MOCK_ODTimerElapsed
#define UT_CUSTOM_MOCK_ODTimerStart
#define UT_CUSTOM_MOCK_memmove
#define UT_CUSTOM_MOCK_memset
#define UT_CUSTOM_MOCK_od_get_key
#define UT_CUSTOM_MOCK_strlen
#ifdef ODPLAT_NIX
#define UT_CUSTOM_MOCK_strncasecmp
#else
#define UT_CUSTOM_MOCK_strnicmp
#endif
#ifdef OD_THREAD_SUPPORT
#define UT_CUSTOM_MOCK_od_sleep
#endif

#define MOCK_GET_KEY 120
#define MOCK_SLEEP 121
#define MOCK_TIMER_ELAPSED 122
#define MOCK_TIMER_START 123

static char ut_keys[16];
static unsigned short ut_key_count;
static unsigned short ut_key_index;
static BOOL ut_elapsed[16];
static unsigned short ut_elapsed_count;
static unsigned short ut_elapsed_index;
#ifdef OD_THREAD_SUPPORT
static BOOL ut_disconnect_on_sleep;
#endif

static void reset_fixture(void)
{
   utm_memset(ut_keys, 0, sizeof(ut_keys));
   utm_memset(ut_elapsed, 0, sizeof(ut_elapsed));
   ut_key_count = 0;
   ut_key_index = 0;
   ut_elapsed_count = 0;
   ut_elapsed_index = 0;
#ifdef OD_THREAD_SUPPORT
   bODInitialized = TRUE;
   ut_disconnect_on_sleep = FALSE;
#endif
}

BOOL utm_ODTimerElapsed(tODTimer *timer)
{
   (void)timer;
   UT_ASSERT(ut_elapsed_index < ut_elapsed_count);
   ut_mock_called(MOCK_TIMER_ELAPSED);
   return ut_elapsed[ut_elapsed_index++];
}

void utm_ODTimerStart(tODTimer *timer, tODMilliSec duration)
{
   UT_ASSERT_NOT_NULL(timer);
   UT_ASSERT_EQ_UINT(660, duration);
   ut_mock_called(MOCK_TIMER_START);
}

void *utm_memmove(void *destination, const void *source, size_t size)
{
   unsigned char *to = (unsigned char *)destination;
   const unsigned char *from = (const unsigned char *)source;
   if(to < from)
   {
      size_t index;
      for(index = 0; index < size; ++index)
         to[index] = from[index];
   }
   else if(to > from)
   {
      while(size != 0)
      {
         --size;
         to[size] = from[size];
      }
   }
   return destination;
}

void *utm_memset(void *destination, int value, size_t size)
{
   unsigned char *to = (unsigned char *)destination;
   size_t index;
   for(index = 0; index < size; ++index)
      to[index] = (unsigned char)value;
   return destination;
}

char ODCALL utm_od_get_key(BOOL wait)
{
   UT_ASSERT_EQ_INT(FALSE, wait);
   UT_ASSERT(ut_key_index < ut_key_count);
   ut_mock_called(MOCK_GET_KEY);
   return ut_keys[ut_key_index++];
}

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0')
      ++length;
   return length;
}

static int compare_without_case(const char *left, const char *right,
   size_t size)
{
   size_t index;
   for(index = 0; index < size; ++index)
   {
      int l = tolower((unsigned char)left[index]);
      int r = tolower((unsigned char)right[index]);
      if(l != r || l == 0 || r == 0)
         return l - r;
   }
   return 0;
}

#ifdef ODPLAT_NIX
int utm_strncasecmp(const char *left, const char *right, size_t size)
{
   return compare_without_case(left, right, size);
}
#else
int utm_strnicmp(const char *left, const char *right, size_t size)
{
   return compare_without_case(left, right, size);
}
#endif

#ifdef OD_THREAD_SUPPORT
void ODCALL utm_od_sleep(tODMilliSec duration)
{
   UT_ASSERT_EQ_UINT(0, duration);
   ut_mock_called(MOCK_SLEEP);
   if(ut_disconnect_on_sleep)
      bODInitialized = FALSE;
}
#endif

static void one_character_match_returns_immediately(void)
{
   reset_fixture();
   ut_keys[0] = 'a';
   ut_key_count = 1;

   UT_ASSERT_EQ_INT(TRUE, utt_ODWaitNoCase("A", 660));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_GET_KEY));
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_TIMER_ELAPSED));
}

static void sliding_window_matches_without_case(void)
{
   reset_fixture();
   ut_keys[0] = 'x';
   ut_keys[1] = 'A';
   ut_keys[2] = 'b';
   ut_keys[3] = 'C';
   ut_key_count = 4;
   ut_elapsed[0] = FALSE;
   ut_elapsed[1] = FALSE;
   ut_elapsed[2] = FALSE;
   ut_elapsed_count = 3;

   UT_ASSERT_EQ_INT(TRUE, utt_ODWaitNoCase("abcde", 660));
   UT_ASSERT_EQ_UINT(4, ut_mock_count(MOCK_GET_KEY));
   UT_ASSERT_EQ_UINT(3, ut_mock_count(MOCK_TIMER_ELAPSED));
}

static void timeout_without_input_returns_false(void)
{
   reset_fixture();
   ut_keys[0] = 0;
   ut_keys[1] = 0;
   ut_key_count = 2;
   ut_elapsed[0] = FALSE;
   ut_elapsed[1] = TRUE;
   ut_elapsed_count = 2;

   UT_ASSERT_EQ_INT(FALSE, utt_ODWaitNoCase("xyz", 660));
   UT_ASSERT_EQ_UINT(2, ut_mock_count(MOCK_TIMER_ELAPSED));
#ifdef OD_THREAD_SUPPORT
   UT_ASSERT_EQ_UINT(2, ut_mock_count(MOCK_SLEEP));
#endif
}

#ifdef OD_THREAD_SUPPORT
static void disconnect_while_yielding_returns_false(void)
{
   reset_fixture();
   ut_keys[0] = 0;
   ut_key_count = 1;
   ut_disconnect_on_sleep = TRUE;

   UT_ASSERT_EQ_INT(FALSE, utt_ODWaitNoCase("xyz", 660));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_SLEEP));
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_TIMER_ELAPSED));
}
#endif

static const UTTestCase ut_cases[] = {
   {"one-character match", one_character_match_returns_immediately},
   {"sliding case-insensitive window", sliding_window_matches_without_case},
   {"timeout without input", timeout_without_input_returns_false},
#ifdef OD_THREAD_SUPPORT
   {"disconnect while yielding", disconnect_while_yielding_returns_false},
#endif
};
