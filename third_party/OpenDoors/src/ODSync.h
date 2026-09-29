/* Internal synchronization and serialized API-flow services. */

#ifndef _INC_ODSYNC
#define _INC_ODSYNC

#include "ODTypes.h"

/* Internal session lifecycle.  Once a session reaches the terminal state,
 * public API entry is permanently disabled for the life of the process. */
typedef enum
{
   kODLifecycleNeverStarted,
   kODLifecycleInitializing,
   kODLifecycleActive,
   kODLifecycleExitPending,
   kODLifecycleFinalizing,
   kODLifecycleTerminal
} tODLifecycleState;

extern tODLifecycleState eODLifecycleState;
extern INT nODPendingExitErrorLevel;
extern BOOL bODPendingExitTermCall;
extern BOOL bODPendingExitNoExit;
extern BOOL bODExitPrologueComplete;
extern BOOL bODExitRequestedDuringInitialization;

#ifdef OD_THREAD_SUPPORT
#include <windows.h>
typedef struct { CRITICAL_SECTION cs; } tODMutex;
#else
typedef struct { int unused; } tODMutex;
#endif

tODResult ODMutexInitialize(tODMutex *pMutex);
void ODMutexDestroy(tODMutex *pMutex);
void ODMutexLock(tODMutex *pMutex);
void ODMutexUnlock(tODMutex *pMutex);

tODResult ODSyncSessionInitialize(void);
void ODSyncSessionShutdown(void);
BOOL ODSyncSessionActive(void);
BOOL ODSyncAPILevelActive(void);
BOOL ODSyncAPIIsNested(void);
BOOL ODSyncPublicCallAllowed(void);
void ODSyncAPIEntry(void);
void ODSyncAPIExit(void);
BOOL ODSyncAPICheckpoint(void);
unsigned ODSyncAPIRelease(void);
void ODSyncAPIReacquire(unsigned nSavedAPILevel);

#endif /* _INC_ODSYNC */
