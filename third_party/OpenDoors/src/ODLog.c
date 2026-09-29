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
 *        File: ODLog.c
 *
 * Description: Implements the logfile subsystem.
 *
 *   Revisions: Date          Ver   Who  Change
 *              ---------------------------------------------------------------
 *              Oct 13, 1994  6.00  BP   New file header format.
 *              Dec 09, 1994  6.00  BP   Standardized coding style.
 *              Aug 19, 1995  6.00  BP   32-bit portability.
 *              Nov 16, 1995  6.00  BP   Removed oddoor.h, added odcore.h.
 *              Dec 30, 1995  6.00  BP   Added ODCALL for calling convention.
 *              Feb 19, 1996  6.00  BP   Changed version number to 6.00.
 *              Mar 03, 1996  6.10  BP   Begin version 6.10.
 *              Aug 10, 2003  6.23  SH   *nix support
 */

#define BUILDING_OPENDOORS

#include <errno.h>
#include <stdio.h>
#include <time.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODFormat.h"
#include "ODGen.h"
#include "ODInEx.h"
#include "ODKrnl.h"


/* Private logfile file handle */
static FILE *logfile_pointer;


/* Private helper functions. */
static BOOL ODCALL ODLogWriteStandardMsg(INT nLogfileMessage);
static BOOL ODCALL ODLogClose(INT nReason);
static BOOL ODVCALL ODLogFormatWorkString(const char *pszFormat, ...);
BOOL ODCALL ODLogTimeRecordSucceeded(time_t nUnixTime,
   const struct tm *ptmTimeRecord);


/* ----------------------------------------------------------------------------
 * ODLogTimeRecordSucceeded()                          *** PRIVATE FUNCTION ***
 *
 * Validates the results returned while obtaining a logfile timestamp.
 */
BOOL ODCALL ODLogTimeRecordSucceeded(time_t nUnixTime,
   const struct tm *ptmTimeRecord)
{
   return(nUnixTime != (time_t)-1 && ptmTimeRecord != NULL);
}


/* ----------------------------------------------------------------------------
 * ODLogFormatWorkString()                             *** PRIVATE FUNCTION ***
 *
 * Formats a configured logfile message without exceeding the shared work
 * buffer. This uses the C89-compatible bounded formatter on 16-bit DOS.
 */
static BOOL ODVCALL ODLogFormatWorkString(const char *pszFormat, ...)
{
   va_list ArgumentList;
   int nWritten;

   va_start(ArgumentList, pszFormat);
   nWritten = ODVsnprintf(szODWorkString, sizeof(szODWorkString), pszFormat,
      ArgumentList);
   va_end(ArgumentList);

   if(nWritten < 0 || (size_t)nWritten >= sizeof(szODWorkString))
   {
      szODWorkString[0] = '\0';
      od_control.od_error = ERR_LIMIT;
      return(FALSE);
   }
   return(TRUE);
}


/* ----------------------------------------------------------------------------
 * ODLogEnable()
 *
 * This function is called from od_init() when the user explicitly includes the
 * OpenDoors logfile option using the od_control.od_logfile setting.
 *
 * Parameters: None.
 *
 *     Return: void
 */
ODAPIDEF void ODCALL ODLogEnable(void)
{
   if(!ODSyncPublicCallAllowed()) return;
   /* At this time, this function simply maps to a call to od_log_open(). */
   od_log_open();
}


/* ----------------------------------------------------------------------------
 * od_log_open()
 *
 * Called to begin logfile operations. This is when the first message is
 * written to the logfile, indicating that the user is entering OpenDoors.
 *
 * Parameters: None.
 *
 *     Return: TRUE on success, or FALSE on failure.
 */
ODAPIDEF BOOL ODCALL od_log_open()
{
   time_t nUnixTime;
   struct tm *ptmTimeRecord;
   int nWriteError;

   /* Log function entry if running in trace mode. */
   TRACE(TRACE_API, "od_log_open()");

   /* Initialize OpenDoors if not already done. */
   if(!bODInitialized) od_init();
   OD_RETURN_IF_SESSION_ENDED(FALSE);

   /* An existing stream already represents the active logging session. */
   if(logfile_pointer != NULL) return(TRUE);

   /* Don't open logfile if it has been disabled in config file, etc. */
   if(od_control.od_logfile_disable) return(TRUE);

   /* Open actual logfile. */
   if((logfile_pointer=fopen(od_control.od_logfile_name, "a")) == NULL)
   {
      return(FALSE);
   }

   /* Get the current time. */
   nUnixTime = time(NULL);
   ptmTimeRecord = nUnixTime == (time_t)-1 ? NULL : localtime(&nUnixTime);
   if(!ODLogTimeRecordSucceeded(nUnixTime, ptmTimeRecord))
   {
      nWriteError = errno;
      fclose(logfile_pointer);
      logfile_pointer = NULL;
      pfLogWrite = NULL;
      pfLogClose = NULL;
      errno = nWriteError;
      return(FALSE);
   }

   /* Print logfile tear line. */
   if(fprintf(logfile_pointer, "\n----------  %s %02d %s %02d, %s\n",
      od_control.od_day[ptmTimeRecord->tm_wday],
      ptmTimeRecord->tm_mday,
      od_control.od_month[ptmTimeRecord->tm_mon],
      ptmTimeRecord->tm_year % 100,
      od_program_name) < 0)
   {
      nWriteError = errno;
      fclose(logfile_pointer);
      logfile_pointer = NULL;
      pfLogWrite = NULL;
      pfLogClose = NULL;
      errno = nWriteError;
      return(FALSE);
   }

   /* Print message of door start up. */
   if(!ODLogFormatWorkString(
      (char *)od_control.od_logfile_messages[11], od_control.user_name)
      || !od_log_write(szODWorkString))
   {
      nWriteError = errno;
      fclose(logfile_pointer);
      logfile_pointer = NULL;
      pfLogWrite = NULL;
      pfLogClose = NULL;
      errno = nWriteError;
      return(FALSE);
   }

   /* Set internal function hooks to enable calling of logfile features */
   /* from elsewhere in OpenDoors. */
   pfLogWrite = ODLogWriteStandardMsg;
   pfLogClose = ODLogClose;

   return(TRUE);
}


/* ----------------------------------------------------------------------------
 * ODLogWriteStandardMsg()                             *** PRIVATE FUNCTION ***
 *
 * Function called to write a standard message to the logfile.
 *
 * Parameters: nLogfileMessage   - Index of the standard message to write to
 *                                 the logfile.
 *
 *     Return: TRUE on success, or FALSE on failure.
 */
static BOOL ODCALL ODLogWriteStandardMsg(INT nLogfileMessage)
{
   BOOL bWriteSucceeded;

   if(nLogfileMessage < 0 || nLogfileMessage > 11)
   {
      return(FALSE);
   }

   bWriteSucceeded = od_log_write(
      (char *)od_control.od_logfile_messages[nLogfileMessage]);
   if(!bWriteSucceeded)
   {
      return(FALSE);
   }

   if(nLogfileMessage == 8)
   {
      if(!ODLogFormatWorkString(od_control.od_logfile_messages[12],
         od_control.user_reasonforchat))
      {
         return(FALSE);
      }
      szODWorkString[67] = '\0';
      return(od_log_write(szODWorkString));
   }

   return(TRUE);
}


/* ----------------------------------------------------------------------------
 * od_log_write()
 *
 * Called to write a message to the logfile.
 *
 * Parameters: pszMessage  - Pointer to a string containing the message text.
 *
 *     Return: TRUE on success, or FALSE on failure.
 */
ODAPIDEF BOOL ODCALL od_log_write(const char *pszMessage)
{
   char *pszFormat;
   time_t nUnixTime;
   struct tm *ptmTimeRecord;
   int nWriteError;

   /* Verify that OpenDoors has been initialized. */
   if(!bODInitialized) od_init();
   OD_RETURN_IF_SESSION_ENDED(FALSE);

   OD_API_ENTRY();

   /* Stop if logfile has been disabled in config file, etc. */
   if(od_control.od_logfile_disable)
   {
      OD_API_EXIT();
      return(TRUE);
   }

   /* If logfile has not yet been opened, then open it. */
   if(logfile_pointer==NULL)
   {
      if(!od_log_open())
      {
         OD_API_EXIT();
         return(FALSE);
      }
   }

   /* Get the current system time. */
   nUnixTime=time(NULL);
   ptmTimeRecord = nUnixTime == (time_t)-1 ? NULL : localtime(&nUnixTime);
   if(!ODLogTimeRecordSucceeded(nUnixTime, ptmTimeRecord))
   {
      nWriteError = errno;
      OD_API_EXIT();
      errno = nWriteError;
      return(FALSE);
   }

   /* Determine which logfile format string to use. */
   if(ptmTimeRecord->tm_hour<10)
   {
      pszFormat=(char *)">  %1.1d:%02d:%02d  %s\n";
   }
   else
   {
      pszFormat=(char *)"> %2.2d:%02d:%02d  %s\n";
   }

   /* Write a line to the logfile. */
   if(fprintf(logfile_pointer, pszFormat, ptmTimeRecord->tm_hour,
      ptmTimeRecord->tm_min, ptmTimeRecord->tm_sec, pszMessage) < 0
      || fflush(logfile_pointer) == EOF)
   {
      nWriteError = errno;
      OD_API_EXIT();
      errno = nWriteError;
      return(FALSE);
   }

   OD_API_EXIT();
   return(TRUE);
}


/* ----------------------------------------------------------------------------
 * ODLogClose()                                        *** PRIVATE FUNCTION ***
 *
 * Writes final entry to the logfile when OpenDoors is exiting.
 *
 * Parameters: nReason  - Specifies the reason why OpenDoors is exiting.
 *
 *     Return: TRUE if the final entry and close succeeded, or FALSE if either
 *             operation failed.
 */
static BOOL ODCALL ODLogClose(INT nReason)
{
   BOOL bCloseSucceeded = TRUE;
   BOOL bLimitFailure = FALSE;
   BOOL bHaveRuntimeError = FALSE;
   int nRuntimeError = 0;

   /* If logfile has not been opened, then abort. */
   if(logfile_pointer==NULL) return(TRUE);

   /* Write the final entry unless logging was disabled after this stream was
    * opened. The stream itself must still be closed in either case. */
   if(!od_control.od_logfile_disable)
   {
      if(bPreOrExit)
      {
         if(!od_log_write((char *)od_control.od_logfile_messages[13]))
         {
            bCloseSucceeded = FALSE;
            bHaveRuntimeError = TRUE;
            nRuntimeError = errno;
         }
      }
      else if(btExitReason<=5 && btExitReason>=1)
      {
         if(!od_log_write(
            (char *)od_control.od_logfile_messages[btExitReason-1]))
         {
            bCloseSucceeded = FALSE;
            bHaveRuntimeError = TRUE;
            nRuntimeError = errno;
         }
      }
      else
      {
         if(ODLogFormatWorkString(
            (char *)od_control.od_logfile_messages[5], nReason))
         {
            if(!od_log_write(szODWorkString))
            {
               bCloseSucceeded = FALSE;
               bHaveRuntimeError = TRUE;
               nRuntimeError = errno;
            }
         }
         else
         {
            bCloseSucceeded = FALSE;
            bLimitFailure = TRUE;
         }
      }
   }

   /* Close the logfile. */
   if(fclose(logfile_pointer) != 0)
   {
      if(!bHaveRuntimeError)
      {
         bHaveRuntimeError = TRUE;
         nRuntimeError = errno;
      }
      bCloseSucceeded = FALSE;
   }

   /* Prevent further use of logfile without first re-opening it. */
   pfLogWrite = NULL;
   pfLogClose = NULL;
   logfile_pointer = NULL;

   if(!bCloseSucceeded)
   {
      if(!bLimitFailure)
      {
         od_control.od_error = ERR_GENERALFAILURE;
      }
      if(bHaveRuntimeError)
      {
         errno = nRuntimeError;
      }
      return(FALSE);
   }
   return(TRUE);
}
