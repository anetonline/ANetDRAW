/* OpenDoors Online Software Programming Toolkit
 * (C) Copyright 1991 - 1999 by Brian Pirie.
 *
 * Oct-2001 door32.sys/socket modifications by Rob Swindell (www.synchro.net)
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
 *        File: ODInEx2.c
 *
 * Description: Performs OpenDoors initialization and shutdown operations
 *              (od_init() and od_exit()), including drop file I/O. This
 *              module is broken into two files, ODInEx1.c and ODInEx2.c.
 *
 *   Revisions: Date          Ver   Who  Change
 *              ---------------------------------------------------------------
 *              Oct 13, 1994  6.00  BP   New file header format.
 *              Oct 21, 1994  6.00  BP   Further isolated com routines.
 *              Oct 29, 1994  6.00  BP   New EXITINFO.BBS timelimit writing.
 *              Nov 01, 1994  6.00  BP   New directory access functions.
 *              Dec 09, 1994  6.00  BP   Standardized coding style.
 *              Dec 13, 1994  6.00  BP   Remove include of dir.h.
 *              Dec 31, 1994  6.00  BP   Add DIR_ATTRIB_ARCH in file search.
 *              Dec 31, 1994  6.00  BP   Move _mt_init to new func in odplat.c
 *              Jan 01, 1995  6.00  BP   _waitdrain() -> ODWaitDrain().
 *              Aug 19, 1995  6.00  BP   32-bit portability.
 *              Nov 11, 1995  6.00  BP   Removed register keyword.
 *              Nov 14, 1995  6.00  BP   Added include of odscrn.h.
 *              Nov 16, 1995  6.00  BP   Removed oddoor.h, added odcore.h.
 *              Nov 17, 1995  6.00  BP   Use new input queue mechanism.
 *              Dec 30, 1995  6.00  BP   Added ODCALL for calling convention.
 *              Jan 01, 1996  6.00  BP   Added od_disable_dtr, DIS_DTR_DISABLE.
 *              Jan 04, 1996  6.00  BP   tODInQueueEvent -> tODInputEvent.
 *              Jan 19, 1996  6.00  BP   Don't use atexit() under Win32.
 *              Jan 21, 1996  6.00  BP   Try DTR disable sequence twice.
 *              Jan 21, 1996  6.00  BP   Use ODScrnShowMessage().
 *              Jan 23, 1996  6.00  BP   Added od_exiting.
 *              Jan 23, 1996  6.00  BP   Use ODProcessExit() instead of exit().
 *              Jan 30, 1996  6.00  BP   Replaced od_yield() with od_sleep().
 *              Jan 30, 1996  6.00  BP   Add ODInQueueGetNextEvent() timeout.
 *              Jan 31, 1996  6.00  BP   Support new SFDOORS.DAT format.
 *              Feb 02, 1996  6.00  BP   Added RA 2.50 EXITINFO.BBS support.
 *              Feb 09, 1996  6.00  BP   Correctly translate RA 2.x sex field.
 *              Feb 19, 1996  6.00  BP   Changed version number to 6.00.
 *              Feb 23, 1996  6.00  BP   Make DTR disable code shared.
 *              Mar 03, 1996  6.10  BP   Begin version 6.10.
 *              Mar 06, 1996  6.10  BP   Added TRIBBS.SYS support.
 *              Mar 27, 1996  6.10  BP   Added WCNODEID to
 *              Jan 13, 1997  6.10  BP   Fixes for Door32 support.
 *              Oct 19, 2001  6.20  RS   Added TCP/IP socket (telnet) support.
 *              Aug 10, 2003  6.23  SH   *nix support
 */

#define BUILDING_OPENDOORS

#include <errno.h>
#ifdef HAS_INTTYPES_H
#include <inttypes.h>
typedef uint32_t tODPrintUInt32;
#else
#define PRIu32 "lu"
typedef unsigned long tODPrintUInt32;
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "OpenDoor.h"
#include "ODStr.h"
#include "ODCore.h"
#include "ODGen.h"
#include "ODCom.h"
#include "ODPlat.h"
#include "ODTypes.h"
#include "ODScrn.h"
#include "ODVScrn.h"
#include "ODInQue.h"
#include "ODKrnl.h"
#include "ODInEx.h"
#include "ODRsv.h"
#include "ODUtil.h"
#ifdef ODPLAT_WIN32
#include "ODFrame.h"
#endif


/* Time difference leeway for door information files to be considered to */
/* have been written during the same exit (door execution session).      */
#define DROPFILE_TIME_LEEWAY 10

/* Maximum length of modem response string. */
#define MAX_RESPONSE_LEN 40

/* Maximum time to wait for modem response string, in milliseconds. */
#define RESPONSE_TIMEOUT 2000


/* Environment variables that specify directories where drop files may be */
/* found.                                                                 */
static char *apszEnvVarNames[] =
{
   "RA",
   "QUICK",
   "PCB",
   "BBS",
   "WCNODEID",
   "SBBSNODE",
};
#define NUM_DIR_ENV_VARS DIM(apszEnvVarNames)


/* Local helper functions. */
static INT ODSearchInDir(char **papszFileNames, INT nNumFileNames,
   char *pszFound, char *pszDirectory);

typedef struct tODDropFileWriter
{
   FILE *pFile;
   INT nErrorCode;
   int nRuntimeError;
} tODDropFileWriter;

BOOL ODCALL ODDropFileOpen(tODDropFileWriter *pWriter,
   const char *pszPath, const char *pszMode);
BOOL ODCALL ODDropFileWrite(tODDropFileWriter *pWriter,
   const void *pData, size_t nSize);
void ODVCALL ODTextDropFileWrite(tODDropFileWriter *pWriter,
   const char *pszFormat, ...);
BOOL ODCALL ODDropFileClose(tODDropFileWriter *pWriter);

static void ODDropFileRecordWriteFailure(tODDropFileWriter *pWriter);


/* Converts a valid, nondecreasing pair of timestamps to elapsed minutes. */
BOOL ODCALL ODGetElapsedMinutes(DWORD *pdwMinutes, time_t nStartTime,
   time_t nEndTime)
{
   if(pdwMinutes == NULL || nStartTime == (time_t)-1
      || nEndTime == (time_t)-1 || nEndTime < nStartTime)
   {
      return(FALSE);
   }

   return(ODDWordDivide(pdwMinutes, NULL, nEndTime - nStartTime, 60L));
}


/* Opens a drop file and records a caller-visible failure without using any
 * additional output buffer. */
BOOL ODCALL ODDropFileOpen(tODDropFileWriter *pWriter,
   const char *pszPath, const char *pszMode)
{
   pWriter->pFile = fopen(pszPath, pszMode);
   pWriter->nErrorCode = ERR_NONE;
   pWriter->nRuntimeError = 0;
   if(pWriter->pFile == NULL)
   {
      pWriter->nErrorCode = ERR_FILEOPEN;
      pWriter->nRuntimeError = errno;
      od_control.od_error = ERR_FILEOPEN;
      return(FALSE);
   }
   return(TRUE);
}


/* Retains the first output failure for a drop-file stream. */
static void ODDropFileRecordWriteFailure(tODDropFileWriter *pWriter)
{
   if(pWriter->nErrorCode == ERR_NONE)
   {
      pWriter->nErrorCode = ERR_GENERALFAILURE;
      pWriter->nRuntimeError = errno;
   }
}


/* Writes one binary block, suppressing output after the first failure. */
BOOL ODCALL ODDropFileWrite(tODDropFileWriter *pWriter,
   const void *pData, size_t nSize)
{
   if(pWriter->nErrorCode != ERR_NONE)
   {
      return(FALSE);
   }

   if(fwrite(pData, 1, nSize, pWriter->pFile) != nSize)
   {
      ODDropFileRecordWriteFailure(pWriter);
      return(FALSE);
   }
   return(TRUE);
}


/* Writes one field or line, retaining the first failure and suppressing all
 * later output for the same file. */
void ODVCALL ODTextDropFileWrite(tODDropFileWriter *pWriter,
   const char *pszFormat, ...)
{
   va_list ArgumentList;

   if(pWriter->nErrorCode != ERR_NONE)
   {
      return;
   }

   va_start(ArgumentList, pszFormat);
   if(vfprintf(pWriter->pFile, pszFormat, ArgumentList) < 0)
   {
      ODDropFileRecordWriteFailure(pWriter);
   }
   va_end(ArgumentList);
}


/* Commits and closes a drop file, reporting the first write or close
 * failure after ensuring that the stream is no longer owned. */
BOOL ODCALL ODDropFileClose(tODDropFileWriter *pWriter)
{
   if(pWriter->pFile == NULL)
   {
      if(pWriter->nErrorCode != ERR_NONE)
      {
         od_control.od_error = pWriter->nErrorCode;
         errno = pWriter->nRuntimeError;
      }
      return(FALSE);
   }

   if(fclose(pWriter->pFile) != 0 && pWriter->nErrorCode == ERR_NONE)
   {
      pWriter->nErrorCode = ERR_GENERALFAILURE;
      pWriter->nRuntimeError = errno;
   }
   pWriter->pFile = NULL;

   if(pWriter->nErrorCode != ERR_NONE)
   {
      od_control.od_error = pWriter->nErrorCode;
      errno = pWriter->nRuntimeError;
      return(FALSE);
   }
   return(TRUE);
}

/* Currently, the following functions are only used in the Win32 version. */
#ifdef ODPLAT_WIN32
static BOOL ODSendModemCommand(char *pszCommand, int nRetries);
static BOOL ODSendModemCommandOnce(char *pszCommand);
static BOOL ODWaitForString(char *pszResponse, tODMilliSec ResponseTimeout);
static void ODAPISleep(tODMilliSec Milliseconds);
#if defined(OD_DIAGNOSTICS) && defined(ODPLAT_WIN32)
static void ODDiagnosticMessage(const char *pszText, const char *pszTitle);
#endif
#endif /* ODPLAT_WIN32 */

#ifdef OD_DIAGNOSTICS
static char szDebugWorkString[500] = "";
#endif /* OD_DIAGNOSTICS */



/* ----------------------------------------------------------------------------
 * od_exit()
 *
 * Shuts down OpenDoors operations. Normally, the program is exited as soon
 * as OpenDoors is shutdown.
 *
 * Parameters: nErrorLevel - Result code to exit program with.
 *
 *             bTermCall   - TRUE to disconnect the user before exiting,
 *                           FALSE to leave the user connected.
 *
 *     Return: void
 */
ODAPIDEF void ODCALL od_exit(INT nErrorLevel, BOOL bTermCall)
{
   BYTE btCount;
   tODDropFileWriter TextDropFile;
   tODDropFileWriter BinaryDropFile;
   time_t nMaxTime;
   time_t nDoorEndTime;
   void *pWindow = NULL;
   DWORD dwActiveMinutes;
   static BOOL bExiting = FALSE;
   BOOL bNestedAPI;
   BOOL bReturnToCaller;
#ifdef OD_THREAD_SUPPORT
   unsigned nSavedAPILevel;
#endif

   /* Log function entry if running in trace mode */
   TRACE(TRACE_API, "od_exit()");

#if defined(OD_DIAGNOSTICS) && defined(ODPLAT_WIN32)
   if(od_control.od_internal_debug)
   {
      ODDiagnosticMessage("Starting up od_exit()", "OpenDoors Diagnostics");
   }
#endif

   /* The first exit request wins. */
   if(eODLifecycleState == kODLifecycleInitializing)
   {
      if(!bODExitRequestedDuringInitialization)
      {
         nODPendingExitErrorLevel = nErrorLevel;
         bODPendingExitTermCall = bTermCall;
         bODExitRequestedDuringInitialization = TRUE;
      }
      return;
   }
   if(eODLifecycleState == kODLifecycleExitPending
      || eODLifecycleState == kODLifecycleTerminal)
   {
      od_control.od_error = ERR_GENERALFAILURE;
      return;
   }

   /* If this is a recursive od_exit() call, then ignore it. */
   if(bExiting)
   {
      return;
   }
   bExiting = TRUE;

   /* If user called od_exit() before doing anything else, then we first */
   /* initialize OpenDoors in order to shutdown and exit.                */
   if(!bODInitialized)
   {
      nODPendingExitErrorLevel = nErrorLevel;
      bODPendingExitTermCall = bTermCall;
      bODExitRequestedDuringInitialization = TRUE;
      od_init();
   }
   if(!bODInitialized || eODLifecycleState == kODLifecycleTerminal)
   {
      bExiting = FALSE;
      return;
   }
   bNestedAPI = ODSyncAPIIsNested();
   OD_API_ENTRY();

   if(!bODExitPrologueComplete)
   {
      /* Update remaining time. */
      od_control.user_timelimit += od_control.od_maxtime_deduction;

   /* Calculate deducted time */
   nDoorEndTime = time(NULL);
   if(ODGetElapsedMinutes(&dwActiveMinutes, nStartupUnixTime, nDoorEndTime))
   {
      od_control.user_time_used += ((nInitialRemaining
         - od_control.user_timelimit) - (int)dwActiveMinutes);
   }

   /* Reset to original bps rate that was stored in drop file */
      od_control.baud = dwFileBPS;

   /* If function hook is defined. */
      if(od_control.od_before_exit != NULL)
      {
         /* Then call it. */
         (*od_control.od_before_exit)();
      }

      nODPendingExitErrorLevel = nErrorLevel;
      bODPendingExitTermCall = bTermCall;
      bODPendingExitNoExit = od_control.od_noexit;
      bODExitPrologueComplete = TRUE;

      /* In no-exit mode an exit raised inside another public call is
       * finalized by that call's outermost API-exit boundary. */
      if(od_control.od_noexit && bNestedAPI)
      {
         eODLifecycleState = kODLifecycleExitPending;
#ifdef ODPLAT_WIN32
         if(ODPlatGetWindowsSubsystem() == kODWindowsSubsystemGUI)
            ODFrameRequestShutdown(hFrameThread);
#endif
         bExiting = FALSE;
         OD_API_EXIT();
         return;
      }
   }

   bReturnToCaller = bODPendingExitNoExit;
   eODLifecycleState = kODLifecycleFinalizing;

   /* The before-exit hook may need the reservation. Release it before the
    * remainder of shutdown can block or terminate the process. */
   ODReserveSessionShutdown();

   if(bTermCall && od_control.od_hanging_up != NULL)
   {
      pWindow = ODScrnShowMessage(od_control.od_hanging_up, 0);
   }
   else if(!bTermCall)
   {
      pWindow = ODScrnShowMessage(od_control.od_exiting, 0);
   }

   if(szOriginalDir != NULL)
   {
      ODDirChangeCurrent(szOriginalDir);
      free(szOriginalDir);
      szOriginalDir=NULL;
   }

   if(od_control.od_extended_info)     /* Update EXITINFO.BBS, if applicable */
   {
      ODMakeFilename(szExitinfoBBSPath, szExitinfoBBSPath, "EXITINFO.BBS",
         sizeof(szExitinfoBBSPath));
      if(ODDropFileOpen(&BinaryDropFile, szExitinfoBBSPath, "r+b"))
      {
         switch(od_control.od_info_type)
         {
            case RA2EXITINFO:
               pRA2ExitInfoRecord->baud = (unsigned int)od_control.baud;
               pRA2ExitInfoRecord->num_calls = od_control.system_calls;
               ODStringCToPascal(pRA2ExitInfoRecord->last_caller,35,od_control.system_last_caller);
               ODStringCToPascal(pRA2ExitInfoRecord->sLastHandle,35,od_control.system_last_handle);
               ODStringCToPascal(pRA2ExitInfoRecord->start_date,8,od_control.timelog_start_date);
               memcpy(pRA2ExitInfoRecord->busyperhour,
                  od_control.timelog_busyperhour,
                  sizeof(pRA2ExitInfoRecord->busyperhour));
               memcpy(pRA2ExitInfoRecord->busyperday,
                  od_control.timelog_busyperday,
                  sizeof(pRA2ExitInfoRecord->busyperday));
               ODStringCToPascal(pRA2ExitInfoRecord->name,35,od_control.user_name);
               ODStringCToPascal(pRA2ExitInfoRecord->location,25,od_control.user_location);
               ODStringCToPascal(pRA2ExitInfoRecord->organisation,50,od_control.user_org);
               for(btCount=0;btCount<3;++btCount)
                  ODStringCToPascal(pRA2ExitInfoRecord->address[btCount],50,od_control.user_address[btCount]);
               ODStringCToPascal(pRA2ExitInfoRecord->handle,35,od_control.user_handle);
               ODStringCToPascal(pRA2ExitInfoRecord->comment,80,od_control.user_comment);
               pRA2ExitInfoRecord->password_crc=od_control.user_pwd_crc;
               ODStringCToPascal(pRA2ExitInfoRecord->dataphone,15,od_control.user_dataphone);
               ODStringCToPascal(pRA2ExitInfoRecord->homephone,15,od_control.user_homephone);
               ODStringCToPascal(pRA2ExitInfoRecord->lasttime,5,od_control.user_lasttime);
               ODStringCToPascal(pRA2ExitInfoRecord->lastdate,8,od_control.user_lastdate);
               pRA2ExitInfoRecord->attrib=od_control.user_attribute;
               pRA2ExitInfoRecord->attrib2=od_control.user_attrib2;
               memcpy(&pRA2ExitInfoRecord->flags,&od_control.user_flags,14);
               pRA2ExitInfoRecord->sec=od_control.user_security;
               pRA2ExitInfoRecord->lastread=od_control.user_lastread;
               memcpy(&pRA2ExitInfoRecord->nocalls,&od_control.user_numcalls,29);
               pRA2ExitInfoRecord->group=od_control.user_group;
               memcpy(&pRA2ExitInfoRecord->combinedrecord,&od_control.user_combinedrecord,200);
               ODStringCToPascal(pRA2ExitInfoRecord->firstcall,8,od_control.user_firstcall);
               ODStringCToPascal(pRA2ExitInfoRecord->birthday,8,od_control.user_birthday);
               ODStringCToPascal(pRA2ExitInfoRecord->subdate,8,od_control.user_subdate);
               pRA2ExitInfoRecord->screenwidth=od_control.user_screenwidth;
               pRA2ExitInfoRecord->language=od_control.user_language;
               pRA2ExitInfoRecord->dateformat=od_control.user_date_format;
               ODStringCToPascal(pRA2ExitInfoRecord->forwardto,35,od_control.user_forward_to);
               memcpy(&pRA2ExitInfoRecord->msgarea,&od_control.user_msg_area,15);
               pRA2ExitInfoRecord->sex = (od_control.user_sex == 'M') ? 1 : 2;
               pRA2ExitInfoRecord->btAttribute3=od_control.user_attrib3;
               ODStringCToPascal(pRA2ExitInfoRecord->sPassword,15,od_control.user_password);
               pRA2ExitInfoRecord->status=od_control.event_status;
               ODStringCToPascal(pRA2ExitInfoRecord->starttime,5,od_control.event_starttime);
               memcpy(&pRA2ExitInfoRecord->errorlevel,&od_control.event_errorlevel,3);
               ODStringCToPascal(pRA2ExitInfoRecord->lasttimerun,8,od_control.event_last_run);
               memcpy(&pRA2ExitInfoRecord->netmailentered,&od_control.user_netmailentered,2);
               ODStringCToPascal(pRA2ExitInfoRecord->logintime,5,od_control.user_logintime);
               ODStringCToPascal(pRA2ExitInfoRecord->logindate,8,od_control.user_logindate);
               memcpy(&pRA2ExitInfoRecord->timelimit,&od_control.user_timelimit,6);
               memcpy(&pRA2ExitInfoRecord->userrecord,&od_control.user_num,8);
               ODStringCToPascal(pRA2ExitInfoRecord->timeofcreation,5,od_control.user_timeofcreation);
               pRA2ExitInfoRecord->logonpasswordcrc=od_control.user_logon_pwd_crc;
               pRA2ExitInfoRecord->wantchat=od_control.user_wantchat;
               pRA2ExitInfoRecord->deducted_time=od_control.user_deducted_time;
               for(btCount=0;btCount<50;++btCount)
                  ODStringCToPascal(pRA2ExitInfoRecord->menustack[btCount],8,od_control.user_menustack[btCount]);
               pRA2ExitInfoRecord->menustackpointer=od_control.user_menustackpointer;
               memcpy(&pRA2ExitInfoRecord->error_free,&od_control.user_error_free,3);
               ODStringCToPascal(pRA2ExitInfoRecord->emsi_crtdef,40,od_control.user_emsi_crtdef);
               ODStringCToPascal(pRA2ExitInfoRecord->emsi_protocols,40,od_control.user_emsi_protocols);
               ODStringCToPascal(pRA2ExitInfoRecord->emsi_capabilities,40,od_control.user_emsi_capabilities);
               ODStringCToPascal(pRA2ExitInfoRecord->emsi_requests,40,od_control.user_emsi_requests);
               ODStringCToPascal(pRA2ExitInfoRecord->emsi_software,40,od_control.user_emsi_software);
               memcpy(&pRA2ExitInfoRecord->hold_attr1,&od_control.user_hold_attr1,3);
               ODStringCToPascal(pRA2ExitInfoRecord->page_reason,77,od_control.user_reasonforchat);
               if(bRAStatus)
               {
                  pRA2ExitInfoRecord->status_line = btCurrentStatusLine + 1;
               }

               ODStringCToPascal(pRA2ExitInfoRecord->last_cost_menu,9,od_control.user_last_cost_menu);
               pRA2ExitInfoRecord->menu_cost_per_min=od_control.user_menu_cost;
               pRA2ExitInfoRecord->has_rip=od_control.user_rip;
               pRA2ExitInfoRecord->btRIPVersion=od_control.user_rip_ver;

               ODExitInfoRA2Endian(pRA2ExitInfoRecord, FALSE);
               (void)ODDropFileWrite(&BinaryDropFile, pRA2ExitInfoRecord,
                  sizeof(tRA2ExitInfoRecord));
               break;

            case EXITINFO:
               ODStringCToPascal(pExitInfoRecord->bbs.ra.timeofcreation,5,od_control.user_timeofcreation);
               ODStringCToPascal(pExitInfoRecord->bbs.ra.logonpassword,15,od_control.user_logonpassword);
               pExitInfoRecord->bbs.ra.wantchat=od_control.user_wantchat;

               if(!ODWriteExitInfoPrimitive(BinaryDropFile.pFile,476))
               {
                  ODDropFileRecordWriteFailure(&BinaryDropFile);
               }
               break;


            case RA1EXITINFO:
               pExtendedExitInfo->deducted_time=od_control.user_deducted_time;

               for(btCount=0;btCount<50;++btCount)
               {
                  ODStringCToPascal(pExtendedExitInfo->menustack[btCount],8,od_control.user_menustack[btCount]);
               }

               pExtendedExitInfo->menustackpointer=od_control.user_menustackpointer;
               ODStringCToPascal(pExtendedExitInfo->userhandle,35,od_control.user_handle);
               ODStringCToPascal(pExtendedExitInfo->comment,80,od_control.user_comment);
               ODStringCToPascal(pExtendedExitInfo->firstcall,8,od_control.user_firstcall);
               memcpy(pExtendedExitInfo->combinedrecord,od_control.user_combinedrecord,25);
               ODStringCToPascal(pExtendedExitInfo->birthday,8,od_control.user_birthday);
               ODStringCToPascal(pExtendedExitInfo->subdate,8,od_control.user_subdate);
               pExtendedExitInfo->screenwidth=od_control.user_screenwidth;
               pExtendedExitInfo->msgarea = (BYTE)od_control.user_msg_area;
               pExtendedExitInfo->filearea = (BYTE)od_control.user_file_area;
               pExtendedExitInfo->language=od_control.user_language;
               pExtendedExitInfo->dateformat=od_control.user_date_format;
               ODStringCToPascal(pExtendedExitInfo->forwardto,35,od_control.user_forward_to);
               memcpy(&pExtendedExitInfo->error_free,&od_control.user_error_free,3);
               ODStringCToPascal(pExtendedExitInfo->emsi_crtdef,40,od_control.user_emsi_crtdef);
               ODStringCToPascal(pExtendedExitInfo->emsi_protocols,40,od_control.user_emsi_protocols);
               ODStringCToPascal(pExtendedExitInfo->emsi_capabilities,40,od_control.user_emsi_capabilities);
               ODStringCToPascal(pExtendedExitInfo->emsi_requests,40,od_control.user_emsi_requests);
               ODStringCToPascal(pExtendedExitInfo->emsi_software,40,od_control.user_emsi_software);
               memcpy(&pExtendedExitInfo->hold_attr1,&od_control.user_hold_attr1,3);

               ODStringCToPascal(pExitInfoRecord->bbs.ra.timeofcreation,5,od_control.user_timeofcreation);
               ODStringCToPascal(pExitInfoRecord->bbs.ra.logonpassword,15,od_control.user_logonpassword);
               pExitInfoRecord->bbs.ra.wantchat=od_control.user_wantchat;

               if(!ODWriteExitInfoPrimitive(BinaryDropFile.pFile,476))
               {
                  ODDropFileRecordWriteFailure(&BinaryDropFile);
               }
               ODExitInfoExtendedEndian(pExtendedExitInfo, FALSE);
               (void)ODDropFileWrite(&BinaryDropFile, pExtendedExitInfo, 1017);
               break;


            case QBBS275EXITINFO:
               pExitInfoRecord->elapsed=nInitialElapsed;
               pExitInfoRecord->bbs.qbbs.qwantchat=od_control.user_wantchat;
               pExitInfoRecord->bbs.qbbs.gosublevel=od_control.user_menustackpointer;
               for(btCount=0;btCount<pExitInfoRecord->bbs.qbbs.gosublevel;++btCount)
               {
                  ODStringCToPascal(pExitInfoRecord->bbs.qbbs.menustack[btCount],8,od_control.user_menustack[btCount]);
               }
               ODStringCToPascal(pExitInfoRecord->bbs.qbbs.menu,8,od_control.user_menustack[od_control.user_menustackpointer]);
               pExitInfoRecord->bbs.qbbs.externlogoff = bTermCall ? 1 : 0;
               pExitInfoRecord->bbs.qbbs.ripactive = od_control.user_rip ? 1 : 0;

               if(!ODWriteExitInfoPrimitive(BinaryDropFile.pFile,644))
               {
                  ODDropFileRecordWriteFailure(&BinaryDropFile);
               }
         }

         (void)ODDropFileClose(&BinaryDropFile);
      }

      free(pRA2ExitInfoRecord);
      pRA2ExitInfoRecord = NULL;
      free(pExitInfoRecord);
      pExitInfoRecord = NULL;
      free(pExtendedExitInfo);
      pExtendedExitInfo = NULL;
   }


   switch(od_control.od_info_type)
   {
      case DOORSYS_GAP:
      case DOORSYS_WILDCAT:
         if(!ODDropFileOpen(&TextDropFile, szDropFilePath, "w"))
         {
            break;
         }
         if(od_control.baud==0L)
         {
            ODTextDropFileWrite(&TextDropFile,"COM0:\n");
         }
         else
         {
            ODTextDropFileWrite(&TextDropFile,"COM%d:\n",od_control.port+1);
         }
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[0]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[1]);
         ODTextDropFileWrite(&TextDropFile,"%u\n",od_control.od_node);
         switch(btDoorSYSLock)
         {
            case 0:
               ODTextDropFileWrite(&TextDropFile,"%" PRIu32 "\n",(tODPrintUInt32)od_control.baud);
               break;
            case 1:
               ODTextDropFileWrite(&TextDropFile,"N\n");
               break;
            case 2:
               ODTextDropFileWrite(&TextDropFile,"Y\n");
         }
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[3]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[4]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[5]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[22]);
         strupr(od_control.user_name);
         ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_name);
         ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_location);
         ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_homephone);
         ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_dataphone);
         ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_password);
         ODTextDropFileWrite(&TextDropFile,"%u\n",od_control.user_security);
         ODTextDropFileWrite(&TextDropFile,"%" PRIu32 "\n",(tODPrintUInt32)od_control.user_numcalls);
         ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_lastdate);
         ODTextDropFileWrite(&TextDropFile,"%u\n",(signed int)od_control.user_timelimit*60);
         ODTextDropFileWrite(&TextDropFile,"%d\n",od_control.user_timelimit);
         if(od_control.user_rip)
         {
            ODTextDropFileWrite(&TextDropFile,"RIP\n");
         }
         else if(od_control.user_ansi)
         {
            ODTextDropFileWrite(&TextDropFile,"GR\n");
         }
         else
         {
            ODTextDropFileWrite(&TextDropFile,"NG\n");
         }
         ODTextDropFileWrite(&TextDropFile,"%d\n",od_control.user_screen_length);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[8]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[9]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[10]);
         ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_subdate);
         ODTextDropFileWrite(&TextDropFile,"%u\n",od_control.user_num);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[6]);
         ODTextDropFileWrite(&TextDropFile,"%" PRIu32 "\n",(tODPrintUInt32)od_control.user_uploads);
         ODTextDropFileWrite(&TextDropFile,"%" PRIu32 "\n",(tODPrintUInt32)od_control.user_downloads);
         ODTextDropFileWrite(&TextDropFile,"%" PRIu32 "\n",(tODPrintUInt32)od_control.user_todayk);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[21]);


         if(od_control.od_info_type==DOORSYS_WILDCAT)
         {
            ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_birthday);
            ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[11]);
            ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[12]);
            ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.sysop_name);
            strupr(od_control.user_handle);
            ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_handle);
            ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.event_starttime);
            if(od_control.user_error_free)
               ODTextDropFileWrite(&TextDropFile,"Y\n");
            else
               ODTextDropFileWrite(&TextDropFile,"N\n");
            ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[7]);
            ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[13]);
            ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[14]);
            ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[15]);
            ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[16]);
            ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_logintime);
            ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_lasttime);
            ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[18]);
            ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[19]);
            ODTextDropFileWrite(&TextDropFile,"%" PRIu32 "\n",(tODPrintUInt32)od_control.user_upk);
            ODTextDropFileWrite(&TextDropFile,"%" PRIu32 "\n",(tODPrintUInt32)od_control.user_downk);
            ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_comment);
            ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[20]);
            ODTextDropFileWrite(&TextDropFile,"%u\n",od_control.user_messages);
         }

         (void)ODDropFileClose(&TextDropFile);
         break;


      case DOORSYS_DRWY:
         if(!ODDropFileOpen(&TextDropFile, szDropFilePath, "w"))
         {
            break;
         }
         ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_name);

         if(od_control.baud==0L)
         {
            ODTextDropFileWrite(&TextDropFile,"-1\n");
         }
         else
         {
            ODTextDropFileWrite(&TextDropFile,"%d\n",od_control.port+1);
         }

         ODTextDropFileWrite(&TextDropFile,"%" PRIu32 "\n",(tODPrintUInt32)od_control.baud);

         ODTextDropFileWrite(&TextDropFile,"%d\n",od_control.user_timelimit);

         if(od_control.user_ansi)
         {
            ODTextDropFileWrite(&TextDropFile,"G\n");
         }
         else
         {
            ODTextDropFileWrite(&TextDropFile,"M\n");
         }

         (void)ODDropFileClose(&TextDropFile);
         break;


      case SFDOORSDAT:
         if(!ODDropFileOpen(&TextDropFile, szDropFilePath, "w"))
         {
            break;
         }

         ODTextDropFileWrite(&TextDropFile,"%u\n",od_control.user_num);
         ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_name);
         ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_password);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[0]);
         ODTextDropFileWrite(&TextDropFile,"%" PRIu32 "\n",(tODPrintUInt32)od_control.baud);
         ODTextDropFileWrite(&TextDropFile,"%d\n",od_control.port+1);
         ODTextDropFileWrite(&TextDropFile,"%d\n",od_control.user_timelimit);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[13]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[14]);
         if(od_control.user_ansi)
         {
            ODTextDropFileWrite(&TextDropFile,"TRUE\n");
         }
         else
         {
            ODTextDropFileWrite(&TextDropFile,"FALSE\n");
         }
         ODTextDropFileWrite(&TextDropFile,"%u\n",od_control.user_security);
         ODTextDropFileWrite(&TextDropFile,"%" PRIu32 "\n",(tODPrintUInt32)od_control.user_uploads);
         ODTextDropFileWrite(&TextDropFile,"%" PRIu32 "\n",(tODPrintUInt32)od_control.user_downloads);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[1]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[2]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[3]);
         if(od_control.sysop_next)
         {
            ODTextDropFileWrite(&TextDropFile,"TRUE\n");
         }
         else
         {
            ODTextDropFileWrite(&TextDropFile,"FALSE\n");
         }
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[4]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[5]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[6]);
         if(od_control.user_error_free)
         {
            ODTextDropFileWrite(&TextDropFile,"TRUE\n");
         }
         else
         {
            ODTextDropFileWrite(&TextDropFile,"FALSE\n");
         }

         ODTextDropFileWrite(&TextDropFile,"%u\n",od_control.user_msg_area);
         ODTextDropFileWrite(&TextDropFile,"%u\n",od_control.user_file_area);
         ODTextDropFileWrite(&TextDropFile,"%u\n",od_control.od_node);

         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[10]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[11]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[12]);
         ODTextDropFileWrite(&TextDropFile,"%" PRIu32 "\n",(tODPrintUInt32)od_control.user_todayk);
         ODTextDropFileWrite(&TextDropFile,"%" PRIu32 "\n",(tODPrintUInt32)od_control.user_upk);
         ODTextDropFileWrite(&TextDropFile,"%" PRIu32 "\n",(tODPrintUInt32)od_control.user_downk);
         ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_homephone);
         ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_location);
         if(apszDropFileInfo[15][0]!='\0')
         {
            ODTextDropFileWrite(&TextDropFile, "%s", apszDropFileInfo[15]);
            ODTextDropFileWrite(&TextDropFile, od_control.user_rip ? "TRUE\n" : "FALSE\n");
            ODTextDropFileWrite(&TextDropFile, od_control.user_wantchat ? "TRUE\n"
               : "FALSE\n");
            ODTextDropFileWrite(&TextDropFile, "%s", apszDropFileInfo[17]);
            ODTextDropFileWrite(&TextDropFile, "%d\n", od_control.od_com_irq);
            ODTextDropFileWrite(&TextDropFile, "%d\n", od_control.od_com_address);
            ODTextDropFileWrite(&TextDropFile, "%s", apszDropFileInfo[18]);
         }
         (void)ODDropFileClose(&TextDropFile);
         break;


      case CHAINTXT:
         if(!ODDropFileOpen(&TextDropFile, szDropFilePath, "w"))
         {
            break;
         }
         ODTextDropFileWrite(&TextDropFile,"%d\n",od_control.user_num);
         ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_handle);
         ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_name);
         ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_callsign);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[0]);
         ODTextDropFileWrite(&TextDropFile,"%c\n",od_control.user_sex);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[1]);
         ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_lastdate);
         ODTextDropFileWrite(&TextDropFile,"%d\n",od_control.user_screenwidth);
         ODTextDropFileWrite(&TextDropFile,"%d\n",od_control.user_screen_length);
         ODTextDropFileWrite(&TextDropFile,"%d\n",od_control.user_security);
         ODTextDropFileWrite(&TextDropFile,"%d\n",bIsSysop);
         ODTextDropFileWrite(&TextDropFile,"%d\n",bIsCoSysop);
         ODTextDropFileWrite(&TextDropFile,"%d\n",od_control.user_ansi);
         if(od_control.baud==0L)
         {
            ODTextDropFileWrite(&TextDropFile,"0\n");
         }
         else
         {
            ODTextDropFileWrite(&TextDropFile,"1\n");
         }
         ODTextDropFileWrite(&TextDropFile,"    %d.00\n",od_control.user_timelimit*60);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[3]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[4]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[5]);
         if(od_control.baud==0L)
         {
            ODTextDropFileWrite(&TextDropFile,"KB\n");
         }
         else
         {
            ODTextDropFileWrite(&TextDropFile,"%" PRIu32 "\n",(tODPrintUInt32)od_control.baud);
         }
         ODTextDropFileWrite(&TextDropFile,"%d\n",od_control.port+1);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[6]);
         ODTextDropFileWrite(&TextDropFile,"%s\n",od_control.user_password);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[2]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[7]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[8]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[9]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[10]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[11]);
         ODTextDropFileWrite(&TextDropFile,"%s",apszDropFileInfo[12]);
         (void)ODDropFileClose(&TextDropFile);
         break;

      case TRIBBSSYS:
         if(!ODDropFileOpen(&TextDropFile, szDropFilePath, "w"))
         {
            break;
         }
         ODTextDropFileWrite(&TextDropFile, "%u\n", od_control.user_num);
         ODTextDropFileWrite(&TextDropFile, "%s\n", od_control.user_name);
         ODTextDropFileWrite(&TextDropFile, "%s\n", od_control.user_password);
         ODTextDropFileWrite(&TextDropFile, "%u\n", od_control.user_security);
         ODTextDropFileWrite(&TextDropFile, "%c\n", od_control.user_expert ? 'Y' : 'N');
         ODTextDropFileWrite(&TextDropFile, "%c\n", od_control.user_ansi ? 'Y' : 'N');
         ODTextDropFileWrite(&TextDropFile, "%d\n", od_control.user_timelimit);
         ODTextDropFileWrite(&TextDropFile, "%s\n", od_control.user_homephone);
         ODTextDropFileWrite(&TextDropFile, "%s\n", od_control.user_location);
         od_control.user_birthday[2] = '/';
         od_control.user_birthday[5] = '/';
         ODTextDropFileWrite(&TextDropFile, "%s\n", od_control.user_birthday);
         ODTextDropFileWrite(&TextDropFile, "%d\n", od_control.od_node);
         ODTextDropFileWrite(&TextDropFile, "%d\n", od_control.port + 1);
         ODTextDropFileWrite(&TextDropFile, "%" PRIu32 "\n", (tODPrintUInt32)od_control.od_connect_speed);
         ODTextDropFileWrite(&TextDropFile, "%" PRIu32 "\n", (tODPrintUInt32)od_control.baud);
         ODTextDropFileWrite(&TextDropFile, "%c\n", (od_control.od_com_flow_control
            == COM_RTSCTS_FLOW) ? 'Y' : 'N');
         ODTextDropFileWrite(&TextDropFile, "%c\n", od_control.user_error_free ? 'Y' : 'N');
         ODTextDropFileWrite(&TextDropFile, "%s\n", od_control.system_name);
         ODTextDropFileWrite(&TextDropFile, "%s\n", od_control.sysop_name);
         ODTextDropFileWrite(&TextDropFile, "%s\n", od_control.user_handle);
         ODTextDropFileWrite(&TextDropFile, "%c\n", od_control.user_rip ? 'Y' : 'N');
         (void)ODDropFileClose(&TextDropFile);
         break;
   }

   /* Deallocate temorary strings. */
   for(btCount=0;btCount<25;++btCount)
   {
      free(apszDropFileInfo[btCount]);
   }

   /* If logfile system is active. */
   if(pfLogClose != NULL)
   {
      /* Then close the logfile. */
      (void)(*pfLogClose)(nErrorLevel);
   }

   /* Disconnect the remote user if required. */
   if(od_control.baud && bTermCall)
   {
      BOOL bCarrier;

      /* Wait up to ten seconds for bufffer to drain. */
      ODWaitDrain(10000);

      /* Wait up to five seconds for no carrier */
#ifdef OD_THREAD_SUPPORT
      nSavedAPILevel = ODSyncAPIRelease();
#endif
      ODComSetDTR(hSerialPort, FALSE);
      nMaxTime = time(NULL) + 5L;

      do
      {
         ODComCarrier(hSerialPort, &bCarrier);
#ifdef OD_THREAD_SUPPORT
         if(bCarrier) ODThreadSleep(10);
#endif
      } while(bCarrier && time(NULL) <= nMaxTime);

      /* Raise DTR signal again. */
      ODComSetDTR(hSerialPort, TRUE);
#ifdef OD_THREAD_SUPPORT
      ODSyncAPIReacquire(nSavedAPILevel);
#endif
   }

   /* In Win32 version, disable DTR before closing serial port, if */
   /* required.                                                    */
#ifdef ODPLAT_WIN32
   /* If we are operating in remote mode, and we should not hangup on the */
   /* caller ...                                                          */
   if(!bTermCall && od_control.baud)
   {
      ODInExDisableDTR();
   }
#endif /* ODPLAT_WIN32 */

   /* Remove the message that indicates we are in the process of exiting */
   /* or hanging up.                                                     */
   ODScrnRemoveMessage(pWindow);

#if !defined(ODPLAT_WIN32)
   /* Reset output area boundary to the entire screen. */
   ODScrnSetBoundary(1,1,80,25);

   /* Reset text color. */
   ODScrnSetAttribute(0x07);

   /* Clear screen if neccesary. */   
   if(od_control.od_clear_on_exit)
   {
      ODScrnClear();
   }
   else
   {
      ODScrnSetCursorPos(1, 1);
   }
#endif /* !ODPLAT_WIN32 */
#ifdef ODPLAT_WIN32
   if(ODPlatGetWindowsSubsystem() == kODWindowsSubsystemConsole)
   {
      ODScrnSetBoundary(1, 1, 80, 25);
      ODScrnSetAttribute(0x07);
      if(od_control.od_clear_on_exit)
         ODScrnClear();
      else
         ODScrnSetCursorPos(1, 1);
      ODScrnPublish();
   }
#endif

   /* Drop the API nesting level while stopping and joining the Windows UI. */
#ifdef OD_THREAD_SUPPORT
   nSavedAPILevel = ODSyncAPIRelease();
#endif

#if defined(OD_DIAGNOSTICS) && defined(ODPLAT_WIN32)
   if(od_control.od_internal_debug)
   {
      ODDiagnosticMessage("Terminating user-interface threads",
         "OpenDoors Diagnostics");
   }
#endif

#ifdef ODPLAT_WIN32
   /* Stop the only producer of deferred UI changes before destroying the
    * queue and its mutex. */
   if(ODPlatGetWindowsSubsystem() == kODWindowsSubsystemGUI)
      ODFrameShutdown(&hFrameThread);
#endif /* ODPLAT_WIN32 */

   /* Shutdown the OpenDoors kernel. */
   ODKrnlShutdown();

#if defined(OD_DIAGNOSTICS) && defined(ODPLAT_WIN32)
   if(od_control.od_internal_debug)
   {
      ODDiagnosticMessage("Shutting down local screen",
         "OpenDoors Diagnostics");
   }
#endif
   /* Shutdown OpenDoors local screen module. */
   ODSessionScreenShutdown();
   ODScrnShutdown();

#if defined(OD_DIAGNOSTICS) && defined(ODPLAT_WIN32)
   if(od_control.od_internal_debug)
   {
      ODDiagnosticMessage("Performing any final serial port deallocation",
         "OpenDoors Diagnostics");
   }
#endif
   /* If not operating in local mode, then deallocate serial port resources. */
   if(od_control.baud != 0)
   {
      /* Close serial port. */
      ODComClose(hSerialPort);

      /* Deallocate serial port object. */
      ODComFree(hSerialPort);
   }

#if defined(OD_DIAGNOSTICS) && defined(ODPLAT_WIN32)
   if(od_control.od_internal_debug)
   {
      ODDiagnosticMessage("Deallocating common queue",
         "OpenDoors Diagnostics");
   }
#endif
   /* Deallocate input buffer. */
   ODInQueueFree(hODInputQueue);

#if defined(OD_DIAGNOSTICS) && defined(ODPLAT_WIN32)
   if(od_control.od_internal_debug)
   {
      ODDiagnosticMessage("Going to inactive mode", "OpenDoors Diagnostics");
   }
#endif

#ifdef OD_THREAD_SUPPORT
   ODSyncAPIReacquire(nSavedAPILevel);
#endif

   /* OpenDoors is no longer active. */
   bODInitialized = FALSE;
   hSerialPort = NULL;
   hODInputQueue = NULL;
   eODLifecycleState = kODLifecycleTerminal;
   bODExitPrologueComplete = FALSE;

   /* od_exit() is no longer active. */
   bExiting = FALSE;

   /* If the client does not want a call to od_exit() to shutdown the */
   /* application, but just to shutdown OpenDoors, then return now.   */
   if(bReturnToCaller)
   {
      OD_API_EXIT();
      return;
   }

   /* If exit() has already been called, then do not call it again. */
   if(bPreOrExit)
   {
      OD_API_EXIT();
      return;
   }

#if defined(OD_DIAGNOSTICS) && defined(ODPLAT_WIN32)
   if(od_control.od_internal_debug)
   {
      ODDiagnosticMessage("Terminating process", "OpenDoors Diagnostics");
   }
#endif
   /* Exit with appropriate errorlevel. */
   ODProcessExit(nErrorLevel);
}


/* ----------------------------------------------------------------------------
 * ODSearchForDropFile()
 *
 * Searches for a door information (drop) file, given a list of possible drop
 * file names. Searches for the drop file first in the directory specified
 * by od_control.info_path. If a directory was specified in the configuration
 * file, this is where that directory name would be stored. This function will
 * then proceed to search the current directory and any directories specified
 * by recognized environment variables, until either a drop file is found, or
 * until all possibilities are exhaused.
 *
 * If a directory contains more than one supported dropfile, the choice of
 * drop files is narrowed to the most recently written file, and any files
 * written in the ten seconds before that file was written. Of these files,
 * the file with the highest priority (based on its position in the list of
 * possible drop file names) is selected. This heuristic attempts to ignore
 * any "old" drop files that may still be hanging around from another
 * program or another login session, while still choosing the file with the
 * most information.
 *
 * Parameters: papszFileNames - Array of possible drop file names.
 *
 *             nNumFilesNames - The number of names in papszFileNames.
 *
 *             pszFound       - If a drop file was found, this string
 *                              will be changed to point to the filename
 *                              of the file that was found.
 *
 *             pszDirectory   - If a drop file was found, this string
 *                              will be changed to contain the name of
 *                              the directory in which the file was found.
 *
 *     Return: Index in the array of the file that was found, or -1 if no
 *             potential drop file was found.
 */
INT ODSearchForDropFile(char **papszFileNames, INT nNumFileNames,
   char *pszFound, char *pszDirectory)
{
   BYTE btCount;
   char *pszEnvVarSetting;
   INT nResult;

   ASSERT(papszFileNames != NULL);
   ASSERT(nNumFileNames > 0);
   ASSERT(pszFound != NULL);

   /* First, look for the drop file(s) in the directory specified by */
   /* od_control.info_path.                                          */
   if(strlen(od_control.info_path) != 0)
   {
      if((nResult = ODSearchInDir(papszFileNames, nNumFileNames, pszFound,
         od_control.info_path)) != -1)
      {
         if(pszDirectory != NULL) strcpy(pszDirectory, od_control.info_path);
         return(nResult);
      }
   }

   /* Next, look for the drop file(s) in the current directory. */
   if((nResult = ODSearchInDir(papszFileNames, nNumFileNames, pszFound,
      "."DIRSEP_STR)) != -1)
   {
      if(pszDirectory != NULL) strcpy(pszDirectory, "."DIRSEP_STR);
      return(nResult);
   }

   /* Look through array of environment variables, checking whether any of */
   /* them specify the name of a directory in which a drop file can be     */
   /* found.                                                               */
   ASSERT(DIM(apszEnvVarNames) == NUM_DIR_ENV_VARS);
   for(btCount = 0; btCount < NUM_DIR_ENV_VARS; ++btCount)
   {
      if((pszEnvVarSetting = (char *)getenv(apszEnvVarNames[btCount])) != NULL)
      {
         if((nResult = ODSearchInDir(papszFileNames, nNumFileNames, pszFound,
            pszEnvVarSetting)) != -1)
         {
            if(pszDirectory != NULL) strcpy(pszDirectory,pszEnvVarSetting);
            return(nResult);
         }
      }
   }

   return(-1);
}


/* ----------------------------------------------------------------------------
 * ODSearchInDir()                                     *** PRIVATE FUNCTION ***
 *
 * Private helper function used by ODSearchForDropFile(). Searches for a drop
 * file in a single specified directory. The heuristic for selecting a drop
 * file, if more than one exists, is described in the header for the
 * ODSearchForDropFile() function.
 *
 * Parameters: papszFileNames - Array of possible drop file names.
 *
 *             nNumFilesNames - The number of names in papszFileNames.
 *
 *             pszFound       - If a drop file was found, this string
 *                              will be changed to point to the filename
 *                              of the file that was found.
 *
 *             pszDirectory   - Name of the directory to search in.
 *
 *     Return: Index in the array of the file that was found, or -1 if no
 *             potential drop file was found.
 */
static INT ODSearchInDir(char **papszFileNames, INT nNumFileNames,
   char *pszFound, char *pszDirectory)
{
   BYTE btCount;
   char szFullName[80];
   INT nFound = -1;
   tODDirHandle hDir;
   tODDirEntry DirEntry;
   time_t LatestTime = 0;

   ASSERT(papszFileNames != NULL);
   ASSERT(nNumFileNames > 0);
   ASSERT(pszFound != NULL);
   ASSERT(pszDirectory != NULL);

   for(btCount=0; btCount < nNumFileNames; ++btCount)
   {
      /* Do not consider DORINFO1.DEF if a DORINFOx.DEF for this node has */
      /* been found. */
      if(btCount == 2 && nFound == 1)
      {
         continue;
      }

      ASSERT(papszFileNames[btCount] != NULL);

      ODMakeFilename(szFullName, pszDirectory, (char *)papszFileNames[btCount],
         sizeof(szFullName));

      /* Attempt to open directory. */
      if(ODDirOpen(szFullName, DIR_ATTRIB_NORMAL | DIR_ATTRIB_ARCH, &hDir)
         == kODRCSuccess)
      {
         /* Read the first matching entry in the directory. */
         ODDirRead(hDir, &DirEntry);

         if(nFound == -1
            || DirEntry.LastWriteTime > LatestTime + DROPFILE_TIME_LEEWAY)
         {
            if(!ODFileAccessMode(szFullName, 4))
            {
               nFound=btCount;
               LatestTime = DirEntry.LastWriteTime;
            }
         }

         /* Close the open directory. */
         ODDirClose(hDir);
      }
   }

   if(nFound != -1)
   {
      ODMakeFilename(pszFound, pszDirectory, (char *)papszFileNames[nFound],
         160);
   }

   return(nFound);
}


#define OD_EXITINFO_FROM_LE16(field) do { \
   const BYTE *pODBytes = (const BYTE *)&(field); \
   (field) = (WORD)((WORD)pODBytes[0] | ((WORD)pODBytes[1] << 8)); \
} while(0)

#define OD_EXITINFO_FROM_LE32(field) do { \
   const BYTE *pODBytes = (const BYTE *)&(field); \
   (field) = (DWORD)((DWORD)pODBytes[0] | ((DWORD)pODBytes[1] << 8) \
      | ((DWORD)pODBytes[2] << 16) | ((DWORD)pODBytes[3] << 24)); \
} while(0)

#define OD_EXITINFO_TO_LE16(field) do { \
   WORD wODValue = (WORD)(field); \
   BYTE *pODBytes = (BYTE *)&(field); \
   pODBytes[0] = (BYTE)wODValue; \
   pODBytes[1] = (BYTE)(wODValue >> 8); \
} while(0)

#define OD_EXITINFO_TO_LE32(field) do { \
   DWORD dwODValue = (DWORD)(field); \
   BYTE *pODBytes = (BYTE *)&(field); \
   pODBytes[0] = (BYTE)dwODValue; \
   pODBytes[1] = (BYTE)(dwODValue >> 8); \
   pODBytes[2] = (BYTE)(dwODValue >> 16); \
   pODBytes[3] = (BYTE)(dwODValue >> 24); \
} while(0)


/* ----------------------------------------------------------------------------
 * ODExitInfoPrimitiveEndian()
 *
 * Converts every multi-byte field in a byte-packed pre-RA2 EXITINFO.BBS
 * record between its little-endian disk representation and host values.
 */
void ODExitInfoPrimitiveEndian(tExitInfoRecord *pRecord,
   BOOL bFromLittleEndian)
{
   unsigned nIndex;

   if(bFromLittleEndian)
   {
      OD_EXITINFO_FROM_LE16(pRecord->baud);
      OD_EXITINFO_FROM_LE32(pRecord->num_calls);
      for(nIndex = 0; nIndex < 24; ++nIndex)
         OD_EXITINFO_FROM_LE16(pRecord->busyperhour[nIndex]);
      for(nIndex = 0; nIndex < 7; ++nIndex)
         OD_EXITINFO_FROM_LE16(pRecord->busyperday[nIndex]);
      OD_EXITINFO_FROM_LE16(pRecord->credit);
      OD_EXITINFO_FROM_LE16(pRecord->pending);
      OD_EXITINFO_FROM_LE16(pRecord->posted);
      OD_EXITINFO_FROM_LE16(pRecord->lastread);
      OD_EXITINFO_FROM_LE16(pRecord->sec);
      OD_EXITINFO_FROM_LE16(pRecord->nocalls);
      OD_EXITINFO_FROM_LE16(pRecord->ups);
      OD_EXITINFO_FROM_LE16(pRecord->downs);
      OD_EXITINFO_FROM_LE16(pRecord->upk);
      OD_EXITINFO_FROM_LE16(pRecord->downk);
      OD_EXITINFO_FROM_LE16(pRecord->todayk);
      OD_EXITINFO_FROM_LE16(pRecord->elapsed);
      OD_EXITINFO_FROM_LE16(pRecord->screenlen);
      OD_EXITINFO_FROM_LE16(pRecord->xirecord);
      OD_EXITINFO_FROM_LE16(pRecord->timelimit);
      OD_EXITINFO_FROM_LE32(pRecord->loginsec);
      OD_EXITINFO_FROM_LE32(pRecord->net_credit);
      OD_EXITINFO_FROM_LE16(pRecord->userrecord);
      OD_EXITINFO_FROM_LE16(pRecord->readthru);
      OD_EXITINFO_FROM_LE16(pRecord->numberpages);
      OD_EXITINFO_FROM_LE16(pRecord->downloadlimint);
      OD_EXITINFO_FROM_LE16(pRecord->bbs.qbbs.screenlength);
   }
   else
   {
      OD_EXITINFO_TO_LE16(pRecord->baud);
      OD_EXITINFO_TO_LE32(pRecord->num_calls);
      for(nIndex = 0; nIndex < 24; ++nIndex)
         OD_EXITINFO_TO_LE16(pRecord->busyperhour[nIndex]);
      for(nIndex = 0; nIndex < 7; ++nIndex)
         OD_EXITINFO_TO_LE16(pRecord->busyperday[nIndex]);
      OD_EXITINFO_TO_LE16(pRecord->credit);
      OD_EXITINFO_TO_LE16(pRecord->pending);
      OD_EXITINFO_TO_LE16(pRecord->posted);
      OD_EXITINFO_TO_LE16(pRecord->lastread);
      OD_EXITINFO_TO_LE16(pRecord->sec);
      OD_EXITINFO_TO_LE16(pRecord->nocalls);
      OD_EXITINFO_TO_LE16(pRecord->ups);
      OD_EXITINFO_TO_LE16(pRecord->downs);
      OD_EXITINFO_TO_LE16(pRecord->upk);
      OD_EXITINFO_TO_LE16(pRecord->downk);
      OD_EXITINFO_TO_LE16(pRecord->todayk);
      OD_EXITINFO_TO_LE16(pRecord->elapsed);
      OD_EXITINFO_TO_LE16(pRecord->screenlen);
      OD_EXITINFO_TO_LE16(pRecord->xirecord);
      OD_EXITINFO_TO_LE16(pRecord->timelimit);
      OD_EXITINFO_TO_LE32(pRecord->loginsec);
      OD_EXITINFO_TO_LE32(pRecord->net_credit);
      OD_EXITINFO_TO_LE16(pRecord->userrecord);
      OD_EXITINFO_TO_LE16(pRecord->readthru);
      OD_EXITINFO_TO_LE16(pRecord->numberpages);
      OD_EXITINFO_TO_LE16(pRecord->downloadlimint);
      OD_EXITINFO_TO_LE16(pRecord->bbs.qbbs.screenlength);
   }
}


/* ----------------------------------------------------------------------------
 * ODExitInfoExtendedEndian()
 *
 * Converts the multi-byte field in a byte-packed RemoteAccess 1.x extension.
 */
void ODExitInfoExtendedEndian(tExtendedExitInfo *pRecord,
   BOOL bFromLittleEndian)
{
   if(bFromLittleEndian)
      OD_EXITINFO_FROM_LE16(pRecord->deducted_time);
   else
      OD_EXITINFO_TO_LE16(pRecord->deducted_time);
}


/* ----------------------------------------------------------------------------
 * ODExitInfoRA2Endian()
 *
 * Converts every multi-byte field in a byte-packed RemoteAccess 2.x record.
 */
void ODExitInfoRA2Endian(tRA2ExitInfoRecord *pRecord,
   BOOL bFromLittleEndian)
{
   unsigned nIndex;

   if(bFromLittleEndian)
   {
      OD_EXITINFO_FROM_LE16(pRecord->baud);
      OD_EXITINFO_FROM_LE32(pRecord->num_calls);
      for(nIndex = 0; nIndex < 24; ++nIndex)
         OD_EXITINFO_FROM_LE16(pRecord->busyperhour[nIndex]);
      for(nIndex = 0; nIndex < 7; ++nIndex)
         OD_EXITINFO_FROM_LE16(pRecord->busyperday[nIndex]);
      OD_EXITINFO_FROM_LE32(pRecord->password_crc);
      OD_EXITINFO_FROM_LE32(pRecord->credit);
      OD_EXITINFO_FROM_LE32(pRecord->pending);
      OD_EXITINFO_FROM_LE16(pRecord->posted);
      OD_EXITINFO_FROM_LE16(pRecord->sec);
      OD_EXITINFO_FROM_LE32(pRecord->lastread);
      OD_EXITINFO_FROM_LE32(pRecord->nocalls);
      OD_EXITINFO_FROM_LE32(pRecord->ups);
      OD_EXITINFO_FROM_LE32(pRecord->downs);
      OD_EXITINFO_FROM_LE32(pRecord->upk);
      OD_EXITINFO_FROM_LE32(pRecord->downk);
      OD_EXITINFO_FROM_LE32(pRecord->todayk);
      OD_EXITINFO_FROM_LE16(pRecord->elapsed);
      OD_EXITINFO_FROM_LE16(pRecord->screenlen);
      OD_EXITINFO_FROM_LE16(pRecord->group);
      for(nIndex = 0; nIndex < 200; ++nIndex)
         OD_EXITINFO_FROM_LE16(pRecord->combinedrecord[nIndex]);
      OD_EXITINFO_FROM_LE16(pRecord->msgarea);
      OD_EXITINFO_FROM_LE16(pRecord->filearea);
      OD_EXITINFO_FROM_LE16(pRecord->file_group);
      OD_EXITINFO_FROM_LE32(pRecord->xirecord);
      OD_EXITINFO_FROM_LE16(pRecord->msg_group);
      OD_EXITINFO_FROM_LE16(pRecord->timelimit);
      OD_EXITINFO_FROM_LE32(pRecord->loginsec);
      OD_EXITINFO_FROM_LE16(pRecord->userrecord);
      OD_EXITINFO_FROM_LE16(pRecord->readthru);
      OD_EXITINFO_FROM_LE16(pRecord->numberpages);
      OD_EXITINFO_FROM_LE16(pRecord->downloadlimit);
      OD_EXITINFO_FROM_LE32(pRecord->logonpasswordcrc);
      OD_EXITINFO_FROM_LE16(pRecord->deducted_time);
      OD_EXITINFO_FROM_LE16(pRecord->menu_cost_per_min);
   }
   else
   {
      OD_EXITINFO_TO_LE16(pRecord->baud);
      OD_EXITINFO_TO_LE32(pRecord->num_calls);
      for(nIndex = 0; nIndex < 24; ++nIndex)
         OD_EXITINFO_TO_LE16(pRecord->busyperhour[nIndex]);
      for(nIndex = 0; nIndex < 7; ++nIndex)
         OD_EXITINFO_TO_LE16(pRecord->busyperday[nIndex]);
      OD_EXITINFO_TO_LE32(pRecord->password_crc);
      OD_EXITINFO_TO_LE32(pRecord->credit);
      OD_EXITINFO_TO_LE32(pRecord->pending);
      OD_EXITINFO_TO_LE16(pRecord->posted);
      OD_EXITINFO_TO_LE16(pRecord->sec);
      OD_EXITINFO_TO_LE32(pRecord->lastread);
      OD_EXITINFO_TO_LE32(pRecord->nocalls);
      OD_EXITINFO_TO_LE32(pRecord->ups);
      OD_EXITINFO_TO_LE32(pRecord->downs);
      OD_EXITINFO_TO_LE32(pRecord->upk);
      OD_EXITINFO_TO_LE32(pRecord->downk);
      OD_EXITINFO_TO_LE32(pRecord->todayk);
      OD_EXITINFO_TO_LE16(pRecord->elapsed);
      OD_EXITINFO_TO_LE16(pRecord->screenlen);
      OD_EXITINFO_TO_LE16(pRecord->group);
      for(nIndex = 0; nIndex < 200; ++nIndex)
         OD_EXITINFO_TO_LE16(pRecord->combinedrecord[nIndex]);
      OD_EXITINFO_TO_LE16(pRecord->msgarea);
      OD_EXITINFO_TO_LE16(pRecord->filearea);
      OD_EXITINFO_TO_LE16(pRecord->file_group);
      OD_EXITINFO_TO_LE32(pRecord->xirecord);
      OD_EXITINFO_TO_LE16(pRecord->msg_group);
      OD_EXITINFO_TO_LE16(pRecord->timelimit);
      OD_EXITINFO_TO_LE32(pRecord->loginsec);
      OD_EXITINFO_TO_LE16(pRecord->userrecord);
      OD_EXITINFO_TO_LE16(pRecord->readthru);
      OD_EXITINFO_TO_LE16(pRecord->numberpages);
      OD_EXITINFO_TO_LE16(pRecord->downloadlimit);
      OD_EXITINFO_TO_LE32(pRecord->logonpasswordcrc);
      OD_EXITINFO_TO_LE16(pRecord->deducted_time);
      OD_EXITINFO_TO_LE16(pRecord->menu_cost_per_min);
   }
}


/* ----------------------------------------------------------------------------
 * ODReadExitInfoPrimitive()
 *
 * Reads the core a of pre-RA2 style EXITINFO.BBS file.
 *
 * Parameters: pfDropFile - Pointer to already open EXITINFO.BBS file.
 *
 *             nCount     - Specifies the number of bytes to read.
 *
 *     Return: TRUE on success, or FALSE on failure.
 */
BOOL ODReadExitInfoPrimitive(FILE *pfDropFile, INT nCount)
{
   if((pExitInfoRecord=calloc(1, sizeof(tExitInfoRecord)))==NULL) return(FALSE);

   if(fread(pExitInfoRecord,1,nCount,pfDropFile)!=(size_t)nCount)
   {
      free(pExitInfoRecord);
      pExitInfoRecord = NULL;
      return(FALSE);
   }

   ODExitInfoPrimitiveEndian(pExitInfoRecord, TRUE);

                                          /* now we read all the data from the */
                                          /* EXITINFO structure to the OpenDoors */
                                          /* control structure. This may look */
                                          /* a bit messy, but it gets the job */
                                          /* done, and allows the programmer */
                                          /* to access all the strings in C */
                                          /* format instead of Pascal */
   od_control.baud=pExitInfoRecord->baud;
   od_control.system_calls=pExitInfoRecord->num_calls;
   ODStringPascalToC(od_control.system_last_caller,pExitInfoRecord->last_caller,35);
   ODStringPascalToC(od_control.timelog_start_date,pExitInfoRecord->start_date,8);
   memcpy(od_control.timelog_busyperhour, pExitInfoRecord->busyperhour,
      sizeof(od_control.timelog_busyperhour));
   memcpy(od_control.timelog_busyperday, pExitInfoRecord->busyperday,
      sizeof(od_control.timelog_busyperday));
   ODStringPascalToC(od_control.user_name,pExitInfoRecord->uname,35);
   ODStringPascalToC(od_control.user_location,pExitInfoRecord->uloc,25);
   ODStringPascalToC(od_control.user_password,pExitInfoRecord->password,15);
   ODStringPascalToC(od_control.user_dataphone,pExitInfoRecord->dataphone,12);
   ODStringPascalToC(od_control.user_homephone,pExitInfoRecord->homephone,12);
   ODStringPascalToC(od_control.user_lasttime,pExitInfoRecord->lasttime,5);
   ODStringPascalToC(od_control.user_lastdate,pExitInfoRecord->lastdate,8);
   memcpy(&od_control.user_attribute,&pExitInfoRecord->attrib,5);
   od_control.user_net_credit=pExitInfoRecord->credit;
   od_control.user_pending=pExitInfoRecord->pending;
   od_control.user_messages=pExitInfoRecord->posted;
   od_control.user_lastread=pExitInfoRecord->lastread;
   od_control.user_security=pExitInfoRecord->sec;
   od_control.user_numcalls=pExitInfoRecord->nocalls;
   od_control.user_uploads=pExitInfoRecord->ups;
   od_control.user_downloads=pExitInfoRecord->downs;
   od_control.user_upk=pExitInfoRecord->upk;
   od_control.user_downk=pExitInfoRecord->downk;
   od_control.user_todayk=pExitInfoRecord->todayk;
   memcpy(&od_control.user_time_used,&pExitInfoRecord->elapsed,6);
   od_control.user_group=pExitInfoRecord->group;
   od_control.user_xi_record=pExitInfoRecord->xirecord;
   od_control.event_status=pExitInfoRecord->status;
   ODStringPascalToC(od_control.event_starttime,pExitInfoRecord->starttime,5);
   memcpy(&od_control.event_errorlevel,&pExitInfoRecord->errorlevel,3);
   ODStringPascalToC(od_control.event_last_run,pExitInfoRecord->lasttimerun,8);
   memcpy(&od_control.user_netmailentered,&pExitInfoRecord->netmailentered,2);
   ODStringPascalToC(od_control.user_logintime,pExitInfoRecord->logintime,5);
   ODStringPascalToC(od_control.user_logindate,pExitInfoRecord->logindate,8);

   /* Note that the timelimit field is skipped here. This value has already */
   /* been read from the DORINFOx.DEF file, and is not consistently written */
   /* to the EXITINFO.BBS file by various BBS packages.                     */

   memcpy(&od_control.user_loginsec,&pExitInfoRecord->loginsec,16);
   od_control.user_ansi=od_control.user_attribute&8;
   od_control.user_avatar=od_control.user_attrib2&2;

   return(TRUE);
}


/* ----------------------------------------------------------------------------
 * ODWriteExitInfoPrimitive()
 *
 * Writes the core a of pre-RA2 style EXITINFO.BBS file.
 *
 * Parameters: pfDropFile - Pointer to already open EXITINFO.BBS file.
 *
 *             nCount     - Number of bytes to be written.
 *
 *     Return: TRUE on success, or FALSE on failure.
 */
INT ODWriteExitInfoPrimitive(FILE *pfDropFile, INT nCount)
{
   INT nToReturn;
   DWORD dwActiveMinutes;
   INT nUserTimeLost;
   INT nTimeSubtractedBySysop;
   time_t nCurrentUnixTime;

   pExitInfoRecord->num_calls=od_control.system_calls;
   ODStringCToPascal(pExitInfoRecord->last_caller,35,od_control.system_last_caller);
   ODStringCToPascal(pExitInfoRecord->start_date,8,od_control.timelog_start_date);
   memcpy(pExitInfoRecord->busyperhour, od_control.timelog_busyperhour,
      sizeof(pExitInfoRecord->busyperhour));
   memcpy(pExitInfoRecord->busyperday, od_control.timelog_busyperday,
      sizeof(pExitInfoRecord->busyperday));
   ODStringCToPascal(pExitInfoRecord->uname,35,od_control.user_name);
   ODStringCToPascal(pExitInfoRecord->uloc,25,od_control.user_location);
   ODStringCToPascal(pExitInfoRecord->password,15,od_control.user_password);
   ODStringCToPascal(pExitInfoRecord->dataphone,12,od_control.user_dataphone);
   ODStringCToPascal(pExitInfoRecord->homephone,12,od_control.user_homephone);
   ODStringCToPascal(pExitInfoRecord->lasttime,5,od_control.user_lasttime);
   ODStringCToPascal(pExitInfoRecord->lastdate,8,od_control.user_lastdate);
   memcpy(&pExitInfoRecord->attrib,&od_control.user_attribute,5);
   pExitInfoRecord->credit=(WORD)od_control.user_net_credit;
   pExitInfoRecord->pending=(WORD)od_control.user_pending;
   pExitInfoRecord->posted=(WORD)od_control.user_messages;
   pExitInfoRecord->lastread=(WORD)od_control.user_lastread;
   pExitInfoRecord->sec=(WORD)od_control.user_security;
   pExitInfoRecord->nocalls=(WORD)od_control.user_numcalls;
   pExitInfoRecord->ups=(WORD)od_control.user_uploads;
   pExitInfoRecord->downs=(WORD)od_control.user_downloads;
   pExitInfoRecord->upk=(WORD)od_control.user_upk;
   pExitInfoRecord->downk=(WORD)od_control.user_downk;
   pExitInfoRecord->todayk=(WORD)od_control.user_todayk;
   memcpy(&pExitInfoRecord->elapsed,&od_control.user_time_used,6);
   pExitInfoRecord->group = (BYTE)od_control.user_group;
   pExitInfoRecord->xirecord=(WORD)od_control.user_xi_record;
   pExitInfoRecord->status=od_control.event_status;
   pExitInfoRecord->status=od_control.event_status;
   ODStringCToPascal(pExitInfoRecord->starttime,5,od_control.event_starttime);
   memcpy(&pExitInfoRecord->errorlevel,&od_control.event_errorlevel,3);
   ODStringCToPascal(pExitInfoRecord->lasttimerun,8,od_control.event_last_run);
   memcpy(&pExitInfoRecord->netmailentered,&od_control.user_netmailentered,2);
   ODStringCToPascal(pExitInfoRecord->logintime,5,od_control.user_logintime);
   ODStringCToPascal(pExitInfoRecord->logindate,8,od_control.user_logindate);

   /* Calculate new time limit based on how time was adjusted during door's */
   /* execution.                                                            */
   nCurrentUnixTime = time(NULL);
   if(ODGetElapsedMinutes(&dwActiveMinutes, nStartupUnixTime,
      nCurrentUnixTime))
   {
      nUserTimeLost = (nInitialRemaining - od_control.user_timelimit);
      nTimeSubtractedBySysop = nUserTimeLost - (int)dwActiveMinutes;
      pExitInfoRecord->timelimit -= nTimeSubtractedBySysop;
   }

   memcpy(&pExitInfoRecord->loginsec,&od_control.user_loginsec,16);

   ODExitInfoPrimitiveEndian(pExitInfoRecord, FALSE);
   nToReturn=(fwrite(pExitInfoRecord,1,nCount,pfDropFile) == (size_t)nCount);
   ODExitInfoPrimitiveEndian(pExitInfoRecord, TRUE);
   return(nToReturn);
}


/* ----------------------------------------------------------------------------
 * ODAtExitCallback()
 *
 * OpenDoors sets up the C library to call back this function when the program
 * is about to exit. OpenDoors uses this function to attempt to trap the
 * condition where the programmer exits the program without explicitly calling
 * od_exit(). If the program is about to exit, and OpenDoors is still active,
 * then od_exit() is called.
 *
 * It is not recommended that the programmer using OpenDoors rely on this
 * mechanism, because:
 *
 *    1. It doesn't seem to be supported by all compilers.
 *
 *    2. It doesn't permit OpenDoors to determine the actual error level
 *       that the program is exiting with in order to report this information
 *       in the log file (if enabled).
 *
 * Parameters: none
 *
 *     Return: void
 */
#ifndef ODPLAT_WIN32
void ODAtExitCallback(void)
{
   if(bODInitialized)
   {
      bPreOrExit = TRUE;
      if(od_control.od_errorlevel[0])
      {
         od_exit(od_control.od_errorlevel[7],FALSE);
      }
      else
      {
         od_exit(6,FALSE);
      }
   }
}
#endif /* !ODPLAT_WIN32 */


/* Currently, these functions are only used in the Win32 version. */
#ifdef ODPLAT_WIN32


/* ----------------------------------------------------------------------------
 * ODSendModemCommand()                                *** PRIVATE FUNCTION ***
 *
 * Sends a sequence of commands to the modem, waiting for the specified
 * response between each command. The command sequence is retried the specified
 * number of times.
 *
 * Parameters: pszCommand - Command string to send to the modem, along with
 *                          response strings. Each of these are separated by
 *                          a space character. A pipe character ('|') denotes a
 *                          CR, and a tilde character ('~') denotes a one
 *                          second pause.
 *
 *             nRetries   - Number of times to retry command sequence.
 *
 *     Return: TRUE on success, or FALSE if some expected response string was
 *             not received from the modem after modem response timeout period.
 */
static BOOL ODSendModemCommand(char *pszCommand, int nRetries)
{
   ASSERT(pszCommand != NULL);
   ASSERT(nRetries >= 1);

   while(nRetries--)
   {
      if(ODSendModemCommandOnce(pszCommand))
      {
         return(TRUE);
      }
   }

   return(FALSE);
}


/* ----------------------------------------------------------------------------
 * ODSendModemCommandOnce()                            *** PRIVATE FUNCTION ***
 *
 * Sends a series of commands to the modem, waiting for the specified response
 * between each command.
 *
 * Parameters: pszCommand - Command string to send to the modem, along with
 *                          response strings. Each of these are separated by
 *                          a space character. A pipe character ('|') denotes a
 *                          CR, and a tilde character ('~') denotes a one
 *                          second pause.
 *
 *     Return: TRUE on success, or FALSE if some expected response string was
 *             not received from the modem after modem response timeout period.
 */
static BOOL ODSendModemCommandOnce(char *pszCommand)
{
   char *pchCurrent;
   char szResponse[MAX_RESPONSE_LEN + 1];
   int nResponsePos = 0;
   BOOL bSendingCommand = TRUE;

   ASSERT(pszCommand != NULL);

   /* We must be operating in remote mode. */
   ASSERT(od_control.baud != 0);

   /* Loop through each character in the string. */
   for(pchCurrent = pszCommand; *pchCurrent != '\0'; ++pchCurrent)
   {
      /* What we do with this character depends upon whether we are */
      /* currently sending a command, or waiting for a response.    */
      if(bSendingCommand)
      {
         switch(*pchCurrent)
         {
            case ' ':
               /* A space character denotes that we should toggle between */
               /* sending a command and receiving a response.             */
               bSendingCommand = FALSE;

               /* Start at the beginning of the empty response string. */
               nResponsePos = 0;
               szResponse[0] = '\0';
               break;

            case '|':
               /* A pipe character denotes that a carriage return should be */
               /* send to the modem.                                        */
               ODCoreSendRemoteByte('\r');
#ifdef OD_DIAGNOSTICS
               strcat(szDebugWorkString, "\n");
#endif /* OD_DIAGNOSTICS */
               break;

            case '~':
               /* A tilde character denotes a 1 second pause. */
               ODAPISleep(1000);
               break;

            default:
               /* Otherwise, send this character as is. */
               ODCoreSendRemoteByte((BYTE)*pchCurrent);
#ifdef OD_DIAGNOSTICS
               {
                  char szAppend[2];
                  szAppend[0] = *pchCurrent;
                  szAppend[1] = 0;
                  strcat(szDebugWorkString, szAppend);
               }
#endif /* OD_DIAGNOSTICS */
         }

         ODAPISleep(200);
      }
      else
      {
         /* We are currently building a string that we should wait for. */
         switch(*pchCurrent)
         {
            case ' ':
               /* A space character denotes that we should toggle between */
               /* sending a command and receiving a response.             */

               /* Wait until the response string we have built is received. */
               if(!ODWaitForString(szResponse, RESPONSE_TIMEOUT))
               {
                  /* If string was not received, then return now. */
                  return(FALSE);
               }

               /* Switch to sending command mode. */
               bSendingCommand = TRUE;
               break;

            case '~':
               /* Pauses are ignored in response strings. */
               break;

            default:
               /* Otherwise, add this character to the response string. */
               if(nResponsePos < MAX_RESPONSE_LEN)
               {
                  szResponse[nResponsePos] = *pchCurrent;
                  ++nResponsePos;
                  szResponse[nResponsePos] = '\0';
               }
         }
      }
   }

   /* Return with success. */
   return(TRUE);
}


/* ----------------------------------------------------------------------------
 * ODWaitForString()                                   *** PRIVATE FUNCTION ***
 *
 * Waits for the specified string to be received from the modem, for up to
 * the specified length of time.
 *
 * Parameters: pszResponse     - Pointer to the string to wait for.
 *
 *             ResponseTimeout - The maximum time, in milliseconds, to wait.
 *
 *     Return: TRUE on success, or FALSE if some expected response string was
 *             not received from the modem after modem response timeout period.
 */
static BOOL ODWaitForString(char *pszResponse, tODMilliSec ResponseTimeout)
{
   tODTimer Timer;
   char szReceived[MAX_RESPONSE_LEN + 1] = "\0";
   tODInputEvent InputEvent;
   tODResult Result;
#ifdef OD_THREAD_SUPPORT
   tODMilliSec Slice;
   unsigned nSavedAPILevel;
#endif

   ASSERT(pszResponse != NULL);
   ASSERT(ResponseTimeout > 0);

   /* We must be operating in remote mode. */
   ASSERT(od_control.baud != 0);

   /* If response string is empty, then we don't wait for anything. */
   if(strlen(pszResponse) == 0) return(TRUE);

#ifdef OD_DIAGNOSTICS
   strcat(szDebugWorkString, "[");
#endif /* OD_DIAGNOSTICS */

   ODTimerStart(&Timer, ResponseTimeout);
   while(!ODTimerElapsed(&Timer))
   {
#ifdef OD_THREAD_SUPPORT
      Slice = ODTimerLeft(&Timer);
      if(Slice > 50) Slice = 50;
      nSavedAPILevel = ODSyncAPIRelease();
      Result = ODInQueueGetNextEvent(hODInputQueue, &InputEvent,
         Slice);
      ODSyncAPIReacquire(nSavedAPILevel);
#else
      Result = ODInQueueGetNextEvent(hODInputQueue, &InputEvent,
         ODTimerLeft(&Timer));
#endif
      if(Result == kODRCSuccess)
      {
         if(InputEvent.bFromRemote && InputEvent.EventType == EVENT_CHARACTER)
         {
#ifdef OD_DIAGNOSTICS
            {
               char szAppend[2];
               szAppend[0] = InputEvent.chKeyPress;
               szAppend[1] = 0;
               strcat(szDebugWorkString, szAppend);
            }
#endif /* OD_DIAGNOSTICS */

            /* Add the received character to the received string. */
            if(strlen(szReceived) == MAX_RESPONSE_LEN)
            {
               memmove(szReceived, szReceived + 1, MAX_RESPONSE_LEN);
            }
            szReceived[strlen(szReceived) + 1] = '\0';
            szReceived[strlen(szReceived)] = InputEvent.chKeyPress;

            /* If the sequence has been received, then return with success. */
            if(strstr(szReceived, pszResponse) != NULL)
            {
#ifdef OD_DIAGNOSTICS
               strcat(szDebugWorkString, "]");
#endif /* OD_DIAGNOSTICS */
               return(TRUE);
            }
         }
      }
      else
      {
         /* Poll communications and timers between bounded waits. */
#ifdef OD_THREAD_SUPPORT
         if(!ODSyncAPICheckpoint()) return(FALSE);
#else
         ODAPISleep(0);
#endif
      }
   }

#ifdef OD_DIAGNOSTICS
   strcat(szDebugWorkString, "]");
#endif /* OD_DIAGNOSTICS */

   /* Indicate that string was not received in the time alotted. */
   return(FALSE);
}


/* ----------------------------------------------------------------------------
 * ODAPISleep()                                       *** PRIVATE FUNCTION ***
 *
 * Sleeps from inside an API operation while allowing
 * cooperative kernel work to progress.
 */
static void ODAPISleep(tODMilliSec Milliseconds)
{
#ifdef OD_THREAD_SUPPORT
   unsigned nSavedAPILevel = ODSyncAPIRelease();
   ODThreadSleep(Milliseconds);
   ODSyncAPIReacquire(nSavedAPILevel);
   ODSyncAPICheckpoint();
#else
   od_sleep(Milliseconds);
#endif
}


#if defined(OD_DIAGNOSTICS) && defined(ODPLAT_WIN32)
static void ODDiagnosticMessage(const char *pszText, const char *pszTitle)
{
#ifdef OD_THREAD_SUPPORT
   unsigned nSavedAPILevel = 0;

   if(ODSyncAPILevelActive())
      nSavedAPILevel = ODSyncAPIRelease();
#endif

   MessageBox(NULL, pszText, pszTitle, MB_OK);

#ifdef OD_THREAD_SUPPORT
   if(nSavedAPILevel != 0)
      ODSyncAPIReacquire(nSavedAPILevel);
#endif
}
#endif


/* ----------------------------------------------------------------------------
 * ODInExDisableDTR()
 *
 * Disables DTR response by the modem, if required.
 *
 * Parameters: None
 *
 *     Return: void
 */
void ODInExDisableDTR(void)
{
   BOOL bCarrier;

   /* If we are using the Door32 interface, then do not disable DTR. */
   if(od_control.od_com_method == COM_DOOR32 || od_control.od_com_method == COM_SOCKET)
   {
      return;
   }

   /* Check that carrier detect signal is still present. */
   ODComCarrier(hSerialPort, &bCarrier);
   if(bCarrier)
   {
      /* Only disable DTR response if OpenDoors opened the serial port, */
      /* and DTR disabling has not been explicitly turned off.          */
      if(od_control.od_open_handle == 0
         && !(od_control.od_disable & DIS_DTR_DISABLE))
      {
         if(!ODSendModemCommand(od_control.od_disable_dtr, 2))
         {
#ifdef OD_DIAGNOSTICS
            if(od_control.od_internal_debug)
            {
               ODDiagnosticMessage(szDebugWorkString, "DTR Disable FAILED!");
               szDebugWorkString[0] = '\0';
            }
#endif /* OD_DIAGNOSTICS */
         }
         else
         {
#ifdef OD_DIAGNOSTICS
            if(od_control.od_internal_debug)
            {
               ODDiagnosticMessage(szDebugWorkString,
                  "DTR Disable Succeeded!");
               szDebugWorkString[0] = '\0';
            }
#endif /* OD_DIAGNOSTICS */
         }
      }
   }
}

#endif /* ODPLAT_WIN32 */
