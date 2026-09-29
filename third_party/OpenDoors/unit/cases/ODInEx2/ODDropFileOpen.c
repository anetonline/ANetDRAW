#define UT_CUSTOM_MOCK_fopen
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

static char ut_file;
static FILE *ut_open_result;
static const char *ut_path;
static const char *ut_mode;

FILE *utm_fopen(const char *path, const char *mode)
{
   ut_path = path;
   ut_mode = mode;
   return ut_open_result;
}

static void initializes_a_successful_writer(void)
{
   tODDropFileWriter writer;
   ut_open_result = (FILE *)&ut_file;
   writer.nErrorCode = 99;
   writer.nRuntimeError = 99;
   UT_ASSERT_EQ_INT(TRUE, utt_ODDropFileOpen(&writer, "door.sys", "wb"));
   UT_ASSERT_EQ_PTR(&ut_file, writer.pFile);
   UT_ASSERT_EQ_INT(0, strcmp("door.sys", ut_path));
   UT_ASSERT_EQ_INT(0, strcmp("wb", ut_mode));
   UT_ASSERT_EQ_INT(ERR_NONE, writer.nErrorCode);
   UT_ASSERT_EQ_INT(0, writer.nRuntimeError);
}

static void records_open_failure_and_errno(void)
{
   tODDropFileWriter writer;
   ut_open_result = NULL;
   errno = 37;
   od_control.od_error = ERR_NONE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODDropFileOpen(&writer, "bad", "w"));
   UT_ASSERT_NULL(writer.pFile);
   UT_ASSERT_EQ_INT(ERR_FILEOPEN, writer.nErrorCode);
   UT_ASSERT_EQ_INT(37, writer.nRuntimeError);
   UT_ASSERT_EQ_INT(ERR_FILEOPEN, od_control.od_error);
}

static const UTTestCase ut_cases[] = {
   {"open success", initializes_a_successful_writer},
   {"open failure", records_open_failure_and_errno}
};
