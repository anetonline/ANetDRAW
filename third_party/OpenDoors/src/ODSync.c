/* OpenDoors session synchronization and serialized API nesting. */

#define BUILDING_OPENDOORS

#include <stddef.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODGen.h"
#include "ODKrnl.h"
#ifdef ODPLAT_WIN32
#include "ODScrn.h"
#endif
#include "ODSync.h"

static BOOL bSyncActive;
static unsigned nAPILevel;

tODResult ODMutexInitialize(tODMutex *pMutex)
{
#ifdef OD_THREAD_SUPPORT
   InitializeCriticalSection(&pMutex->cs);
   return(kODRCSuccess);
#else
   (void)pMutex;
   return(kODRCSuccess);
#endif
}

void ODMutexDestroy(tODMutex *pMutex)
{
#ifdef OD_THREAD_SUPPORT
   DeleteCriticalSection(&pMutex->cs);
#else
   (void)pMutex;
#endif
}

void ODMutexLock(tODMutex *pMutex)
{
#ifdef OD_THREAD_SUPPORT
   EnterCriticalSection(&pMutex->cs);
#else
   (void)pMutex;
#endif
}

void ODMutexUnlock(tODMutex *pMutex)
{
#ifdef OD_THREAD_SUPPORT
   LeaveCriticalSection(&pMutex->cs);
#else
   (void)pMutex;
#endif
}

tODResult ODSyncSessionInitialize(void)
{
   if(bSyncActive)
      return(kODRCSuccess);

   nAPILevel = 0;
   bSyncActive = TRUE;
   return(kODRCSuccess);
}

void ODSyncSessionShutdown(void)
{
   if(!bSyncActive)
      return;
   bSyncActive = FALSE;
   nAPILevel = 0;
}

BOOL ODSyncSessionActive(void) { return(bSyncActive); }

BOOL ODSyncAPILevelActive(void)
{
#ifdef OD_THREAD_SUPPORT
   return(bSyncActive && nAPILevel != 0);
#else
   return(FALSE);
#endif
}

BOOL ODSyncAPIIsNested(void)
{
   return(bSyncActive && nAPILevel != 0);
}

BOOL ODSyncPublicCallAllowed(void)
{
   if(eODLifecycleState >= kODLifecycleExitPending)
   {
      od_control.od_error = ERR_GENERALFAILURE;
      return(FALSE);
   }
   return(TRUE);
}

void ODSyncAPIEntry(void)
{
   BOOL bOutermost = nAPILevel == 0;

   ++nAPILevel;
   if(bOutermost && eODLifecycleState == kODLifecycleActive)
      ODKrnlDispatchPending(TRUE);
}

void ODSyncAPIExit(void)
{
   ASSERT(nAPILevel != 0);
   if(nAPILevel == 0) return;
   if(nAPILevel == 1 && eODLifecycleState == kODLifecycleExitPending)
   {
      eODLifecycleState = kODLifecycleFinalizing;
      od_exit(nODPendingExitErrorLevel, bODPendingExitTermCall);
   }
   if(nAPILevel == 1 && eODLifecycleState == kODLifecycleActive)
      ODKrnlDispatchPending(TRUE);
#ifdef ODPLAT_WIN32
   if(nAPILevel == 1 && eODLifecycleState == kODLifecycleActive)
      ODScrnPublish();
#endif
   --nAPILevel;
   if(nAPILevel == 0 && eODLifecycleState == kODLifecycleTerminal)
      ODSyncSessionShutdown();
}

BOOL ODSyncAPICheckpoint(void)
{
   unsigned nSavedAPILevel;

   if(nAPILevel == 0)
      return(eODLifecycleState == kODLifecycleActive);
   nSavedAPILevel = ODSyncAPIRelease();
   ODSyncAPIReacquire(nSavedAPILevel);
   if(eODLifecycleState != kODLifecycleActive)
      return(FALSE);
   ODKrnlDispatchPending(TRUE);
   if(eODLifecycleState == kODLifecycleActive)
      od_kernel();
   return(eODLifecycleState == kODLifecycleActive);
}

unsigned ODSyncAPIRelease(void)
{
   unsigned nSavedAPILevel = nAPILevel;

   ASSERT(nSavedAPILevel != 0);
#ifdef ODPLAT_WIN32
   ODScrnPublish();
#endif
   nAPILevel = 0;
   return(nSavedAPILevel);
}

void ODSyncAPIReacquire(unsigned nSavedAPILevel)
{
   ASSERT(nSavedAPILevel != 0);
   nAPILevel = nSavedAPILevel;
}
