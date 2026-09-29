#define UT_CUSTOM_MOCK_fclose
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

static int ut_close_result;
static int ut_close_errno;
static unsigned ut_close_calls;

int utm_fclose(FILE *file)
{
   ++ut_close_calls;
   UT_ASSERT_NOT_NULL(file);
   errno = ut_close_errno;
   return ut_close_result;
}

static void reports_a_preexisting_failure_without_a_stream(void)
{
   tODDropFileWriter writer;
   writer.pFile = NULL;
   writer.nErrorCode = ERR_FILEOPEN;
   writer.nRuntimeError = 51;
   od_control.od_error = ERR_NONE;
   errno = 0;
   ut_close_calls = 0;
   UT_ASSERT_EQ_INT(FALSE, utt_ODDropFileClose(&writer));
   UT_ASSERT_EQ_INT(ERR_FILEOPEN, od_control.od_error);
   UT_ASSERT_EQ_INT(51, errno);
   UT_ASSERT_EQ_UINT(0, ut_close_calls);

   writer.nErrorCode = ERR_NONE;
   od_control.od_error = ERR_GENERALFAILURE;
   errno = 52;
   UT_ASSERT_EQ_INT(FALSE, utt_ODDropFileClose(&writer));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_INT(52, errno);
}

static void closes_successfully(void)
{
   tODDropFileWriter writer;
   writer.pFile = (FILE *)&writer;
   writer.nErrorCode = ERR_NONE;
   writer.nRuntimeError = 0;
   ut_close_calls = 0;
   ut_close_result = 0;
   ut_close_errno = 0;
   UT_ASSERT_EQ_INT(TRUE, utt_ODDropFileClose(&writer));
   UT_ASSERT_NULL(writer.pFile);
   UT_ASSERT_EQ_UINT(1, ut_close_calls);
}

static void preserves_write_failure_over_close_failure(void)
{
   tODDropFileWriter writer;
   writer.pFile = (FILE *)&writer;
   writer.nErrorCode = ERR_FILEOPEN;
   writer.nRuntimeError = 61;
   ut_close_result = -1;
   ut_close_errno = 62;
   UT_ASSERT_EQ_INT(FALSE, utt_ODDropFileClose(&writer));
   UT_ASSERT_NULL(writer.pFile);
   UT_ASSERT_EQ_INT(ERR_FILEOPEN, od_control.od_error);
   UT_ASSERT_EQ_INT(61, errno);
}

static void records_close_failure(void)
{
   tODDropFileWriter writer;
   writer.pFile = (FILE *)&writer;
   writer.nErrorCode = ERR_NONE;
   writer.nRuntimeError = 0;
   ut_close_result = -1;
   ut_close_errno = 71;
   UT_ASSERT_EQ_INT(FALSE, utt_ODDropFileClose(&writer));
   UT_ASSERT_NULL(writer.pFile);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_INT(71, errno);
}

static const UTTestCase ut_cases[] = {
   {"missing stream", reports_a_preexisting_failure_without_a_stream},
   {"close success", closes_successfully},
   {"write failure wins", preserves_write_failure_over_close_failure},
   {"close failure", records_close_failure}
};
