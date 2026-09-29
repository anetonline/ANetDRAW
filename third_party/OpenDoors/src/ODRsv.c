/* Cooperative named reservations backed by a shared fixed-record file. */

#define BUILDING_OPENDOORS

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODGen.h"
#include "ODKrnl.h"
#include "ODPlat.h"
#include "ODRsv.h"
#include "ODSync.h"

#ifdef ODPLAT_NIX
#include <unistd.h>
#endif
#ifdef ODPLAT_WIN32
#include <io.h>
#include <share.h>
#include <windows.h>
#endif
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
#include <dos.h>
#include <io.h>
#include <share.h>
#ifndef __TURBOC__
#include <sys/locking.h>
#endif
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef S_IREAD
#define S_IREAD S_IRUSR
#endif
#ifndef S_IWRITE
#define S_IWRITE S_IWUSR
#endif

#if defined(ODPLAT_WIN32)
#define OD_RESERVE_OPEN(path, flags, mode) \
   _sopen((path), (flags), _SH_DENYNO, (mode))
#define OD_RESERVE_READ _read
#define OD_RESERVE_WRITE _write
#define OD_RESERVE_LSEEK _lseek
#define OD_RESERVE_CLOSE _close
#elif defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
#ifdef __TURBOC__
#define OD_RESERVE_OPEN(path, flags, mode) \
   open((path), (flags) | SH_DENYNO, (mode))
#else
#define OD_RESERVE_OPEN(path, flags, mode) \
   sopen((path), (flags), SH_DENYNO, (mode))
#endif
#define OD_RESERVE_READ read
#define OD_RESERVE_WRITE write
#define OD_RESERVE_LSEEK lseek
#define OD_RESERVE_CLOSE close
#else
#define OD_RESERVE_OPEN(path, flags, mode) open((path), (flags), (mode))
#define OD_RESERVE_READ read
#define OD_RESERVE_WRITE write
#define OD_RESERVE_LSEEK lseek
#define OD_RESERVE_CLOSE close
#endif

#define OD_RESERVE_RECORD_SIZE 128L
#define OD_RESERVE_HEADER_SIZE 128L
#define OD_RESERVE_COMMIT 0xa5
#define OD_RESERVE_NAME_MAX 31
#define OD_RESERVE_RETRY_TIME 20000UL
#define OD_RESERVE_POLL_DELAY 25UL
#ifdef __TURBOC__
#define OD_RESERVE_DOS_INVALID_FUNCTION 1
#endif

#define OD_RESERVE_HEADER_COMMIT 1
#define OD_RESERVE_HEADER_MAGIC 2
#define OD_RESERVE_HEADER_RECORD_SIZE 10

#define OD_RESERVE_SLOT_COMMIT 1
#define OD_RESERVE_SLOT_KIND 2
#define OD_RESERVE_SLOT_NODE 3
#define OD_RESERVE_SLOT_FLAGS 5
#define OD_RESERVE_SLOT_NAME_LENGTH 6
#define OD_RESERVE_SLOT_NAME 7
#define OD_RESERVE_SLOT_TICKET_LOW 38
#define OD_RESERVE_SLOT_TICKET_HIGH 42
#define OD_RESERVE_SLOT_OWNER 46
#define OD_RESERVE_SLOT_STARTED 50

#define OD_RESERVE_KIND_LOCAL 1
#define OD_RESERVE_KIND_NODE 2
#define OD_RESERVE_FLAG_PRESENT 0x01
#define OD_RESERVE_FLAG_CHOOSING 0x02

#define OD_RESERVE_GET_WORD(p) ((WORD)((p)[0] | ((WORD)(p)[1] << 8)))
#define OD_RESERVE_GET_DWORD(p) ((DWORD)(p)[0] \
   | ((DWORD)(p)[1] << 8) | ((DWORD)(p)[2] << 16) \
   | ((DWORD)(p)[3] << 24))
#define OD_RESERVE_PUT_WORD(p, v) do { \
   (p)[0] = (BYTE)((v) & 0xffU); \
   (p)[1] = (BYTE)(((v) >> 8) & 0xffU); \
} while(0)
#define OD_RESERVE_PUT_DWORD(p, v) do { \
   (p)[0] = (BYTE)((v) & 0xffUL); \
   (p)[1] = (BYTE)(((v) >> 8) & 0xffUL); \
   (p)[2] = (BYTE)(((v) >> 16) & 0xffUL); \
   (p)[3] = (BYTE)(((v) >> 24) & 0xffUL); \
} while(0)

typedef enum
{
   kODReserveLockError = -1,
   kODReserveLockBusy = 0,
   kODReserveLockAcquired = 1
} tODReserveLockResult;

typedef enum
{
   kODReserveRecordError = -1,
   kODReserveRecordBusy = 0,
   kODReserveRecordLive = 1,
   kODReserveRecordStale = 2
} tODReserveRecordResult;

static char *pszODReservePath;
static int hODReserveFile = -1;
static DWORD dwODReserveRecord;
static DWORD dwODReserveRecordCount;
static DWORD dwODReserveTicketLow;
static DWORD dwODReserveTicketHigh;
static BOOL bODReserveActive;
static BOOL bODReserveDetached;
static BOOL bODReserveRequested;
static BOOL bODReserveAcquired;
static BYTE btODReserveKind;
static WORD wODReserveNode;
static char szODReserveName[OD_RESERVE_NAME_MAX + 1];
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
static BOOL bODReserveDOSLocksUnavailable;
#endif

static tODReserveLockResult ODReserveRangeLock(int hFile, long nOffset,
   long nLength, BOOL bWrite)
{
#ifdef ODPLAT_NIX
   struct flock Lock;

   memset(&Lock, 0, sizeof(Lock));
   Lock.l_type = bWrite ? F_WRLCK : F_RDLCK;
   Lock.l_whence = SEEK_SET;
   Lock.l_start = (off_t)nOffset;
   Lock.l_len = (off_t)nLength;
   if(fcntl(hFile, F_SETLK, &Lock) == 0)
      return(kODReserveLockAcquired);
   return(errno == EACCES || errno == EAGAIN
      ? kODReserveLockBusy : kODReserveLockError);
#else
#ifdef ODPLAT_WIN32
   HANDLE hOSFile = (HANDLE)_get_osfhandle(hFile);
   DWORD dwError;

   (void)bWrite;
   if(LockFile(hOSFile, (DWORD)nOffset, 0, (DWORD)nLength, 0))
      return(kODReserveLockAcquired);
   dwError = GetLastError();
   return(dwError == ERROR_LOCK_VIOLATION || dwError == ERROR_SHARING_VIOLATION
      ? kODReserveLockBusy : kODReserveLockError);
#else /* !ODPLAT_NIX && !ODPLAT_WIN32 */
   int nResult;

   (void)bWrite;
   if(bODReserveDOSLocksUnavailable)
      return(kODReserveLockAcquired);
#ifdef __TURBOC__
   nResult = lock(hFile, nOffset, nLength);
#else
   if(OD_RESERVE_LSEEK(hFile, nOffset, SEEK_SET) != nOffset)
      return(kODReserveLockError);
   nResult = locking(hFile, LK_NBLCK, (unsigned long)nLength);
#endif
   if(nResult == 0)
      return(kODReserveLockAcquired);
#ifdef __TURBOC__
   if(_doserrno == OD_RESERVE_DOS_INVALID_FUNCTION)
#else
   if(errno == EINVAL)
#endif
   {
      /* DOS reports invalid function when SHARE (and any redirector
       * providing record locks) is absent. With no sharing service there
       * can be no competing filesystem participant. */
      bODReserveDOSLocksUnavailable = TRUE;
      return(kODReserveLockAcquired);
   }
   if(errno == EACCES)
      return(kODReserveLockBusy);
#if defined(EDEADLK) && !defined(__TURBOC__)
   if(errno == EDEADLK)
      return(kODReserveLockBusy);
#endif
   return(kODReserveLockError);
#endif /* !ODPLAT_WIN32 */
#endif /* !ODPLAT_NIX */
}

static BOOL ODReserveRangeUnlock(int hFile, long nOffset, long nLength)
{
#ifdef ODPLAT_NIX
   struct flock Lock;

   memset(&Lock, 0, sizeof(Lock));
   Lock.l_type = F_UNLCK;
   Lock.l_whence = SEEK_SET;
   Lock.l_start = (off_t)nOffset;
   Lock.l_len = (off_t)nLength;
   return(fcntl(hFile, F_SETLK, &Lock) == 0);
#else
#ifdef ODPLAT_WIN32
   HANDLE hOSFile = (HANDLE)_get_osfhandle(hFile);
   return(UnlockFile(hOSFile, (DWORD)nOffset, 0, (DWORD)nLength, 0));
#else /* !ODPLAT_NIX && !ODPLAT_WIN32 */
   if(bODReserveDOSLocksUnavailable)
      return(TRUE);
#ifdef __TURBOC__
   return(unlock(hFile, nOffset, nLength) == 0);
#else
   if(OD_RESERVE_LSEEK(hFile, nOffset, SEEK_SET) != nOffset)
      return(FALSE);
   return(locking(hFile, LK_UNLCK, (unsigned long)nLength) == 0);
#endif
#endif /* !ODPLAT_WIN32 */
#endif /* !ODPLAT_NIX */
}

static void ODReservePause(tODMilliSec Milliseconds)
{
   tODTimer Timer;

   ODTimerStart(&Timer, Milliseconds);
   ODTimerWaitForElapse(&Timer);
}

static tODReserveLockResult ODReserveAcquire(long nOffset, long nLength,
   BOOL bWrite, tODMilliSec TimeToWait)
{
   tODTimer Timer;
   tODMilliSec Delay = 10UL;
   tODReserveLockResult Result;

   ODTimerStart(&Timer, TimeToWait);
   for(;;)
   {
      Result = ODReserveRangeLock(hODReserveFile, nOffset, nLength, bWrite);
      if(Result == kODReserveLockAcquired || TimeToWait == 0
         || ODTimerElapsed(&Timer))
      {
         return(Result);
      }
      ODReservePause(Delay);
      if(Delay < 250UL)
      {
         Delay *= 2UL;
         if(Delay > 250UL) Delay = 250UL;
      }
   }
}

static BOOL ODReserveTransfer(int hFile, long nOffset, BYTE *pBuffer,
   size_t nSize, BOOL bWrite)
{
   long nTransferred;

   if(OD_RESERVE_LSEEK(hFile, nOffset, SEEK_SET) != nOffset)
      return(FALSE);
   while(nSize != 0)
   {
      if(bWrite)
      {
         nTransferred = (long)OD_RESERVE_WRITE(hFile, pBuffer,
            (unsigned int)nSize);
      }
      else
      {
         nTransferred = (long)OD_RESERVE_READ(hFile, pBuffer,
            (unsigned int)nSize);
      }
      if(nTransferred <= 0 || (size_t)nTransferred > nSize)
         return(FALSE);
      pBuffer += (size_t)nTransferred;
      nSize -= (size_t)nTransferred;
   }
   return(TRUE);
}

/* The caller supplies a timeout for lock contention. A transfer failure is
 * retried with the same bounded backoff rather than exposing transient SHARE
 * behavior as permanent corruption. */
static tODReserveLockResult ODReserveAccess(long nOffset, BYTE *pBuffer,
   size_t nSize, BOOL bWrite, BOOL bExclusive, tODMilliSec TimeToWait)
{
   tODTimer Timer;
   tODMilliSec Delay = 10UL;
   tODReserveLockResult Result;
   BOOL bTransferred;

   ODTimerStart(&Timer, TimeToWait);
   for(;;)
   {
      Result = ODReserveRangeLock(hODReserveFile, nOffset, (long)nSize,
         bExclusive);
      if(Result == kODReserveLockAcquired)
      {
         bTransferred = ODReserveTransfer(hODReserveFile, nOffset, pBuffer,
            nSize, bWrite);
         if(!ODReserveRangeUnlock(hODReserveFile, nOffset, (long)nSize))
            bTransferred = FALSE;
         if(bTransferred)
            return(kODReserveLockAcquired);
         Result = kODReserveLockError;
      }
      if(TimeToWait == 0 || ODTimerElapsed(&Timer))
         return(Result);
      ODReservePause(Delay);
      if(Delay < 250UL)
      {
         Delay *= 2UL;
         if(Delay > 250UL) Delay = 250UL;
      }
   }
}

static BOOL ODReserveCountRecords(DWORD *pdwCount)
{
   long nLength;
   long nPayloadLength;

   nLength = OD_RESERVE_LSEEK(hODReserveFile, 0L, SEEK_END);
   if(nLength < OD_RESERVE_HEADER_SIZE)
      return(FALSE);
   nPayloadLength = nLength - OD_RESERVE_HEADER_SIZE;
   *pdwCount = (DWORD)(nPayloadLength / OD_RESERVE_RECORD_SIZE);
   if(nPayloadLength % OD_RESERVE_RECORD_SIZE != 0)
      ++*pdwCount;
   return(TRUE);
}

static tODReserveLockResult ODReserveSnapshotRecords(DWORD *pdwCount,
   tODMilliSec TimeToWait)
{
   tODReserveLockResult Result;

   Result = ODReserveAcquire(0L, 1L, FALSE, TimeToWait);
   if(Result != kODReserveLockAcquired)
      return(Result);
   if(!ODReserveCountRecords(pdwCount))
      Result = kODReserveLockError;
   if(!ODReserveRangeUnlock(hODReserveFile, 0L, 1L))
      Result = kODReserveLockError;
   return(Result);
}

/* Reports whether a record is live, stale, temporarily busy, or unreadable. */
static tODReserveRecordResult ODReserveReadLiveRecord(DWORD dwRecord,
   BYTE *pachRecord, tODMilliSec TimeToWait)
{
   long nBase = OD_RESERVE_HEADER_SIZE
      + (long)dwRecord * OD_RESERVE_RECORD_SIZE;
   tODReserveLockResult Result;

   Result = ODReserveRangeLock(hODReserveFile, nBase, 1L, TRUE);
   if(Result == kODReserveLockAcquired)
   {
      (void)ODReserveRangeUnlock(hODReserveFile, nBase, 1L);
      return(kODReserveRecordStale);
   }
   if(Result == kODReserveLockError)
      return(kODReserveRecordError);
   Result = ODReserveAccess(nBase + 1L, pachRecord + 1,
      OD_RESERVE_RECORD_SIZE - 1, FALSE, FALSE, TimeToWait);
   if(Result == kODReserveLockBusy)
      return(kODReserveRecordBusy);
   if(Result == kODReserveLockError)
      return(kODReserveRecordError);
   if(pachRecord[OD_RESERVE_SLOT_COMMIT] != OD_RESERVE_COMMIT
      || (pachRecord[OD_RESERVE_SLOT_KIND] != OD_RESERVE_KIND_LOCAL
         && pachRecord[OD_RESERVE_SLOT_KIND] != OD_RESERVE_KIND_NODE)
      || pachRecord[OD_RESERVE_SLOT_NAME_LENGTH] > OD_RESERVE_NAME_MAX
      || (pachRecord[OD_RESERVE_SLOT_FLAGS]
         & ~(OD_RESERVE_FLAG_PRESENT | OD_RESERVE_FLAG_CHOOSING)) != 0)
   {
      /* It may have exited between the lifetime probe and payload read. */
      Result = ODReserveRangeLock(hODReserveFile, nBase, 1L, TRUE);
      if(Result == kODReserveLockAcquired)
      {
         (void)ODReserveRangeUnlock(hODReserveFile, nBase, 1L);
         return(kODReserveRecordStale);
      }
      return(kODReserveRecordError);
   }
   return(kODReserveRecordLive);
}

static BOOL ODReserveWriteOwn(BYTE btFlags, const char *pszName,
   DWORD dwTicketLow, DWORD dwTicketHigh)
{
   BYTE achRecord[OD_RESERVE_RECORD_SIZE];
   long nBase = OD_RESERVE_HEADER_SIZE
      + (long)dwODReserveRecord * OD_RESERVE_RECORD_SIZE;
   size_t nNameLength = pszName == NULL ? 0 : strlen(pszName);

   memset(achRecord, 0, sizeof(achRecord));
   achRecord[OD_RESERVE_SLOT_COMMIT] = OD_RESERVE_COMMIT;
   achRecord[OD_RESERVE_SLOT_KIND] = btODReserveKind;
   OD_RESERVE_PUT_WORD(achRecord + OD_RESERVE_SLOT_NODE, wODReserveNode);
   achRecord[OD_RESERVE_SLOT_FLAGS] = btFlags;
   achRecord[OD_RESERVE_SLOT_NAME_LENGTH] = (BYTE)nNameLength;
   if(nNameLength != 0)
      memcpy(achRecord + OD_RESERVE_SLOT_NAME, pszName, nNameLength);
   OD_RESERVE_PUT_DWORD(achRecord + OD_RESERVE_SLOT_TICKET_LOW, dwTicketLow);
   OD_RESERVE_PUT_DWORD(achRecord + OD_RESERVE_SLOT_TICKET_HIGH, dwTicketHigh);
#ifdef ODPLAT_WIN32
   OD_RESERVE_PUT_DWORD(achRecord + OD_RESERVE_SLOT_OWNER,
      (DWORD)GetCurrentProcessId());
#else
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   OD_RESERVE_PUT_DWORD(achRecord + OD_RESERVE_SLOT_OWNER, (DWORD)_psp);
#else
   OD_RESERVE_PUT_DWORD(achRecord + OD_RESERVE_SLOT_OWNER, (DWORD)getpid());
#endif
#endif
   OD_RESERVE_PUT_DWORD(achRecord + OD_RESERVE_SLOT_STARTED,
      (DWORD)time(NULL));
   return(ODReserveAccess(nBase + 1L, achRecord + 1,
      OD_RESERVE_RECORD_SIZE - 1, TRUE, TRUE,
      OD_RESERVE_RETRY_TIME) == kODReserveLockAcquired);
}

static void ODReserveDetach(void)
{
   if(hODReserveFile >= 0)
   {
      (void)OD_RESERVE_CLOSE(hODReserveFile);
      hODReserveFile = -1;
   }
   bODReserveActive = FALSE;
   bODReserveDetached = TRUE;
   bODReserveRequested = FALSE;
   bODReserveAcquired = FALSE;
   szODReserveName[0] = '\0';
   od_control.od_error = ERR_GENERALFAILURE;
   if(eODLifecycleState == kODLifecycleActive)
      (void)od_log_write("Reservation synchronization failed; detached.");
}

static BOOL ODReserveNameValid(const char *pszName)
{
   size_t nIndex;
   size_t nLength;

   if(pszName == NULL)
      return(FALSE);
   nLength = strlen(pszName);
   if(nLength == 0 || nLength > OD_RESERVE_NAME_MAX)
      return(FALSE);
   for(nIndex = 0; nIndex < nLength; ++nIndex)
   {
      if((unsigned char)pszName[nIndex] < 0x20
         || (unsigned char)pszName[nIndex] > 0x7e)
      {
         return(FALSE);
      }
   }
   return(TRUE);
}

static BOOL ODReserveTicketGreater(DWORD dwLeftHigh, DWORD dwLeftLow,
   DWORD dwRightHigh, DWORD dwRightLow)
{
   return(dwLeftHigh > dwRightHigh
      || (dwLeftHigh == dwRightHigh && dwLeftLow > dwRightLow));
}

static BOOL ODReserveTicketPrecedes(DWORD dwPeerHigh, DWORD dwPeerLow,
   DWORD dwPeerRecord)
{
   return(dwPeerHigh < dwODReserveTicketHigh
      || (dwPeerHigh == dwODReserveTicketHigh
         && (dwPeerLow < dwODReserveTicketLow
            || (dwPeerLow == dwODReserveTicketLow
               && dwPeerRecord < dwODReserveRecord))));
}

static BOOL ODReserveOpenRegistry(BOOL *pbCreated)
{
   BYTE achEmpty[OD_RESERVE_HEADER_SIZE];
   int hFile;

   *pbCreated = FALSE;
   hFile = OD_RESERVE_OPEN(pszODReservePath, O_RDWR | O_BINARY,
      S_IREAD | S_IWRITE);
   if(hFile < 0 && errno == ENOENT)
   {
      hFile = OD_RESERVE_OPEN(pszODReservePath,
         O_RDWR | O_CREAT | O_EXCL | O_BINARY, S_IREAD | S_IWRITE);
      if(hFile >= 0)
         *pbCreated = TRUE;
      else if(errno == EEXIST)
         hFile = OD_RESERVE_OPEN(pszODReservePath, O_RDWR | O_BINARY,
            S_IREAD | S_IWRITE);
   }
   if(hFile < 0)
      return(FALSE);
   hODReserveFile = hFile;
   if(*pbCreated)
   {
      memset(achEmpty, 0, sizeof(achEmpty));
      if(!ODReserveTransfer(hODReserveFile, 0L, achEmpty,
         sizeof(achEmpty), TRUE))
      {
         return(FALSE);
      }
   }
   return(TRUE);
}

/* Returns with the registry lock held only on success. */
static BOOL ODReservePrepareHeader(BOOL bCreated)
{
   /* Turbo C 2.01 misaddresses function-local initialized const byte arrays
    * in optimized large-model builds. */
   static BYTE achMagic[8] =
      {'O', 'D', 'R', 'S', 'Y', 'N', 'C', '1'};
   BYTE achHeader[OD_RESERVE_RECORD_SIZE];
   tODTimer RetryTimer;

   ODTimerStart(&RetryTimer, OD_RESERVE_RETRY_TIME);
   for(;;)
   {
      if(ODReserveAcquire(0L, 1L, TRUE, OD_RESERVE_RETRY_TIME)
         != kODReserveLockAcquired)
      {
         return(FALSE);
      }
      memset(achHeader, 0, sizeof(achHeader));
      if(bCreated)
      {
         memcpy(achHeader + OD_RESERVE_HEADER_MAGIC, achMagic,
            sizeof(achMagic));
         OD_RESERVE_PUT_WORD(achHeader + OD_RESERVE_HEADER_RECORD_SIZE,
            (WORD)OD_RESERVE_RECORD_SIZE);
         if(!ODReserveTransfer(hODReserveFile, 2L, achHeader + 2,
            OD_RESERVE_RECORD_SIZE - 2, TRUE))
         {
            (void)ODReserveRangeUnlock(hODReserveFile, 0L, 1L);
            return(FALSE);
         }
         achHeader[OD_RESERVE_HEADER_COMMIT] = OD_RESERVE_COMMIT;
         if(!ODReserveTransfer(hODReserveFile, OD_RESERVE_HEADER_COMMIT,
            achHeader + OD_RESERVE_HEADER_COMMIT, 1, TRUE))
         {
            (void)ODReserveRangeUnlock(hODReserveFile, 0L, 1L);
            return(FALSE);
         }
         return(TRUE);
      }

      if(ODReserveTransfer(hODReserveFile, 1L, achHeader + 1,
            OD_RESERVE_RECORD_SIZE - 1, FALSE)
         && achHeader[OD_RESERVE_HEADER_COMMIT] == OD_RESERVE_COMMIT)
      {
         if(memcmp(achHeader + OD_RESERVE_HEADER_MAGIC, achMagic,
            sizeof(achMagic)) == 0
            && OD_RESERVE_GET_WORD(achHeader
               + OD_RESERVE_HEADER_RECORD_SIZE) == OD_RESERVE_RECORD_SIZE)
         {
            return(TRUE);
         }
         od_control.od_error = ERR_FILEREAD;
         (void)ODReserveRangeUnlock(hODReserveFile, 0L, 1L);
         return(FALSE);
      }
      if(!ODReserveRangeUnlock(hODReserveFile, 0L, 1L))
         return(FALSE);
      if(ODTimerElapsed(&RetryTimer))
      {
         od_control.od_error = ERR_FILEREAD;
         return(FALSE);
      }
      ODReservePause(25UL);
   }
}

static void ODReserveSetIdentity(void)
{
   btODReserveKind = od_control.od_force_local
#if !defined(ODPLAT_NIX)
      || od_control.baud == 0
#endif
      ? OD_RESERVE_KIND_LOCAL : OD_RESERVE_KIND_NODE;
   wODReserveNode = btODReserveKind == OD_RESERVE_KIND_LOCAL
      ? 0 : od_control.od_node;
}

static BOOL ODReserveCheckIdentities(void)
{
   BYTE achRecord[OD_RESERVE_RECORD_SIZE];
   DWORD dwRecord;
   DWORD dwCompleteRecords;
   long nBase;
   long nLength;
   tODReserveLockResult Result;

   if(!ODReserveCountRecords(&dwODReserveRecordCount))
      return(FALSE);
   nLength = OD_RESERVE_LSEEK(hODReserveFile, 0L, SEEK_END);
   if(nLength < OD_RESERVE_HEADER_SIZE)
      return(FALSE);
   dwCompleteRecords = (DWORD)((nLength - OD_RESERVE_HEADER_SIZE)
      / OD_RESERVE_RECORD_SIZE);
   dwODReserveRecord = dwODReserveRecordCount;
   for(dwRecord = 0; dwRecord < dwCompleteRecords; ++dwRecord)
   {
      nBase = OD_RESERVE_HEADER_SIZE
         + (long)dwRecord * OD_RESERVE_RECORD_SIZE;
      Result = ODReserveAccess(nBase + 1L, achRecord + 1,
         OD_RESERVE_RECORD_SIZE - 1, FALSE, FALSE,
         OD_RESERVE_RETRY_TIME);
      if(Result != kODReserveLockAcquired)
         return(FALSE);
      if(achRecord[OD_RESERVE_SLOT_COMMIT] != OD_RESERVE_COMMIT)
         continue;
      if((achRecord[OD_RESERVE_SLOT_KIND] != OD_RESERVE_KIND_LOCAL
            && achRecord[OD_RESERVE_SLOT_KIND] != OD_RESERVE_KIND_NODE)
         || achRecord[OD_RESERVE_SLOT_NAME_LENGTH] > OD_RESERVE_NAME_MAX
         || (achRecord[OD_RESERVE_SLOT_FLAGS]
            & ~(OD_RESERVE_FLAG_PRESENT | OD_RESERVE_FLAG_CHOOSING)) != 0)
      {
         od_control.od_error = ERR_FILEREAD;
         return(FALSE);
      }
      if(achRecord[OD_RESERVE_SLOT_KIND] == btODReserveKind
         && OD_RESERVE_GET_WORD(achRecord + OD_RESERVE_SLOT_NODE)
            == wODReserveNode)
      {
         if(dwODReserveRecord != dwODReserveRecordCount)
         {
            od_control.od_error = ERR_GENERALFAILURE;
            return(FALSE);
         }
         Result = ODReserveAcquire(nBase, 1L, TRUE,
            OD_RESERVE_RETRY_TIME);
         if(Result != kODReserveLockAcquired)
         {
            od_control.od_error = ERR_GENERALFAILURE;
            return(FALSE);
         }
         dwODReserveRecord = dwRecord;
      }
   }
   return(TRUE);
}

/* Appends a record and returns with its lifetime lock held on success. */
static BOOL ODReserveAppendParticipant(void)
{
   BYTE achRecord[OD_RESERVE_RECORD_SIZE];
   long nBase;

   if(dwODReserveRecord < dwODReserveRecordCount)
      return(TRUE);
   dwODReserveRecord = dwODReserveRecordCount;
   nBase = OD_RESERVE_HEADER_SIZE
      + (long)dwODReserveRecord * OD_RESERVE_RECORD_SIZE;
   achRecord[0] = 0;
   if(!ODReserveTransfer(hODReserveFile,
      nBase + OD_RESERVE_RECORD_SIZE - 1L, achRecord, 1, TRUE))
   {
      return(FALSE);
   }
   if(ODReserveAcquire(nBase, 1L, TRUE, OD_RESERVE_RETRY_TIME)
      != kODReserveLockAcquired)
   {
      return(FALSE);
   }
   memset(achRecord, 0, sizeof(achRecord));
   achRecord[OD_RESERVE_SLOT_KIND] = btODReserveKind;
   OD_RESERVE_PUT_WORD(achRecord + OD_RESERVE_SLOT_NODE, wODReserveNode);
   achRecord[OD_RESERVE_SLOT_FLAGS] = OD_RESERVE_FLAG_PRESENT;
   if(!ODReserveTransfer(hODReserveFile, nBase + 2L, achRecord + 2,
      OD_RESERVE_RECORD_SIZE - 2, TRUE))
   {
      (void)ODReserveRangeUnlock(hODReserveFile, nBase, 1L);
      return(FALSE);
   }
   achRecord[OD_RESERVE_SLOT_COMMIT] = OD_RESERVE_COMMIT;
   if(!ODReserveTransfer(hODReserveFile, nBase + 1L, achRecord + 1,
      1, TRUE))
   {
      (void)ODReserveRangeUnlock(hODReserveFile, nBase, 1L);
      return(FALSE);
   }
   ++dwODReserveRecordCount;
   return(TRUE);
}

tODResult ODReserveSessionInitialize(void)
{
   BOOL bCreated = FALSE;

   if(pszODReservePath == NULL || bODReserveActive)
      return(kODRCSuccess);

   if(!ODReserveOpenRegistry(&bCreated))
   {
      od_control.od_error = ERR_FILEOPEN;
      goto failure;
   }
   if(!ODReservePrepareHeader(bCreated))
      goto failure;
   ODReserveSetIdentity();
   if(!ODReserveCheckIdentities())
      goto locked_failure;
   if(!ODReserveAppendParticipant())
      goto locked_failure;

   bODReserveActive = TRUE;
   bODReserveDetached = FALSE;
   bODReserveRequested = FALSE;
   bODReserveAcquired = FALSE;
   szODReserveName[0] = '\0';
   if(!ODReserveWriteOwn(OD_RESERVE_FLAG_PRESENT, NULL, 0UL, 0UL))
      goto locked_failure;
   if(!ODReserveRangeUnlock(hODReserveFile, 0L, 1L))
      goto failure;
   return(kODRCSuccess);

locked_failure:
   (void)ODReserveRangeUnlock(hODReserveFile, 0L, 1L);
failure:
   if(hODReserveFile >= 0)
   {
      (void)OD_RESERVE_CLOSE(hODReserveFile);
      hODReserveFile = -1;
   }
   bODReserveActive = FALSE;
   if(od_control.od_error == ERR_NONE)
      od_control.od_error = ERR_GENERALFAILURE;
   return(kODRCFileAccessError);
}

void ODReserveSessionShutdown(void)
{
   if(bODReserveActive)
      (void)ODReserveWriteOwn(0, NULL, 0UL, 0UL);
   if(hODReserveFile >= 0)
   {
      (void)OD_RESERVE_CLOSE(hODReserveFile);
      hODReserveFile = -1;
   }
   bODReserveActive = FALSE;
   bODReserveRequested = FALSE;
   bODReserveAcquired = FALSE;
   szODReserveName[0] = '\0';
}

ODAPIDEF BOOL ODCALL od_reserve_configure(const char *pszPath)
{
   char *pszNewPath;

   TRACE(TRACE_API, "od_reserve_configure()");
   if(!ODSyncPublicCallAllowed()) return(FALSE);
   if(bODInitialized || pszPath == NULL || pszPath[0] == '\0')
   {
      od_control.od_error = ERR_PARAMETER;
      return(FALSE);
   }
   pszNewPath = (char *)malloc(strlen(pszPath) + 1);
   if(pszNewPath == NULL)
   {
      od_control.od_error = ERR_MEMORY;
      return(FALSE);
   }
   strcpy(pszNewPath, pszPath);
   free(pszODReservePath);
   pszODReservePath = pszNewPath;
   bODReserveDetached = FALSE;
   return(TRUE);
}

ODAPIDEF BOOL ODCALL od_reserve_request(const char *pszName)
{
   BYTE achRecord[OD_RESERVE_RECORD_SIZE];
   size_t nLength;
   DWORD dwRecord;
   DWORD dwLow = 0UL;
   DWORD dwHigh = 0UL;
   DWORD dwPeerLow;
   DWORD dwPeerHigh;
   tODReserveRecordResult Result;

   TRACE(TRACE_API, "od_reserve_request()");
   if(!ODSyncPublicCallAllowed()) return(FALSE);
   if(pszODReservePath == NULL || !ODReserveNameValid(pszName))
   {
      od_control.od_error = ERR_PARAMETER;
      return(FALSE);
   }
   nLength = strlen(pszName);
   if(!bODInitialized) od_init();
   OD_RETURN_IF_SESSION_ENDED(FALSE);
   OD_API_ENTRY();
   if(!bODReserveActive || bODReserveDetached || bODReserveRequested)
   {
      od_control.od_error = bODReserveRequested ? ERR_PARAMETER
         : ERR_GENERALFAILURE;
      OD_API_EXIT();
      return(FALSE);
   }

   strcpy(szODReserveName, pszName);
   if(!ODReserveWriteOwn(OD_RESERVE_FLAG_PRESENT | OD_RESERVE_FLAG_CHOOSING,
      szODReserveName, 0UL, 0UL))
   {
      ODReserveDetach();
      OD_API_EXIT();
      return(FALSE);
   }
   if(ODReserveSnapshotRecords(&dwODReserveRecordCount,
      OD_RESERVE_RETRY_TIME) != kODReserveLockAcquired)
      goto failure;
   for(dwRecord = 0; dwRecord < dwODReserveRecordCount; ++dwRecord)
   {
      if(dwRecord == dwODReserveRecord) continue;
      Result = ODReserveReadLiveRecord(dwRecord, achRecord,
         OD_RESERVE_RETRY_TIME);
      if(Result == kODReserveRecordStale) continue;
      if(Result != kODReserveRecordLive) goto failure;
      if(achRecord[OD_RESERVE_SLOT_NAME_LENGTH] == nLength
         && memcmp(achRecord + OD_RESERVE_SLOT_NAME, pszName, nLength) == 0)
      {
         dwPeerLow = OD_RESERVE_GET_DWORD(achRecord
            + OD_RESERVE_SLOT_TICKET_LOW);
         dwPeerHigh = OD_RESERVE_GET_DWORD(achRecord
            + OD_RESERVE_SLOT_TICKET_HIGH);
         if(ODReserveTicketGreater(dwPeerHigh, dwPeerLow, dwHigh, dwLow))
         {
            dwHigh = dwPeerHigh;
            dwLow = dwPeerLow;
         }
      }
   }
   ++dwLow;
   if(dwLow == 0UL)
   {
      ++dwHigh;
      if(dwHigh == 0UL) goto failure;
   }
   dwODReserveTicketLow = dwLow;
   dwODReserveTicketHigh = dwHigh;
   if(!ODReserveWriteOwn(OD_RESERVE_FLAG_PRESENT, szODReserveName,
      dwLow, dwHigh))
   {
      goto failure;
   }
   bODReserveRequested = TRUE;
   bODReserveAcquired = FALSE;
   OD_API_EXIT();
   return(TRUE);

failure:
   (void)ODReserveWriteOwn(OD_RESERVE_FLAG_PRESENT, NULL, 0UL, 0UL);
   ODReserveDetach();
   OD_API_EXIT();
   return(FALSE);
}

static tODReserveResult ODReserveWaitCore(tODMilliSec TimeToWait)
{
   BYTE achRecord[OD_RESERVE_RECORD_SIZE];
   DWORD dwRecord;
   DWORD dwPeerLow;
   DWORD dwPeerHigh;
   size_t nLength = strlen(szODReserveName);
   tODReserveLockResult LockResult;
   tODReserveRecordResult Result;

   if(bODReserveAcquired)
      return(OD_RESERVE_ACQUIRED);
   LockResult = ODReserveSnapshotRecords(&dwODReserveRecordCount,
      TimeToWait);
   if(LockResult == kODReserveLockBusy)
      return(OD_RESERVE_PENDING);
   if(LockResult != kODReserveLockAcquired)
      return(OD_RESERVE_ERROR);
   for(dwRecord = 0; dwRecord < dwODReserveRecordCount; ++dwRecord)
   {
      if(dwRecord == dwODReserveRecord) continue;
      Result = ODReserveReadLiveRecord(dwRecord, achRecord, TimeToWait);
      if(Result == kODReserveRecordStale) continue;
      if(Result == kODReserveRecordBusy) return(OD_RESERVE_PENDING);
      if(Result != kODReserveRecordLive) return(OD_RESERVE_ERROR);
      if(achRecord[OD_RESERVE_SLOT_NAME_LENGTH] != nLength
         || memcmp(achRecord + OD_RESERVE_SLOT_NAME,
            szODReserveName, nLength) != 0)
      {
         continue;
      }
      if(achRecord[OD_RESERVE_SLOT_FLAGS] & OD_RESERVE_FLAG_CHOOSING)
         return(OD_RESERVE_PENDING);
      dwPeerLow = OD_RESERVE_GET_DWORD(achRecord
         + OD_RESERVE_SLOT_TICKET_LOW);
      dwPeerHigh = OD_RESERVE_GET_DWORD(achRecord
         + OD_RESERVE_SLOT_TICKET_HIGH);
      if(ODReserveTicketPrecedes(dwPeerHigh, dwPeerLow, dwRecord))
      {
         return(OD_RESERVE_PENDING);
      }
   }
   bODReserveAcquired = TRUE;
   return(OD_RESERVE_ACQUIRED);
}

ODAPIDEF tODReserveResult ODCALL od_reserve_wait(tODMilliSec Milliseconds)
{
   tODTimer Timer;
   tODReserveResult Result;

   TRACE(TRACE_API, "od_reserve_wait()");
   if(!ODSyncPublicCallAllowed()) return(OD_RESERVE_ERROR);
   if(!bODInitialized) od_init();
   OD_RETURN_IF_SESSION_ENDED(OD_RESERVE_ERROR);
   OD_API_ENTRY();
   if(!bODReserveActive || !bODReserveRequested)
   {
      od_control.od_error = ERR_PARAMETER;
      OD_API_EXIT();
      return(OD_RESERVE_ERROR);
   }
   ODTimerStart(&Timer, Milliseconds);
   for(;;)
   {
      Result = ODReserveWaitCore(0);
      if(Result != OD_RESERVE_PENDING || Milliseconds == 0)
         break;
      if(Milliseconds != OD_NO_TIMEOUT && ODTimerElapsed(&Timer))
         break;
      if(!ODSyncAPICheckpoint())
      {
         Result = OD_RESERVE_ERROR;
         break;
      }
      ODReservePause(OD_RESERVE_POLL_DELAY);
   }
   if(Result == OD_RESERVE_ERROR && eODLifecycleState == kODLifecycleActive)
      ODReserveDetach();
   OD_API_EXIT();
   return(Result);
}

ODAPIDEF tODReserveResult ODCALL od_reserve_wait_until(DWORD dwSeconds,
   WORD wMilliseconds)
{
   DWORD dwNowSeconds;
   WORD wNowMilliseconds;
   tODReserveResult Result;

   TRACE(TRACE_API, "od_reserve_wait_until()");
   if(!ODSyncPublicCallAllowed()) return(OD_RESERVE_ERROR);
   if(wMilliseconds >= 1000U)
   {
      od_control.od_error = ERR_PARAMETER;
      return(OD_RESERVE_ERROR);
   }
   if(!bODInitialized) od_init();
   OD_RETURN_IF_SESSION_ENDED(OD_RESERVE_ERROR);
   OD_API_ENTRY();
   if(!bODReserveActive || !bODReserveRequested)
   {
      od_control.od_error = ERR_PARAMETER;
      OD_API_EXIT();
      return(OD_RESERVE_ERROR);
   }
   if(bODReserveAcquired)
   {
      OD_API_EXIT();
      return(OD_RESERVE_ACQUIRED);
   }
   for(;;)
   {
      ODSessionTimeGet(&dwNowSeconds, &wNowMilliseconds);
      if(dwNowSeconds > dwSeconds || (dwNowSeconds == dwSeconds
         && wNowMilliseconds >= wMilliseconds))
      {
         Result = OD_RESERVE_PENDING;
         break;
      }
      Result = ODReserveWaitCore(0);
      if(Result != OD_RESERVE_PENDING) break;
      if(!ODSyncAPICheckpoint())
      {
         Result = OD_RESERVE_ERROR;
         break;
      }
      ODReservePause(OD_RESERVE_POLL_DELAY);
   }
   if(Result == OD_RESERVE_ERROR && eODLifecycleState == kODLifecycleActive)
      ODReserveDetach();
   OD_API_EXIT();
   return(Result);
}

ODAPIDEF BOOL ODCALL od_reserve_end(void)
{
   BOOL bResult;

   TRACE(TRACE_API, "od_reserve_end()");
   if(!ODSyncPublicCallAllowed()) return(FALSE);
   if(!bODInitialized) od_init();
   OD_RETURN_IF_SESSION_ENDED(FALSE);
   OD_API_ENTRY();
   if(!bODReserveActive || !bODReserveRequested)
   {
      od_control.od_error = ERR_PARAMETER;
      OD_API_EXIT();
      return(FALSE);
   }
   bResult = ODReserveWriteOwn(OD_RESERVE_FLAG_PRESENT, NULL, 0UL, 0UL);
   if(bResult)
   {
      bODReserveRequested = FALSE;
      bODReserveAcquired = FALSE;
      szODReserveName[0] = '\0';
      dwODReserveTicketLow = 0UL;
      dwODReserveTicketHigh = 0UL;
   }
   else
   {
      ODReserveDetach();
   }
   OD_API_EXIT();
   return(bResult);
}
