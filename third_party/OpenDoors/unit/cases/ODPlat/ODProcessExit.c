#include <setjmp.h>

#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ExitProcess
#else
#define UT_CUSTOM_MOCK_exit
#endif

static jmp_buf ut_exit_target;
static int ut_exit_code;

#ifdef ODPLAT_WIN32
void WINAPI utm_ExitProcess(UINT exit_code)
{
   ut_exit_code = (int)exit_code;
   longjmp(ut_exit_target, 1);
}
#else
void utm_exit(int exit_code)
{
   ut_exit_code = exit_code;
   longjmp(ut_exit_target, 1);
}
#endif

static void passes_the_exit_code_to_the_platform(void)
{
   ut_exit_code = -1;
   if(setjmp(ut_exit_target) == 0)
   {
      utt_ODProcessExit(23);
      UT_ASSERT(FALSE);
   }
   UT_ASSERT_EQ_INT(23, ut_exit_code);
}

static const UTTestCase ut_cases[] = {
   {"exit code", passes_the_exit_code_to_the_platform}
};
