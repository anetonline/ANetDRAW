#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_odthread_wrapper

static struct odthread_args ut_arguments;
static BOOL ut_malloc_fails;
static unsigned ut_free_calls;
static unsigned ut_create_calls;
static void *ut_expected_parameter;

void *utm_malloc(size_t size)
{
   UT_ASSERT_EQ_UINT(sizeof(ut_arguments), size);
   return(ut_malloc_fails ? NULL : &ut_arguments);
}

void utm_free(void *memory)
{
   ++ut_free_calls;
   UT_ASSERT_EQ_PTR(&ut_arguments, memory);
}

static DWORD OD_THREAD_FUNC ut_thread_proc(void *argument)
{
   (void)argument;
   return(0);
}

#ifdef ODPLAT_WIN32
static unsigned __stdcall utm_odthread_wrapper(void *argument)
{
   (void)argument;
   return(0);
}
#else
static void *utm_odthread_wrapper(void *argument)
{
   (void)argument;
   return(NULL);
}
#endif

#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK__beginthreadex
static DWORD_PTR ut_create_result;

uintptr_t __cdecl utm__beginthreadex(void *security, unsigned stack_size,
   unsigned (__stdcall *start)(void *), void *argument, unsigned flags,
   unsigned *thread_id)
{
   ++ut_create_calls;
   UT_ASSERT_NULL(security);
   UT_ASSERT_EQ_UINT(0, stack_size);
   UT_ASSERT(start == utm_odthread_wrapper);
   UT_ASSERT_EQ_PTR(&ut_arguments, argument);
   UT_ASSERT_EQ_UINT(0, flags);
   UT_ASSERT_NOT_NULL(thread_id);
   *thread_id = 31;
   return((uintptr_t)ut_create_result);
}
#else
#define UT_CUSTOM_MOCK_pthread_create
static int ut_create_result;

int utm_pthread_create(pthread_t *thread, const pthread_attr_t *attributes,
   void *(*start)(void *), void *argument)
{
   ++ut_create_calls;
   UT_ASSERT_NOT_NULL(thread);
   UT_ASSERT_NULL(attributes);
   UT_ASSERT(start == utm_odthread_wrapper);
   UT_ASSERT_EQ_PTR(&ut_arguments, argument);
   *thread = (pthread_t)31;
   return(ut_create_result);
}
#endif

static void reset_create(void)
{
   memset(&ut_arguments, 0, sizeof(ut_arguments));
   ut_malloc_fails = FALSE;
   ut_free_calls = 0;
   ut_create_calls = 0;
   ut_create_result = 0;
   ut_expected_parameter = &ut_create_calls;
}

static void reports_allocation_failure(void)
{
   tODThreadHandle handle;
   reset_create();
   ut_malloc_fails = TRUE;
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_INT(kODRCNoMemory,
      utt_ODThreadCreate(&handle, ut_thread_proc, ut_expected_parameter));
#else
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODThreadCreate(&handle, ut_thread_proc, ut_expected_parameter));
#endif
   UT_ASSERT_EQ_UINT(0, ut_create_calls);
}

static void releases_arguments_when_native_creation_fails(void)
{
   tODThreadHandle handle;
   reset_create();
#ifdef ODPLAT_WIN32
   ut_create_result = 0;
#else
   ut_create_result = 1;
#endif
   UT_ASSERT_EQ_INT(kODRCGeneralFailure,
      utt_ODThreadCreate(&handle, ut_thread_proc, ut_expected_parameter));
   UT_ASSERT_EQ_UINT(1, ut_create_calls);
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
}

static void returns_the_native_thread_handle(void)
{
   tODThreadHandle handle;
   reset_create();
#ifdef ODPLAT_WIN32
   ut_create_result = 41;
#endif
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODThreadCreate(&handle, ut_thread_proc, ut_expected_parameter));
   UT_ASSERT_EQ_PTR(ut_thread_proc, ut_arguments.func);
   UT_ASSERT_EQ_PTR(ut_expected_parameter, ut_arguments.arg);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_PTR((HANDLE)(DWORD_PTR)41, handle);
#else
   UT_ASSERT(handle == (pthread_t)31);
#endif
   UT_ASSERT_EQ_UINT(0, ut_free_calls);
}

static const UTTestCase ut_cases[] = {
   {"allocation failure", reports_allocation_failure},
   {"native failure", releases_arguments_when_native_creation_fails},
   {"success", returns_the_native_thread_handle}
};
