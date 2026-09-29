/* OpenDoors Online Software Programming Toolkit
 * (C) Copyright 1991 - 1999 by Brian Pirie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 *
 *        File: ODKrnl.h
 *
 * Description: Contains internal definitions related to odkrnl.c.
 *
 *   Revisions: Date          Ver   Who  Change
 *              ---------------------------------------------------------------
 *              Jan 01, 1995  6.00  BP   Split off from odcore.c and oddoor.h
 *              Dec 12, 1995  6.00  BP   Added entry, exit and kernel macros.
 *              Jan 12, 1996  6.00  BP   Added bOnlyShiftArrow.
 *              Feb 19, 1996  6.00  BP   Changed version number to 6.00.
 *              Mar 03, 1996  6.10  BP   Begin version 6.10.
 *              Mar 13, 1996  6.10  BP   bOnlyShiftArrow -> nArrowUseCount.
 */

#ifndef _INC_ODKRNL
#define _INC_ODKRNL

#include "ODPlat.h"

#define OD_MIN_USER_TIME_MINUTES 0
#define OD_MAX_USER_TIME_MINUTES 1440

#ifdef ODPLAT_WIN32
typedef struct
{
   HICON hAppIcon;
   char szProgramName[sizeof(od_control.od_prog_name)];
   char szProgramCopyright[sizeof(od_control.od_prog_copyright)];
   char szProgramVersion[sizeof(od_control.od_prog_version)];
   char szUserName[sizeof(od_control.user_name)];
   char szUserLocation[sizeof(od_control.user_location)];
   char szUserReasonForChat[sizeof(od_control.user_reasonforchat)];
   DWORD dwBaud;
   DWORD dwConnectSpeed;
   INT nNode;
   INT nTimeLimit;
   INT nCmdShow;
   WORD wDisable;
   BOOL bUserWantsChat;
   BOOL bInactivityDisabled;
   BOOL bSysopNext;
   BOOL bUserKeyboardOn;
   BOOL bChatActive;
   void (*pfHelpCallback)(void);
   void (*pfConfigCallback)(void);
} tODUIState;
#endif

/* Global kernel-related variables. */
extern tODTimer RunKernelTimer;
extern time_t nNextTimeDeductTime;
extern char chLastControlKey;
extern INT nArrowUseCount;
extern BOOL bForceStatusUpdate;
extern BOOL bSysopColor;

/* Chat mode global variables. */
extern BOOL bIsShell;
extern BOOL bChatted;

/* Kernel function prototypes. */
tODResult ODKrnlInitialize(void);
void ODKrnlShutdown(void);
void ODKrnlHandleLocalKey(WORD wKeyCode);
void ODKrnlEndChatMode(void);
void ODKrnlForceOpenDoorsShutdown(BYTE btReasonForShutdown);
void ODStatStartArrowUse(void);
void ODStatEndArrowUse(void);
void ODKrnlDispatchPending(BOOL bAllowApplicationCallbacks);
void ODKrnlRequestChatToggle(void);
void ODKrnlRequestKeyboardToggle(void);
void ODKrnlRequestSysopNextToggle(void);
void ODKrnlRequestInactivityToggle(void);
void ODKrnlRequestTimeAdjustment(INT nMinutes);
void ODKrnlRequestLockout(void);
void ODKrnlRequestExit(INT nErrorLevel, BOOL bTermCall);
#ifdef ODPLAT_WIN32
void ODKrnlRequestShutdown(BYTE btReasonForShutdown);
BOOL ODKrnlRefreshUIState(void);
void ODKrnlGetUIState(tODUIState *pState);
#endif

/* Run the cooperative kernel from OpenDoors progress points. */
#define CALL_KERNEL_IF_NEEDED()     od_kernel()

#include "ODSync.h"

#define OD_API_ENTRY()              ODSyncAPIEntry();
#define OD_API_EXIT()               ODSyncAPIExit();
/* Keep these as separate guards: Turbo C 2.01 miscompiles the combined
 * Boolean expression in large-model optimized builds. */
#define OD_RETURN_IF_SESSION_ENDED(value) do { \
   if(eODLifecycleState >= kODLifecycleExitPending) { \
      od_control.od_error = ERR_GENERALFAILURE; \
      return(value); \
   } \
   if(!bODInitialized) { \
      od_control.od_error = ERR_GENERALFAILURE; \
      return(value); \
   } \
} while(0)
#define OD_RETURN_VOID_IF_SESSION_ENDED() do { \
   if(eODLifecycleState >= kODLifecycleExitPending) { \
      od_control.od_error = ERR_GENERALFAILURE; \
      return; \
   } \
   if(!bODInitialized) { \
      od_control.od_error = ERR_GENERALFAILURE; \
      return; \
   } \
} while(0)

#endif /* _INC_ODKRNL */
