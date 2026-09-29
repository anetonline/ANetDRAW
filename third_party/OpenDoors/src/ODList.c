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
 *        File: ODList.c
 *
 * Description: Implements the od_list_files() function for displaying
 *              a FILES.BBS file.
 *
 *   Revisions: Date          Ver   Who  Change
 *              ---------------------------------------------------------------
 *              Oct 13, 1994  6.00  BP   New file header format.
 *              Oct 21, 1994  6.00  BP   Further isolated com routines.
 *              Dec 09, 1994  6.00  BP   Use new directory access functions.
 *              Dec 31, 1994  6.00  BP   Remove #ifndef USEINLINE DOS code.
 *              Aug 19, 1995  6.00  BP   32-bit portability.
 *              Nov 11, 1995  6.00  BP   Moved functions from odcore.c
 *              Nov 11, 1995  6.00  BP   Removed register keyword.
 *              Nov 16, 1995  6.00  BP   Removed oddoor.h, added odcore.h.
 *              Dec 12, 1995  6.00  BP   Added entry, exit and kernel macros.
 *              Dec 30, 1995  6.00  BP   Added ODCALL for calling convention.
 *              Feb 19, 1996  6.00  BP   Changed version number to 6.00.
 *              Mar 03, 1996  6.10  BP   Begin version 6.10.
 *              Aug 10, 2003  6.23  SH   *nix support
 */

#define BUILDING_OPENDOORS

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODGen.h"
#include "ODCom.h"
#include "ODPlat.h"
#include "ODKrnl.h"
#include "ODInQue.h"
#include "ODUtil.h"


/* Filename component identifies. */
#define WILDCARDS 0x01
#define EXTENSION 0x02
#define FILENAME  0x04
#define DIRECTORY 0x08
#define DRIVE     0x10


/* Local private helper function prototypes. */
static BOOL ODListFilenameMerge(char *pszEntirePath, size_t nEntirePathSize,
   const char *pszDrive, const char *pszDir, const char *pszName,
   const char *pszExtension);
static BOOL ODListGetFirstWord(char *pszInStr, char *pszOutStr,
   size_t nOutSize);
static char *ODListGetRemainingWords(char *pszInStr);
static INT ODListFilenameSplit(const char *pszEntirePath, char *pszDrive,
   size_t nDriveSize, char *pszDir, size_t nDirSize, char *pszName,
   size_t nNameSize, char *pszExtension, size_t nExtensionSize);


/* ----------------------------------------------------------------------------
 * od_list_files()
 *
 * Displays a list of files available for download, using an extended version
 * of the standard FILES.BBS format index file.
 *
 * Parameters: pszFileSpec - Directory name where the FILES.BBS file can be
 *                           found, or full path and filename of a FILES.BBS
 *                           format index file. This string may contain no more
 *                           than 99 characters.
 *
 *     Return: TRUE on success. Returns FALSE with od_control.od_error set to
 *             ERR_LIMIT if pszFileSpec, an index filename token, or a resolved
 *             entry path is too long, or ERR_FILEOPEN if the directory or
 *             index file cannot be opened.
 */
ODAPIDEF BOOL ODCALL od_list_files(char *pszFileSpec)
{
   BYTE btLineCount = 2;
   BOOL bPausing;
   static char szLine[513];
   static char szFilename[80];
   static char szDrive[3];
   static char szDir[70];
   static char szTemp1[9];
   static char szTemp2[5];
   static char szBaseName[9];
   static char szExtension[5];
   static char szDirectory[100];
   static char szResolvedPath[100];
   INT nFilenameInfo;
   FILE *pfFilesBBS;
   static char *pszCurrent;
   BOOL bIsDir;
   BOOL bUseNextLine = TRUE;
   tODDirHandle hDir;
   tODDirEntry DirEntry;
   size_t nLineLength;
   BOOL bLineComplete;
   char chControlKey;

   /* Log function entry if running in trace mode. */
   TRACE(TRACE_API, "od_list_files()");

   /* Initialize OpenDoors if it hasn't already been done. */
   if(!bODInitialized) od_init();
   OD_RETURN_IF_SESSION_ENDED(FALSE);

   OD_API_ENTRY();

   /* Check user's page pausing setting. */
   bPausing = od_control.od_page_pausing;

   if(od_control.od_extended_info) bPausing = od_control.user_attribute & 0x04;

   /* Parse directory parameter. */
   if(pszFileSpec == NULL)
   {
      strcpy(szODWorkString, ".");
      strcpy(szDirectory, "."DIRSEP_STR);
   }
   else if(*pszFileSpec == '\0')
   {
      strcpy(szODWorkString, ".");
      strcpy(szDirectory, "."DIRSEP_STR);
   }
   else
   {
      if(strlen(pszFileSpec) >= sizeof(szDirectory))
      {
         od_control.od_error = ERR_LIMIT;
         OD_API_EXIT();
         return(FALSE);
      }
      ODStringCopy(szODWorkString, pszFileSpec, sizeof(szODWorkString));
      ODStringCopy(szDirectory, pszFileSpec, sizeof(szDirectory));
      if(szODWorkString[strlen(szODWorkString) - 1] == DIRSEP)
      {
         szODWorkString[strlen(szODWorkString) - 1] = '\0';
      }
   }

   /* Get directory information on path. */
   if(ODDirOpen(szODWorkString, DIR_ATTRIB_ARCH | DIR_ATTRIB_RDONLY
      | DIR_ATTRIB_DIREC, &hDir) != kODRCSuccess)
   {
      od_control.od_error = ERR_FILEOPEN;
      OD_API_EXIT();
      return(FALSE);
   }

   if(ODDirRead(hDir, &DirEntry) != kODRCSuccess)
   {
      ODDirClose(hDir);
      od_control.od_error = ERR_FILEOPEN;
      OD_API_EXIT();
      return(FALSE);
   }

   ODDirClose(hDir);

   /* If it is a directory. */
   if(DirEntry.wAttributes & DIR_ATTRIB_DIREC)
   {
      /* Append FILES.BBS to directory name & open. */
      bIsDir = TRUE;
      if(ODMakeFilename(szODWorkString, szODWorkString, "FILES.BBS",
         sizeof(szODWorkString)) != kODRCSuccess)
      {
         od_control.od_error = ERR_LIMIT;
         OD_API_EXIT();
         return(FALSE);
      }
      if((pfFilesBBS = fopen(szODWorkString, "r")) == NULL)
      {
         od_control.od_error = ERR_FILEOPEN;
         OD_API_EXIT();
         return(FALSE);
      }
   }

   /* If it is not a directory. */
   else
   {
      bIsDir = FALSE;
      if((pfFilesBBS = fopen(szODWorkString,"r")) == NULL)
      {
         od_control.od_error = ERR_FILEOPEN;
         OD_API_EXIT();
         return(FALSE);
      }
   }


   /* Ignore previously pressed control keys. */
   (void)ODInQueueExchangeLastControlKey(hODInputQueue, 0);


   /* Loop until the end of the FILES.BBS file has been reached. */
   for(;;)
   {
      if(fgets(szLine, 512, pfFilesBBS) == NULL) break;
      nLineLength = ODStringNormalizeLine(szLine, &bLineComplete);

      if(!bUseNextLine)
      {
         if(bLineComplete)
         {
            bUseNextLine = TRUE;
         }
         continue;
      }

      if(!bLineComplete)
      {
         bUseNextLine = FALSE;
      }

      chControlKey = ODInQueueExchangeLastControlKey(hODInputQueue, 0);
      if(chControlKey != 0)
      {
         switch(chControlKey)
         {
            case 's':
               if(od_control.od_list_stop)
               {
                  od_clear_keybuffer();
                  fclose(pfFilesBBS);
                  OD_API_EXIT();
                  return(TRUE);
               }
               break;

            case 'p':
               if(od_control.od_list_pause)
               {
                  od_clear_keybuffer();
                  od_get_key(TRUE);
                  if(!bODInitialized)
                  {
                     fclose(pfFilesBBS);
                     OD_API_EXIT();
                     return(TRUE);
                  }
               }
         }
      }

      /* The first fragment of an overlong line is incomplete too. */
      if(!bLineComplete)
      {
         continue;
      }

      /* Determine whether or not this is a comment line. */
      if(szLine[0] == ' ' || nLineLength == 0)

      {
         /* If so, display the line in comment color. */
         od_set_attrib(od_control.od_list_title_col);
         od_disp_str(szLine);
         od_disp_str("\r\n");
         ++btLineCount;
      }

      /* If the line is not a comment. */
      else                             
      {
         /* Extract the first word of the line, */
         if(!ODListGetFirstWord(szLine, szFilename, sizeof(szFilename)))
         {
            fclose(pfFilesBBS);
            od_control.od_error = ERR_LIMIT;
            OD_API_EXIT();
            return(FALSE);
         }

         /* And extract the filename. */
         nFilenameInfo = ODListFilenameSplit(szFilename, szDrive,
            sizeof(szDrive), szDir, sizeof(szDir), szBaseName,
            sizeof(szBaseName), szExtension, sizeof(szExtension));
         if(nFilenameInfo < 0)
         {
            fclose(pfFilesBBS);
            od_control.od_error = ERR_LIMIT;
            OD_API_EXIT();
            return(FALSE);
         }
         if(!((nFilenameInfo & DRIVE) || (nFilenameInfo & DIRECTORY)))
         {
            if(bIsDir)
            {
               if(ODMakeFilename(szResolvedPath, szDirectory, szFilename,
                  sizeof(szResolvedPath)) != kODRCSuccess)
               {
                  fclose(pfFilesBBS);
                  od_control.od_error = ERR_LIMIT;
                  OD_API_EXIT();
                  return(FALSE);
               }
            }
            else
            {
               if(ODListFilenameSplit(szDirectory, szDrive, sizeof(szDrive),
                  szDir, sizeof(szDir), szTemp1, sizeof(szTemp1), szTemp2,
                  sizeof(szTemp2)) < 0)
               {
                  fclose(pfFilesBBS);
                  od_control.od_error = ERR_LIMIT;
                  OD_API_EXIT();
                  return(FALSE);
               }
               if(!ODListFilenameMerge(szResolvedPath,
                  sizeof(szResolvedPath), szDrive, szDir, szBaseName,
                  szExtension))
               {
                  fclose(pfFilesBBS);
                  od_control.od_error = ERR_LIMIT;
                  OD_API_EXIT();
                  return(FALSE);
               }
            }
         }
         else
         {
            ODStringCopy(szResolvedPath, szFilename,
               sizeof(szResolvedPath));
         }

         /* Search for the filespec in directory. */
         if(ODDirOpen(szResolvedPath, DIR_ATTRIB_ARCH | DIR_ATTRIB_RDONLY,
            &hDir) == kODRCSuccess)
         {
            /* Display information on every file that matches. */
            while(ODDirRead(hDir, &DirEntry) == kODRCSuccess)
            {
               od_set_attrib(od_control.od_list_name_col);
               od_printf("%-12.12s  ", DirEntry.szFileName);
               od_set_attrib(od_control.od_list_size_col);
               od_printf("%-6ld   ", DirEntry.dwFileSize);
               od_set_attrib(od_control.od_list_comment_col);
               pszCurrent = ODListGetRemainingWords(szLine);
               if(strlen(pszCurrent) <= 56)
               {
                  od_disp_str(pszCurrent);
                  od_disp_str("\r\n");
               }
               else
               {
                  od_printf("%-56.56s\r\n", pszCurrent);
               }
               ++btLineCount;
            }

            ODDirClose(hDir);
         }

         /* Otherwise, indicate that the file is "Offline". */
         else
         {
            if(!ODListFilenameMerge(szFilename, sizeof(szFilename), "", "",
               szBaseName, szExtension))
            {
               fclose(pfFilesBBS);
               od_control.od_error = ERR_LIMIT;
               OD_API_EXIT();
               return(FALSE);
            }
            od_set_attrib(od_control.od_list_name_col);
            od_printf("%-12.12s ", szFilename);
            od_set_attrib(od_control.od_list_offline_col);
            od_disp_str(od_control.od_offline);
            od_set_attrib(od_control.od_list_comment_col);

            od_printf("%-56.56s\r\n", ODListGetRemainingWords(szLine));
            ++btLineCount;
         }
      }

      /* Check for end of screen & page pausing. */
      if(btLineCount >= od_control.user_screen_length && bPausing)
      {
         /* Provide page pausing at end of each screen. */
         if(ODPagePrompt(&bPausing))
         {
            fclose(pfFilesBBS);
            OD_API_EXIT();
            return(TRUE);
         }

         /* Reset the line number counter. */
         btLineCount = 2;
      }
   }

   /* When finished, close the file. */
   fclose(pfFilesBBS);

   /*  Return with success. */
   OD_API_EXIT();
   return(TRUE);
}


/* ----------------------------------------------------------------------------
 * ODListFilenameMerge()                               *** PRIVATE FUNCTION ***
 *
 * Builds a fully-qualified path name from the provided path component
 * strings.
 *
 * Parameters: pszEntirePath  - Pointer to the destination string where the
 *                              generated path should be stored.
 *
 *             nEntirePathSize - Size of the destination string in bytes.
 *
 *             pszDrive      - Pointer to the drive string.
 *
 *             pszDir        - Pointer to the directory string.
 *
 *             pszName       - Pointer to the base filename string.
 *
 *             pszExtension  - Pointer to the extension name string.
 *
 *     Return: TRUE if the complete path fits, or FALSE otherwise.
 */
static BOOL ODListFilenameMerge(char *pszEntirePath, size_t nEntirePathSize,
   const char *pszDrive, const char *pszDir, const char *pszName,
   const char *pszExtension)
{
   const char *apszComponents[4];
   size_t nPathLength = 0;
   size_t nComponentLength;
   INT nComponent;

   if(pszEntirePath == NULL || nEntirePathSize == 0) return(FALSE);

   pszEntirePath[0] = '\0';

   apszComponents[0] = pszDrive;
   apszComponents[1] = pszDir;
   apszComponents[2] = pszName;
   apszComponents[3] = pszExtension;
   for(nComponent = 0; nComponent < 4; ++nComponent)
   {
      if(apszComponents[nComponent] == NULL) continue;
      nComponentLength = strlen(apszComponents[nComponent]);
      if(nComponentLength > nEntirePathSize - nPathLength - 1)
      {
         pszEntirePath[0] = '\0';
         return(FALSE);
      }
      memcpy(pszEntirePath + nPathLength, apszComponents[nComponent],
         nComponentLength);
      nPathLength += nComponentLength;
      pszEntirePath[nPathLength] = '\0';
   }

   return(TRUE);
}


/* ----------------------------------------------------------------------------
 * ODListFilenameSplit()                               *** PRIVATE FUNCTION ***
 *
 * Splits the provided path string into drive, directory name, file base name
 * and file extension components. Drive prefixes are recognized on DOS and
 * Windows. On UNIX, the drive component is empty and a colon is part of the
 * filename.
 *
 * Parameters: pszEntirePath - A string containing the path to split.
 *
 *             pszDrive      - A string where the drive prefix should be
 *                             stored, or an empty string on UNIX.
 *
 *             nDriveSize    - Size of the drive string in bytes.
 *
 *             pszDir        - A string where the directory name should be
 *                             stored.
 *
 *             nDirSize      - Size of the directory string in bytes.
 *
 *             pszName       - A string where the base filename should be
 *                             stored.
 *
 *             nNameSize     - Size of the base filename string in bytes.
 *
 *             pszExtension  - A string where the filename extension should be
 *                             stored.
 *
 *             nExtensionSize - Size of the extension string in bytes.
 *
 *     Return: One or more flags indicating which components were found in the
 *             provided path name, or -1 if an output component does not fit.
 */
static INT ODListFilenameSplit(const char *pszEntirePath, char *pszDrive,
   size_t nDriveSize, char *pszDir, size_t nDirSize, char *pszName,
   size_t nNameSize, char *pszExtension, size_t nExtensionSize)
{
   char *pchCurrentPos;
   char *pchStart;
   size_t nComponentLength;
   size_t nCopyLength;
   INT nToReturn;

   ASSERT(pszEntirePath != NULL);
   ASSERT(pszDrive != NULL);
   ASSERT(pszDir != NULL);
   ASSERT(pszName != NULL);
   ASSERT(pszExtension != NULL);
   ASSERT(nDriveSize > 0);
   ASSERT(nDirSize > 0);
   ASSERT(nNameSize > 0);
   ASSERT(nExtensionSize > 0);

   pchStart = (char *)pszEntirePath;
   nToReturn = 0;

#ifdef ODPLAT_NIX
   pszDrive[0] = '\0';
#else
   if((pchCurrentPos = strrchr(pchStart,':')) == NULL)
   {
      pszDrive[0] = '\0';
   }
   else
   {
      nCopyLength = (size_t)(pchCurrentPos - pchStart) + 1;
      if(nCopyLength > 2) nCopyLength = 2;
      if(nCopyLength >= nDriveSize) return(-1);
      memcpy(pszDrive, pchStart, nCopyLength);
      pszDrive[nCopyLength] = '\0';
      pchStart = pchCurrentPos + 1;
      nToReturn |= DRIVE;
   }
#endif

   if((pchCurrentPos = strrchr(pchStart, DIRSEP))==NULL)
   {
      pszDir[0] = '\0';
   }
   else
   {
      nComponentLength = (size_t)(pchCurrentPos - pchStart) + 1;
      if(nComponentLength >= nDirSize) return(-1);
      memcpy(pszDir, pchStart, nComponentLength);
      pszDir[nComponentLength] = '\0';
      pchStart = pchCurrentPos + 1;
      nToReturn |= DIRECTORY;
   }

   if(strchr(pchStart,'*') != NULL || strchr(pchStart, '?') != NULL)
   {
      nToReturn |= WILDCARDS;
   }

   if((pchCurrentPos = strrchr(pchStart, '.')) == NULL)
   {
      if(pchStart[0] =='\0')
      {
         pszExtension[0] = '\0';
         pszName[0] = '\0';
      }
      else
      {
         pszExtension[0] = '\0';
         nCopyLength = strlen(pchStart);
         if(nCopyLength > 8) nCopyLength = 8;
         if(nCopyLength >= nNameSize) return(-1);
         memcpy(pszName, pchStart, nCopyLength);
         pszName[nCopyLength] = '\0';
         nToReturn |= FILENAME;
      }
   }
   else
   {
      nToReturn |= FILENAME;
      nToReturn |= EXTENSION;

      nCopyLength = (size_t)(pchCurrentPos - pchStart);
      if(nCopyLength > 8) nCopyLength = 8;
      if(nCopyLength >= nNameSize) return(-1);
      memcpy(pszName, pchStart, nCopyLength);
      pszName[nCopyLength] = '\0';

      nCopyLength = strlen(pchCurrentPos);
      if(nCopyLength > 4) nCopyLength = 4;
      if(nCopyLength >= nExtensionSize) return(-1);
      memcpy(pszExtension, pchCurrentPos, nCopyLength);
      pszExtension[nCopyLength] = '\0';
   }

   return(nToReturn);
}


/* ----------------------------------------------------------------------------
 * ODListGetFirstWord()                                *** PRIVATE FUNCTION ***
 *
 * Returns the first word in a string containing a series of words separated by
 * one or more spaced.
 *
 * Parameters: pszInStr  - String to look in.
 *
 *             pszOutStr - Buffer to store the first word.
 *
 *             nOutSize  - Size of the output buffer in bytes.
 *
 *     Return: TRUE if the complete word fits, or FALSE if it is too long.
 */
static BOOL ODListGetFirstWord(char *pszInStr, char *pszOutStr,
   size_t nOutSize)
{
   char *pchOut = (char *)pszOutStr;
   size_t nRemaining = nOutSize;

   ASSERT(pszInStr != NULL);
   ASSERT(pszOutStr != NULL);
   ASSERT(nOutSize > 0);

   while(*pszInStr && *pszInStr != ' ')
   {
      if(nRemaining <= 1)
      {
         pszOutStr[0] = '\0';
         return(FALSE);
      }
      *pchOut++ = *pszInStr++;
      --nRemaining;
   }
   *pchOut = '\0';

   return(TRUE);
}


/* ----------------------------------------------------------------------------
 * ODListGetRemainingWords()                           *** PRIVATE FUNCTION ***
 *
 * Obtains the remaining words in a string, after the first word. This function
 * is a companion to ODListGetFirstWord(), which obtains just the first word
 * in a string of many words.
 *
 * Parameters: pszInStr  - String to look at.
 *
 *     Return: A pointer to the position in a string of the second word.
 */
static char *ODListGetRemainingWords(char *pszInStr)
{
   char *pchStartOfRemaining = (char *)pszInStr;

   /* Skip over the first word in the string. */
   while(*pchStartOfRemaining && *pchStartOfRemaining != ' ')
   {
      ++pchStartOfRemaining;
   }

   /* Skip over any spaces after the first word. */
   while(*pchStartOfRemaining && *pchStartOfRemaining == ' ')
   {
      ++pchStartOfRemaining;
   }

   /* Return pointer to the rest of the string. */
   return((char *)pchStartOfRemaining);
 }
