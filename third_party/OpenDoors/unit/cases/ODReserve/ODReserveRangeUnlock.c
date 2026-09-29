#if defined(ODPLAT_NIX)
#include <stdarg.h>
#define UT_CUSTOM_MOCK_fcntl
#define UT_CUSTOM_MOCK_memset
static int ut_result;
int utm_fcntl(int fd, int command, ...)
{
   va_list arguments;
   struct flock *lock;
   (void)fd;
   (void)command;
   va_start(arguments, command);
   lock = va_arg(arguments, struct flock *);
   va_end(arguments);
   UT_ASSERT_EQ_INT(F_UNLCK, lock->l_type);
   return(ut_result);
}
void *utm_memset(void *destination, int value, size_t size)
{
   unsigned char *bytes = (unsigned char *)destination;
   size_t index;
   for(index = 0; index < size; ++index) bytes[index] = (unsigned char)value;
   return(destination);
}
#else
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK__get_osfhandle
#define UT_CUSTOM_MOCK_UnlockFile
static BOOL ut_result;
intptr_t utm__get_osfhandle(int fd)
{ UT_ASSERT_EQ_INT(7, fd); return((intptr_t)8); }
BOOL WINAPI utm_UnlockFile(HANDLE file, DWORD low, DWORD high,
   DWORD count_low, DWORD count_high)
{
   (void)file;
   (void)low;
   (void)high;
   (void)count_low;
   (void)count_high;
   return(ut_result);
}
#else /* DOS */
#ifdef __TURBOC__
#define UT_CUSTOM_MOCK_unlock
#else
#define UT_CUSTOM_MOCK_lseek
#define UT_CUSTOM_MOCK_locking
#endif
static int ut_seek_result;
static int ut_result;
#ifndef __TURBOC__
long utm_lseek(int file, long offset, int whence)
{
   UT_ASSERT_EQ_INT(7, file);
   UT_ASSERT_EQ_INT(4, (int)offset);
   UT_ASSERT_EQ_INT(SEEK_SET, whence);
   return(ut_seek_result == 0 ? offset : -1L);
}
#endif
#ifdef __TURBOC__
int utm_unlock(int fd, long offset, long length)
{
   UT_ASSERT_EQ_INT(7, fd);
   UT_ASSERT_EQ_INT(4, (int)offset);
   UT_ASSERT_EQ_INT(8, (int)length);
   return(ut_result);
}
#else
int utm_locking(int fd, int mode, unsigned long length)
{ (void)fd; (void)mode; (void)length; return(ut_result); }
#endif
#endif /* ODPLAT_WIN32 */
#endif /* !ODPLAT_NIX */

static void reports_unlock_results(void)
{
#if defined(ODPLAT_NIX)
   ut_result = 0;
   UT_ASSERT(utt_ODReserveRangeUnlock(7, 4, 8));
   ut_result = -1;
   UT_ASSERT(!utt_ODReserveRangeUnlock(7, 4, 8));
#else
#ifdef ODPLAT_WIN32
   ut_result = TRUE;
   UT_ASSERT(utt_ODReserveRangeUnlock(7, 4, 8));
   ut_result = FALSE;
   UT_ASSERT(!utt_ODReserveRangeUnlock(7, 4, 8));
#else /* DOS */
#ifdef __TURBOC__
   bODReserveDOSLocksUnavailable = TRUE;
   UT_ASSERT(utt_ODReserveRangeUnlock(7, 4, 8));
   bODReserveDOSLocksUnavailable = FALSE;
   ut_result = 0;
   UT_ASSERT(utt_ODReserveRangeUnlock(7, 4, 8));
   ut_result = -1;
   UT_ASSERT(!utt_ODReserveRangeUnlock(7, 4, 8));
#else
   bODReserveDOSLocksUnavailable = TRUE;
   ut_seek_result = -1;
   UT_ASSERT(utt_ODReserveRangeUnlock(7, 4, 8));
   bODReserveDOSLocksUnavailable = FALSE;
   ut_seek_result = -1;
   UT_ASSERT(!utt_ODReserveRangeUnlock(7, 4, 8));
   ut_seek_result = 0;
   ut_result = 0;
   UT_ASSERT(utt_ODReserveRangeUnlock(7, 4, 8));
   ut_result = -1;
   UT_ASSERT(!utt_ODReserveRangeUnlock(7, 4, 8));
#endif
#endif /* ODPLAT_WIN32 */
#endif /* !ODPLAT_NIX */
}

static const UTTestCase ut_cases[] = {
   {"results", reports_unlock_results}
};
