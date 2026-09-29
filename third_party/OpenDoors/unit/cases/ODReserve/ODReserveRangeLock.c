#if defined(ODPLAT_NIX)
#include <stdarg.h>
#define UT_CUSTOM_MOCK_fcntl
#define UT_CUSTOM_MOCK_memset
#define UT_CUSTOM_MOCK___error
#define UT_CUSTOM_MOCK___errno_location
static int ut_fcntl_result;
static short ut_expected_type;
static int ut_errno;
int *utm___error(void) { return(&ut_errno); }
int *utm___errno_location(void) { return(&ut_errno); }
int utm_fcntl(int fd, int command, ...)
{
   va_list arguments;
   struct flock *lock;
   UT_ASSERT_EQ_INT(7, fd);
   UT_ASSERT_EQ_INT(F_SETLK, command);
   va_start(arguments, command);
   lock = va_arg(arguments, struct flock *);
   va_end(arguments);
   UT_ASSERT_EQ_INT(ut_expected_type, lock->l_type);
   UT_ASSERT_EQ_INT(4, (int)lock->l_start);
   UT_ASSERT_EQ_INT(8, (int)lock->l_len);
   return(ut_fcntl_result);
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
#define UT_CUSTOM_MOCK_LockFile
#define UT_CUSTOM_MOCK_GetLastError
static BOOL ut_lock_result;
static DWORD ut_error;
intptr_t utm__get_osfhandle(int fd)
{ UT_ASSERT_EQ_INT(7, fd); return((intptr_t)8); }
BOOL WINAPI utm_LockFile(HANDLE file, DWORD low, DWORD high,
   DWORD count_low, DWORD count_high)
{
   UT_ASSERT_EQ_INT(8, (int)(intptr_t)file);
   UT_ASSERT_EQ_UINT(4, low);
   UT_ASSERT_EQ_UINT(0, high);
   UT_ASSERT_EQ_UINT(8, count_low);
   UT_ASSERT_EQ_UINT(0, count_high);
   return(ut_lock_result);
}
DWORD WINAPI utm_GetLastError(void) { return(ut_error); }
#else /* DOS */
#ifdef __TURBOC__
#define UT_CUSTOM_MOCK_lock
#else
#define UT_CUSTOM_MOCK_lseek
#define UT_CUSTOM_MOCK_locking
#endif
#ifdef __WATCOMC__
#define UT_CUSTOM_MOCK___get_errno_ptr
static int ut_errno;
int *utm___get_errno_ptr(void) { return(&ut_errno); }
#endif
static int ut_seek_result;
static int ut_lock_result;
static int ut_lock_errno;
#ifdef __TURBOC__
static int ut_lock_dos_errno;
#endif
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
int utm_lock(int fd, long offset, long length)
{
   UT_ASSERT_EQ_INT(7, fd);
   UT_ASSERT_EQ_INT(4, (int)offset);
   UT_ASSERT_EQ_INT(8, (int)length);
   errno = ut_lock_errno;
   _doserrno = ut_lock_dos_errno;
   return(ut_lock_result);
}
#else
int utm_locking(int fd, int mode, unsigned long length)
{
   UT_ASSERT_EQ_INT(7, fd);
   UT_ASSERT_EQ_INT(LK_NBLCK, mode);
   UT_ASSERT_EQ_UINT(8, length);
#ifdef __WATCOMC__
   ut_errno = ut_lock_errno;
#else
   errno = ut_lock_errno;
#endif
   return(ut_lock_result);
}
#endif
#endif /* ODPLAT_WIN32 */
#endif /* !ODPLAT_NIX */

static void classifies_range_lock_results(void)
{
#if defined(ODPLAT_NIX)
   ut_expected_type = F_WRLCK;
   ut_fcntl_result = 0;
   UT_ASSERT_EQ_INT(kODReserveLockAcquired,
      utt_ODReserveRangeLock(7, 4, 8, TRUE));
   ut_expected_type = F_RDLCK;
   ut_fcntl_result = -1;
   ut_errno = EACCES;
   UT_ASSERT_EQ_INT(kODReserveLockBusy,
      utt_ODReserveRangeLock(7, 4, 8, FALSE));
   ut_errno = EAGAIN;
   UT_ASSERT_EQ_INT(kODReserveLockBusy,
      utt_ODReserveRangeLock(7, 4, 8, FALSE));
   ut_errno = EIO;
   UT_ASSERT_EQ_INT(kODReserveLockError,
      utt_ODReserveRangeLock(7, 4, 8, FALSE));
#else
#ifdef ODPLAT_WIN32
   ut_lock_result = TRUE;
   UT_ASSERT_EQ_INT(kODReserveLockAcquired,
      utt_ODReserveRangeLock(7, 4, 8, TRUE));
   ut_lock_result = FALSE;
   ut_error = ERROR_LOCK_VIOLATION;
   UT_ASSERT_EQ_INT(kODReserveLockBusy,
      utt_ODReserveRangeLock(7, 4, 8, FALSE));
   ut_error = ERROR_SHARING_VIOLATION;
   UT_ASSERT_EQ_INT(kODReserveLockBusy,
      utt_ODReserveRangeLock(7, 4, 8, FALSE));
   ut_error = ERROR_INVALID_HANDLE;
   UT_ASSERT_EQ_INT(kODReserveLockError,
      utt_ODReserveRangeLock(7, 4, 8, FALSE));
#else /* DOS */
   bODReserveDOSLocksUnavailable = FALSE;
#ifdef __TURBOC__
   ut_lock_result = 0;
   UT_ASSERT_EQ_INT(kODReserveLockAcquired,
      utt_ODReserveRangeLock(7, 4, 8, TRUE));
   ut_lock_result = -1;
   ut_lock_errno = EACCES;
   ut_lock_dos_errno = EACCES;
   UT_ASSERT_EQ_INT(kODReserveLockBusy,
      utt_ODReserveRangeLock(7, 4, 8, TRUE));
   ut_lock_dos_errno = OD_RESERVE_DOS_INVALID_FUNCTION;
   UT_ASSERT_EQ_INT(kODReserveLockAcquired,
      utt_ODReserveRangeLock(7, 4, 8, TRUE));
   ut_lock_result = 0;
   UT_ASSERT_EQ_INT(kODReserveLockAcquired,
      utt_ODReserveRangeLock(7, 4, 8, TRUE));
   bODReserveDOSLocksUnavailable = FALSE;
   ut_lock_result = -1;
   ut_lock_errno = EINVAL;
   ut_lock_dos_errno = EINVDAT;
   UT_ASSERT_EQ_INT(kODReserveLockError,
      utt_ODReserveRangeLock(7, 4, 8, TRUE));
#else
   ut_seek_result = -1;
   UT_ASSERT_EQ_INT(kODReserveLockError,
      utt_ODReserveRangeLock(7, 4, 8, TRUE));
   ut_seek_result = 0;
   ut_lock_result = 0;
   UT_ASSERT_EQ_INT(kODReserveLockAcquired,
      utt_ODReserveRangeLock(7, 4, 8, TRUE));
   ut_lock_result = -1;
   ut_lock_errno = EACCES;
   UT_ASSERT_EQ_INT(kODReserveLockBusy,
      utt_ODReserveRangeLock(7, 4, 8, TRUE));
#if defined(EDEADLK) && !defined(__TURBOC__)
   ut_lock_errno = EDEADLK;
   UT_ASSERT_EQ_INT(kODReserveLockBusy,
      utt_ODReserveRangeLock(7, 4, 8, TRUE));
#endif
   ut_lock_errno = EINVAL;
   UT_ASSERT_EQ_INT(kODReserveLockAcquired,
      utt_ODReserveRangeLock(7, 4, 8, TRUE));
   ut_seek_result = -1;
   UT_ASSERT_EQ_INT(kODReserveLockAcquired,
      utt_ODReserveRangeLock(7, 4, 8, TRUE));
   bODReserveDOSLocksUnavailable = FALSE;
   ut_seek_result = 0;
   ut_lock_errno = EIO;
   UT_ASSERT_EQ_INT(kODReserveLockError,
      utt_ODReserveRangeLock(7, 4, 8, TRUE));
#endif
#endif /* ODPLAT_WIN32 */
#endif /* !ODPLAT_NIX */
}

static const UTTestCase ut_cases[] = {
   {"classify", classifies_range_lock_results}
};
