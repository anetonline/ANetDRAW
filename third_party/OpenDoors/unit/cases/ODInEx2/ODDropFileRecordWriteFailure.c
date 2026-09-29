#if defined(ODPLAT_NIX) || defined(ODPLAT_WIN32) || defined(__WATCOMC__)
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK__errno
#define _errno utm__errno
#elif defined(__WATCOMC__)
#define UT_CUSTOM_MOCK___get_errno_ptr
#define __get_errno_ptr utm___get_errno_ptr
#endif
static int ut_errno;
#ifdef ODPLAT_WIN32
int *utm__errno(void) { return &ut_errno; }
#elif defined(__WATCOMC__)
int *utm___get_errno_ptr(void) { return &ut_errno; }
#else
#define UT_ERRNO_STORAGE ut_errno
#include "../unix_errno_mock.h"
#endif
#endif

static void records_only_the_first_failure(void)
{
   tODDropFileWriter writer;
   writer.nErrorCode = ERR_NONE;
   writer.nRuntimeError = 0;
   errno = 41;
   utt_ODDropFileRecordWriteFailure(&writer);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, writer.nErrorCode);
   UT_ASSERT_EQ_INT(41, writer.nRuntimeError);

   errno = 42;
   utt_ODDropFileRecordWriteFailure(&writer);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, writer.nErrorCode);
   UT_ASSERT_EQ_INT(41, writer.nRuntimeError);
}

static const UTTestCase ut_cases[] = {
   {"first failure", records_only_the_first_failure}
};
