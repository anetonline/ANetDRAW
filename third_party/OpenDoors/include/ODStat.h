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
 *        File: ODStat.h
 *
 * Description: Public interface for writing DOS status line / function key
 *              personalities.
 *
 *   Revisions: Date          Ver   Who  Change
 *              ---------------------------------------------------------------
 *              Nov 13, 1995  6.00  BP   Created.
 *              Jan 12, 1996  6.00  BP   Added ODStatStartArrowUse(), etc.
 *              Feb 19, 1996  6.00  BP   Changed version number to 6.00.
 *              Mar 03, 1996  6.10  BP   Begin version 6.10.
 */

#ifndef _INC_ODSTAT
#define _INC_ODSTAT

#include "OpenDoor.h"

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) \
   || (defined(ODPLAT_WIN32) \
      && (defined(OD_WINDOWS_CONSOLE) || defined(BUILDING_OPENDOORS)))

#ifdef __cplusplus
extern "C" {
#endif

/* Global working string available to all personalities for status line */
/* generation.                                                          */
extern char szStatusText[80];


/* Personality helper functions. */
void ODCALL ODStatAddKey(WORD wKeyCode);
void ODCALL ODStatRemoveKey(WORD wKeyCode);
void ODCALL ODStatGetUserAge(char *pszAge);
void ODCALL ODStatForceStatusUpdate(void);

/* Local-only personality screen output. */
void ODCALL ODScrnDisplayChar(unsigned char chToOutput);
void ODCALL ODScrnDisplayBuffer(const char *pBuffer, INT nCharsToDisplay);
void ODCALL ODScrnDisplayString(const char *pszString);
INT ODVCALL ODScrnPrintf(char *pszFormat, ...);
BOOL ODCALL ODScrnGetText(BYTE btLeft, BYTE btTop, BYTE btRight, BYTE btBottom,
   void *pbtBuffer);
BOOL ODCALL ODScrnPutText(BYTE btLeft, BYTE btTop, BYTE btRight, BYTE btBottom,
   void *pbtBuffer);
void ODCALL ODScrnSetCursorPos(BYTE btColumn, BYTE btRow);
void ODCALL ODScrnSetAttribute(BYTE btAttribute);

#ifdef __cplusplus
}
#endif

#endif /* ODPLAT_DOS || ODPLAT_DOS32 */


#endif /* _INC_ODSTAT */
