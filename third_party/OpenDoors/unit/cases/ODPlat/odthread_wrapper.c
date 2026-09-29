#define UT_CUSTOM_MOCK_free

static struct odthread_args ut_arguments;
static void *ut_expected_argument;
static unsigned ut_free_calls;
static unsigned ut_thread_calls;

void utm_free(void *memory)
{
   ++ut_free_calls;
   UT_ASSERT_EQ_PTR(&ut_arguments, memory);
}

static DWORD OD_THREAD_FUNC ut_thread_proc(void *argument)
{
   ++ut_thread_calls;
   UT_ASSERT_EQ_PTR(ut_expected_argument, argument);
   return(47);
}

static void copies_arguments_before_releasing_the_wrapper(void)
{
   ut_expected_argument = &ut_thread_calls;
   ut_arguments.func = ut_thread_proc;
   ut_arguments.arg = ut_expected_argument;
   ut_free_calls = 0;
   ut_thread_calls = 0;
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_UINT(47, utt_odthread_wrapper(&ut_arguments));
#else
   UT_ASSERT_NULL(utt_odthread_wrapper(&ut_arguments));
#endif
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
   UT_ASSERT_EQ_UINT(1, ut_thread_calls);
}

static const UTTestCase ut_cases[] = {
   {"argument ownership", copies_arguments_before_releasing_the_wrapper}
};
