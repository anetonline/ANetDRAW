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
 *        File: ODScrn.c
 *
 * Description: Functions used to access the local display screen buffer, which
 *              keeps a copy of the text that is displayed on the remote
 *              terminal. The local display screen buffer also displays the
 *              OpenDoors status lines on some platforms. In addition to
 *              maintaining the current screen buffer, the odscrn.c module
 *              also contains the code to display this buffer on the screen.
 *
 *   Revisions: Date          Ver   Who  Change
 *              ---------------------------------------------------------------
 *              Oct 13, 1994  6.00  BP   New file header format.
 *              Dec 09, 1994  6.00  BP   Standardized coding style.
 *              Dec 31, 1994  6.00  BP   Remove #ifndef USEINLINE DOS code.
 *              Dec 31, 1994  6.00  BP   Use new multitasker variable.
 *              Nov 11, 1995  6.00  BP   Removed register keyword.
 *              Nov 14, 1995  6.00  BP   32-bit portability.
 *              Nov 14, 1995  6.00  BP   Created odscrn.h.
 *              Nov 14, 1995  6.00  BP   Make screen size configurable.
 *              Nov 16, 1995  6.00  BP   Removed oddoor.h, added odcore.h.
 *              Nov 21, 1995  6.00  BP   Port to Win32.
 *              Jan 20, 1996  6.00  BP   Prompt for user name if force_local.
 *              Jan 21, 1996  6.00  BP   Added ODScrnShowMessage() and related.
 *              Jan 27, 1996  6.00  BP   Expand tab ('\t') characters.
 *              Jan 27, 1996  6.00  BP   Added ODScrollUpAndInvalidate().
 *              Jan 27, 1996  6.00  BP   Made text-mode window f'ns static.
 *              Jan 31, 1996  6.00  BP   Made them non-static again.
 *              Jan 31, 1996  6.00  BP   Added ODScrnLocalInput().
 *              Feb 06, 1996  6.00  BP   Added od_silent_mode.
 *              Feb 16, 1996  6.00  BP   Make caret visible after local login.
 *              Feb 17, 1996  6.00  BP   Recognize non-ASCII keys under Win32.
 *              Feb 19, 1996  6.00  BP   Changed version number to 6.00.
 *              Feb 21, 1996  6.00  BP   Forward SC_KEYMENU to frame thread.
 *              Feb 21, 1996  6.00  BP   Don't beep in "silent mode".
 *              Mar 03, 1996  6.10  BP   Begin version 6.10.
 *              Mar 13, 1996  6.10  BP   Added od_local_win_col.
 *              Mar 17, 1996  6.10  BP   Terminate string in ODScrnLocalInput()
 *              Mar 19, 1996  6.10  BP   MSVC15 source-level compatibility.
 *              Aug 10, 2003  6.23  SH   *nix support
 */

#define BUILDING_OPENDOORS

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "OpenDoor.h"
#if defined(ODPLAT_DOS)
#include <dos.h>
#endif
#ifdef ODPLAT_DOS32
#include <i86.h>
#endif
#include "ODCore.h"
#include "ODGen.h"
#include "ODPlat.h"
#include "ODScrn.h"
#include "ODUtil.h"
#include "ODFrame.h"
#include "ODInEx.h"
#include "ODFormat.h"
#include "ODSafe.h"
#include "ODSync.h"
#include "ODVScrn.h"
#ifdef ODPLAT_WIN32
#include "ODConsole.h"
#include "ODKrnl.h"
#include "ODRes.h"
#endif /* ODPLAT_WIN32 */


/* ========================================================================= */
/* Definitions of variables used by the local screen module.                 */
/* ========================================================================= */

/* Manifest constants used in this module. */
#define SCREEN_BUFFER_SIZE          (OD_SCREEN_WIDTH * OD_SCREEN_HEIGHT * 2)
#define SCREEN_BUFFER_SEGMENT_SIZE  (SCREEN_BUFFER_SIZE / 16)
#define BYTES_PER_CHAR              2
#define BUFFER_LINE_BYTES           (OD_SCREEN_WIDTH * BYTES_PER_CHAR)
#define LINE_BUFFER_SIZE            (OD_SCREEN_WIDTH + 1)


/* Private variables used by the screen I/O functions. */

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) || defined(ODPLAT_NIX)
static void *pAllocatedBufferMemory;
#endif /* ODPLAT_DOS || ODPLAT_DOS32 || ODPLAT_NIX */

/* Far pointer to video buffer. */
static void ODFAR *pScrnBuffer;

/* Current cursor position. */
static BYTE btCursorColumn;
static BYTE btCursorRow;

/* Current output boundaries. */
static BYTE btLeftBoundary;
static BYTE btTopBoundary;
static BYTE btRightBoundary;
static BYTE btBottomBoundary;

/* Current display color. */
static BYTE btCurrentAttribute;

/* Is scrolling enabled. */
static BOOL bScrollEnabled;

#ifdef ODPLAT_DOS
/* Segment address of video buffer. */
static WORD wBufferSegment;
#endif
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
/* Display page to use. */
static BYTE btDisplayPage;
#endif /* ODPLAT_DOS || ODPLAT_DOS32 */

/* Is cursor currently on. */
static BYTE bCaretOn;

/* The presenter may temporarily hide an off-screen session cursor. Keep the
 * caller's requested local-caret state separate from that temporary state. */
static BOOL bRequestedCaretOn;
static BOOL bCaretPresentationChange;

/* Static temporary working buffer. */
static char szBuffer[LINE_BUFFER_SIZE];


/* Private function prototypes. */
static void ODScrnGetCursorPos(void);
static void ODScrnUpdateCaretPos(void);
static void ODScrnRingBell(void);
static void ODScrnScrollUpOneLine(void);
static void ODScrnScrollUpAndInvalidate(void);


/* ========================================================================= */
/* Implementation of the local screen window for the Win32 platform.         */
/* ========================================================================= */

#ifdef ODPLAT_WIN32

/* Handle to the screen window. */
static HWND hwndScreenWindow;

/* The application flow writes one buffer while the frame thread presents the
 * other. The presentation mutex is held only while ownership is exchanged or
 * while GDI reads the display buffer. */
static void *pDisplayBuffer;
static tODMutex ScreenPresentationMutex;
static BOOL bScreenPresentationActive;
static BOOL bScreenDirty;
static BYTE btDisplayCursorColumn;
static BYTE btDisplayCursorRow;
static BOOL bDisplayCaretOn;
static BOOL ODScrnPublishConsole(void);

/* Does the screen window currently have input focus? */
BOOL bScreenHasFocus;
static BOOL bWinCaretShown;

/* Current font-related information. */
static HFONT hCurrentFont;
static INT nFontCellWidth;
static INT nFontCellHeight;

/* Table to translate from PC text color values used in the screen buffer */
/* to their corresponding RGB values.                                     */
COLORREF acrPCTextColors[] =
{
   RGB(0x00, 0x00, 0x00),
   RGB(0x00, 0x00, 0xc0),
   RGB(0x00, 0xc0, 0x00),
   RGB(0x00, 0xc0, 0xc0),
   RGB(0xc0, 0x00, 0x00),
   RGB(0xc0, 0x00, 0xc0),
   RGB(0xc0, 0xc0, 0x00),
   RGB(0xc0, 0xc0, 0xc0),
   RGB(0x7f, 0x7f, 0x7f),
   RGB(0x00, 0x00, 0xff),
   RGB(0x00, 0xff, 0x00),
   RGB(0x00, 0xff, 0xff),
   RGB(0xff, 0x00, 0x00),
   RGB(0xff, 0x00, 0xff),
   RGB(0xff, 0xff, 0x00),
   RGB(0xff, 0xff, 0xff),
};

/* Table to translate from Windows key codes to OpenDoors key codes. */
typedef struct
{
   int nVirtKey;
   BYTE btODKey;
} tWinKeyToODKey;

tWinKeyToODKey aWinKeyToODKey[] =
{
   {VK_UP,        OD_KEY_UP},
   {VK_DOWN,      OD_KEY_DOWN},
   {VK_LEFT,      OD_KEY_LEFT},
   {VK_RIGHT,     OD_KEY_RIGHT},
   {VK_INSERT,    OD_KEY_INSERT},
   {VK_DELETE,    OD_KEY_DELETE},
   {VK_END,       OD_KEY_END},
   {VK_HOME,      OD_KEY_HOME},
   {VK_PRIOR,     OD_KEY_PGUP},
   {VK_NEXT,      OD_KEY_PGDN},
   {VK_F1,        OD_KEY_F1},
   {VK_F2,        OD_KEY_F2},
   {VK_F3,        OD_KEY_F3},
   {VK_F4,        OD_KEY_F4},
   {VK_F5,        OD_KEY_F5},
   {VK_F6,        OD_KEY_F6},
   {VK_F7,        OD_KEY_F7},
   {VK_F8,        OD_KEY_F8},
   {VK_F9,        OD_KEY_F9},
   {VK_F10,       OD_KEY_F10},
};

/* Utility macros. */
#define COLUMN_AS_XPIXEL(nColumn)   (((INT)(nColumn)) * nFontCellWidth)
#define ROW_AS_YPIXEL(nRow)         (((INT)(nRow)) * nFontCellHeight)
#define XPIXEL_AS_COLUMN(nX)        (((INT)(nX)) / nFontCellWidth)
#define YPIXEL_AS_ROW(nY)           (((INT)(nY)) / nFontCellHeight)

/* Height of the flashing caret, in pixels. */
#define CARET_HEIGHT   3

/* Local function prototypes. */
LRESULT CALLBACK ODScrnWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
   LPARAM lParam);
static HWND ODScrnCreateWin(HWND hwndFrame, HANDLE hInstance);
static void ODScrnPaint(HDC hdc, INT nLeft, INT nTop, INT nRight, INT nBottom);
static void ODScrnInvalidate(BYTE btLeft, BYTE btTop, BYTE btRight,
   BYTE btBottom);
static void ODScrnSetCurrentFont(HWND hwndScreen, HFONT hNewFont);
static void ODScrnSetWinCaretPos(void);


/* ----------------------------------------------------------------------------
 * ODScrnCreateWin()                                   *** PRIVATE FUNCTION ***
 *
 * Creates the local screen window, which covers the client area of the
 * OpenDoors frame window.
 *
 * Parameters: hwndFrame   - Handle to the frame window.
 *
 *             hInstance   - Handle to application instance.
 *
 *     Return: A handle to the newly created window, or NULL on failure.
 */
static HWND ODScrnCreateWin(HWND hwndFrame, HANDLE hInstance)
{
   HWND hwndScreen = NULL;
   WNDCLASS wcScreenWindow;

   ASSERT(hwndFrame != NULL);
   ASSERT(hInstance != NULL);

   /* Register the screen window's window class. */
   memset(&wcScreenWindow, 0, sizeof(wcScreenWindow));
   wcScreenWindow.style = CS_HREDRAW | CS_VREDRAW;
   wcScreenWindow.lpfnWndProc = ODScrnWindowProc;
   wcScreenWindow.cbClsExtra = 0;
   wcScreenWindow.cbWndExtra = 0;
   wcScreenWindow.hInstance = hInstance;
   wcScreenWindow.hIcon = NULL;
   wcScreenWindow.hCursor = LoadCursor(NULL, IDC_ARROW);
   wcScreenWindow.hbrBackground = NULL;
   wcScreenWindow.lpszMenuName = NULL;
   wcScreenWindow.lpszClassName = "ODScreen";

   RegisterClass(&wcScreenWindow);

   /* Create the screen window. */
   if((hwndScreen = CreateWindowEx(
      WS_EX_CLIENTEDGE,
      wcScreenWindow.lpszClassName,
      "",
      WS_CHILD | WS_BORDER,
      0,
      0,
      500,
      300,
      hwndFrame,
      NULL,
      hInstance,
      (LPVOID)hInstance)) == NULL)
   {
      /* On window creation failure, return NULL. */
      return(NULL);
   }

   /* Publish the UI-owned handle to the application flow. */
   ODMutexLock(&ScreenPresentationMutex);
   hwndScreenWindow = hwndScreen;
   ODMutexUnlock(&ScreenPresentationMutex);

   return(hwndScreen);
}


/* ----------------------------------------------------------------------------
 * ODScrnWindowProc()                                  *** PRIVATE FUNCTION ***
 *
 * The local screen window proceedure.
 *
 * Parameters: hwnd   - Handle to the local screen window.
 *
 *             uMsg   - Specifies the message.
 *
 *             wParam - Specifies additional message information. The content
 *                      of this parameter depends on the value of the uMsg
 *                      parameter.
 *
 *             lParam - Specifies additional message information. The content
 *                      of this parameter depends on the value of the uMsg
 *                      parameter.
 *
 *     Return: The return value is the result of the message processing and
 *             depends on the message.
 */
LRESULT CALLBACK ODScrnWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
   LPARAM lParam)
{
   HINSTANCE hInstance;

   ASSERT(hwnd != NULL);

   hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_USERDATA);

   switch(uMsg)
   {
      case WM_SYSCOMMAND:
         /* Move SC_KEYMENU to the frame window so its menu processes */
         /* keyboard activation when the child has input focus.       */
         if(wParam == SC_KEYMENU)
         {
            PostMessage(GetParent(hwnd), uMsg, wParam, lParam);
         }
         else
         {
            return(DefWindowProc(hwnd, uMsg, wParam, lParam));
         }
         break;

      case WM_CREATE:
      {
         CREATESTRUCT *pCreateStruct = (CREATESTRUCT *)lParam;
         hInstance = (HINSTANCE)pCreateStruct->lpCreateParams;
         SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)hInstance);
         break;
      }

      case WM_PAINT:
      {
         PAINTSTRUCT PaintStruct;
         HDC hdc;

         /* Obtain device context and paint information. */
         hdc = BeginPaint(hwnd, &PaintStruct);

         if(hdc != NULL)
         {
            /* Redraw the portion of the window that has been invalidated. */
            ODScrnPaint(hdc,
               XPIXEL_AS_COLUMN(PaintStruct.rcPaint.left),
               YPIXEL_AS_ROW(PaintStruct.rcPaint.top),
               XPIXEL_AS_COLUMN(PaintStruct.rcPaint.right),
               YPIXEL_AS_ROW(PaintStruct.rcPaint.bottom));

            /* Release device context. */
            EndPaint(hwnd, &PaintStruct);
         }
         break;
      }

      case WM_LBUTTONDOWN:
         SetFocus(hwnd);
         break;

      case WM_SETFOCUS:
         /* Turn on the caret when we receive the input focus. */
         /* First, create the caret. */
         CreateCaret(hwnd, NULL, nFontCellWidth, CARET_HEIGHT);

         /* Remember that we now have the input focus. */
         bScreenHasFocus = TRUE;
         bWinCaretShown = FALSE;

         /* Update the position and visibility from the displayed state. */
         ODMutexLock(&ScreenPresentationMutex);
         ODScrnSetWinCaretPos();
         ODMutexUnlock(&ScreenPresentationMutex);
         break;

      case WM_KILLFOCUS:
         /* Remember that we no longer have the input focus. */
         bScreenHasFocus = FALSE;
         bWinCaretShown = FALSE;

         /* Turn off the caret when we loose the input focus. */
         DestroyCaret();
         break;

      case WM_KEYDOWN:
      {
         int nVirtKeyPressed = (int)wParam;
         WORD wRepeatCount = LOWORD(lParam);
         size_t nKeyTableIndex;
         WORD wKey = 0;

         /* Look for a matching key in the OpenDoors key table. */
         for(nKeyTableIndex = 0; nKeyTableIndex < DIM(aWinKeyToODKey);
            ++nKeyTableIndex)
         {
            if(aWinKeyToODKey[nKeyTableIndex].nVirtKey == nVirtKeyPressed)
            {
               wKey = MAKEWORD(0, aWinKeyToODKey[nKeyTableIndex].btODKey);
               break;
            }
         }

         /* If a matching key was found, then add it to the queue. */
         if(wKey != 0)
         {
            while(wRepeatCount--)
            {
               ODKrnlHandleLocalKey(wKey);
            }
         }

         break;
      }

      case WM_DESTROY:
         ODMutexLock(&ScreenPresentationMutex);
         if(hwndScreenWindow == hwnd)
            hwndScreenWindow = NULL;
         ODMutexUnlock(&ScreenPresentationMutex);
         break;

      case WM_CHAR:
      {
         WORD wRepeatCount = LOWORD(lParam);
         BYTE btScanCode = LOBYTE(HIWORD(lParam));
         TCHAR chCharCode = (TCHAR)wParam;
         WORD wKey;
         
         wKey = MAKEWORD(chCharCode, btScanCode);

         /* Loop for each repitition of this key. */
         while(wRepeatCount--)
         {
            ODKrnlHandleLocalKey(wKey);
         }
         break;
      }

      default:
         /* Pass messages that we don't explicitly handle on to the */
         /* default window proc.                                    */
         return(DefWindowProc(hwnd, uMsg, wParam, lParam));
   }

   return(0);
}


/* ----------------------------------------------------------------------------
 * ODScrnPaint()                                       *** PRIVATE FUNCTION ***
 *
 * Draws the specified portion of the screen on the provided device context.
 *
 * Parameters: hdc     - Handle to the device context to draw on.
 *
 *             nLeft   - Left column to draw.
 *
 *             nTop    - Top row to draw.
 *
 *             nRight  - Right column to draw.
 *
 *             nBottom - Bottom row to draw.
 *
 *     Return: void.
 */
static void ODScrnPaint(HDC hdc, INT nLeft, INT nTop, INT nRight, INT nBottom)
{
   INT nIDSavedState;
   INT nCurrentLine;
   INT nStartColumn;
   INT nEndColumn;
   BYTE *pbtBufferContents;
   char achStringToOutput[OD_SCREEN_WIDTH];
   char *pchNextChar;
   BYTE btCurrentAttribute;

   ASSERT(hdc != NULL);
   ASSERT(nLeft >= 0);
   ASSERT(nTop >= 0);
   ASSERT(nRight >= nLeft);
   ASSERT(nBottom >= nTop);

   /* The display buffer remains owned by the frame thread until painting is
    * complete. The application flow may continue changing its other buffer, but
    * cannot publish a replacement while GDI is reading this one. */
   ODMutexLock(&ScreenPresentationMutex);

   /* Ensure that parameters are within valid range. */
   if(nRight >= OD_SCREEN_WIDTH) nRight = OD_SCREEN_WIDTH - 1;
   if(nBottom >= OD_SCREEN_HEIGHT) nBottom = OD_SCREEN_HEIGHT - 1;

   /* Save the current state of the device context so that we can restore */
   /* it before returning.                                                */
   nIDSavedState = SaveDC(hdc);

   /* Setup device context for displaying text from the screen buffer. */
   SetBkMode(hdc, OPAQUE);
   SelectObject(hdc, hCurrentFont);

   /* Loop through each line that is to be painted. */
   for(nCurrentLine = nTop; nCurrentLine <= nBottom; ++nCurrentLine)
   {
      /* Obtain a pointer to the first byte representing this line in */
      /* the screen buffer.                                           */
      pbtBufferContents = (BYTE *)pDisplayBuffer +
         ((nCurrentLine * OD_SCREEN_WIDTH) + nLeft) * 2;

      /* Loop for each portion of this line that can be drawn in a single */
      /* TextOut() call.                                                  */
      for(nStartColumn = nLeft; nStartColumn <= nRight;
         nStartColumn = nEndColumn)
      {
         /* Begin constructing a string containing the text to output */
         /* in this call to TextOut().                                */
         pchNextChar = achStringToOutput;

         /* Determine the color of this portion. */
         btCurrentAttribute = pbtBufferContents[1];

         /* Loop, finding the first column that has an incompatible color. */
         for(nEndColumn = nStartColumn; nEndColumn <= nRight; ++nEndColumn)
         {
            /* Stop looping if we come to a non-equivalent color */
            /* attribute.                                        */
            if(btCurrentAttribute != pbtBufferContents[1])
            {
               break;
            }

            /* Otherwise, add this character to the string to output. */
            *pchNextChar++ = *pbtBufferContents;

            /* Move to the next position in the buffer. */
            pbtBufferContents += 2;
         }

         /* Change current display colors to match the current color */
         /* attribute.                                               */
         SetTextColor(hdc, acrPCTextColors[btCurrentAttribute & 0x0f]);
         SetBkColor(hdc, acrPCTextColors[(btCurrentAttribute & 0xf0) >> 4]);

         /* Output the string. */
         TextOut(hdc,
            COLUMN_AS_XPIXEL(nStartColumn),
            ROW_AS_YPIXEL(nCurrentLine),
            achStringToOutput,
            (nEndColumn - nStartColumn));
      }
   }

   /* Restore the device context to its original state before this function */
   /* was called.                                                           */
   RestoreDC(hdc, nIDSavedState);
   ODScrnSetWinCaretPos();
   ODMutexUnlock(&ScreenPresentationMutex);
}


/* ----------------------------------------------------------------------------
 * ODScrnInvalidate()                                  *** PRIVATE FUNCTION ***
 *
 * Marks the application screen generation dirty. The complete generation is
 * published at the next outer API boundary.
 *
 * Parameters: btLeft   - The left most column to invalidate.
 *
 *             btTop    - The top most row to invalidate.
 *
 *             btRight  - The right most column to invalidate.
 *
 *             btBottom - The bottom most row to invalidate.
 *
 *     Return: void.
 */
static void ODScrnInvalidate(BYTE btLeft, BYTE btTop, BYTE btRight,
   BYTE btBottom)
{
   UNUSED(btLeft);
   UNUSED(btTop);
   UNUSED(btRight);
   UNUSED(btBottom);
   bScreenDirty = TRUE;
}


/* ----------------------------------------------------------------------------
 * ODScrnPublish()
 *
 * Publishes the application's completed screen state to the Windows frame
 * thread. This is called only at a serialized API boundary.
 */
void ODScrnPublish(void)
{
   void *pOldDisplayBuffer;

   if(!bScreenPresentationActive || !bScreenDirty)
      return;
   if(ODPlatGetWindowsSubsystem() == kODWindowsSubsystemConsole)
   {
      if(ODScrnPublishConsole())
         bScreenDirty = FALSE;
      return;
   }

   ODMutexLock(&ScreenPresentationMutex);
   pOldDisplayBuffer = pDisplayBuffer;
   pDisplayBuffer = pScrnBuffer;
   pScrnBuffer = pOldDisplayBuffer;
   memcpy(pScrnBuffer, pDisplayBuffer, SCREEN_BUFFER_SIZE);
   btDisplayCursorColumn = btCursorColumn + btLeftBoundary;
   btDisplayCursorRow = btCursorRow + btTopBoundary;
   bDisplayCaretOn = bCaretOn;
   bScreenDirty = FALSE;
   if(hwndScreenWindow != NULL)
      InvalidateRect(hwndScreenWindow, NULL, FALSE);
   ODMutexUnlock(&ScreenPresentationMutex);
}


/* ----------------------------------------------------------------------------
 * ODScrnSetCurrentFont()                              *** PRIVATE FUNCTION ***
 *
 * Changes the current font to be used for drawing, updating anything that
 * needs updating.
 *
 * Parameters: hwndScreen - Handle to the screen window.
 *
 *             nNewFont   - Handle to the font to switch to.
 *
 *     Return: void.
 */
static void ODScrnSetCurrentFont(HWND hwndScreen, HFONT hNewFont)
{
   HDC hdc;
   INT nIDSavedState;
   TEXTMETRIC TextMetrics;

   /* Obtain a handle to the a device context for the screen window. */
   hdc = GetDC(hwndScreen);

   /* If we are unable to obtian a device context, then return without */
   /* doing anything.                                                  */
   if(hdc == NULL)
   {
      return;
   }

   /* Change the current font. */
   hCurrentFont = hNewFont;

   /* Obtain text metrics from the device context, and then release the */
   /* device context.                                                   */
   nIDSavedState = SaveDC(hdc);
   SelectObject(hdc, hCurrentFont);
   GetTextMetrics(hdc, &TextMetrics);
   RestoreDC(hdc, nIDSavedState);
   ReleaseDC(hwndScreen, hdc);

   /* Determine the new size of a character cell. */
   nFontCellWidth = TextMetrics.tmMaxCharWidth;
   nFontCellHeight = TextMetrics.tmHeight;

   /* Force window sizes to be adjusted for the new font size. */
   ODScrnAdjustWindows();
   ODScrnAdjustWindows();
}


/* ----------------------------------------------------------------------------
 * ODScrnAdjustWindows()
 *
 * Resizes and repositions the screen window to the appropriate size based
 * on the current font, portions of the frame window's client area that are
 * in use, etc. Other windows whose size depends on the size of the screen
 * window are also updated.
 *
 * Parameters: None.
 *
 *     Return: void.
 */
void ODScrnAdjustWindows(void)
{
   INT nNewClientWidth;
   INT nNewClientHeight;
   RECT rcClient;
   RECT rcWindow;
   INT nNonClientWidth;
   INT nNonClientHeight;
   INT nScreenWindowWidth;
   INT nScreenWindowHeight;
   HWND hwndFrame;
   INT nTopFrameUsed;
   INT nBottomFrameUsed;
   HWND hwndScreen;

   hwndScreen = hwndScreenWindow;
   ASSERT(hwndScreen != NULL);

   hwndFrame = GetParent(hwndScreen);
   ASSERT(hwndFrame != NULL);

   /* Determine areas of the frame window's client area that are already */
   /* in use.                                                            */
   nTopFrameUsed = ODFrameGetUsedClientAtTop(hwndFrame);
   nBottomFrameUsed = ODFrameGetUsedClientAtBottom(hwndFrame);

   /* Determine the new required size of the window's client area. */
   nNewClientWidth = nFontCellWidth * OD_SCREEN_WIDTH;
   nNewClientHeight = nFontCellHeight * OD_SCREEN_HEIGHT;

   /* Determine the size of the window's non-client area. */
   GetClientRect(hwndScreen, &rcClient);
   GetWindowRect(hwndScreen, &rcWindow);
   nNonClientWidth = (rcWindow.right - rcWindow.left)
      - (rcClient.right - rcClient.left);
   nNonClientHeight = (rcWindow.bottom - rcWindow.top)
      - (rcClient.bottom - rcClient.top);

   /* Determine the overall size required for the screen window. */
   nScreenWindowWidth = nNewClientWidth + nNonClientWidth;
   nScreenWindowHeight = nNewClientHeight + nNonClientHeight;

   /* Resize the screen window accordingly. */
   SetWindowPos(hwndScreen, NULL, 0, nTopFrameUsed, nScreenWindowWidth,
      nScreenWindowHeight, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOZORDER);

   /* Resize the OpenDoors frame window (which is the screen window's  */
   /* parent) so that the screen window just fill's the frame window's */
   /* remaining client area.                                           */
   GetClientRect(hwndFrame, &rcClient);
   GetWindowRect(hwndFrame, &rcWindow);
   nNonClientWidth = (rcWindow.right - rcWindow.left)
      - (rcClient.right - rcClient.left);
   nNonClientHeight = (rcWindow.bottom - rcWindow.top)
      - (rcClient.bottom - rcClient.top);

   SetWindowPos(hwndFrame, NULL, 0, 0, nScreenWindowWidth + nNonClientWidth,
      nScreenWindowHeight + nNonClientHeight + nTopFrameUsed
      + nBottomFrameUsed,
      SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOZORDER);
}

/* ----------------------------------------------------------------------------
 * ODScrnStartWindow()
 *
 * Creates and initializes the local screen child on the frame UI thread.
 */
tODResult ODScrnStartWindow(HANDLE hInstance, HWND hwndFrame)
{
   tODUIState State;
   HWND hwndScreen;

   ASSERT(hInstance != NULL);
   ASSERT(hwndFrame != NULL);

   hwndScreen = ODScrnCreateWin(hwndFrame, hInstance);
   if(hwndScreen == NULL)
      return(kODRCGeneralFailure);

   ODScrnSetCurrentFont(hwndScreen, GetStockObject(OEM_FIXED_FONT));
   ODKrnlGetUIState(&State);

   if(bPromptForUserName)
   {
      if(DialogBox(hInstance, MAKEINTRESOURCE(IDD_LOGIN), hwndFrame,
         ODInitLoginDlgProc) == IDCANCEL)
      {
         return(kODRCGeneralFailure);
      }
      SetFocus(hwndScreen);
   }

   if(State.nCmdShow == SW_MINIMIZE || State.nCmdShow == SW_SHOWMINIMIZED ||
      State.nCmdShow == SW_SHOWMINNOACTIVE)
      ShowWindow(hwndFrame, SW_SHOWMINNOACTIVE);
   else
      ShowWindow(hwndFrame, SW_RESTORE);

   ShowWindow(hwndScreen, SW_SHOW);
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODScrnStopWindow()
 *
 * Destroys the local screen child on the frame UI thread.
 */
void ODScrnStopWindow(void)
{
   HWND hwndScreen;

   ODMutexLock(&ScreenPresentationMutex);
   hwndScreen = hwndScreenWindow;
   ODMutexUnlock(&ScreenPresentationMutex);
   if(hwndScreen != NULL && IsWindow(hwndScreen))
      DestroyWindow(hwndScreen);
}


/* ----------------------------------------------------------------------------
 * ODScrnSetFocusToWindow()
 *
 * Sets the current input focus to the screen window.
 *
 * Parameters: none
 *
 *     Return: void
 */
void ODScrnSetFocusToWindow(void)
{
   if(hwndScreenWindow != NULL)
   {
      SetFocus(hwndScreenWindow);
   }
}


/* ----------------------------------------------------------------------------
 * ODScrnSetWinCaretPos()
 *
 * Repositions the Windows caret to the position of our cursor, if
 * appropriate.
 *
 * Parameters: none
 *
 *     Return: void
 */
static void ODScrnSetWinCaretPos(void)
{
   /* The caller holds ScreenPresentationMutex, so cursor position and
    * visibility come from one published display generation. */
   if(bScreenHasFocus)
   {
      SetCaretPos(COLUMN_AS_XPIXEL(btDisplayCursorColumn),
         ROW_AS_YPIXEL(btDisplayCursorRow + 1) - CARET_HEIGHT);
      if(bDisplayCaretOn && !bWinCaretShown)
      {
         ShowCaret(hwndScreenWindow);
         bWinCaretShown = TRUE;
      }
      else if(!bDisplayCaretOn && bWinCaretShown)
      {
         HideCaret(hwndScreenWindow);
         bWinCaretShown = FALSE;
      }
   }
}


#endif /* ODPLAT_WIN32 */



/* ========================================================================= */
/* Functions used throughout OpenDoors to manipulate local screen buffer.    */
/* ========================================================================= */

/* ----------------------------------------------------------------------------
 * ODScrnInitialize()
 *
 * Initializes the local screen module.
 *
 * Parameters: none
 *
 *     Return: kODRCSuccess on success, or an error code on failure.
 */
tODResult ODScrnInitialize(void)
{
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   BOOL bClear = TRUE;
#endif

#ifdef ODPLAT_DOS32
   if(od_control.od_silent_mode)
   {
      pAllocatedBufferMemory = malloc(SCREEN_BUFFER_SIZE);
      if(pAllocatedBufferMemory == NULL)
         return(kODRCNoMemory);
      pScrnBuffer = pAllocatedBufferMemory;
   }
   else
   {
      union REGS Registers;
      BYTE btDisplayMode;
      DWORD dwVideoAddress;

      memset(&Registers, 0, sizeof(Registers));
      Registers.h.ah = 0x0f;
      int386(0x10, &Registers, &Registers);
      btDisplayMode = Registers.h.al & 0x7f;
      btDisplayPage = Registers.h.bh;
      if(btDisplayMode == 0x07)
      {
         dwVideoAddress = 0x000b0000UL;
      }
      else if(btDisplayMode == 0x21)
      {
         memset(&Registers, 0, sizeof(Registers));
         Registers.w.ax = 0x0007;
         int386(0x10, &Registers, &Registers);
         btDisplayPage = 0;
         bClear = FALSE;
         dwVideoAddress = 0x000b0000UL;
      }
      else
      {
         if(btDisplayMode != 0x02 && btDisplayMode != 0x03)
         {
            memset(&Registers, 0, sizeof(Registers));
            Registers.w.ax = 0x0003;
            int386(0x10, &Registers, &Registers);
            btDisplayPage = 0;
            bClear = FALSE;
         }
         dwVideoAddress = 0x000b8000UL;
      }
      pScrnBuffer = (void *)(dwVideoAddress
         + (DWORD)SCREEN_BUFFER_SIZE * btDisplayPage);
   }
#endif /* ODPLAT_DOS32 */

#if defined(ODPLAT_DOS) || defined(ODPLAT_NIX)
   /* In silent mode, we perform all output in a block of memory that is */
   /* never displayed.                                                   */
   /* *nix is always in "silent mode"                                    */
#ifndef ODPLAT_NIX
   if(od_control.od_silent_mode)
   {
#endif
      /* Allocate memory for screen buffer, using standard pointer type */
      /* for current memory model.                                      */
      pAllocatedBufferMemory = malloc(SCREEN_BUFFER_SIZE);

      if(pAllocatedBufferMemory == NULL)
      {
         return(kODRCNoMemory);
      }

      /* Set the screen buffer far pointer to point to the allocated */
      /* buffer.                                                     */
      pScrnBuffer = pAllocatedBufferMemory;
#ifndef ODPLAT_NIX
   }
   else
   {
      BYTE btDisplayMode = 0;

      /* Get current video mode. */
      ASM    push si
      ASM    push di
      ASM    mov ah, 0x0f
      ASM    int 0x10
      ASM    mov btDisplayMode, al
      ASM    pop di
      ASM    pop si

      switch(btDisplayMode & 0x7f)
      {
         /* No need to change mode, already colour 80x25. */
         case 0x02:
         case 0x03:
            wBufferSegment = 0xb800;
            pScrnBuffer = (void ODFAR *)0xb8000000L;
            bClear = TRUE;
            break;

         /* No need to change mode, already monochrome 80x25. */
         case 0x07:
            wBufferSegment = 0xb000;
            pScrnBuffer = (void ODFAR *)0xb0000000L;
            bClear = TRUE;
            break;

         /* Must change mode to monochrome 80x25. */
         case 0x21:
            wBufferSegment = 0xb000;
            pScrnBuffer = (void ODFAR *)0xb0000000L;
            bClear = FALSE;

            /* set mode to 0x07 */
            ASM    push si
            ASM    push di
            ASM    mov ax, 0x0007
            ASM    int 0x10
            ASM    pop di
            ASM    pop si
            break;

         /* Must change mode to colour 80x25. */
         default:
            wBufferSegment = 0xb800;
            pScrnBuffer = (void ODFAR *)0xb8000000L;
            bClear = FALSE;

            /* set mode to 0x03. */
            ASM    push si
            ASM    push di
            ASM    mov ax, 0x0003
            ASM    int 0x10
            ASM    pop di
            ASM    pop si
      }



      /* Adjust address for display page which is being used. */
      ASM    push si
      ASM    push di
      ASM    mov ah, 0x0f
      ASM    int 0x10
      ASM    mov btDisplayPage, bh
      ASM    pop di
      ASM    pop si

      if(btDisplayPage!=0)
      {
         wBufferSegment += (SCREEN_BUFFER_SEGMENT_SIZE * btDisplayPage);
         pScrnBuffer = (char ODFAR *)pScrnBuffer
            + (SCREEN_BUFFER_SIZE * btDisplayPage);
      }

      if(ODMultitasker == kMultitaskerDV)
      {
         /* Determine address of DV screen buffer. */
         /* This doesn't check rows, bh = rows, bl = columns. */
#ifdef __WATCOMC__
         union REGS Registers;

         Registers.x.ax = 0x2b02;
         Registers.x.cx = 0x4445;
         Registers.x.dx = 0x5351;
         intdos(&Registers, &Registers);
         if(Registers.x.bx == 0x1950)
         {
            wBufferSegment = Registers.x.dx;
            pScrnBuffer = (void ODFAR *)((unsigned long)wBufferSegment << 16);
         }
#else
         ASM    mov ax, 0x2b02
         ASM    mov cx, 0x4445
         ASM    mov dx, 0x5351
         ASM    int 0x21
         ASM    cmp bx, 0x1950
         ASM    jne no_change
         ASM    mov wBufferSegment, dx

         pScrnBuffer = MK_FP(wBufferSegment, 0);
   no_change: ;
#endif
      }
   }
#endif /* ODPLAT_DOS */
#endif /* ODPLAT_DOS/NIX */

#ifdef ODPLAT_WIN32
   /* Allocate the application screen buffer used by both presenters. */
   pScrnBuffer = malloc(SCREEN_BUFFER_SIZE);

   if(pScrnBuffer == NULL)
   {
      return(kODRCNoMemory);
   }
   if(ODPlatGetWindowsSubsystem() == kODWindowsSubsystemConsole)
   {
      if(!od_control.od_silent_mode && !ODConsoleInitialize())
      {
         if(od_control.baud == 0)
         {
            free(pScrnBuffer);
            pScrnBuffer = NULL;
            return(kODRCGeneralFailure);
         }
         od_control.od_silent_mode = TRUE;
      }
   }
   else
   {
      pDisplayBuffer = malloc(SCREEN_BUFFER_SIZE);
      if(pDisplayBuffer == NULL)
      {
         free(pScrnBuffer);
         pScrnBuffer = NULL;
         return(kODRCNoMemory);
      }
      if(ODMutexInitialize(&ScreenPresentationMutex) != kODRCSuccess)
      {
         free(pDisplayBuffer);
         free(pScrnBuffer);
         pDisplayBuffer = NULL;
         pScrnBuffer = NULL;
         return(kODRCGeneralFailure);
      }
   }
   bScreenPresentationActive = TRUE;
   bScreenDirty = FALSE;
   hwndScreenWindow = NULL;
#endif /* ODPLAT_WIN32 */

   /* Initialize display system variables. */
   btLeftBoundary = 0;
   btRightBoundary = 79;
   btTopBoundary = 0;
   btBottomBoundary = 24;
   btCurrentAttribute = 0x07;
   bScrollEnabled = 1;

   /* Clear local screen unless an existing DOS screen was preserved. */
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   if(bClear)
      ODScrnClear();
#else
   ODScrnClear();
#endif

   /* Enable flashing cursor. */
   bCaretOn = FALSE;
   ODScrnEnableCaret(TRUE);

#ifdef ODPLAT_WIN32
   /* Before the GUI exists, make both presentation buffers represent the
    * same initial generation. Console output is first published after the
    * dynamically sized session screen has been initialized. */
   if(pDisplayBuffer != NULL)
      memcpy(pDisplayBuffer, pScrnBuffer, SCREEN_BUFFER_SIZE);
   btDisplayCursorColumn = btCursorColumn + btLeftBoundary;
   btDisplayCursorRow = btCursorRow + btTopBoundary;
   bDisplayCaretOn = bCaretOn;
   bScreenDirty = ODPlatGetWindowsSubsystem() == kODWindowsSubsystemConsole;
#endif

   /* Return with success. */
   return(kODRCSuccess);
}


/* ----------------------------------------------------------------------------
 * ODScrnShutdown()
 *
 * De-initializes the screen module.
 *
 * Parameters: none
 *
 *     Return: void
 */
void ODScrnShutdown(void)
{
#ifdef ODPLAT_WIN32
   if(bScreenPresentationActive)
   {
      bScreenPresentationActive = FALSE;
      bScreenDirty = FALSE;

      /* Deallocate both buffer generations after the frame thread joined. */
      free(pScrnBuffer);
      free(pDisplayBuffer);
      pScrnBuffer = NULL;
      pDisplayBuffer = NULL;
      if(ODPlatGetWindowsSubsystem() == kODWindowsSubsystemConsole)
         ODConsoleShutdown();
      else
         ODMutexDestroy(&ScreenPresentationMutex);
   }
#else /* !ODPLAT_WIN32 */
   /* In silent mode, we must deallocate screen buffer memory. */
   /* *nix is always in silent mode                            */
#ifndef ODPLAT_NIX
   if(od_control.od_silent_mode && pAllocatedBufferMemory != NULL)
   {
#endif
      free(pAllocatedBufferMemory);
      pAllocatedBufferMemory = NULL;
      pScrnBuffer = NULL;
#ifndef ODPLAT_NIX
   }
#endif
#endif
}


/* ----------------------------------------------------------------------------
 * ODScrnSetBoundary()
 *
 * Sets the current boundary area on the screen. All output is constrained
 * within this boundary area.
 *
 * Parameters: btLeft   - 1-based column number of the left edge of the area.
 *
 *             btTop    - 1-based row number of the top edge of the area.
 *
 *             btRight  - 1-based column number of the right edge of the area.
 *
 *             btBottom - 1-based row number of the bottom edge of the area.
 *
 *     Return: void
 */
void ODScrnSetBoundary(BYTE btLeft, BYTE btTop, BYTE btRight, BYTE btBottom)
{
   if(ODSessionScreenIsEmulating())
   {
      ODSessionScreenSetBoundary(btLeft, btTop, btRight, btBottom);
      return;
   }
   /* Set internal window location variables. */
   btLeftBoundary = btLeft - 1;
   btRightBoundary = btRight - 1;
   btTopBoundary = btTop - 1;
   btBottomBoundary = btBottom - 1;

   /* Ensure that the cursor is located within the new window boundaries. */
   if(btCursorColumn > btRightBoundary - btLeftBoundary)
   {
      btCursorColumn = btRightBoundary - btLeftBoundary;
   }
   else if(btCursorColumn < btLeftBoundary)
   {
      btCursorColumn = btLeftBoundary;
   }

   if(btCursorRow > btBottomBoundary - btTopBoundary)
   {
      btCursorRow = btBottomBoundary - btTopBoundary;
   }
   else if(btCursorRow < btTopBoundary)
   {
      btCursorRow = btTopBoundary;
   }

   /* Execute the position flashing cursor primitive. */
   ODScrnUpdateCaretPos();
}


/* ----------------------------------------------------------------------------
 * ODScrnSetCursorPos()
 *
 * Sets the current cursor position. The cursor position is where the caret
 * (flashing cursor) appears (if it is currently turned on), and is the
 * location where ODScrnDisplayChar(), ODScrnDisplayString() and ODScrnPrintf()
 * will perform their output. Each of these functions, update the cursor
 * position to the next character cell after the end of their output. Other
 * ODScrn...() functions may also change the current cursor position.
 *
 * Parameters: btColumn - The 1-based column number where the cursor will
 *                        be placed.
 *
 *     Return: void
 */
void ODCALL ODScrnSetCursorPos(BYTE btColumn, BYTE btRow)
{
   if(ODSessionScreenIsEmulating())
   {
      ODSessionScreenSetCursorPos(btColumn, btRow);
      return;
   }
   /* Set internal cursor position values. */
   btCursorColumn = btColumn - 1;
   btCursorRow = btRow - 1;

   /* Ensure that cursor falls within the current output window. */
   if(btCursorColumn > btRightBoundary - btLeftBoundary)
      btCursorColumn = btRightBoundary - btLeftBoundary;

   if(btCursorRow > btBottomBoundary - btTopBoundary)
      btCursorRow = btBottomBoundary - btTopBoundary;

   /* Execute the position flashing cursor primitive. */
   ODScrnUpdateCaretPos();
}


/* ----------------------------------------------------------------------------
 * ODScrnSetAttribute()
 *
 * Sets the current display attribute, to be used by ODScrnDisplayChar(),
 * ODScrnDisplayString(), ODScrnPrintf() and ODScrnClear(). The display
 * attribute byte is always in the IBM color attribute format, with the
 * lower 4 bits indicating the foreground color, and the next 3 bits
 * indicating the background color. The upper bit specifies whether the text
 * is flashing, although this code may not actually show flashing text on
 * all platforms.
 *
 * Parameters: btAttribute - The new color attribute to use.
 *
 *     Return: void
 */
void ODCALL ODScrnSetAttribute(BYTE btAttribute)
{
   if(ODSessionScreenIsEmulating())
   {
      ODSessionScreenSetAttribute(btAttribute);
      return;
   }
   /* Set internal display colour attribute. */
   btCurrentAttribute = btAttribute;
}


/* ----------------------------------------------------------------------------
 * ODScrnEnableScrolling()
 *
 * Enables or disables scrolling of text within the currently defined boundary
 * area when a carriage return is sent with the cursor located on the bottom
 * line of bounary area.
 *
 * Parameters: bEnable - TRUE to enable scrolling, FALSE to disable scrolling.
 *
 *     Return: void
 */
void ODScrnEnableScrolling(BOOL bEnable)
{
   if(ODSessionScreenIsEmulating())
   {
      ODSessionScreenEnableScrolling(bEnable);
      return;
   }
   /* Stores the current scrolling setting. */
   bScrollEnabled = bEnable;
}


/* ----------------------------------------------------------------------------
 * ODScrnEnableCaret()
 *
 * Turns the caret (flashing indicator of the current cursor location) on or
 * off. Under the Win32 platform, the caret is always active when the
 * window has input focus, and inactive at any other time. Hene, under
 * Win32, this function has no effect.
 *
 * Parameters: bEnable - TRUE to turn on the flashing caret, FALSE to turn it
 *                       off.
 *
 *     Return: void
 */
void ODScrnEnableCaret(BOOL bEnable)
{
   if(!bCaretPresentationChange)
      bRequestedCaretOn = bEnable;

#ifdef ODPLAT_WIN32
   if(bCaretOn != (BYTE)bEnable)
   {
      bCaretOn = (BYTE)bEnable;
      bScreenDirty = TRUE;
   }
#endif

#ifdef ODPLAT_DOS
   if(bCaretOn == bEnable) return;

   bCaretOn = bEnable;

   /* Execute the cursor on / off primitive. */
#ifdef __WATCOMC__
   {
      union REGS Registers;

      Registers.h.ah = 0x03;
      Registers.h.bh = btDisplayPage;
      int86(0x10, &Registers, &Registers);
      Registers.h.ch &= 0x1f;
      if(!bCaretOn)
         Registers.h.ch |= 0x20;
      Registers.h.ah = 0x01;
      int86(0x10, &Registers, &Registers);
   }
#else
   ASM    push si
   ASM    push di
   ASM    mov ah, 0x03
   ASM    mov bh, btDisplayPage
   ASM    int 0x10

   /* ch = start line, cl = end line. */
   ASM    push cx                
   ASM    mov ah, 0x0f
   ASM    int 0x10
   ASM    pop cx

   /* al = video mode. */
   ASM    push ax
   ASM    and ch, 0x1f
   ASM    mov al, bCaretOn
   ASM    and al, al
   ASM    jnz set_cursor
   /* ch bits 5-6 = blink attr */
   /*               00 = normal */
   /*               01 = invisible */
   ASM    or ch, 0x20
set_cursor:
   ASM    pop ax
   ASM    mov bh, btDisplayPage
   ASM    mov ah, 0x01
   ASM    int 0x10
   ASM    pop di
   ASM    pop si
#endif


   if(bCaretOn)
   {
      /* Turn on the local caret, updating its position. */
      ODScrnUpdateCaretPos();
   }
   else
   {
      /* Turn off the local caret. */
      ASM    mov ah, 0x02
      ASM    mov bh, btDisplayPage
      ASM    mov dh, OD_SCREEN_HEIGHT
      ASM    mov dl, OD_SCREEN_WIDTH
      ASM    push si
      ASM    push di
      ASM    int 0x10
      ASM    pop di
      ASM    pop si
   }
#endif /* ODPLAT_DOS */

#ifdef ODPLAT_DOS32
   {
      union REGS Registers;

      if(bCaretOn == bEnable)
         return;
      bCaretOn = bEnable;
      memset(&Registers, 0, sizeof(Registers));
      Registers.h.ah = 0x03;
      Registers.h.bh = btDisplayPage;
      int386(0x10, &Registers, &Registers);
      Registers.h.ch &= 0x1f;
      if(!bCaretOn)
         Registers.h.ch |= 0x20;
      Registers.h.ah = 0x01;
      int386(0x10, &Registers, &Registers);
      if(bCaretOn)
         ODScrnUpdateCaretPos();
   }
#endif /* ODPLAT_DOS32 */
}


/* ----------------------------------------------------------------------------
 * ODScrnGetTextInfo()
 *
 * Fills a structure with information about the current display settings,
 * including the position of the current boundary area (output window),
 * color attribute and cursor location.
 *
 * Parameters: pTextInfo - Pointer to the structure to store the current text
 *                         settings information in.
 *
 *     Return: void
 */
void ODScrnGetTextInfo(tODScrnTextInfo *pTextInfo)
{
   if(ODSessionScreenIsEmulating())
   {
      tODVScreenInfo Info = { 0 };
      ODSessionScreenGetInfo(&Info);
      pTextInfo->wintop = (BYTE)(Info.wintop > 255 ? 255 : Info.wintop);
      pTextInfo->winleft = (BYTE)(Info.winleft > 255 ? 255 : Info.winleft);
      pTextInfo->winright = (BYTE)(Info.winright > 255 ? 255 : Info.winright);
      pTextInfo->winbottom = (BYTE)(Info.winbottom > 255 ? 255 : Info.winbottom);
      pTextInfo->attribute = Info.attribute;
      pTextInfo->curx = (BYTE)(Info.curx > 255 ? 255 : Info.curx);
      pTextInfo->cury = (BYTE)(Info.cury > 255 ? 255 : Info.cury);
      return;
   }
   pTextInfo->wintop = btTopBoundary + 1;
   pTextInfo->winleft = btLeftBoundary + 1;
   pTextInfo->winright = btRightBoundary + 1;
   pTextInfo->winbottom = btBottomBoundary + 1;
   pTextInfo->attribute = btCurrentAttribute;
   pTextInfo->curx = btCursorColumn + 1;
   pTextInfo->cury = btCursorRow + 1;
}


/* ----------------------------------------------------------------------------
 * ODScrnPrintf()
 *
 * Performs formatted output within the current boundary area.
 *
 * Parameters: pszFormat - Format string, which is in the same format as is
 *                         used by the standard C printf() function.
 *
 *             The semantics of additional parameters is specified by the
 *             contents of the pszFormat string.
 *
 *     Return: The standard printf() return value.
 */
INT ODVCALL ODScrnPrintf(char *pszFormat, ...)
{
   va_list pArgumentList;
   INT nToReturn;
   INT nWritten;
   size_t nBufferSize;
   char *pszBuffer = szBuffer;

   va_start(pArgumentList, pszFormat);
   nToReturn = ODVsnprintf(szBuffer, 0, pszFormat, pArgumentList);
   va_end(pArgumentList);

   if(nToReturn < 0 || !ODSizeAdd((size_t)nToReturn, 1, &nBufferSize))
      return(-1);

   if(nBufferSize > sizeof(szBuffer))
   {
      pszBuffer = (char *)malloc(nBufferSize);
      if(pszBuffer == NULL)
         return(-1);
   }

   /* Generate string to display. */
   va_start(pArgumentList, pszFormat);
   nWritten = ODVsnprintf(pszBuffer,
      pszBuffer == szBuffer ? sizeof(szBuffer) : nBufferSize,
      pszFormat, pArgumentList);
   va_end(pArgumentList);

   if(nWritten != nToReturn)
   {
      if(pszBuffer != szBuffer)
         free(pszBuffer);
      return(-1);
   }

   /* Display generated string. */
   ODScrnDisplayString(pszBuffer);

   if(pszBuffer != szBuffer)
      free(pszBuffer);

   /* Return appropriate value. */
   return (nToReturn);
}


/* ----------------------------------------------------------------------------
 * ODScrnDisplayChar()
 *
 * Writes a single character within the current boundary area, advancing the
 * cursor.
 *
 * Parameters: chToOutput  - The character to display.
 *
 *     Return: void
 */
void ODCALL ODScrnDisplayChar(unsigned char chToOutput)
{
   BYTE ODFAR *pbtDest;

   if(ODSessionScreenIsEmulating())
   {
      ODSessionScreenDisplayChar(chToOutput);
      return;
   }
   
   ODScrnGetCursorPos();

   if(btCursorColumn > btRightBoundary - btLeftBoundary)
   {
      btCursorColumn = btRightBoundary - btLeftBoundary;
   }

   if(btCursorRow > btBottomBoundary - btTopBoundary)
   {
      btCursorRow = btBottomBoundary - btTopBoundary;
   }

   switch(chToOutput)
   {
      /* If character is a carriage return. */
      case '\r':
         btCursorColumn = 0;
         break;

      /* If character is a line feed. */
      case '\n':
         /* If cursor is at bottom of output window. */
         if(btCursorRow == btBottomBoundary - btTopBoundary)
         {
            /* Scroll the screen up by one line. */
            ODScrnScrollUpAndInvalidate();
         }
         /* If cursor is not at bottom of output window. */
         else
         {
            /* Move the cursor down one line. */
            ++btCursorRow;
         }
         break;

      case '\b':
         /* If backspace. */
         if(btCursorColumn != 0) --btCursorColumn;
         break;

      case '\t':
         /* If tab character. */
         btCursorColumn = ((btCursorColumn / 8) + 1) * 8;
         if(btCursorColumn > btRightBoundary - btLeftBoundary)
         {
            btCursorColumn = 0;

            /* If moving cursor down one line advances past end of window. */
            if(++btCursorRow > btBottomBoundary - btTopBoundary)
            {
               /* Move cursor back to bottom line of window. */
               btCursorRow = btBottomBoundary - btTopBoundary;

               /* Scroll the screen up by one line. */
               ODScrnScrollUpAndInvalidate();
            }
         }
         break;

      case '\a':
         /* If bell. */
         ODScrnRingBell();
         break;

      /* If character is not a control character. */
      default:
         /* Output character to display buffer. */
         pbtDest = (BYTE ODFAR *)pScrnBuffer
            + ((btTopBoundary + btCursorRow) * BUFFER_LINE_BYTES
            + (btLeftBoundary + btCursorColumn) * BYTES_PER_CHAR);
         *pbtDest++ = chToOutput;
         *pbtDest = btCurrentAttribute;

         ASSERT(pbtDest >= (BYTE ODFAR *)pScrnBuffer);
         ASSERT(pbtDest < (BYTE ODFAR *)pScrnBuffer + SCREEN_BUFFER_SIZE);

#ifdef ODPLAT_WIN32
         /* Force the updated area of the screen window to be redrawn. */
         ODScrnInvalidate((BYTE)(btCursorColumn + btLeftBoundary),
            (BYTE)(btCursorRow + btTopBoundary),
            (BYTE)(btCursorColumn + btLeftBoundary),
            (BYTE)(btCursorRow + btTopBoundary));
#endif /* ODPLAT_WIN32 */

         /* Advance cursor. If at end of line ... */
         if(++btCursorColumn > btRightBoundary - btLeftBoundary)
         {
            /* Wrap cursor if necessary. */
            btCursorColumn = 0;

            /* If moving cursor down one line advances past end of window. */
            if(++btCursorRow > btBottomBoundary - btTopBoundary)
            {
               /* Move cursor back to bottom line of window. */
               btCursorRow = btBottomBoundary - btTopBoundary;

               /* Scroll the screen up by one line. */
               ODScrnScrollUpAndInvalidate();
            }
         }
   }

   /* Execute the update flashing cursor primitive. */
   ODScrnUpdateCaretPos();
}


/* ----------------------------------------------------------------------------
 * ODScrnGetCursorPos()                                *** PRIVATE FUNCTION ***
 *
 * Updates the current cursor position (output position) from the location of
 * the caret (flashing cursor). This function doesn't do anything on the
 * Win32 platform, since we nobody else can reposition the cursor.
 *
 * Parameters: none
 *
 *     Return: void
 */
static void ODScrnGetCursorPos(void)
{
#ifdef ODPLAT_DOS
   if(!bCaretOn) return;

   ASM    mov ah, 0x03
   ASM    mov bh, btDisplayPage
   ASM    push si
   ASM    push di
   ASM    int 0x10
   ASM    pop di
   ASM    pop si
   ASM    sub dh, btTopBoundary
   ASM    mov btCursorRow, dh
   ASM    sub dl, btLeftBoundary
   ASM    mov btCursorColumn, dl
#endif /* ODPLAT_DOS */
#ifdef ODPLAT_DOS32
   if(bCaretOn)
   {
      union REGS Registers;

      memset(&Registers, 0, sizeof(Registers));
      Registers.h.ah = 0x03;
      Registers.h.bh = btDisplayPage;
      int386(0x10, &Registers, &Registers);
      btCursorRow = Registers.h.dh - btTopBoundary;
      btCursorColumn = Registers.h.dl - btLeftBoundary;
   }
#endif /* ODPLAT_DOS32 */
}


/* ----------------------------------------------------------------------------
 * ODScrnUpdateCaretPos()                              *** PRIVATE FUNCTION ***
 *
 * Updates the position of the caret (flashing cursor) from the current cursor
 * location (output position).
 *
 * Parameters: none
 *
 *     Return: void
 */
static void ODScrnUpdateCaretPos(void)
{
#ifdef ODPLAT_DOS
   if(!bCaretOn) return;

   /* Update position of flashing cursor on screen */
   ASM    mov ah, 0x02
   ASM    mov bh, btDisplayPage
   ASM    mov dh, btCursorRow
   ASM    add dh, btTopBoundary
   ASM    mov dl, btCursorColumn
   ASM    add dl, btLeftBoundary
   ASM    push si
   ASM    push di
   ASM    int 0x10
   ASM    pop di
   ASM    pop si
#endif /* ODPLAT_DOS */

#ifdef ODPLAT_DOS32
   if(bCaretOn)
   {
      union REGS Registers;

      memset(&Registers, 0, sizeof(Registers));
      Registers.h.ah = 0x02;
      Registers.h.bh = btDisplayPage;
      Registers.h.dh = btCursorRow + btTopBoundary;
      Registers.h.dl = btCursorColumn + btLeftBoundary;
      int386(0x10, &Registers, &Registers);
   }
#endif /* ODPLAT_DOS32 */

#ifdef ODPLAT_WIN32
   bScreenDirty = TRUE;
#endif /* ODPLAT_WIN32 */
}


static void ODScrnRingBell(void)
{
   if(od_control.od_silent_mode)
      return;
   ODPlatRingBell();
}


/* ----------------------------------------------------------------------------
 * ODScrnClear()
 *
 * Clears the text within the currently defined boundary area, setting the
 * display attribute of the entire boundary area to the current display
 * color.
 *
 * Parameters: none
 *
 *     Return: void
 */
void ODScrnClear(void)
{
   WORD ODFAR *pDest;
   WORD wBlank;
   BYTE btCurColumn;
   BYTE btCurLine;
   BYTE btColumnStart;
   BYTE btSkip;

   if(ODSessionScreenIsEmulating())
   {
      ODSessionScreenClear();
      return;
   }
   pDest = (WORD ODFAR *)pScrnBuffer
      + ((btTopBoundary * OD_SCREEN_WIDTH) + btLeftBoundary);
   wBlank = (((WORD)btCurrentAttribute) << 8) | 32;
   btCurLine = (btBottomBoundary - btTopBoundary) + 1;
   btColumnStart = (btRightBoundary - btLeftBoundary) + 1;
   btSkip = OD_SCREEN_WIDTH - btColumnStart;

   /* Clear contents of current window. */
   do
   {
      btCurColumn = btColumnStart;
      do
      {
         ASSERT(pDest >= (WORD ODFAR *)pScrnBuffer);
         ASSERT(pDest <= (WORD ODFAR *)pScrnBuffer + 2000);
         *(pDest++) = wBlank;
      } while ((--btCurColumn) != 0);
      pDest += btSkip;
   } while((--btCurLine) != 0);

   /* Move cursor to top left-hand corner of current window. */
   btCursorColumn = btCursorRow = 0;

   /* Execute the update flashing cursor primitive. */
   ODScrnUpdateCaretPos();

#ifdef ODPLAT_WIN32
   /* Force the updated area of the screen window to be redrawn. */
   ODScrnInvalidate(btLeftBoundary, btTopBoundary, btRightBoundary,
      btBottomBoundary);
#endif /* ODPLAT_WIN32 */
}


/* ----------------------------------------------------------------------------
 * ODScrnScrollUpAndInvalidate()                       *** PRIVATE FUNCTION ***
 *
 * Scrolls the entire screen up by one line, only if scrolling is enabled.
 * If scrolling is performed, invalidates area that was scrolled. Scrolling
 * is accomplished using ODScrnScrollUpOneLine().
 *
 * Parameters: none
 *
 *     Return: void
 */
static void ODScrnScrollUpAndInvalidate(void)
{
   /* If scrolling is enabled. */
   if(bScrollEnabled)
   {
      /* Execute the scroll primitive. */
      ODScrnScrollUpOneLine();

#ifdef ODPLAT_WIN32
      /* Force the updated area of the screen window to be redrawn. */
      ODScrnInvalidate(btLeftBoundary, btTopBoundary, btRightBoundary,
         btBottomBoundary);
#endif /* ODPLAT_WIN32 */
   }
}


/* ----------------------------------------------------------------------------
 * ODScrnScrollUpOneLine()                             *** PRIVATE FUNCTION ***
 *
 * Scrolls the area within the current output boundary up one line, leaving the
 * newly created line at the bottom of the area blank, with the current display
 * attribute.
 *
 * Parameters: none
 *
 *     Return: void
 */
static void ODScrnScrollUpOneLine(void)
{
   WORD ODFAR *pwDest = (WORD ODFAR *)pScrnBuffer
      + (btTopBoundary * OD_SCREEN_WIDTH + btLeftBoundary);
   WORD ODFAR *pwSource;
   BYTE btCurColumn;
   BYTE btCurLine = btBottomBoundary - btTopBoundary;
   BYTE btColumnStart = btRightBoundary - btLeftBoundary + 1;
   BYTE btSkip = OD_SCREEN_WIDTH - btColumnStart;
   WORD wBlank = (((WORD)btCurrentAttribute) << 8) | 32;

   pwSource = pwDest + OD_SCREEN_WIDTH;

   ASSERT(btSkip >= 0 && btSkip <= OD_SCREEN_WIDTH);

   /* Move text in area of window up one line. */
   while(btCurLine-- != 0)
   {
      btCurColumn = btColumnStart;
      do
      {
         ASSERT(pwDest >= (WORD ODFAR *)pScrnBuffer);
         ASSERT(pwDest <= (WORD ODFAR *)pScrnBuffer + 2000);
         ASSERT(pwSource >= (WORD ODFAR *)pScrnBuffer);
         ASSERT(pwSource <= (WORD ODFAR *)pScrnBuffer+2000);
         *(pwDest++) = *(pwSource++);
      } while((--btCurColumn) != 0);
      pwDest += btSkip;
      pwSource += btSkip;
   }

   /* Clear newly created line at bottom of window. */
   btCurColumn = btColumnStart;
   do
   {
      ASSERT(pwDest >= (WORD ODFAR *)pScrnBuffer);
      ASSERT(pwDest <= (WORD ODFAR *)pScrnBuffer + 2000);
      *(pwDest++) = wBlank;
   } while((--btCurColumn) != 0);
}


/* ----------------------------------------------------------------------------
 * ODScrnGetText()
 *
 * Copies a portion of the currently displayed text and corresponding color
 * attributes to a buffer provided by the caller.
 *
 * Parameters: btLeft    - Column number of the left edge of the area to copy
 *                         from.
 *
 *             btTop     - Row number of the top edge of the area to copy from.
 *
 *             btRight   - Column number of the right edge of the area to copy
 *                         from.
 *
 *             btBottom  - Row number of the bottom edge of the area to copy
 *                         from.
 *
 *             pbtBuffer - A pointer to the buffer to copy to. It is the
 *                         caller's responsibility to ensure that this buffer
 *                         is large enough. This buffer must be at least
 *                         2 x (Width of area) x (Height of area) bytes in size.
 *
 *     Return: TRUE on success, or FALSE on failure.
 */
BOOL ODCALL ODScrnGetText(BYTE btLeft, BYTE btTop, BYTE btRight, BYTE btBottom,
   void *pbtBuffer)
{
   WORD *pwBuffer;
   WORD ODFAR *pSource;
   BYTE btCurColumn;
   BYTE btCurLine;
   BYTE btColumnStart;
   BYTE btSkip;

   if(ODSessionScreenIsEmulating())
      return(ODSessionScreenGetText(btLeft, btTop, btRight, btBottom,
         pbtBuffer));
   pwBuffer = (WORD *)pbtBuffer;
   pSource = (WORD ODFAR *)pScrnBuffer
      + ((((--btTop) + btTopBoundary)
      * OD_SCREEN_WIDTH) + btLeftBoundary + (--btLeft));
   btCurLine = (--btBottom) - btTop + 1;
   btColumnStart = (--btRight) - btLeft + 1;
   btSkip = OD_SCREEN_WIDTH - btColumnStart;

   ASSERT(btLeft >= 0);
   ASSERT(btTop >= 0);
   ASSERT(btRight <= btRightBoundary - btLeftBoundary);
   ASSERT(btBottom <= btBottomBoundary - btTopBoundary);
   ASSERT(pbtBuffer);

   /* Copy contents of screen block to buffer */
   do 
   {
      btCurColumn = btColumnStart;
      do
      {
         ASSERT(pSource >= (WORD ODFAR *)pScrnBuffer);
         ASSERT(pSource <= (WORD ODFAR *)pScrnBuffer + 2000);
         ASSERT(pwBuffer >= (WORD *)pbtBuffer);
         ASSERT(pwBuffer <= (WORD *)pbtBuffer + 2000);
         *(pwBuffer++) = *(pSource++);
      } while ((--btCurColumn) != 0);
      pSource += btSkip;
   } while((--btCurLine) != 0);

   return(TRUE);
}


/* ----------------------------------------------------------------------------
 * ODScrnPutText()
 *
 * Changes the currently displayed text and corresponding color attributes in
 * the specified area, to the values taken from the buffer. This buffer is in
 * the same format as is produce by the ODScrnGetText() function.
 *
 * Parameters: btLeft    - Column number of the left edge of the area to copy
 *                         to.
 *
 *             btTop     - Row number of the top edge of the area to copy to.
 *
 *             btRight   - Column number of the right edge of the area to copy
 *                         to.
 *
 *             btBottom  - Row number of the bottom edge of the area to copy
 *                         to.
 *
 *             pbtBuffer - A pointer to the buffer to copy from.
 *
 *     Return: TRUE on success, or FALSE on failure.
 */
BOOL ODCALL ODScrnPutText(BYTE btLeft, BYTE btTop, BYTE btRight, BYTE btBottom,
   void *pbtBuffer)
{
   WORD *pwBuffer;
   WORD ODFAR *pDest;
   BYTE btCurColumn;
   BYTE btCurLine;
   BYTE btColumnStart;
   BYTE btSkip;

   if(ODSessionScreenIsEmulating())
      return(ODSessionScreenPutText(btLeft, btTop, btRight, btBottom,
         pbtBuffer));
   pwBuffer = (WORD *)pbtBuffer;
   pDest = (WORD ODFAR *)pScrnBuffer
      + ((((--btTop) + btTopBoundary)
      * OD_SCREEN_WIDTH) + btLeftBoundary + (--btLeft));
   btCurLine = (--btBottom) - btTop + 1;
   btColumnStart = (--btRight) - btLeft + 1;
   btSkip = OD_SCREEN_WIDTH - btColumnStart;

   ASSERT(btLeft >= 0 && btTop >= 0);
   ASSERT(btLeft <= btRightBoundary - btLeftBoundary);
   ASSERT(btTop <= btBottomBoundary - btTopBoundary);
   ASSERT(btRight >= 0 && btBottom >= 0);
   ASSERT(btRight <= btRightBoundary - btLeftBoundary);
   ASSERT(btBottom <= btBottomBoundary - btTopBoundary);
   ASSERT(pbtBuffer != NULL);

   /* Copy contents of screen block to buffer. */
   do
   {
      btCurColumn = btColumnStart;
      do
      {
         ASSERT(pDest >= (WORD ODFAR *)pScrnBuffer);
         ASSERT(pDest <= (WORD ODFAR *)pScrnBuffer + 2000);
         ASSERT(pwBuffer >= (WORD *)pbtBuffer);
         ASSERT(pwBuffer <= (WORD *)pbtBuffer + 2000);
         *(pDest++) = *(pwBuffer++);
      } while ((--btCurColumn) != 0);
      pDest += btSkip;
   } while((--btCurLine) != 0);

#ifdef ODPLAT_WIN32
   /* Force the updated area of the screen window to be redrawn. */
   ODScrnInvalidate((BYTE)(btLeftBoundary + btLeft), 
      (BYTE)(btTopBoundary + btTop),
      (BYTE)(btLeftBoundary + btRight),
      (BYTE)(btTopBoundary + btBottom));
#endif /* ODPLAT_WIN32 */

   return(TRUE);
}


/* ----------------------------------------------------------------------------
 * ODScrnDisplayString()
 *
 * Copies the contents of a string to the display, using the currently set
 * color attributes. The cursor location is updated to the end of the string
 * on the screen.
 *
 * Parameters: pszString - Pointer to the string to display.
 *
 *     Return: void.
 */
void ODCALL ODScrnDisplayString(const char *pszString)
{
   if(ODSessionScreenIsEmulating())
   {
      ODSessionScreenDisplayString(pszString);
      return;
   }
   ODScrnDisplayBuffer(pszString, strlen(pszString));
}


/* ----------------------------------------------------------------------------
 * ODScrnDisplayBuffer()
 *
 * Copies the contents of a buffer to the display, using the currently set
 * color attributes. The cursor location is updated to the end of the text
 * displayed to the screen.
 *
 * Parameters: pBuffer         - Pointer to a buffer containing the
 *                               character(s) to display.
 *
 *             nCharsToDisplay - Count of number of characters to display from
 *                               the buffer.
 *
 *     Return: void.
 */
void ODCALL ODScrnDisplayBuffer(const char *pBuffer, INT nCharsToDisplay)
{
   const char *pchCurrentChar;
   INT nCharsLeft;
   BYTE ODFAR *pDest;
   BYTE btLeftColumn;
   BYTE btAttribute;
   BYTE btCurrentColumn;
   BYTE btBottom;
#ifdef ODPLAT_WIN32
   BOOL bAnythingInvalid = FALSE;
   BYTE btLeftMost;
   BYTE btRightMost;
   BYTE btTopMost;
   BYTE btBottomMost;
#endif /* ODPLAT_WIN32 */

   if(ODSessionScreenIsEmulating())
   {
      ODSessionScreenDisplayBuffer(pBuffer, nCharsToDisplay);
      return;
   }
   pchCurrentChar = pBuffer;
   nCharsLeft = nCharsToDisplay;
   btAttribute = btCurrentAttribute;
   btBottom = btBottomBoundary - btTopBoundary;

   ASSERT(pBuffer != NULL);
   ASSERT(nCharsToDisplay >= 0);

   ODScrnGetCursorPos();

   if(btCursorColumn > btRightBoundary - btLeftBoundary)
   {
      btCursorColumn = btRightBoundary - btLeftBoundary;
   }

   if(btCursorRow > btBottomBoundary - btTopBoundary)
   {
      btCursorRow = btBottomBoundary - btTopBoundary;
   }

   btCurrentColumn = btCursorColumn;

   btLeftColumn = btRightBoundary - (btCurrentColumn + btLeftBoundary);
   pDest = (BYTE ODFAR *) pScrnBuffer + (((btTopBoundary + btCursorRow)
      * BUFFER_LINE_BYTES)
      + (btLeftBoundary + btCursorColumn) * BYTES_PER_CHAR);

   while(nCharsLeft--)
   {
      ASSERT(pDest >= (BYTE ODFAR *)pScrnBuffer);
      ASSERT(pDest <= (BYTE ODFAR *)pScrnBuffer + SCREEN_BUFFER_SIZE);
      switch(*pchCurrentChar)
      {
         case '\r':
            btCurrentColumn = 0;
            btLeftColumn = btRightBoundary - btLeftBoundary;
            pDest = (BYTE ODFAR *)pScrnBuffer + ((btTopBoundary + btCursorRow)
               * BUFFER_LINE_BYTES + btLeftBoundary * BYTES_PER_CHAR);
            pchCurrentChar++;
            break;
         case '\n':
            if (btCursorRow < btBottom)
            {
               ++btCursorRow;
               pDest += BUFFER_LINE_BYTES;
            }
            else if(bScrollEnabled)
            {
               ODScrnScrollUpOneLine();
#ifdef ODPLAT_WIN32
               /* Entire boundary area is now invalid. */
               bAnythingInvalid = TRUE;
               btLeftMost = btLeftBoundary;
               btRightMost = btRightBoundary;
               btTopMost = btTopBoundary;
               btBottomMost = btBottomBoundary;
#endif /* ODPLAT_WIN32 */
            }
            pchCurrentChar++;
            break;

         case '\a':
            /* If bell */
            if(!od_control.od_silent_mode)
            {
               ODScrnRingBell();
            }
            pchCurrentChar++;
            break;

         case '\t':
            /* If tab character. */
            btCurrentColumn = ((btCurrentColumn / 8) + 1) * 8;
            if(btCurrentColumn > btRightBoundary - btLeftBoundary)
            {
               btCurrentColumn = 0;

               /* If moving cursor down one line advances past end of window. */
               if(++btCursorRow > btBottomBoundary - btTopBoundary)
               {
                  /* Move cursor back to bottom line of window. */
                  btCursorRow = btBottomBoundary - btTopBoundary;

                  /* If scrolling is enabled. */
                  if(bScrollEnabled)
                  {
                     /* Execute the scroll primitive .*/
                     ODScrnScrollUpOneLine();
#ifdef ODPLAT_WIN32
                     /* Entire boundary area is now invalid. */
                     bAnythingInvalid = TRUE;
                     btLeftMost = btLeftBoundary;
                     btRightMost = btRightBoundary;
                     btTopMost = btTopBoundary;
                     btBottomMost = btBottomBoundary;
#endif /* ODPLAT_WIN32 */
                  }
               }
            }

            /* Determine new buffer destination address. */
            pDest = (BYTE ODFAR *) pScrnBuffer
               + (((btTopBoundary + btCursorRow) * BUFFER_LINE_BYTES)
               + (btLeftBoundary + btCurrentColumn) * BYTES_PER_CHAR);
            pchCurrentChar++;
            break;

         case '\b':
            if(btCurrentColumn > 0)
            {
               --btCurrentColumn;
               pDest-=2;
               btLeftColumn++;
            }
            pchCurrentChar++;
            break;

         default:
            *(pDest++) = *(pchCurrentChar++);
            *(pDest++) = btAttribute;

#ifdef ODPLAT_WIN32
            /* Expand area to invalidate, if needed. */
            if(!bAnythingInvalid)
            {
               bAnythingInvalid = TRUE;
               btLeftMost = btLeftBoundary + btCurrentColumn;
               btRightMost = btLeftBoundary + btCurrentColumn;
               btTopMost = btTopBoundary + btCursorRow;
               btBottomMost = btTopBoundary + btCursorRow;
            }
            else
            {
               BYTE btColumn = btLeftBoundary + btCurrentColumn;
               BYTE btRow = btTopBoundary + btCursorRow;

               if(btColumn < btLeftMost) btLeftMost = btColumn;
               if(btColumn > btRightMost) btRightMost = btColumn;
               if(btRow > btBottomMost) btBottomMost = btRow;
            }
#endif /* ODPLAT_WIN32 */

            if(btLeftColumn--)
            {
               ++btCurrentColumn;
            }
            else
            {
               btCurrentColumn = 0;
               btLeftColumn = btRightBoundary - btLeftBoundary;

               if(btCursorRow < btBottom)
               {
                  ++btCursorRow;
               }
               else if(bScrollEnabled)
               {
                  ODScrnScrollUpOneLine();
#ifdef ODPLAT_WIN32
                  /* Entire boundary area is now invalid. */
                  bAnythingInvalid = TRUE;
                  btLeftMost = btLeftBoundary;
                  btRightMost = btRightBoundary;
                  btTopMost = btTopBoundary;
                  btBottomMost = btBottomBoundary;
#endif /* ODPLAT_WIN32 */
               }

               pDest = (BYTE ODFAR *)pScrnBuffer
                  + ((btTopBoundary + btCursorRow)
                  * BUFFER_LINE_BYTES + btLeftBoundary * BYTES_PER_CHAR);
            }
      }
   }

   btCursorColumn = btCurrentColumn;
   ODScrnUpdateCaretPos();

#ifdef ODPLAT_WIN32
   if(bAnythingInvalid)
   {
      /* Force the updated area of the screen window to be redrawn. */
      ODScrnInvalidate(btLeftMost, btTopMost, btRightMost,
         btBottomMost);
   }
#endif /* ODPLAT_WIN32 */
}


/* ----------------------------------------------------------------------------
 * ODScrnCopyText()
 *
 * Copies the contents of the specified area on the screen to another location
 * on the screen. The destination location must be such that the entire area
 * specified as the source can be displayed without falling off the edge of the
 * screen.
 *
 * Parameters: btLeft       - Column number of the left edge of the area to
 *                            copy from.
 *
 *             btTop        - Row number of the top edge of the area to copy
 *                            from.
 *
 *             btRight      - Column number of the right edge of the area to
 *                            copy from.
 *
 *             btBottom     - Row number of the bottom edge of the area to
 *                            copy from.
 *
 *             btDestColumn - Column number where the upper right corner of
 *                            the area should be copied to.
 *
 *             btDestRow    - Row number where the upper right cornder of the
 *                            area should be copied to.
 *
 *     Return: TRUE on success, or FALSE on failure. May fail due to
 *             insufficient available memory.
 */
BOOL ODScrnCopyText(BYTE btLeft, BYTE btTop, BYTE btRight, BYTE btBottom,
   BYTE btDestColumn, BYTE btDestRow)
{
   void *pScrnBuffer;

   if(ODSessionScreenIsEmulating())
      return(ODSessionScreenCopyText(btLeft, btTop, btRight, btBottom,
         btDestColumn, btDestRow));
   ASSERT(btLeft >= 0 && btTop >= 0);
   ASSERT(btLeft <= btRightBoundary - btLeftBoundary);
   ASSERT(btTop <= btBottomBoundary - btTopBoundary);
   ASSERT(btRight >= 0 && btBottom >= 0);
   ASSERT(btRight <= btRightBoundary - btLeftBoundary);
   ASSERT(btBottom <= btBottomBoundary - btTopBoundary);
   ASSERT(btDestColumn >= 0 && btDestRow >= 0);
   ASSERT(btDestColumn <= btRightBoundary - btLeftBoundary);
   ASSERT(btDestRow <= btBottomBoundary - btTopBoundary);

   if(btLeft > btRightBoundary - btLeftBoundary
      || btTop > btBottomBoundary - btTopBoundary
      || btRight > btRightBoundary - btLeftBoundary
      || btBottom > btBottomBoundary - btTopBoundary
      || btDestColumn > btRightBoundary - btLeftBoundary
      || btDestRow > btBottomBoundary - btTopBoundary)
   {
      return(FALSE);
   }

   if(btLeft > btRight
      || btTop > btBottom
      || btDestColumn + (btRight - btLeft)
         > btRightBoundary - btLeftBoundary
      || btDestRow + (btBottom - btTop)
         > btBottomBoundary - btTopBoundary)
   {
      return(FALSE);
   }


   if((pScrnBuffer = malloc((btRight - btLeft + 1) * (btBottom - btTop + 1)
      * BYTES_PER_CHAR)) == NULL)
   {
      /* Insufficient memory, return with failure. */
      return (FALSE);
   }

   ODScrnGetText(btLeft, btTop, btRight, btBottom, pScrnBuffer);
   ODScrnPutText(btDestColumn, btDestRow,
      (BYTE)(btRight + (btDestColumn - btLeft)),
      (BYTE)(btBottom + (btDestRow - btTop)), pScrnBuffer);
   free(pScrnBuffer);

   return(TRUE);
}


/* ----------------------------------------------------------------------------
 * ODScrnClearToEndOfLine()
 *
 * Clears the contents of the current line, from the current cursor location
 * to the end of the line.
 *
 * Parameters: none
 *
 *     Return: void
 */
void ODScrnClearToEndOfLine(void)
{
   unsigned char btCharsToDelete;
   BYTE ODFAR *pDest;
   BYTE btAttribute;

   if(ODSessionScreenIsEmulating())
   {
      ODSessionScreenClearToEndOfLine();
      return;
   }
   btCharsToDelete = btRightBoundary - (btLeftBoundary + btCursorColumn);
   pDest = (BYTE ODFAR *) pScrnBuffer
      + (((btTopBoundary + btCursorRow) * BUFFER_LINE_BYTES)
      + (btLeftBoundary + btCursorColumn) * BYTES_PER_CHAR);
   btAttribute = btCurrentAttribute;

   while(btCharsToDelete--)
   {
      *(pDest++) = ' ';
      *(pDest++) = btAttribute;
   }

#ifdef ODPLAT_WIN32
   /* Force the updated area of the screen window to be redrawn. */
   ODScrnInvalidate((BYTE)(btLeftBoundary + btCursorColumn),
      (BYTE)(btTopBoundary + btCursorRow), btRightBoundary,
      (BYTE)(btTopBoundary + btCursorRow));
#endif /* ODPLAT_WIN32 */
}


/* ----------------------------------------------------------------------------
 * ODScrnCreateWindow()
 *
 * Creates a text-based window on the local terminal.
 *
 * Parameters: btLeft           - Column numebr of the left of the window.
 *
 *             btTop            - Row number of the top of the window.
 *
 *             btRight          - Column number of the right of the window.
 *
 *             btBottom         - Row number of the bottom of the window.
 *
 *             btAttribute      - Display attribute for the window boarder and
 *                                the area inside the window.
 *
 *             pszTitle         - Pointer to a string containing the title to
 *                                display, or "" for none.
 *
 *             btTitleAttribute - Display attribute for the title text.
 *
 *     Return: void
 */
#if defined(OD_TEXTMODE) || defined(OD_HEADLESS) || defined(ODPLAT_WIN32)
void *ODScrnCreateWindow(BYTE btLeft, BYTE btTop, BYTE btRight,
   BYTE btBottom, BYTE btAttribute, char *pszTitle, BYTE btTitleAttribute)
{
   void *pUnder;
   INT nBetween;
   INT nColumn;
   INT nCount;
   INT nFirst;
   int nTitleWidth;

   ASSERT(pszTitle != NULL);

   /* Alocate space to store screen contents "under" window. */
   if((pUnder = malloc((btRight - btLeft + 1) * (btBottom - btTop + 1)
      * BYTES_PER_CHAR + 4 * sizeof(BYTE))) == NULL)
   {
      return(NULL);
   }

   /* Store the window's position in the buffer. */
   ((BYTE *)pUnder)[0] = btLeft;
   ((BYTE *)pUnder)[1] = btTop;
   ((BYTE *)pUnder)[2] = btRight;
   ((BYTE *)pUnder)[3] = btBottom;

   /* Retrieve screen contents in window area. */
   ODScrnGetText(btLeft, btTop, btRight, btBottom, ((BYTE *)pUnder) + 4);

   /* Determine area between left & right of window, distance of line before */
   /* title, and distance of line after title.                               */
   if(strlen(pszTitle) == 0)
   {
      nTitleWidth = 0;
   }
   else
   {
      nTitleWidth = strlen(pszTitle) + 2;
   }
   nCount = (nBetween = btRight - btLeft - 1) - nTitleWidth;
   nCount -= (nFirst = nCount / 2);

   /* Prepare to begin drawing window at upper left corner */
   ODScrnSetCursorPos(btLeft, btTop);
   ODScrnSetAttribute(btAttribute);

   /* Draw first line of window */
   ODScrnDisplayChar((unsigned char)214);
   while(nFirst--) ODScrnDisplayChar((unsigned char)196);
   if(strlen(pszTitle) != 0)
   {
      ODScrnSetAttribute(btTitleAttribute);
      ODScrnDisplayChar(' ');
      ODScrnDisplayString(pszTitle);
      ODScrnDisplayChar(' ');
      ODScrnSetAttribute(btAttribute);
   }
   while(nCount--) ODScrnDisplayChar((unsigned char)196);
   ODScrnDisplayChar((unsigned char)183);

   /* Draw working lines of window */
   for(nCount = btTop + 1; nCount < btBottom; ++nCount)
   {
      ODScrnSetCursorPos(btLeft, (BYTE)nCount);
      ODScrnDisplayChar((unsigned char)186);
      for(nColumn = 0; nColumn < nBetween; ++nColumn)
         ODScrnDisplayChar(' ');
      ODScrnDisplayChar((unsigned char)186);
   }

   /* Draw last line of window */
   ODScrnSetCursorPos(btLeft, btBottom);
   ODScrnDisplayChar((unsigned char)211);
   while(nBetween--) ODScrnDisplayChar((unsigned char)196);
   ODScrnDisplayChar((unsigned char)189);

   /* return pointer to buffer */
   return(pUnder);
}
#endif /* OD_TEXTMODE || OD_HEADLESS || ODPLAT_WIN32 */


/* ----------------------------------------------------------------------------
 * ODScrnDestroyWindow()
 *
 * Removes a text-based window that was created by ODScrnCreateWindow().
 *
 * Parameters: pWindow   - Pointer to the buffer returned by the corresponding
 *                         call to ODScrnCreateWindow().
 *
 *     Return: void
 */
#if defined(OD_TEXTMODE) || defined(OD_HEADLESS) || defined(ODPLAT_WIN32)
void ODScrnDestroyWindow(void *pWindow)
{
   BYTE btLeft;
   BYTE btTop;
   BYTE btRight;
   BYTE btBottom;
   BYTE *pabtWindow = (BYTE *)pWindow;

   ASSERT(pWindow != NULL);

   /* Determine the location of the window. */
   btLeft = pabtWindow[0];
   btTop = pabtWindow[1];
   btRight = pabtWindow[2];
   btBottom = pabtWindow[3];

   /* Restore original screen contents under the window. */
   ODScrnPutText(btLeft, btTop, btRight, btBottom, ((BYTE *)pWindow) + 4);

   /* Deallocate window buffer. */
   free(pWindow);
}
#endif /* OD_TEXTMODE || OD_HEADLESS || ODPLAT_WIN32 */


/* ----------------------------------------------------------------------------
 * ODScrnLocalInput()
 *
 * Inputs a string, only displaying input on local screen.
 *
 * Parameters: btLeft    - Column number of the left end of the input
 *                         field.
 *
 *             btRow     - Row number where the input field appears.
 *
 *             pszString - Location where user's input should be stored. Must
 *                         be initialized.
 *
 *             nMaxChars - The maximum number of characters that may be
 *                         accepted for input into the string.
 *
 *     Return: void
 */
#if defined(OD_TEXTMODE) || defined(ODPLAT_WIN32)
void ODScrnLocalInput(BYTE btLeft, BYTE btRow, char *pszString,
   BYTE btMaxChars)
{
   BYTE btCount;
   BYTE btCurrentPos;
   BOOL bAnyKeysPressed = FALSE;
   tODInputEvent InputEvent;

   /* Draw initial input field. */
   ODScrnSetCursorPos(btLeft, btRow);
   ODScrnDisplayString(pszString);
   for(btCount = strlen(pszString); btCount <= btMaxChars; ++btCount)
   {
      ODScrnDisplayChar(177);
   }

   /* Start with the cursor at the end of the input field. */
   btCurrentPos = strlen(pszString);

   /* Loop until the user presses enter. */
   for(;;)
   {
      /* Position the cursor at the appropriate location. */
      ODScrnSetCursorPos((BYTE)(btLeft + btCurrentPos), btRow);

#ifdef ODPLAT_WIN32
      /* Console local input runs during od_init(), before the normal outer
       * API-exit publication point exists.  Present the completed field
       * before waiting for the cooperative keyboard poll. */
      if(ODPlatGetWindowsSubsystem() == kODWindowsSubsystemConsole)
         ODScrnPublish();
#endif

      /* Obtain the next input event. */
      if(ODInQueueGetNextEvent(hODInputQueue, &InputEvent, OD_NO_TIMEOUT)
         != kODRCSuccess)
      {
         return;
      }

      switch(InputEvent.chKeyPress)
      {
         case '\b':
            /* If user presses [Backspace], then move back if we are not at */
            /* the left of the input field.                                 */
            if(btCurrentPos > 0)
            {
               /* Backspace, removing last character from string. */
               btCurrentPos--;
               ODScrnSetCursorPos((BYTE)(btLeft + btCurrentPos), btRow);
               ODScrnDisplayChar(177);
               pszString[btCurrentPos] = '\0';
            }
            break;

         case '\n':
         case '\r':
            /* If user presses [Enter], then exit from the function. */
            return;

         case '\0':
            /* In the case of a multi-character sequence, skip the next */
            /* character from the input queue.                          */
            ODInQueueGetNextEvent(hODInputQueue, &InputEvent, OD_NO_TIMEOUT);
            break;

         default:
            /* If this is a valid string character for the string. */
            if(InputEvent.chKeyPress >= ' ')
            {
               /* If no keys have been pressed yet, then erase the entire */
               /* string first.                                           */
               if(!bAnyKeysPressed)
               {
                  btCurrentPos = 0;
                  ODScrnSetCursorPos(btLeft, btRow);
                  for(btCount = 0; btCount <= btMaxChars; ++btCount)
                  {
                     ODScrnDisplayChar(177);
                  }
                  ODScrnSetCursorPos(btLeft, btRow);
               }

               /* If we are not at the end of the string, then add the */
               /* character to the string.                             */
               if(btCurrentPos < btMaxChars)
               {
                  /* Display the new character. */
                  ODScrnDisplayChar(InputEvent.chKeyPress);

                  /* Add the character to the string. */
                  pszString[btCurrentPos] = InputEvent.chKeyPress;

                  /* Update the current cursor position. */
                  ++btCurrentPos;

                  /* Terminate the string. */
                  pszString[btCurrentPos] = '\0';
               }
            }
      }

      /* Note that a key has now been pressed. */
      bAnyKeysPressed = TRUE;
   }
}
#endif /* OD_TEXTMODE || ODPLAT_WIN32 */


/* ----------------------------------------------------------------------------
 * ODScrnShowMessage()
 *
 * Displays a message window with the specified message text. Unlike the
 * Windows MessageBox() function, this message box is removed by the caller
 * of the function rather than the user.
 *
 * Parameters: pszText - Pointer to message text to be displayed. This string
 *                       must continue to exist until after the
 *                       ODScrnRemoveMessage() function is called.
 *
 *             nFlags  - Currently unused, must be 0.
 *
 *     Return: A pointer which must be passed to ODScrnRemoveMessage() in
 *             order to remove this message from the screen. A return value
 *             of NULL does not necessarily indicate window creation failure,
 *             and should still be passed to a corresponding call to
 *             ODScrnRemoveMessage().
 */
void *ODScrnShowMessage(char *pszText, int nFlags)
{
   ASSERT(pszText != NULL);
   ASSERT(nFlags == 0);

   /* In silent mode, this function does nothing. */
   if(od_control.od_silent_mode) return(NULL);

#ifdef ODPLAT_WIN32
   if(ODPlatGetWindowsSubsystem() == kODWindowsSubsystemGUI)
   {
      char *pszMessageCopy;
      size_t nMessageLength;

      nMessageLength = strlen(pszText) + 1;
      pszMessageCopy = (char *)malloc(nMessageLength);
      if(pszMessageCopy == NULL)
         return(NULL);
      memcpy(pszMessageCopy, pszText, nMessageLength);
      if(!PostMessage(GetParent(hwndScreenWindow), WM_SHOW_MESSAGE,
         (WPARAM)nFlags, (LPARAM)pszMessageCopy))
      {
         free(pszMessageCopy);
      }
      return(NULL);
   }
#endif /* ODPLAT_WIN32 */
   {
      int nWindowWidth;
      int nLeftColumn;
      char szMessage[74];
      void *pWindow;

      UNUSED(nFlags);

      ODStringCopy(szMessage, pszText, sizeof(szMessage));

      ODStoreTextInfo();

      nWindowWidth = strlen(szMessage) + 4;
      nLeftColumn = 40 - (nWindowWidth / 2);
      if((pWindow = ODScrnCreateWindow((BYTE)nLeftColumn, 10,
         (BYTE)(nLeftColumn + (nWindowWidth - 1)), 14,
         od_control.od_local_win_col, "", od_control.od_local_win_col))
         == NULL)
      {
         return(NULL);
      }

      ODScrnSetCursorPos((BYTE)(42 - (nWindowWidth / 2)), 12);
      ODScrnDisplayString(szMessage);
      ODRestoreTextInfo();

      ODScrnEnableCaret(FALSE);

      return(pWindow);
   }
}


/* ----------------------------------------------------------------------------
 * ODScrnRemoveMessage()
 *
 * Removes a message that was shown by a previous call to ODScrnShowMessage().
 *
 * Parameters: pMessageInfo - Pointer to the buffer returned by the
 *                            corresponding call to ODScrnShowMessage().
 *
 *     Return: void
 */
void ODScrnRemoveMessage(void *pMessageInfo)
{
   /* In silent mode, this function does nothing. */
   if(od_control.od_silent_mode) return;

#ifdef ODPLAT_WIN32
   if(ODPlatGetWindowsSubsystem() == kODWindowsSubsystemGUI)
   {
      PostMessage(GetParent(hwndScreenWindow), WM_REMOVE_MESSAGE, 0, 0L);
      return;
   }
#endif /* ODPLAT_WIN32 */
   /* If pMessageInfo is NULL, then we do nothing. */
   if(pMessageInfo == NULL) return;

   ODStoreTextInfo();
   ODScrnDestroyWindow(pMessageInfo);
   ODRestoreTextInfo();
   ODScrnEnableCaret(TRUE);
}


/* ========================================================================= */
/* Private, dynamically-sized session screen.                                */
/*                                                                           */
/* ODScrn remains the fixed local-screen compatibility interface.  Remote     */
/* semantic output is tracked here and is painted into ODScrn only through    */
/* ODSessionScreenPresent().                                                  */
/* ========================================================================= */

typedef struct
{
   BYTE ODFAR *pCells;
   INT nWidth;
   INT nHeight;
   INT nLeft;
   INT nTop;
   INT nRight;
   INT nBottom;
   INT nCursorColumn;
   INT nCursorRow;
   BYTE btAttribute;
   BOOL bScrolling;
   BOOL bDirty;
   INT nDirtyLeft;
   INT nDirtyTop;
   INT nDirtyRight;
   INT nDirtyBottom;
} tODSessionScreen;

static tODSessionScreen SessionScreen;
static BOOL bSessionScreenAvailable;
static BOOL bSessionScreenEmulating;
static INT nSessionScreenError = ERR_NONE;

static BOOL ODSessionScreenRectValid(INT nLeft, INT nTop, INT nRight,
   INT nBottom)
{
   INT nWindowWidth;
   INT nWindowHeight;

   nWindowWidth = SessionScreen.nRight - SessionScreen.nLeft + 1;
   nWindowHeight = SessionScreen.nBottom - SessionScreen.nTop + 1;
   return(nLeft >= 1 && nTop >= 1 && nLeft <= nRight && nTop <= nBottom
      && nRight <= nWindowWidth && nBottom <= nWindowHeight);
}

static BYTE ODFAR *ODSessionScreenCell(INT nColumn, INT nRow)
{
   size_t nCell;

   nCell = ((size_t)nRow * (size_t)SessionScreen.nWidth) + (size_t)nColumn;
   return(SessionScreen.pCells + nCell * 2U);
}

#ifdef ODPLAT_WIN32
static void ODScrnConsoleCopyPersonalityRow(BYTE *pDestination, INT nWidth,
   INT nSourceRow)
{
   BYTE *pSource;
   BYTE btLeftAttribute;
   BYTE btRightAttribute;
   INT nCopyWidth;
   INT nOffset;
   INT nColumn;

   pSource = (BYTE *)pScrnBuffer + nSourceRow * BUFFER_LINE_BYTES;
   nCopyWidth = nWidth < OD_SCREEN_WIDTH ? nWidth : OD_SCREEN_WIDTH;
   nOffset = nWidth > OD_SCREEN_WIDTH ? (nWidth - OD_SCREEN_WIDTH) / 2 : 0;
   btLeftAttribute = pSource[1] & 0xf0;
   btRightAttribute = pSource[(OD_SCREEN_WIDTH - 1) * 2 + 1] & 0xf0;
   for(nColumn = 0; nColumn < nOffset; ++nColumn)
   {
      pDestination[nColumn * 2] = ' ';
      pDestination[nColumn * 2 + 1] = btLeftAttribute;
   }
   memcpy(pDestination + nOffset * 2, pSource, (size_t)nCopyWidth * 2U);
   for(nColumn = nOffset + nCopyWidth; nColumn < nWidth; ++nColumn)
   {
      pDestination[nColumn * 2] = ' ';
      pDestination[nColumn * 2 + 1] = btRightAttribute;
   }
}

static BOOL ODScrnPublishConsole(void)
{
   BYTE *pCells;
   BYTE *pDestination;
   BYTE *pSource;
   INT nRequestedWidth;
   INT nRequestedHeight;
   INT nWidth;
   INT nHeight;
   INT nRemoteHeight;
   INT nTopRows;
   INT nBottomRows;
   INT nRow;
   INT nColumn;
   INT nCursorColumn;
   INT nCursorRow;

   if(!ODConsoleAvailable() || od_control.od_silent_mode)
      return(TRUE);
   nRequestedWidth = bSessionScreenAvailable ? SessionScreen.nWidth
      : od_control.user_screenwidth;
   nRemoteHeight = bSessionScreenAvailable ? SessionScreen.nHeight
      : od_control.user_screen_length;
   nTopRows = btOutputTop > 0 ? btOutputTop - 1 : 0;
   nBottomRows = btOutputBottom < OD_SCREEN_HEIGHT
      ? OD_SCREEN_HEIGHT - btOutputBottom : 0;
   nRequestedHeight = nRemoteHeight + nTopRows + nBottomRows;
   ODConsoleSetSize(nRequestedWidth, nRequestedHeight, &nWidth, &nHeight);
   pCells = (BYTE *)malloc((size_t)nWidth * (size_t)nHeight * 2U);
   if(pCells == NULL)
      return(FALSE);
   for(nRow = 0; nRow < nHeight; ++nRow)
   {
      pDestination = pCells + (size_t)nRow * (size_t)nWidth * 2U;
      for(nColumn = 0; nColumn < nWidth; ++nColumn)
      {
         pDestination[nColumn * 2] = ' ';
         pDestination[nColumn * 2 + 1] = 0x07;
      }
      if(!bSessionScreenAvailable)
      {
         /* Local mode uses the same fixed 80x25 buffer as DOS. */
         if(nRow < OD_SCREEN_HEIGHT)
            ODScrnConsoleCopyPersonalityRow(pDestination, nWidth, nRow);
      }
      else if(nRow < nTopRows)
         ODScrnConsoleCopyPersonalityRow(pDestination, nWidth, nRow);
      else if(nRow < nTopRows + nRemoteHeight)
      {
         nColumn = nWidth < SessionScreen.nWidth
            ? nWidth : SessionScreen.nWidth;
         pSource = ODSessionScreenCell(0, nRow - nTopRows);
         memcpy(pDestination, pSource, (size_t)nColumn * 2U);
      }
      else
      {
         INT nPersonalityRow = btOutputBottom
            + nRow - nTopRows - nRemoteHeight;
         if(nPersonalityRow < OD_SCREEN_HEIGHT)
            ODScrnConsoleCopyPersonalityRow(pDestination, nWidth,
               nPersonalityRow);
      }
   }
   nCursorColumn = bSessionScreenAvailable
      ? SessionScreen.nLeft + SessionScreen.nCursorColumn : btCursorColumn;
   nCursorRow = bSessionScreenAvailable
      ? nTopRows + SessionScreen.nTop + SessionScreen.nCursorRow
      : btCursorRow;
   if(!ODConsoleWrite(pCells, nWidth, nHeight, nCursorColumn, nCursorRow,
      bCaretOn))
   {
      free(pCells);
      return(FALSE);
   }
   free(pCells);
   return(TRUE);
}
#endif /* ODPLAT_WIN32 */

static void ODSessionScreenMarkDirty(INT nLeft, INT nTop, INT nRight,
   INT nBottom)
{
   if(!SessionScreen.bDirty)
   {
      SessionScreen.bDirty = TRUE;
      SessionScreen.nDirtyLeft = nLeft;
      SessionScreen.nDirtyTop = nTop;
      SessionScreen.nDirtyRight = nRight;
      SessionScreen.nDirtyBottom = nBottom;
      return;
   }
   if(nLeft < SessionScreen.nDirtyLeft)
      SessionScreen.nDirtyLeft = nLeft;
   if(nTop < SessionScreen.nDirtyTop)
      SessionScreen.nDirtyTop = nTop;
   if(nRight > SessionScreen.nDirtyRight)
      SessionScreen.nDirtyRight = nRight;
   if(nBottom > SessionScreen.nDirtyBottom)
      SessionScreen.nDirtyBottom = nBottom;
}

void ODSessionScreenInitialize(INT nMinimumWidth, INT nMinimumHeight)
{
   INT nWidth;
   INT nHeight;
   unsigned long ulCells;
   unsigned long ulBytes;
   size_t nBytes;
   BYTE ODFAR *pCurrent;
   unsigned long ulCount;

   memset(&SessionScreen, 0, sizeof(SessionScreen));
   bSessionScreenAvailable = FALSE;
   bSessionScreenEmulating = FALSE;
   nSessionScreenError = ERR_NONE;

   if(od_control.baud == 0)
      return;

   nWidth = od_control.user_screenwidth;
   nHeight = od_control.user_screen_length;
   if(nWidth < nMinimumWidth)
      nWidth = nMinimumWidth;
   if(nHeight < nMinimumHeight)
      nHeight = nMinimumHeight;
   if(nWidth < 1 || nHeight < 1)
   {
      nSessionScreenError = ERR_LIMIT;
      return;
   }

   ulCells = (unsigned long)nWidth * (unsigned long)nHeight;
#ifndef ODPLAT_DOS
   if(ulCells > 0xffffffffUL / 2UL)
   {
      nSessionScreenError = ERR_LIMIT;
      return;
   }
#endif
   ulBytes = ulCells * 2UL;
#ifdef ODPLAT_DOS
   /* Leave room for the extended snapshot header in one DOS object. */
   if(ulBytes > 65486UL)
   {
      nSessionScreenError = ERR_LIMIT;
      return;
   }
#endif
   nBytes = (size_t)ulBytes;

   SessionScreen.pCells = (BYTE ODFAR *)malloc(nBytes);
   if(SessionScreen.pCells == NULL)
   {
      nSessionScreenError = ERR_MEMORY;
      return;
   }

   SessionScreen.nWidth = nWidth;
   SessionScreen.nHeight = nHeight;
   SessionScreen.nLeft = 0;
   SessionScreen.nTop = 0;
   SessionScreen.nRight = nWidth - 1;
   SessionScreen.nBottom = nHeight - 1;
   SessionScreen.btAttribute = 0x07;
   SessionScreen.bScrolling = TRUE;

   pCurrent = SessionScreen.pCells;
   for(ulCount = 0; ulCount < ulCells; ++ulCount)
   {
      *pCurrent++ = ' ';
      *pCurrent++ = 0x07;
   }
   bSessionScreenAvailable = TRUE;
}

void ODSessionScreenShutdown(void)
{
   if(SessionScreen.pCells != NULL)
      free(SessionScreen.pCells);
   memset(&SessionScreen, 0, sizeof(SessionScreen));
   bSessionScreenAvailable = FALSE;
   bSessionScreenEmulating = FALSE;
}

BOOL ODSessionScreenAvailable(void)
{
   return(bSessionScreenAvailable);
}

INT ODSessionScreenError(void)
{
   return(nSessionScreenError);
}

INT ODSessionScreenWidth(void)
{
   return(SessionScreen.nWidth);
}

INT ODSessionScreenHeight(void)
{
   return(SessionScreen.nHeight);
}

void ODSessionScreenGetInfo(tODVScreenInfo *pInfo)
{
   if(pInfo == NULL || !bSessionScreenAvailable)
      return;
   pInfo->winleft = SessionScreen.nLeft + 1;
   pInfo->wintop = SessionScreen.nTop + 1;
   pInfo->winright = SessionScreen.nRight + 1;
   pInfo->winbottom = SessionScreen.nBottom + 1;
   pInfo->attribute = SessionScreen.btAttribute;
   pInfo->curx = SessionScreen.nCursorColumn + 1;
   pInfo->cury = SessionScreen.nCursorRow + 1;
   pInfo->scrolling = SessionScreen.bScrolling;
}

void ODSessionScreenSetBoundary(INT nLeft, INT nTop, INT nRight, INT nBottom)
{
   if(!bSessionScreenAvailable)
      return;
   if(nLeft < 1 || nTop < 1 || nLeft > nRight || nTop > nBottom
      || nRight > SessionScreen.nWidth || nBottom > SessionScreen.nHeight)
      return;
   SessionScreen.nLeft = nLeft - 1;
   SessionScreen.nTop = nTop - 1;
   SessionScreen.nRight = nRight - 1;
   SessionScreen.nBottom = nBottom - 1;
   if(SessionScreen.nCursorColumn > nRight - nLeft)
      SessionScreen.nCursorColumn = nRight - nLeft;
   if(SessionScreen.nCursorRow > nBottom - nTop)
      SessionScreen.nCursorRow = nBottom - nTop;
}

void ODSessionScreenSetCursorPos(INT nColumn, INT nRow)
{
   INT nWidth;
   INT nHeight;

   if(!bSessionScreenAvailable)
      return;
   nWidth = SessionScreen.nRight - SessionScreen.nLeft + 1;
   nHeight = SessionScreen.nBottom - SessionScreen.nTop + 1;
   if(nColumn < 1)
      nColumn = 1;
   if(nRow < 1)
      nRow = 1;
   if(nColumn > nWidth)
      nColumn = nWidth;
   if(nRow > nHeight)
      nRow = nHeight;
   SessionScreen.nCursorColumn = nColumn - 1;
   SessionScreen.nCursorRow = nRow - 1;
}

void ODSessionScreenSetAttribute(BYTE btAttribute)
{
   if(bSessionScreenAvailable)
      SessionScreen.btAttribute = btAttribute;
}

void ODSessionScreenEnableScrolling(BOOL bEnable)
{
   if(bSessionScreenAvailable)
      SessionScreen.bScrolling = bEnable;
}

static void ODSessionScreenScrollUpOneLine(void)
{
   INT nRow;
   INT nWidth;
   BYTE ODFAR *pDest;
   BYTE ODFAR *pSource;
   INT nColumn;

   if(!SessionScreen.bScrolling)
      return;
   nWidth = SessionScreen.nRight - SessionScreen.nLeft + 1;
   for(nRow = SessionScreen.nTop; nRow < SessionScreen.nBottom; ++nRow)
   {
      pDest = ODSessionScreenCell(SessionScreen.nLeft, nRow);
      pSource = ODSessionScreenCell(SessionScreen.nLeft, nRow + 1);
      memmove(pDest, pSource, (size_t)nWidth * 2U);
   }
   pDest = ODSessionScreenCell(SessionScreen.nLeft, SessionScreen.nBottom);
   for(nColumn = 0; nColumn < nWidth; ++nColumn)
   {
      *pDest++ = ' ';
      *pDest++ = SessionScreen.btAttribute;
   }
   ODSessionScreenMarkDirty(SessionScreen.nLeft, SessionScreen.nTop,
      SessionScreen.nRight, SessionScreen.nBottom);
}

void ODSessionScreenDisplayChar(unsigned char chToOutput)
{
   BYTE ODFAR *pDest;
   INT nWidth;
   INT nHeight;

   if(!bSessionScreenAvailable)
      return;
   nWidth = SessionScreen.nRight - SessionScreen.nLeft + 1;
   nHeight = SessionScreen.nBottom - SessionScreen.nTop + 1;
   switch(chToOutput)
   {
      case '\r':
         SessionScreen.nCursorColumn = 0;
         break;
      case '\n':
         if(SessionScreen.nCursorRow == nHeight - 1)
            ODSessionScreenScrollUpOneLine();
         else
            ++SessionScreen.nCursorRow;
         break;
      case '\b':
         if(SessionScreen.nCursorColumn != 0)
            --SessionScreen.nCursorColumn;
         break;
      case '\t':
         SessionScreen.nCursorColumn =
            ((SessionScreen.nCursorColumn / 8) + 1) * 8;
         if(SessionScreen.nCursorColumn >= nWidth)
         {
            SessionScreen.nCursorColumn = 0;
            if(++SessionScreen.nCursorRow >= nHeight)
            {
               SessionScreen.nCursorRow = nHeight - 1;
               ODSessionScreenScrollUpOneLine();
            }
         }
         break;
      case '\a':
         ODScrnRingBell();
         break;
      default:
         pDest = ODSessionScreenCell(
            SessionScreen.nLeft + SessionScreen.nCursorColumn,
            SessionScreen.nTop + SessionScreen.nCursorRow);
         *pDest++ = chToOutput;
         *pDest = SessionScreen.btAttribute;
         ODSessionScreenMarkDirty(
            SessionScreen.nLeft + SessionScreen.nCursorColumn,
            SessionScreen.nTop + SessionScreen.nCursorRow,
            SessionScreen.nLeft + SessionScreen.nCursorColumn,
            SessionScreen.nTop + SessionScreen.nCursorRow);
         if(++SessionScreen.nCursorColumn >= nWidth)
         {
            SessionScreen.nCursorColumn = 0;
            if(++SessionScreen.nCursorRow >= nHeight)
            {
               SessionScreen.nCursorRow = nHeight - 1;
               ODSessionScreenScrollUpOneLine();
            }
         }
         break;
   }
}

void ODSessionScreenDisplayBuffer(const char *pBuffer, INT nCharsToDisplay)
{
   while(nCharsToDisplay-- > 0)
      ODSessionScreenDisplayChar((unsigned char)*pBuffer++);
}

void ODSessionScreenDisplayString(const char *pszString)
{
   if(pszString != NULL)
      ODSessionScreenDisplayBuffer(pszString, (INT)strlen(pszString));
}

void ODSessionScreenClear(void)
{
   INT nRow;
   INT nColumn;
   BYTE ODFAR *pDest;

   if(!bSessionScreenAvailable)
      return;
   for(nRow = SessionScreen.nTop; nRow <= SessionScreen.nBottom; ++nRow)
   {
      pDest = ODSessionScreenCell(SessionScreen.nLeft, nRow);
      for(nColumn = SessionScreen.nLeft;
         nColumn <= SessionScreen.nRight; ++nColumn)
      {
         *pDest++ = ' ';
         *pDest++ = SessionScreen.btAttribute;
      }
   }
   SessionScreen.nCursorColumn = 0;
   SessionScreen.nCursorRow = 0;
   ODSessionScreenMarkDirty(SessionScreen.nLeft, SessionScreen.nTop,
      SessionScreen.nRight, SessionScreen.nBottom);
}

void ODSessionScreenClearToEndOfLine(void)
{
   INT nColumn;
   BYTE ODFAR *pDest;

   if(!bSessionScreenAvailable)
      return;
   pDest = ODSessionScreenCell(SessionScreen.nLeft
      + SessionScreen.nCursorColumn,
      SessionScreen.nTop + SessionScreen.nCursorRow);
   for(nColumn = SessionScreen.nLeft + SessionScreen.nCursorColumn;
      nColumn <= SessionScreen.nRight; ++nColumn)
   {
      *pDest++ = ' ';
      *pDest++ = SessionScreen.btAttribute;
   }
   ODSessionScreenMarkDirty(
      SessionScreen.nLeft + SessionScreen.nCursorColumn,
      SessionScreen.nTop + SessionScreen.nCursorRow,
      SessionScreen.nRight,
      SessionScreen.nTop + SessionScreen.nCursorRow);
}

BOOL ODSessionScreenGetText(INT nLeft, INT nTop, INT nRight, INT nBottom,
   void *pBuffer)
{
   INT nRow;
   INT nWidth;
   BYTE *pDest;
   BYTE ODFAR *pSource;

   if(!bSessionScreenAvailable || pBuffer == NULL
      || !ODSessionScreenRectValid(nLeft, nTop, nRight, nBottom))
      return(FALSE);
   nWidth = nRight - nLeft + 1;
   pDest = (BYTE *)pBuffer;
   for(nRow = nTop - 1; nRow < nBottom; ++nRow)
   {
      pSource = ODSessionScreenCell(SessionScreen.nLeft + nLeft - 1,
         SessionScreen.nTop + nRow);
      memcpy(pDest, pSource, (size_t)nWidth * 2U);
      pDest += nWidth * 2;
   }
   return(TRUE);
}

BOOL ODSessionScreenPutText(INT nLeft, INT nTop, INT nRight, INT nBottom,
   const void *pBuffer)
{
   INT nRow;
   INT nWidth;
   const BYTE *pSource;
   BYTE ODFAR *pDest;

   if(!bSessionScreenAvailable || pBuffer == NULL
      || !ODSessionScreenRectValid(nLeft, nTop, nRight, nBottom))
      return(FALSE);
   nWidth = nRight - nLeft + 1;
   pSource = (const BYTE *)pBuffer;
   for(nRow = nTop - 1; nRow < nBottom; ++nRow)
   {
      pDest = ODSessionScreenCell(SessionScreen.nLeft + nLeft - 1,
         SessionScreen.nTop + nRow);
      memcpy(pDest, pSource, (size_t)nWidth * 2U);
      pSource += nWidth * 2;
   }
   ODSessionScreenMarkDirty(SessionScreen.nLeft + nLeft - 1,
      SessionScreen.nTop + nTop - 1,
      SessionScreen.nLeft + nRight - 1,
      SessionScreen.nTop + nBottom - 1);
   return(TRUE);
}

BOOL ODSessionScreenCopyText(INT nLeft, INT nTop, INT nRight, INT nBottom,
   INT nDestColumn, INT nDestRow)
{
   INT nWidth;
   INT nHeight;
   INT nRow;
   INT nStep;
   INT nStart;
   INT nEnd;
   BYTE ODFAR *pSource;
   BYTE ODFAR *pDest;

   if(!ODSessionScreenRectValid(nLeft, nTop, nRight, nBottom))
      return(FALSE);
   nWidth = nRight - nLeft + 1;
   nHeight = nBottom - nTop + 1;
   if(!ODSessionScreenRectValid(nDestColumn, nDestRow,
      nDestColumn + nWidth - 1, nDestRow + nHeight - 1))
      return(FALSE);
   if(nDestRow > nTop)
   {
      nStart = nHeight - 1;
      nEnd = -1;
      nStep = -1;
   }
   else
   {
      nStart = 0;
      nEnd = nHeight;
      nStep = 1;
   }
   for(nRow = nStart; nRow != nEnd; nRow += nStep)
   {
      pSource = ODSessionScreenCell(SessionScreen.nLeft + nLeft - 1,
         SessionScreen.nTop + nTop - 1 + nRow);
      pDest = ODSessionScreenCell(SessionScreen.nLeft + nDestColumn - 1,
         SessionScreen.nTop + nDestRow - 1 + nRow);
      memmove(pDest, pSource, (size_t)nWidth * 2U);
   }
   ODSessionScreenMarkDirty(SessionScreen.nLeft + nDestColumn - 1,
      SessionScreen.nTop + nDestRow - 1,
      SessionScreen.nLeft + nDestColumn + nWidth - 2,
      SessionScreen.nTop + nDestRow + nHeight - 2);
   return(TRUE);
}

void ODSessionScreenPresent(void)
{
   tODScrnTextInfo LocalInfo;
   BYTE abtRow[OD_SCREEN_WIDTH * 2];
   INT nWidth;
   INT nHeight;
   INT nLeft;
   INT nRight;
   INT nTop;
   INT nBottom;
   INT nRow;

   if(!bSessionScreenAvailable || bSessionScreenEmulating)
      return;
   ODScrnGetTextInfo(&LocalInfo);
   nWidth = LocalInfo.winright - LocalInfo.winleft + 1;
   nHeight = LocalInfo.winbottom - LocalInfo.wintop + 1;
   if(nWidth > SessionScreen.nWidth)
      nWidth = SessionScreen.nWidth;
   if(nHeight > SessionScreen.nHeight)
      nHeight = SessionScreen.nHeight;
   if(nWidth > OD_SCREEN_WIDTH)
      nWidth = OD_SCREEN_WIDTH;
   nLeft = SessionScreen.nDirtyLeft;
   nRight = SessionScreen.nDirtyRight;
   nTop = SessionScreen.nDirtyTop;
   nBottom = SessionScreen.nDirtyBottom;
   if(nLeft < 0)
      nLeft = 0;
   if(nTop < 0)
      nTop = 0;
   if(nRight >= nWidth)
      nRight = nWidth - 1;
   if(nBottom >= nHeight)
      nBottom = nHeight - 1;
   if(SessionScreen.bDirty && nLeft <= nRight && nTop <= nBottom)
   {
      for(nRow = nTop; nRow <= nBottom; ++nRow)
      {
         memcpy(abtRow, ODSessionScreenCell(nLeft, nRow),
            (size_t)(nRight - nLeft + 1) * 2U);
         ODScrnPutText((BYTE)(nLeft + 1), (BYTE)(nRow + 1),
            (BYTE)(nRight + 1), (BYTE)(nRow + 1), abtRow);
      }
   }
   SessionScreen.bDirty = FALSE;
   ODScrnSetAttribute(SessionScreen.btAttribute);
   if(SessionScreen.nCursorColumn < nWidth
      && SessionScreen.nCursorRow < nHeight)
   {
      ODScrnSetCursorPos((BYTE)(SessionScreen.nCursorColumn + 1),
         (BYTE)(SessionScreen.nCursorRow + 1));
      bCaretPresentationChange = TRUE;
      ODScrnEnableCaret(bRequestedCaretOn);
      bCaretPresentationChange = FALSE;
   }
   else
   {
      bCaretPresentationChange = TRUE;
      ODScrnEnableCaret(FALSE);
      bCaretPresentationChange = FALSE;
   }
}

void ODSessionScreenBeginEmulation(void)
{
   if(bSessionScreenAvailable)
      bSessionScreenEmulating = TRUE;
}

void ODSessionScreenEndEmulation(void)
{
   if(bSessionScreenAvailable)
   {
      bSessionScreenEmulating = FALSE;
      ODSessionScreenPresent();
   }
}

BOOL ODSessionScreenIsEmulating(void)
{
   return(bSessionScreenEmulating);
}

#define OD_SESSION_SNAPSHOT_HEADER_SIZE 48U

static void ODSessionSnapshotPutDWORD(BYTE *pDest, DWORD dwValue)
{
   pDest[0] = (BYTE)(dwValue & 0xffUL);
   pDest[1] = (BYTE)((dwValue >> 8) & 0xffUL);
   pDest[2] = (BYTE)((dwValue >> 16) & 0xffUL);
   pDest[3] = (BYTE)((dwValue >> 24) & 0xffUL);
}

static DWORD ODSessionSnapshotGetDWORD(const BYTE *pSource)
{
   return((DWORD)pSource[0] | ((DWORD)pSource[1] << 8)
      | ((DWORD)pSource[2] << 16) | ((DWORD)pSource[3] << 24));
}

static BOOL ODSessionSnapshotDimensions(INT *pnWidth, INT *pnHeight)
{
   tODScrnTextInfo Info;

   if(bSessionScreenAvailable)
   {
      *pnWidth = SessionScreen.nWidth;
      *pnHeight = SessionScreen.nHeight;
      return(TRUE);
   }
   if(od_control.baud != 0 && nSessionScreenError != ERR_NONE)
      return(FALSE);
   ODScrnGetTextInfo(&Info);
   *pnWidth = Info.winright - Info.winleft + 1;
   *pnHeight = Info.winbottom - Info.wintop + 1;
   return(TRUE);
}

DWORD ODSessionScreenSnapshotSize(void)
{
   INT nWidth;
   INT nHeight;
   unsigned long ulPayload;

   if(!ODSessionSnapshotDimensions(&nWidth, &nHeight))
      return(0);
   ulPayload = (unsigned long)nWidth * (unsigned long)nHeight * 2UL;
   if(ulPayload > 0xffffffffUL - OD_SESSION_SNAPSHOT_HEADER_SIZE)
      return(0);
   return((DWORD)(OD_SESSION_SNAPSHOT_HEADER_SIZE + ulPayload));
}

BOOL ODSessionScreenSave(void *pBuffer, DWORD dwBufferSize)
{
   BYTE *pBytes;
   DWORD dwRequired;
   INT nWidth;
   INT nHeight;
   INT nLeft;
   INT nTop;
   INT nRight;
   INT nBottom;
   INT nCursorColumn;
   INT nCursorRow;
   BYTE btAttribute;
   BOOL bScrolling;
   tODScrnTextInfo LocalInfo;

   dwRequired = ODSessionScreenSnapshotSize();
   if(dwRequired == 0 || pBuffer == NULL || dwBufferSize < dwRequired)
      return(FALSE);
   pBytes = (BYTE *)pBuffer;
   if(bSessionScreenAvailable)
   {
      nWidth = SessionScreen.nWidth;
      nHeight = SessionScreen.nHeight;
      nLeft = SessionScreen.nLeft + 1;
      nTop = SessionScreen.nTop + 1;
      nRight = SessionScreen.nRight + 1;
      nBottom = SessionScreen.nBottom + 1;
      nCursorColumn = SessionScreen.nCursorColumn + 1;
      nCursorRow = SessionScreen.nCursorRow + 1;
      btAttribute = SessionScreen.btAttribute;
      bScrolling = SessionScreen.bScrolling;
   }
   else
   {
      ODScrnGetTextInfo(&LocalInfo);
      nWidth = LocalInfo.winright - LocalInfo.winleft + 1;
      nHeight = LocalInfo.winbottom - LocalInfo.wintop + 1;
      nLeft = 1;
      nTop = 1;
      nRight = nWidth;
      nBottom = nHeight;
      nCursorColumn = LocalInfo.curx;
      nCursorRow = LocalInfo.cury;
      btAttribute = LocalInfo.attribute;
      bScrolling = TRUE;
   }

   pBytes[0] = 'O';
   pBytes[1] = 'D';
   pBytes[2] = 'S';
   pBytes[3] = '1';
   ODSessionSnapshotPutDWORD(pBytes + 4, 1UL);
   ODSessionSnapshotPutDWORD(pBytes + 8, dwRequired);
   ODSessionSnapshotPutDWORD(pBytes + 12, (DWORD)nWidth);
   ODSessionSnapshotPutDWORD(pBytes + 16, (DWORD)nHeight);
   ODSessionSnapshotPutDWORD(pBytes + 20, (DWORD)nLeft);
   ODSessionSnapshotPutDWORD(pBytes + 24, (DWORD)nTop);
   ODSessionSnapshotPutDWORD(pBytes + 28, (DWORD)nRight);
   ODSessionSnapshotPutDWORD(pBytes + 32, (DWORD)nBottom);
   ODSessionSnapshotPutDWORD(pBytes + 36, (DWORD)nCursorColumn);
   ODSessionSnapshotPutDWORD(pBytes + 40, (DWORD)nCursorRow);
   pBytes[44] = btAttribute;
   pBytes[45] = (BYTE)(bScrolling ? 1 : 0);
   pBytes[46] = 0;
   pBytes[47] = 0;

   if(bSessionScreenAvailable)
   {
      memcpy(pBytes + OD_SESSION_SNAPSHOT_HEADER_SIZE,
         SessionScreen.pCells, (size_t)(dwRequired
         - OD_SESSION_SNAPSHOT_HEADER_SIZE));
      return(TRUE);
   }
   return(ODScrnGetText(1, 1, (BYTE)nWidth, (BYTE)nHeight,
      pBytes + OD_SESSION_SNAPSHOT_HEADER_SIZE));
}

BOOL ODSessionScreenRestore(const void *pBuffer, DWORD dwBufferSize)
{
   const BYTE *pBytes;
   DWORD dwTotal;
   DWORD dwWidth;
   DWORD dwHeight;
   DWORD dwLeft;
   DWORD dwTop;
   DWORD dwRight;
   DWORD dwBottom;
   DWORD dwCursorColumn;
   DWORD dwCursorRow;
   DWORD dwExpected;
   INT nCurrentWidth;
   INT nCurrentHeight;
   INT nRow;
   INT nColumn;
   INT nLastColumn;
   const BYTE *pRow;

   if(pBuffer == NULL || dwBufferSize < OD_SESSION_SNAPSHOT_HEADER_SIZE)
      return(FALSE);
   pBytes = (const BYTE *)pBuffer;
   if(pBytes[0] != 'O' || pBytes[1] != 'D' || pBytes[2] != 'S'
      || pBytes[3] != '1' || ODSessionSnapshotGetDWORD(pBytes + 4) != 1UL)
      return(FALSE);
   dwTotal = ODSessionSnapshotGetDWORD(pBytes + 8);
   dwWidth = ODSessionSnapshotGetDWORD(pBytes + 12);
   dwHeight = ODSessionSnapshotGetDWORD(pBytes + 16);
   dwLeft = ODSessionSnapshotGetDWORD(pBytes + 20);
   dwTop = ODSessionSnapshotGetDWORD(pBytes + 24);
   dwRight = ODSessionSnapshotGetDWORD(pBytes + 28);
   dwBottom = ODSessionSnapshotGetDWORD(pBytes + 32);
   dwCursorColumn = ODSessionSnapshotGetDWORD(pBytes + 36);
   dwCursorRow = ODSessionSnapshotGetDWORD(pBytes + 40);
   if(dwWidth == 0 || dwHeight == 0 || dwWidth > 0xffffffffUL / dwHeight
      || dwWidth * dwHeight >
         (0xffffffffUL - OD_SESSION_SNAPSHOT_HEADER_SIZE) / 2UL)
      return(FALSE);
   dwExpected = OD_SESSION_SNAPSHOT_HEADER_SIZE + dwWidth * dwHeight * 2UL;
   if(dwTotal != dwExpected || dwBufferSize < dwTotal
      || !ODSessionSnapshotDimensions(&nCurrentWidth, &nCurrentHeight)
      || dwWidth != (DWORD)nCurrentWidth || dwHeight != (DWORD)nCurrentHeight
      || dwLeft < 1)
      return(FALSE);
   if(dwTop < 1 || dwLeft > dwRight || dwTop > dwBottom
      || dwRight > dwWidth || dwBottom > dwHeight || dwCursorColumn < 1)
      return(FALSE);
   if(dwCursorColumn > dwRight - dwLeft + 1 || dwCursorRow < 1
      || dwCursorRow > dwBottom - dwTop + 1 || pBytes[45] > 1
      || pBytes[46] != 0 || pBytes[47] != 0)
      return(FALSE);

   if(bSessionScreenAvailable)
   {
      if(od_control.user_ansi || od_control.user_avatar)
      {
         if(!od_puttext(1, 1, nCurrentWidth, nCurrentHeight,
            (void *)(pBytes + OD_SESSION_SNAPSHOT_HEADER_SIZE)))
            return(FALSE);
      }
      else
      {
         od_clr_scr();
         pRow = pBytes + OD_SESSION_SNAPSHOT_HEADER_SIZE;
         for(nRow = 1;; ++nRow)
         {
            nLastColumn = nCurrentWidth;
            while(nLastColumn > 1
               && (pRow[(nLastColumn - 1) * 2] == ' '
                  || pRow[(nLastColumn - 1) * 2] == 0))
               --nLastColumn;
            if(nRow == (INT)dwCursorRow
               && nLastColumn >= (INT)dwCursorColumn)
               nLastColumn = (INT)dwCursorColumn - 1;
            for(nColumn = 0; nColumn < nLastColumn; ++nColumn)
               od_putch((char)pRow[nColumn * 2]);
            if(nRow == (INT)dwCursorRow)
               break;
            if(nLastColumn != nCurrentWidth)
               od_disp_str("\r\n");
            pRow += (size_t)nCurrentWidth * 2U;
         }
      }
      memcpy(SessionScreen.pCells,
         pBytes + OD_SESSION_SNAPSHOT_HEADER_SIZE,
         (size_t)(dwTotal - OD_SESSION_SNAPSHOT_HEADER_SIZE));
      SessionScreen.nLeft = (INT)dwLeft - 1;
      SessionScreen.nTop = (INT)dwTop - 1;
      SessionScreen.nRight = (INT)dwRight - 1;
      SessionScreen.nBottom = (INT)dwBottom - 1;
      SessionScreen.nCursorColumn = (INT)dwCursorColumn - 1;
      SessionScreen.nCursorRow = (INT)dwCursorRow - 1;
      SessionScreen.btAttribute = pBytes[44];
      SessionScreen.bScrolling = pBytes[45] != 0;
      ODSessionScreenMarkDirty(0, 0, SessionScreen.nWidth - 1,
         SessionScreen.nHeight - 1);
      ODSessionScreenPresent();
      if(od_control.user_ansi || od_control.user_avatar)
      {
         od_set_cursor((INT)dwCursorRow, (INT)dwCursorColumn);
         od_set_attrib(pBytes[44]);
      }
      return(TRUE);
   }

   if(!ODScrnPutText(1, 1, (BYTE)nCurrentWidth, (BYTE)nCurrentHeight,
      (void *)(pBytes + OD_SESSION_SNAPSHOT_HEADER_SIZE)))
      return(FALSE);
   ODScrnSetBoundary((BYTE)dwLeft, (BYTE)dwTop, (BYTE)dwRight,
      (BYTE)dwBottom);
   ODScrnSetCursorPos((BYTE)dwCursorColumn, (BYTE)dwCursorRow);
   ODScrnSetAttribute(pBytes[44]);
   ODScrnEnableScrolling(pBytes[45] != 0);
   return(TRUE);
}
