/* Windows console presentation and keyboard support. */

#ifndef _INC_ODCONSOLE
#define _INC_ODCONSOLE

#include "ODTypes.h"

#ifdef ODPLAT_WIN32
BOOL ODConsoleInitialize(void);
void ODConsoleShutdown(void);
BOOL ODConsoleAvailable(void);
void ODConsoleSetSize(INT nWidth, INT nHeight, INT *pnActualWidth,
   INT *pnActualHeight);
BOOL ODConsoleWrite(const BYTE *pCells, INT nWidth, INT nHeight,
   INT nCursorColumn, INT nCursorRow, BOOL bCursorOn);
BOOL ODConsoleReadKey(WORD *pwKey, BYTE *pbtShiftStatus);
#endif

#endif
