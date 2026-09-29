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
 *        File: ODKrnl.c
 *
 * Description: Contains the OpenDoors kernel, which is responsible for many
 *              of the core functions which continue regardless of what the
 *              client program is doing. The implementation of this file is
 *              central to the OpenDoors architecture. The functionality
 *              implemented by the OpenDoors kernel includes (but is not
 *              limited to):
 *
 *                     - Obtaining and  input from the user, through the modem
 *                       and possibly the local keyboard.
 *                     - Monitoring maximum time and inactivity time limits.
 *                     - Responding to loss of carrier.
 *                     - Forcing the status line to be updated regularily,
 *                       on platforms that it exists.
 *                     - Implementing the system operator <-> remote user chat
 *                       mode.
 *
 *   Revisions: Date          Ver   Who  Change
 *              ---------------------------------------------------------------
 *              Jan 01, 1995  6.00  BP   Split off from odcore.c
 *              Nov 11, 1995  6.00  BP   Removed register keyword.
 *              Nov 14, 1995  6.00  BP   Added include of odscrn.h.
 *              Nov 15, 1995  6.00  BP   32-bit portability.
 *              Nov 16, 1995  6.00  BP   Removed oddoor.h, added odcore.h.
 *              Nov 17, 1995  6.00  BP   Use new input queue mechanism.
 *              Nov 21, 1995  6.00  BP   Ported to Win32.
 *              Dec 12, 1995  6.00  BP   Added entry, exit and kernel macros.
 *              Dec 13, 1995  6.00  BP   Moved chat mode code to ODKrnl.h.
 *              Dec 24, 1995  6.00  BP   od_chat_active = TRUE on chat start.
 *              Dec 30, 1995  6.00  BP   Added ODCALL for calling convention.
 *              Jan 04, 1996  6.00  BP   tODInQueueEvent -> tODInputEvent.
 *              Jan 12, 1996  6.00  BP   Added bOnlyShiftArrow.
 *              Jan 30, 1996  6.00  BP   Replaced od_yield() with od_sleep().
 *              Jan 30, 1996  6.00  BP   Add semaphore timeout.
 *              Feb 06, 1996  6.00  BP   Added od_silent_mode.
 *              Feb 19, 1996  6.00  BP   Changed version number to 6.00.
 *              Feb 23, 1996  6.00  BP   Only create active semapore once.
 *              Mar 03, 1996  6.10  BP   Begin version 6.10.
 *              Mar 06, 1996  6.10  BP   Prevent TC generated N_SCOPY@ call.
 *              Mar 13, 1996  6.10  BP   bOnlyShiftArrow -> nArrowUseCount.
 *              Mar 19, 1996  6.10  BP   MSVC15 source-level compatibility.
 *              Oct 22, 2001  6.21  RS   Lowered thread priorities to normal.
 *              Aug 10, 2003  6.23  SH   *nix support
 */

#define BUILDING_OPENDOORS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>

#include "OpenDoor.h"
#if (defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)) && defined(__WATCOMC__)
#include <bios.h>
#endif
#ifdef ODPLAT_NIX
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#endif
#include "ODCore.h"
#include "ODGen.h"
#include "ODPlat.h"
#include "ODCom.h"
#include "ODKrnl.h"
#include "ODScrn.h"
#include "ODInQue.h"
#include "ODInEx.h"
#include "ODSync.h"
#ifdef ODPLAT_WIN32
#include "ODConsole.h"
#include "ODFrame.h"
#endif /* ODPLAT_WIN32 */


/* Misc performance tuning. */
#define STATUS_UPDATE_PERIOD        3L
#define CHAT_YIELD_PERIOD           25L

/* Pending command identifiers. */
#define KERNEL_FUNC_CHATTOGGLE      0x0001

#ifdef ODPLAT_WIN32
typedef enum
{
   kODUIChangeChat,
   kODUIChangeKeyboard,
   kODUIChangeSysopNext,
   kODUIChangeInactivity,
   kODUIChangeTime,
   kODUIChangeLockout,
   kODUIChangeShutdown,
   kODUIChangeExit
} tODUIChangeType;

typedef struct tODUIChange
{
   struct tODUIChange *pNext;
   tODUIChangeType Type;
   BOOL bValue;
   INT nValue;
   BYTE btReason;
} tODUIChange;
#endif

/* Private function prototypes. */
static void ODKrnlHandleReceivedChar(char chReceived, BOOL bFromRemote);
static BOOL ODKrnlTimeUpdate(BOOL bAllowApplicationCallbacks);
static void ODKrnlChatCleanup(void);
static void ODKrnlChatMode(void);

/* Helpers used by the asynchronous Windows UI thread. */
#ifdef ODPLAT_WIN32
static BOOL ODKrnlQueueUIChange(tODUIChangeType Type, INT nValue,
   BYTE btReason);
#endif /* ODPLAT_WIN32 */
static BOOL ODKrnlDeliverTimeMessage(char *pszMessage,
   BYTE btReasonForShutdown, BOOL bAllowApplicationCallbacks);

/* Local working variables. */
#ifdef ODPLAT_WIN32
static tODMutex KernelStateLock;
static BOOL bKernelStateLockInitialized;
static tODUIChange *pPendingUIHead;
static tODUIChange *pPendingUITail;
#ifdef ODPLAT_WIN32
static tODUIState UIState;
#endif
#endif /* ODPLAT_WIN32 */
static BOOL bKernelActive = FALSE;
static BOOL bWarnedAboutInactivity = FALSE;
static INT16 nLastInactivitySetting = 0;
static time_t nNextStatusUpdateTime;
static INT nKrnlFuncPending;
static BOOL bLastStatusSetting;
static INT16 nChatOriginalAttrib;

/* Global kernel-related variables. */
tODTimer RunKernelTimer;
time_t nNextTimeDeductTime;
char chLastControlKey = '\0';
INT nArrowUseCount = 0;
BOOL bForceStatusUpdate = FALSE;
BOOL bIsShell;


/* ----------------------------------------------------------------------------
 * ODStatStartArrowUse()
 *
 * Called by OpenDoors when it needs to use the arrow keys, and so they
 * shouldn't be used by the status line.
 *
 * Parameters: None
 *
 *     Return: void
 */
void ODStatStartArrowUse(void)
{
   ++nArrowUseCount;
}


/* ----------------------------------------------------------------------------
 * ODStatEndArrowUse()
 *
 * Called by OpenDoors when it no longer needs to use the arrow keys, and so
 * they can again be used by the status line.
 *
 * Parameters: None
 *
 *     Return: void
 */
void ODStatEndArrowUse(void)
{
   ASSERT(nArrowUseCount > 0);
   --nArrowUseCount;
}



/* ========================================================================= */
/* Core of the OpenDoors Kernel.                                             */
/* ========================================================================= */

/* ----------------------------------------------------------------------------
 * ODKrnlInitialize()
 *
 * Initializes cooperative kernel activities and pending UI state.
 *
 * Parameters: none
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODKrnlInitialize(void)
{
#ifdef ODPLAT_NIX
   sigset_t		block;
#endif
   
#ifdef ODPLAT_NIX
   /* Carrier state is polled by the kernel; do not catch SIGHUP. */
   sigemptyset(&block);
   sigaddset(&block,SIGHUP);
   sigprocmask(SIG_BLOCK,&block,NULL);
#endif

   /* Initialize time of next status update and next time deduction. */
   nNextStatusUpdateTime = time(NULL) + STATUS_UPDATE_PERIOD;
   nNextTimeDeductTime = time(NULL) + 60L;
   bLastStatusSetting = od_control.od_status_on = TRUE;

   /* Initially, no kernel functions are pending. */
   nKrnlFuncPending = 0;

   /* Initially, the kernel is not active. */
   bKernelActive = FALSE;

#ifdef ODPLAT_WIN32
   if(ODPlatGetWindowsSubsystem() == kODWindowsSubsystemGUI
      && !bKernelStateLockInitialized)
   {
      if(ODMutexInitialize(&KernelStateLock) != kODRCSuccess)
         return(kODRCGeneralFailure);
      bKernelStateLockInitialized = TRUE;
   }

   ASSERT(pPendingUIHead == NULL);
   ASSERT(pPendingUITail == NULL);
   if(pPendingUIHead != NULL || pPendingUITail != NULL)
      return(kODRCGeneralFailure);
#endif /* ODPLAT_WIN32 */

   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODKrnlShutdown()
 *
 * Shuts down kernel activities.
 *
 * Parameters: none
 *
 *     Return: void
 */
void ODKrnlShutdown(void)
{
#ifdef ODPLAT_WIN32
   tODUIChange *pChange;
   tODUIChange *pNext;

   if(!bKernelStateLockInitialized)
      return;

   ODMutexLock(&KernelStateLock);
   pChange = pPendingUIHead;
   pPendingUIHead = NULL;
   pPendingUITail = NULL;
   ODMutexUnlock(&KernelStateLock);

   while(pChange != NULL)
   {
      pNext = pChange->pNext;
      free(pChange);
      pChange = pNext;
   }

   ODMutexDestroy(&KernelStateLock);
   bKernelStateLockInitialized = FALSE;
#endif
}


/* ----------------------------------------------------------------------------
 * od_kernel()
 *
 * Carries out any kernel tasks that must be performed through regular,
 * explicit calls to this function,
 *
 * Parameters: none
 *
 *     Return: void
 */
extern tODMilliSec ODMaxMSToWait;
ODAPIDEF void ODCALL od_kernel(void)
{
   char ch;
   tODInputEvent InputEvent;
   tODResult InputResult;
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) || defined(ODPLAT_WIN32)
   WORD wKey;
   BYTE btShiftStatus;
   INT nKeyIndex;
   char *pszShellName;
#endif
   BOOL bCarrier;

   /* Log function entry if running in trace mode. */
   TRACE(TRACE_API, "od_kernel()");

   /* Initialize OpenDoors if not already done. */
   if(!bODInitialized) od_init();
   OD_RETURN_VOID_IF_SESSION_ENDED();

   /* If this is an attempt at a re-entrant call to od_kernel() from another */
   /* function called by a currently active od_kernel(), then return without */
   /* doing anything.                                                        */
   if(bKernelActive) return;

   OD_API_ENTRY();

   /* Note that kernel is active to prevent recursive calls to the kernel. */
   bKernelActive = TRUE;

   /* Call od_ker_exec function if required. */
   if(eODLifecycleState == kODLifecycleActive
      && od_control.od_ker_exec != NULL)
   {
      (*od_control.od_ker_exec)();
      if(!bODInitialized || eODLifecycleState >= kODLifecycleExitPending)
         goto kernel_finished;
   }

   /* If not operating in local mode, then perform remote-mode specific */
   /* activies.                                                         */
   if(od_control.baud != 0)
   {
      /* If carrier detection is enabled, then shutdown OpenDoors if */
      /* the carrier detect signal is no longer high.                */
      if(!(od_control.od_disable&DIS_CARRIERDETECT))
      {
         ODComCarrier(hSerialPort, &bCarrier);
         if(!bCarrier)
         {
            ODKrnlForceOpenDoorsShutdown(ERRORLEVEL_NOCARRIER);
         }
      }
      /* Loop, obtaining any new characters from the serial port and */
      /* adding them to the common local/remote input queue.         */
      for(;;)
      {
         if(od_control.od_user_keyboard_on
            && ODInQueueReserveEvent(hODInputQueue) != kODRCSuccess)
         {
            break;
         }
         InputResult = ODComGetByte(hSerialPort, &ch, FALSE);
         if(InputResult != kODRCSuccess)
         {
            if(od_control.od_user_keyboard_on)
               ODInQueueCancelReservedEvent(hODInputQueue);
            break;
         }
         if(od_control.od_user_keyboard_on)
         {
            InputEvent.EventType = EVENT_CHARACTER;
            InputEvent.bFromRemote = TRUE;
            InputEvent.chKeyPress = ch;
            ODInQueueCommitReservedEvent(hODInputQueue, &InputEvent);
         }
         ODMaxMSToWait = 0;
      }
   }

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) || defined(ODPLAT_WIN32)
check_keyboard_again:
#ifdef ODPLAT_WIN32
   if(ODPlatGetWindowsSubsystem() != kODWindowsSubsystemConsole)
      goto after_console_text_interface;
#endif
    if(nKrnlFuncPending && !bShellChatActive)
    {
       if(nKrnlFuncPending & KERNEL_FUNC_CHATTOGGLE)
       {
          nKrnlFuncPending &=~ KERNEL_FUNC_CHATTOGGLE;
          goto chat_pressed;
       }
    }

   /* Don't check local keyboard if sysop DIS_SYSOP_KEYS is set, or if we */
   /* are operatingin silent mode.                                        */
   if(od_control.od_disable & DIS_SYSOP_KEYS
      || od_control.od_silent_mode)
   {
      goto after_key_check;
   }

#ifdef ODPLAT_WIN32
   if(!ODConsoleReadKey(&wKey, &btShiftStatus))
      goto after_key_check;
#else /* !ODPLAT_WIN32 */
#if defined(__WATCOMC__)
   if(_bios_keybrd(_KEYBRD_READY) == 0)
      goto after_key_check;
   wKey = _bios_keybrd(_KEYBRD_READ);
   btShiftStatus = (BYTE)_bios_keybrd(_KEYBRD_SHIFTSTATUS);
#else
   ASM    mov ah, 1
   ASM    push si
   ASM    push di
   ASM    int 0x16
   ASM    jnz key_waiting
   ASM    pop di
   ASM    pop si
   ASM    jmp after_key_check
key_waiting:
   ASM    mov ah, 0
   ASM    int 0x16
   ASM    mov wKey, ax
   ASM    mov ah, 2
   ASM    int 0x16
   ASM    mov btShiftStatus, al
   ASM    pop di
   ASM    pop si
#endif /* __WATCOMC__ */
#endif /* !ODPLAT_WIN32 */

      if(nArrowUseCount > 0 && (wKey == 0x4800 || wKey == 0x5000)
         && !(btShiftStatus & 2))
      {
         /* Pass key on to od_local_input, if it is defined. */
         if(od_control.od_local_input != NULL)
         {
            (*od_control.od_local_input)(wKey);
         }

         /* Add this key to the local/remote input queue. */
         ODKrnlHandleLocalKey(wKey);
      }

      /* If hangup key is pressed. */
      else if(wKey == od_control.key_hangup)
      {
         ODKrnlForceOpenDoorsShutdown(ERRORLEVEL_HANGUP);
      }

      /* If drop to BBS key is pressed. */
      else if(wKey == od_control.key_drop2bbs)
      {
         ODKrnlForceOpenDoorsShutdown(ERRORLEVEL_DROPTOBBS);
      }

      else if(wKey == od_control.key_dosshell)
      {
         if(!bShellChatActive)
         {
            if(pfLogWrite != NULL)
               (*pfLogWrite)(6);

            /* If function hook is defined. */
            if(od_control.od_cbefore_shell != NULL)
            {
               /* Then call it. */
               bShellChatActive = TRUE;
               (*od_control.od_cbefore_shell)();
               bShellChatActive = FALSE;
            }

            if(od_control.od_before_shell != NULL)
               od_disp_str(od_control.od_before_shell);

            if((pszShellName = (char *)getenv("COMSPEC")) == NULL)
            {
               pszShellName = (char *)"COMMAND.COM";
            }
            bIsShell = TRUE;
            od_spawnvpe(P_WAIT, pszShellName, NULL, NULL);
            bIsShell = FALSE;

            if(od_control.od_after_shell != NULL)
               od_disp_str(od_control.od_after_shell);

            /* If a function hook is defined. */
            if(od_control.od_cafter_shell != NULL)
            {
               /* Then call it. */
               bShellChatActive = TRUE;
               (*od_control.od_cafter_shell)();
               bShellChatActive = FALSE;
            }

            if(pfLogWrite != NULL)
               (*pfLogWrite)(7);
         }
      }

      /* If toggle chat mode key is pressed. */
      else if(wKey == od_control.key_chat)
      {
chat_pressed:
         if(!bShellChatActive || od_control.od_chat_active)
         {
            /* If chat mode is active. */
            if(od_control.od_chat_active)
            {
               /* Signal exit of chat mode. */
               ODKrnlEndChatMode();
            }

            /* If chat mode is off. */
            else
            {
               /* Enable second call to kernel. */
               bKernelActive = FALSE;

               /* Enter chat mode. */
               ODKrnlChatMode();

               /* Disable second call to kernel. */
               bKernelActive = TRUE;
            }
         }
         else
         {
            if(nKrnlFuncPending & KERNEL_FUNC_CHATTOGGLE)
            {
               nKrnlFuncPending &= ~KERNEL_FUNC_CHATTOGGLE;
            }
            else
            {
               nKrnlFuncPending |= KERNEL_FUNC_CHATTOGGLE;
            }
         }
      }

      /* If sysop next key is pressed. */
      else if(wKey == od_control.key_sysopnext)
      {
         /* Toggle sysop next setting. */
         od_control.sysop_next = !od_control.sysop_next;

         /* Update status line. */
         goto statup;
      }

      /* If ESCape key is pressed and we are in chat mode. */
      else if((wKey&0xff) == 27 && od_control.od_chat_active)
      {
         /* Signal exit from chat mode. */
         od_control.od_chat_active = FALSE;
      }

      /* If lockout user key is pressed. */
      else if(wKey == od_control.key_lockout)
      {
         /* Set the user's access security level to 0. */
         od_control.user_security = 0;

         /* Shutdown OpenDoors. */
         ODKrnlForceOpenDoorsShutdown(ERRORLEVEL_HANGUP);
      }


      /* If toggle keyboard off key is pressed. */
      else if(wKey == od_control.key_keyboardoff)
      {
         /* Toggle user keyboard settings. */
         od_control.od_user_keyboard_on =! od_control.od_user_keyboard_on;

         /* Update status line. */
         goto statup;
      }

      /* If increase time key is pressed. */
      else if(wKey == od_control.key_moretime)
      {
         /* If time limit is less than maximum possible time limit. */
         if(od_control.user_timelimit < 1440)
         {
             /* Increase time left online. */
            ++od_control.user_timelimit;
         }

         /* Update status line. */
         goto statup;
      }

      /* If decrease time key is pressed. */
      else if(wKey == od_control.key_lesstime)
      {
         /* Never let user's time limit be set to a negative value. */
         if(od_control.user_timelimit > 0)
         {
            /* Decrease user's timelimit. */
            --od_control.user_timelimit;
         }

         /* Update the status line. */
         goto statup;
      }

      else
      {
         for(nKeyIndex = 0; nKeyIndex < 9; ++nKeyIndex)
         {
            if(wKey == od_control.key_status[nKeyIndex])
            {
               if(btCurrentStatusLine != nKeyIndex
                  && od_control.od_status_on)
               {
                  od_set_statusline(nKeyIndex);
               }
               goto check_keyboard_again;
            }
         }

         /* Look for user-defined hotkeys. */
         for(nKeyIndex = 0; nKeyIndex < od_control.od_num_keys; ++nKeyIndex)
         {
            /* If it matches. */
            if(wKey == (WORD)od_control.od_hot_key[nKeyIndex])
            {
               /* Record keypress. */
               od_control.od_last_hot = wKey;

               /* Notify the current personality. */
               (*pfCurrentPersonality)(21);

               /* Check for a hotkey function. */
               if(od_control.od_hot_function[nKeyIndex] != NULL)
               {
                  /* Call it if it exists. */
                  (*od_control.od_hot_function[nKeyIndex])();
               }

               /* Stop searching. */
               break;
            }
         }

         /* If no hotkeys found. */
         if(nKeyIndex >= od_control.od_num_keys)
         {
            /* Pass key on to od_local_input, if it is defined. */
            if(od_control.od_local_input != NULL)
            {
               (*od_control.od_local_input)(wKey);
            }

            /* Add this key to the local/remote input queue. */
            ODKrnlHandleLocalKey(wKey);
         }
      }
   goto check_keyboard_again;

after_key_check:

   /* If status line has been turned on since last call to kernel. */
   if(bLastStatusSetting != od_control.od_status_on)
   {
      /* Generate the status line. */
      od_set_statusline(0);
   }

   bLastStatusSetting = od_control.od_status_on;

   if(od_control.od_update_status_now)
   {
      od_set_statusline(btCurrentStatusLine);
      od_control.od_update_status_now = FALSE;
   }

   /* Update status line when needed. */
   if(nNextStatusUpdateTime < time(NULL) || bForceStatusUpdate)
   {
statup:
      nNextStatusUpdateTime = time(NULL) + STATUS_UPDATE_PERIOD;

      /* Turn off status line update force flag */
      bForceStatusUpdate = FALSE;

      if(od_control.od_status_on && btCurrentStatusLine != 8)
      {
         /* Store console settings. */
         ODStoreTextInfo();

         /* Enable writes to whole screen. */
         ODScrnSetBoundary(1, 1, 80, 25);
         ODScrnEnableCaret(FALSE);
         (*pfCurrentPersonality)((BYTE)(10 + btCurrentStatusLine));
         ODRestoreTextInfo();
         ODScrnEnableCaret(TRUE);
      }
   }
#endif

#ifdef ODPLAT_WIN32
after_console_text_interface:
#endif
   ODKrnlTimeUpdate(TRUE);

   ODTimerStart(&RunKernelTimer, 250);

kernel_finished:
   OD_API_EXIT();

   bKernelActive = FALSE;
}


/* ----------------------------------------------------------------------------
 * ODKrnlHandleLocalKey()
 *
 * Called when a key is pressed on the local keyboard that should be placed
 * in the common local/remote input queue. This function is not called for
 * sysop function keys.
 *
 * Parameters: wKeyCode
 *
 *     Return: void
 */
void ODKrnlHandleLocalKey(WORD wKeyCode)
{
   BOOL bLocalInputDisabled;

#ifdef ODPLAT_WIN32
   if(ODPlatGetWindowsSubsystem() == kODWindowsSubsystemConsole)
      bLocalInputDisabled = (od_control.od_disable & DIS_LOCAL_INPUT) != 0;
   else
   {
      tODUIState State;
      ODKrnlGetUIState(&State);
      bLocalInputDisabled = (State.wDisable & DIS_LOCAL_INPUT) != 0;
   }
#else
   bLocalInputDisabled = (od_control.od_disable & DIS_LOCAL_INPUT) != 0;
#endif

   /* If local keyboard input by sysop has not been disabled. */
   if(!bLocalInputDisabled)
   {
      if((wKeyCode & 0xff) == 0)
      {
         tODInputEvent Events[2];
         Events[0].EventType = Events[1].EventType = EVENT_CHARACTER;
         Events[0].bFromRemote = Events[1].bFromRemote = FALSE;
         Events[0].chKeyPress = '\0';
         Events[1].chKeyPress = (char)(wKeyCode >> 8);
         ODInQueueAddEvents(hODInputQueue, Events, 2);
      }
      else
      {
         ODKrnlHandleReceivedChar((char)wKeyCode, FALSE);
      }
   }
}


/* ----------------------------------------------------------------------------
 * ODKrnlHandleReceivedChar()                          *** PRIVATE FUNCTION ***
 *
 * Called when a character is received from the local or remote system.
 *
 * Parameters: chReceived  - Character that should be handled.
 *
 *             bFromRemote - TRUE if this character was received from the
 *                           remote system, FALSE if it originated from the
 *                           local console.
 *
 *     Return: void
 */
static void ODKrnlHandleReceivedChar(char chReceived, BOOL bFromRemote)
{
   tODInputEvent InputEvent;
   /* If we are operating in remote mode, and remote user keyboard has been */
   /* disabled by the sysop, then return, ignoring this character.          */
   if(bFromRemote && !od_control.od_user_keyboard_on)
   {
      return;
   }

   /* Add this input event to the local/remote common input queue. */
   InputEvent.EventType = EVENT_CHARACTER;
   InputEvent.bFromRemote = bFromRemote;
   InputEvent.chKeyPress = chReceived;
   ODInQueueAddEvent(hODInputQueue, &InputEvent);

}


/* ----------------------------------------------------------------------------
 * ODKrnlTimeUpdate()                                  *** PRIVATE FUNCTION ***
 *
 * Performs regular updating of time remaining online, inactivity time, and
 * forces OpenDoors to exit if a time limit has been exceeded.
 *
 * Parameters: None
 *
 *     Return: void
 */
static BOOL ODKrnlTimeUpdate(BOOL bAllowApplicationCallbacks)
{
   time_t CurrentTime;
   static char szTemp[80];

   /* Obtain the current time. */
   CurrentTime = time(NULL);

   /* If inactivity setting has changed. */
   if(nLastInactivitySetting != od_control.od_inactivity)
   {
      /* If it was previously disabled. */
      if(nLastInactivitySetting == 0)
      {
         /* Prevent immediate timeout. */
         ODInQueueResetLastActivity(hODInputQueue);
      }

      /* Store current value. */
      nLastInactivitySetting = od_control.od_inactivity;
   }

   /* Check user keyboard inactivity. */
   if((ODInQueueGetLastActivity(hODInputQueue) + od_control.od_inactivity)
      < CurrentTime)
   {
      /* If timeout, display message. */
      if(od_control.od_inactivity != 0 && !od_control.od_disable_inactivity)
      {
         ODKrnlDeliverTimeMessage(od_control.od_inactivity_timeout,
            ERRORLEVEL_INACTIVITY, bAllowApplicationCallbacks);
         return(TRUE);
      }
   }

   /* If less than 5s left of inactivity. */
   else if(ODInQueueGetLastActivity(hODInputQueue) + od_control.od_inactivity
      < CurrentTime + od_control.od_inactive_warning)
   {
      if(!bWarnedAboutInactivity && od_control.od_inactivity != 0
         && !od_control.od_disable_inactivity)
      {
         /* Warn the user. */
         if(ODKrnlDeliverTimeMessage(od_control.od_inactivity_warning, 0,
            bAllowApplicationCallbacks)) return(TRUE);
         /* Don't warn the user a second time. */
         bWarnedAboutInactivity = TRUE;
      }
   }
   else
   {
      /* Re-enable inactivity warning. */
      bWarnedAboutInactivity = FALSE;
   }

   /* If chat mode is active. */
   if(od_control.od_chat_active)
   {
      /* Prevent the user's time from being drained. */
      nNextTimeDeductTime = time(NULL) + 60;
   }

   /* Apply every elapsed minute since the preceding cooperative update. */
   while(CurrentTime >= nNextTimeDeductTime && bODInitialized)
   {
      /* Next time update should occur 60 seconds after this one was */
      /* scheduled.                                                  */
      nNextTimeDeductTime += 60;

      /* Force status line to be updated immediately. */
      bForceStatusUpdate = TRUE;

      /* Decrement time left. */
      --od_control.user_timelimit;

      /* If the user's time limit is close to expiring, then notify */
      /* the user.                                                  */
      if(od_control.user_timelimit <= 3 &&
         od_control.user_timelimit > 0 &&
         !(od_control.od_disable & DIS_TIMEOUT))
      {
         /* If less than 3 mins left, tell user. */
         sprintf(szTemp, od_control.od_time_warning,
            od_control.user_timelimit);
         if(ODKrnlDeliverTimeMessage(szTemp, 0,
            bAllowApplicationCallbacks)) return(TRUE);
      }

   }

   /* If user has no time left. */
   if(od_control.user_timelimit <= 0
      && !(od_control.od_disable & DIS_TIMEOUT))
   {
      /* Notify the user. */
      ODKrnlDeliverTimeMessage(od_control.od_no_time, ERRORLEVEL_TIMEOUT,
         bAllowApplicationCallbacks);
      return(TRUE);
   }

   return(FALSE);
}


/* ----------------------------------------------------------------------------
 * ODKrnlDeliverTimeMessage()                           *** PRIVATE FUNCTION ***
 *
 * Delivers a kernel-generated time message in the application API flow.
 *
 *     Return: TRUE if timer processing must stop.
 */
static BOOL ODKrnlDeliverTimeMessage(char *pszMessage,
   BYTE btReasonForShutdown, BOOL bAllowApplicationCallbacks)
{
   (void)bAllowApplicationCallbacks;

   if(od_control.od_time_msg_func == NULL)
      od_disp_str(pszMessage);
   else
      (*od_control.od_time_msg_func)(pszMessage);

   if(!bODInitialized)
      return(TRUE);
   if(btReasonForShutdown != 0)
   {
      ODKrnlForceOpenDoorsShutdown(btReasonForShutdown);
      return(TRUE);
   }
   return(FALSE);
}

#ifdef ODPLAT_WIN32
static BOOL ODKrnlQueueUIChange(tODUIChangeType Type, INT nValue,
   BYTE btReason)
{
   tODUIChange *pChange;

   if(ODPlatGetWindowsSubsystem() != kODWindowsSubsystemGUI
      || eODLifecycleState != kODLifecycleActive)
      return(FALSE);
   pChange = malloc(sizeof(*pChange));

   if(pChange == NULL)
   {
#ifdef ODPLAT_WIN32
      MessageBeep(MB_ICONEXCLAMATION);
#endif
      return(FALSE);
   }

   pChange->pNext = NULL;
   pChange->Type = Type;
   pChange->bValue = FALSE;
   pChange->nValue = nValue;
   pChange->btReason = btReason;

   ODMutexLock(&KernelStateLock);
#ifdef ODPLAT_WIN32
   switch(Type)
   {
      case kODUIChangeChat:
         pChange->bValue = !UIState.bChatActive;
         UIState.bChatActive = pChange->bValue;
         break;
      case kODUIChangeKeyboard:
         pChange->bValue = !UIState.bUserKeyboardOn;
         UIState.bUserKeyboardOn = pChange->bValue;
         break;
      case kODUIChangeSysopNext:
         pChange->bValue = !UIState.bSysopNext;
         UIState.bSysopNext = pChange->bValue;
         break;
      case kODUIChangeInactivity:
         pChange->bValue = !UIState.bInactivityDisabled;
         UIState.bInactivityDisabled = pChange->bValue;
         break;
      case kODUIChangeTime:
         UIState.nTimeLimit = MAX(OD_MIN_USER_TIME_MINUTES,
            MIN(OD_MAX_USER_TIME_MINUTES, UIState.nTimeLimit + nValue));
         break;
      default:
         break;
   }
#endif

   if(pPendingUITail == NULL)
      pPendingUIHead = pChange;
   else
      pPendingUITail->pNext = pChange;
   pPendingUITail = pChange;
   ODMutexUnlock(&KernelStateLock);

#ifdef ODPLAT_WIN32
   ODFrameControlStateChanged();
#endif
   return(TRUE);
}

void ODKrnlRequestShutdown(BYTE btReasonForShutdown)
{
   (void)ODKrnlQueueUIChange(kODUIChangeShutdown, 0,
      btReasonForShutdown);
}
#endif

#ifdef ODPLAT_WIN32
BOOL ODKrnlRefreshUIState(void)
{
   BOOL bQueueEmpty;

   ASSERT(bKernelStateLockInitialized);
   if(!bKernelStateLockInitialized)
      return(FALSE);

   ODMutexLock(&KernelStateLock);
   bQueueEmpty = pPendingUIHead == NULL;
   if(bQueueEmpty)
   {
      UIState.hAppIcon = od_control.od_app_icon;
      memcpy(UIState.szProgramName, od_control.od_prog_name,
         sizeof(UIState.szProgramName));
      memcpy(UIState.szProgramCopyright, od_control.od_prog_copyright,
         sizeof(UIState.szProgramCopyright));
      memcpy(UIState.szProgramVersion, od_control.od_prog_version,
         sizeof(UIState.szProgramVersion));
      memcpy(UIState.szUserName, od_control.user_name,
         sizeof(UIState.szUserName));
      memcpy(UIState.szUserLocation, od_control.user_location,
         sizeof(UIState.szUserLocation));
      memcpy(UIState.szUserReasonForChat, od_control.user_reasonforchat,
         sizeof(UIState.szUserReasonForChat));
      UIState.dwBaud = od_control.baud;
      UIState.dwConnectSpeed = od_control.od_connect_speed;
      UIState.nNode = od_control.od_node;
      UIState.nTimeLimit = od_control.user_timelimit;
      UIState.nCmdShow = od_control.od_cmd_show;
      UIState.wDisable = od_control.od_disable;
      UIState.bUserWantsChat = od_control.user_wantchat;
      UIState.bInactivityDisabled = od_control.od_disable_inactivity;
      UIState.bSysopNext = od_control.sysop_next;
      UIState.bUserKeyboardOn = od_control.od_user_keyboard_on;
      UIState.bChatActive = od_control.od_chat_active;
      UIState.pfHelpCallback = od_control.od_help_callback;
      UIState.pfConfigCallback = od_control.od_config_callback;
   }
   ODMutexUnlock(&KernelStateLock);

   if(bQueueEmpty)
      ODFrameControlStateChanged();
   return(bQueueEmpty);
}

void ODKrnlGetUIState(tODUIState *pState)
{
   ASSERT(pState != NULL);
   ASSERT(bKernelStateLockInitialized);
   if(pState == NULL || !bKernelStateLockInitialized)
      return;

   ODMutexLock(&KernelStateLock);
   memcpy(pState, &UIState, sizeof(*pState));
   ODMutexUnlock(&KernelStateLock);
}
#endif


/* ----------------------------------------------------------------------------
 * ODKrnlForceOpenDoorsShutdown()
 *
 * Called to force the application to exit due to some event in OpenDoors,
 * such as loss of carrier, user inactivity timeout, the hangup command
 * being chosen by the system operator, etc. The only time when OpenDoors
 * is shutdown without going through this function should be as a result of
 * an explicit call to od_exit() by the client application.
 *
 * Parameters: btReasonForShutdown - An OpenDoors exit reason code.
 *
 *     Return: Never returns.
 */
void ODKrnlForceOpenDoorsShutdown(BYTE btReasonForShutdown)
{
   BOOL bHangup;

   /* Determine whether we should hangup on the user before exiting. */
   if(btReasonForShutdown == ERRORLEVEL_HANGUP
      || btReasonForShutdown == ERRORLEVEL_INACTIVITY)
   {
      bHangup = TRUE;
   }
   else
   {
      bHangup = FALSE;
   }

   /* Record exit reason in global variable. */
   btExitReason = btReasonForShutdown - 1;

   /* Use the client-defined errorlevel, if any. */
   if(od_control.od_errorlevel[0])
   {
      od_exit(od_control.od_errorlevel[btReasonForShutdown], bHangup);
   }

   /* Otherwise, use the default OpenDoors errorlevel. */
   else
   {
      od_exit(btReasonForShutdown - 1, bHangup);
   }
}

void ODKrnlRequestChatToggle(void)
{
#ifdef ODPLAT_WIN32
   (void)ODKrnlQueueUIChange(kODUIChangeChat, 0, 0);
#else
   nKrnlFuncPending ^= KERNEL_FUNC_CHATTOGGLE;
#endif
}

void ODKrnlRequestKeyboardToggle(void)
{
#ifdef ODPLAT_WIN32
   (void)ODKrnlQueueUIChange(kODUIChangeKeyboard, 0, 0);
#else
   od_control.od_user_keyboard_on = !od_control.od_user_keyboard_on;
#endif
}

void ODKrnlRequestSysopNextToggle(void)
{
#ifdef ODPLAT_WIN32
   (void)ODKrnlQueueUIChange(kODUIChangeSysopNext, 0, 0);
#else
   od_control.sysop_next = !od_control.sysop_next;
#endif
}

void ODKrnlRequestInactivityToggle(void)
{
#ifdef ODPLAT_WIN32
   (void)ODKrnlQueueUIChange(kODUIChangeInactivity, 0, 0);
#else
   od_control.od_disable_inactivity = !od_control.od_disable_inactivity;
#endif
}

void ODKrnlRequestTimeAdjustment(INT nMinutes)
{
#ifdef ODPLAT_WIN32
   (void)ODKrnlQueueUIChange(kODUIChangeTime, nMinutes, 0);
#else
   od_control.user_timelimit += nMinutes;
#endif
}

void ODKrnlRequestLockout(void)
{
#ifdef ODPLAT_WIN32
   (void)ODKrnlQueueUIChange(kODUIChangeLockout, 0,
      ERRORLEVEL_HANGUP);
#else
   od_control.user_security = 0;
   ODKrnlForceOpenDoorsShutdown(ERRORLEVEL_HANGUP);
#endif
}

void ODKrnlRequestExit(INT nErrorLevel, BOOL bTermCall)
{
#ifdef ODPLAT_WIN32
   (void)ODKrnlQueueUIChange(kODUIChangeExit, nErrorLevel,
      (BYTE)bTermCall);
#else
   od_exit(nErrorLevel, bTermCall);
#endif
}

void ODKrnlDispatchPending(BOOL bAllowApplicationCallbacks)
{
#ifdef ODPLAT_WIN32
   tODUIChange *pChange;
   tODUIChange *pNext;

   if(!bKernelStateLockInitialized) return;
   (void)bAllowApplicationCallbacks;

   for(;;)
   {
      ODMutexLock(&KernelStateLock);
      pChange = pPendingUIHead;
      pPendingUIHead = NULL;
      pPendingUITail = NULL;
      ODMutexUnlock(&KernelStateLock);

      if(pChange == NULL)
      {
#ifdef ODPLAT_WIN32
         if(!ODKrnlRefreshUIState())
            continue;
#endif
         return;
      }

      while(pChange != NULL)
      {
         pNext = pChange->pNext;
         switch(pChange->Type)
         {
            case kODUIChangeChat:
               if(pChange->bValue && !od_control.od_chat_active)
                  ODKrnlChatMode();
               else if(!pChange->bValue && od_control.od_chat_active)
                  ODKrnlEndChatMode();
               break;
            case kODUIChangeKeyboard:
               od_control.od_user_keyboard_on = pChange->bValue;
               break;
            case kODUIChangeSysopNext:
               od_control.sysop_next = pChange->bValue;
               break;
            case kODUIChangeInactivity:
               od_control.od_disable_inactivity = pChange->bValue;
               break;
            case kODUIChangeTime:
               od_control.user_timelimit = MAX(OD_MIN_USER_TIME_MINUTES,
                  MIN(OD_MAX_USER_TIME_MINUTES,
                  od_control.user_timelimit + pChange->nValue));
               break;
            case kODUIChangeLockout:
               od_control.user_security = 0;
               ODKrnlForceOpenDoorsShutdown(pChange->btReason);
               break;
            case kODUIChangeShutdown:
               ODKrnlForceOpenDoorsShutdown(pChange->btReason);
               break;
            case kODUIChangeExit:
               od_exit(pChange->nValue, pChange->btReason != 0);
               break;
            default:
               /* Ignore a corrupt or future queue node safely. */
               break;
         }
         free(pChange);
         pChange = pNext;

         if(!bODInitialized
            || eODLifecycleState != kODLifecycleActive)
         {
            while(pChange != NULL)
            {
               pNext = pChange->pNext;
               free(pChange);
               pChange = pNext;
            }
            return;
         }
      }
   }
#else
   (void)bAllowApplicationCallbacks;
#endif
}



/* ========================================================================= */
/* OpenDoors chat mode.                                                      */
/* ========================================================================= */

BOOL bChatted;
BOOL bSysopColor;

/* ----------------------------------------------------------------------------
 * ODKrnlEndChatMode()
 *
 * Forces chat mode to exit.
 *
 * Parameters: None
 *
 *     Return: void
 */
void ODKrnlEndChatMode(void)
{
   od_control.od_chat_active = FALSE;
}


/* ----------------------------------------------------------------------------
 * od_chat()
 *
 * Allows the client application to activate the line-by-line default chat
 * mode provided by OpenDoors, allowing the local sysop and remote user to
 * communicate with one another in real time.
 *
 * Parameters: none
 *
 *     Return: void
 */
ODAPIDEF void ODCALL od_chat(void)
{
   /* Log function entry if running in trace mode. */
   TRACE(TRACE_API, "od_chat()");

   /* Initialize OpenDoors if it hasn't already been done. */
   if(!bODInitialized) od_init();
   OD_RETURN_VOID_IF_SESSION_ENDED();

   OD_API_ENTRY();

   /* Set the main chat active flag in od_control. */
   od_control.od_chat_active = TRUE;

   ODKrnlChatMode();

   OD_API_EXIT();
}


/* ----------------------------------------------------------------------------
 * ODKrnlChatMode()                                    *** PRIVATE FUNCTION ***
 *
 * Implements the OpenDoors chat mode.
 *
 * Parameters: None
 *
 *     Return: void
 */
static void ODKrnlChatMode(void)
{
   BYTE chKeyPressed;
   char szCurrentWord[79];
   BYTE btWordLength = 0;
   BYTE btCurrentColumn = 0;
   char *pchCurrent;
   BYTE btCount;
   tODTimer Timer;

   /* Empty current word string. */
   szCurrentWord[0] = '\0';

   /* Save current display color attribute. */
   nChatOriginalAttrib = od_control.od_cur_attrib;

   /* Record that sysop has entered chat mode. */
   bChatted = TRUE;

   /* Turn off "user wants to chat" indicator, and force the status line. */
   /* to be updated.                                                      */
   od_control.user_wantchat = FALSE;

   bForceStatusUpdate = TRUE;
   CALL_KERNEL_IF_NEEDED();

   /* Note that chat mode is now active. */
   od_control.od_chat_active = TRUE;

   /* If a pre-chat function hook has been defined, then call it. */
   if(od_control.od_cbefore_chat!=NULL)
   {
      bShellChatActive = TRUE;
      (*od_control.od_cbefore_chat)();
      bShellChatActive = FALSE;

      /* If chat has been deactivated, then return right away */
      if(!od_control.od_chat_active) goto cleanup;
   }

   /* Display a message indicating that the sysop has entered chat mode. */
   od_set_attrib(od_control.od_chat_color1);
   if(od_control.od_before_chat != NULL)
      od_disp_str(od_control.od_before_chat);

   /* Currently set to sysop color. */
   bSysopColor = TRUE;

   /* If the logfile system is hooked up, then write a log entry */
   /* indicating that the sysop has entered chat mode.           */
   if(pfLogWrite != NULL)
   {
      (*pfLogWrite)(9);
   }

   /* Start a timer that will elapse after 25 milliseconds. */
   ODTimerStart(&Timer, CHAT_YIELD_PERIOD);

   /* Loop while sysop chat mode is stilil on. */
   while(od_control.od_chat_active)
   {
      /* Obtain the next key from the user. */
      chKeyPressed = od_get_key(FALSE);

      /* If color not set correctly. */
      if(od_control.od_last_input != bSysopColor)
      {
         /* If sysop was last person to type. */
         if(od_control.od_last_input)
         {
            /* Switch to sysop text color. */
            od_set_attrib(od_control.od_chat_color1);
         }
         else
         {
            /* Otherwise, switch to the user text color. */
            od_set_attrib(od_control.od_chat_color2);
         }

         /* Record current color setting. */
         bSysopColor = od_control.od_last_input;
      }

      /* If this is a displayable character. */
      if(chKeyPressed >= 32)
      {
         /* Display the character that was typed. */
         od_putch(chKeyPressed);

         /* If the user pressed spacebar, then this is the end of the */
         /* previous word. */
         if(chKeyPressed == 32)
         {
            btWordLength = 0;
            szCurrentWord[0] = 0;
         }

         /* Add this character to the current word, if we haven't exceeded */
         /* the maximum word length.                                       */
         else if(btWordLength < 70)
         {
            szCurrentWord[btWordLength++] = chKeyPressed;
            szCurrentWord[btWordLength] = '\0';
         }

         /* If we are not yet at the end of the line, then increment the */
         /* current column number.                                       */
         if(btCurrentColumn < 75)
         {
            ++btCurrentColumn;
         }

         /* If we are at the end of the line. */
         else
         {
            /* If the current word should be wrapped to the next line. */
            if(btWordLength < 70 && btWordLength > 0)
            {
               /* Generate a string to erase the word from the current line. */
               pchCurrent = (char *)szODWorkString;
               for(btCount = 0; btCount < btWordLength; ++btCount)
               {
                  *(pchCurrent++) = 8;
               }

               for(btCount = 0; btCount < btWordLength; ++btCount)
               {
                  *(pchCurrent++) = ' ';
               }

               *pchCurrent = '\0';

               /* Display the string to erase the old word. */
               od_disp_str(szODWorkString);

               /* Move to the next line. */               
               od_disp_str("\r\n");

               /* Redisplay the word on the next line. */
               od_disp_str(szCurrentWord);

               /* Update current column number. */               
               btCurrentColumn = btWordLength;
            }

            /* If we have reached the end of the line, but word wrap should */
            /* not be performed.                                            */
            else
            {
               /* Move to the next line. */
               od_disp_str("\r\n");

               /* Update the current column number. */
               btCurrentColumn = 0;
            }

            /* Reset the current word information. */
            btWordLength = 0;
            szCurrentWord[0] = 0;
         }
      }

      /* If the backspace key was pressed. */
      else if(chKeyPressed == 8)
      {
         /* Send backspace sequence. */
         od_disp_str(szBackspaceWithDelete);

         /* If we are in the middle of a word, then we must remove the */
         /* last character of the word.                                */         
         if(btWordLength > 0)
         {
            szCurrentWord[--btWordLength] = '\0';
         }

         /* Update the current column number. */
         if(btCurrentColumn > 0) --btCurrentColumn;
      }

      /* If the enter key was pressed. */
      else if(chKeyPressed == 13)
      {
         /* Send carriage return / line feed sequence. */
         od_disp_str("\r\n");

         /* Reset the current word contents. */
         btWordLength = 0;
         szCurrentWord[0] = 0;

         /* Update the current column number. */
         btCurrentColumn = 0;
      }

      /* If the sysop pressed the escape key. */
      else if(chKeyPressed == 27 && od_control.od_last_input)
      {
         /* Exit chat mode. */
         goto cleanup;
      }

      /* Give up processor after 25 milliseconds elapsed. */
      else if(ODTimerElapsed(&Timer))
      {
         od_sleep(0);

         /* Restart the timer, so that it will elapse after another */
         /* 25 milliseconds.                                        */
         ODTimerStart(&Timer, CHAT_YIELD_PERIOD);
      }
   }

cleanup:
   ODKrnlChatCleanup();
}


/* ----------------------------------------------------------------------------
 * ODKrnlChatCleanup()                                 *** PRIVATE FUNCTION ***
 *
 * Performs post-chat operations, such as resetting the original display
 * color, etc.
 *
 * Parameters: None
 *
 *     Return: void
 */
static void ODKrnlChatCleanup(void)
{
   od_set_attrib(od_control.od_chat_color1);

   /* Indicate that chat mode is exiting. */
   if(od_control.od_after_chat != NULL)
   {
      od_disp_str(od_control.od_after_chat);
   }

   /* If an after chat function has been provided, then call it. */
   if(od_control.od_cafter_chat != NULL)
   {
      bShellChatActive = TRUE;
      (*od_control.od_cafter_chat)();
      bShellChatActive = FALSE;
   }

   /* If the logfile system is hooked up, then write a line to the log */
   /* indicating that chat mode has been exited.                       */
   if(pfLogWrite != NULL)
   {
      (*pfLogWrite)(10);
   }

   /* Restore original display color attribute. */
   od_set_attrib(nChatOriginalAttrib);

   /* Record that chat mode is no longer active. */
   od_control.od_chat_active = FALSE;

}
