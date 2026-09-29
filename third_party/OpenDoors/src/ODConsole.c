/* Windows console presentation and keyboard support. */

#define BUILDING_OPENDOORS

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "OpenDoor.h"
#include "ODConsole.h"

#ifdef ODPLAT_WIN32

static HANDLE hConsoleInput = INVALID_HANDLE_VALUE;
static HANDLE hConsoleOutput = INVALID_HANDLE_VALUE;
static DWORD dwSavedInputMode;
static DWORD dwSavedOutputMode;
static UINT nSavedInputCodePage;
static UINT nSavedOutputCodePage;
static CONSOLE_CURSOR_INFO SavedCursorInfo;
static COORD SavedBufferSize;
static SMALL_RECT SavedWindow;
static BOOL bConsoleActive;
static BOOL bConsoleOwned;
static WORD wRepeatedKey;
static BYTE btRepeatedShiftStatus;
static WORD wRepeatCount;
static CHAR_INFO *pConsoleCells;
static size_t nConsoleCellCapacity;

static BYTE ODConsoleShiftStatus(DWORD dwControlState)
{
   BYTE btResult = 0;

   if(dwControlState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
      btResult |= 0x04;
   if(dwControlState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
      btResult |= 0x08;
   if(dwControlState & SHIFT_PRESSED)
      btResult |= 0x02;
   if(dwControlState & SCROLLLOCK_ON)
      btResult |= 0x10;
   if(dwControlState & NUMLOCK_ON)
      btResult |= 0x20;
   if(dwControlState & CAPSLOCK_ON)
      btResult |= 0x40;
   return(btResult);
}

BOOL ODConsoleInitialize(void)
{
   CONSOLE_SCREEN_BUFFER_INFO Info;
   BOOL bReplaceAttachedConsole = FALSE;

   if(bConsoleActive)
      return(TRUE);

open_console:
   hConsoleInput = CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
   if(hConsoleInput == INVALID_HANDLE_VALUE)
   {
      if(!AllocConsole())
      {
         /* Console-subsystem programs can inherit a pseudo-console for which
          * CONIN$ cannot be opened, while AllocConsole() reports that a
          * console is already attached.  Detach it and request the real
          * screen buffer local mode requires. */
         FreeConsole();
         if(!AllocConsole())
            return(FALSE);
      }
      bConsoleOwned = TRUE;
      hConsoleInput = CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE,
         FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
      if(hConsoleInput == INVALID_HANDLE_VALUE)
      {
         ODConsoleShutdown();
         return(FALSE);
      }
   }
   hConsoleOutput = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
   if(hConsoleOutput == INVALID_HANDLE_VALUE)
      goto unusable_console;
   if(!GetConsoleMode(hConsoleInput, &dwSavedInputMode)
      || !GetConsoleMode(hConsoleOutput, &dwSavedOutputMode)
      || !GetConsoleCursorInfo(hConsoleOutput, &SavedCursorInfo)
      || !GetConsoleScreenBufferInfo(hConsoleOutput, &Info))
   {
unusable_console:
      bReplaceAttachedConsole = !bConsoleOwned
         && !bReplaceAttachedConsole;
      ODConsoleShutdown();
      if(bReplaceAttachedConsole)
      {
         /* A pseudo-console can provide CONIN$/CONOUT$ handles without
          * behaving as an attached screen buffer.  FreeConsole() may report
          * that there was no real console in that state, so allocation is
          * the operation that determines whether recovery succeeded. */
         FreeConsole();
         if(!AllocConsole())
            return(FALSE);
         bConsoleOwned = TRUE;
         goto open_console;
      }
      return(FALSE);
   }

   SavedBufferSize = Info.dwSize;
   SavedWindow = Info.srWindow;
   nSavedInputCodePage = GetConsoleCP();
   nSavedOutputCodePage = GetConsoleOutputCP();
   SetConsoleCP(437);
   SetConsoleOutputCP(437);
   SetConsoleMode(hConsoleInput,
      (dwSavedInputMode | ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT)
      & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT
         | ENABLE_QUICK_EDIT_MODE));
   wRepeatCount = 0;
   bConsoleActive = TRUE;
   return(TRUE);
}

void ODConsoleShutdown(void)
{
   if(bConsoleActive)
   {
      SMALL_RECT SmallWindow;

      SetConsoleMode(hConsoleInput, dwSavedInputMode);
      SetConsoleMode(hConsoleOutput, dwSavedOutputMode);
      SetConsoleCP(nSavedInputCodePage);
      SetConsoleOutputCP(nSavedOutputCodePage);
      SetConsoleCursorInfo(hConsoleOutput, &SavedCursorInfo);

      SmallWindow.Left = 0;
      SmallWindow.Top = 0;
      SmallWindow.Right = 0;
      SmallWindow.Bottom = 0;
      SetConsoleWindowInfo(hConsoleOutput, TRUE, &SmallWindow);
      SetConsoleScreenBufferSize(hConsoleOutput, SavedBufferSize);
      SetConsoleWindowInfo(hConsoleOutput, TRUE, &SavedWindow);
   }
   if(hConsoleInput != INVALID_HANDLE_VALUE)
      CloseHandle(hConsoleInput);
   if(hConsoleOutput != INVALID_HANDLE_VALUE)
      CloseHandle(hConsoleOutput);
   hConsoleInput = INVALID_HANDLE_VALUE;
   hConsoleOutput = INVALID_HANDLE_VALUE;
   bConsoleActive = FALSE;
   if(bConsoleOwned)
      FreeConsole();
   bConsoleOwned = FALSE;
   wRepeatCount = 0;
   free(pConsoleCells);
   pConsoleCells = NULL;
   nConsoleCellCapacity = 0;
}

BOOL ODConsoleAvailable(void)
{
   return(bConsoleActive);
}

void ODConsoleSetSize(INT nWidth, INT nHeight, INT *pnActualWidth,
   INT *pnActualHeight)
{
   COORD Largest;
   COORD Size;
   COORD ExpandedSize;
   CONSOLE_SCREEN_BUFFER_INFO Info;
   SMALL_RECT Window;
   BOOL bResizeSucceeded;
   INT nWindowWidth;
   INT nWindowHeight;

   if(nWidth < 1) nWidth = 1;
   if(nHeight < 1) nHeight = 1;
   if(!bConsoleActive)
   {
      *pnActualWidth = nWidth;
      *pnActualHeight = nHeight;
      return;
   }

   if(nWidth > SHRT_MAX) nWidth = SHRT_MAX;
   if(nHeight > SHRT_MAX) nHeight = SHRT_MAX;
   Size.X = (SHORT)nWidth;
   Size.Y = (SHORT)nHeight;
   Largest = GetLargestConsoleWindowSize(hConsoleOutput);
   nWindowWidth = nWidth;
   nWindowHeight = nHeight;
   if(Largest.X > 0 && nWindowWidth > Largest.X)
      nWindowWidth = Largest.X;
   if(Largest.Y > 0 && nWindowHeight > Largest.Y)
      nWindowHeight = Largest.Y;
   Window.Left = 0;
   Window.Top = 0;
   Window.Right = (SHORT)(nWindowWidth - 1);
   Window.Bottom = (SHORT)(nWindowHeight - 1);

   bResizeSucceeded = TRUE;
   if(GetConsoleScreenBufferInfo(hConsoleOutput, &Info))
   {
      /* A mixed resize can shrink one dimension while growing the other.
       * First make a buffer large enough for both the old and new windows,
       * then resize the window, and only then trim the buffer. */
      ExpandedSize.X = Info.dwSize.X > Size.X ? Info.dwSize.X : Size.X;
      ExpandedSize.Y = Info.dwSize.Y > Size.Y ? Info.dwSize.Y : Size.Y;
      if(ExpandedSize.X != Info.dwSize.X
         || ExpandedSize.Y != Info.dwSize.Y)
      {
         bResizeSucceeded = SetConsoleScreenBufferSize(hConsoleOutput,
            ExpandedSize);
      }
      if(bResizeSucceeded)
         bResizeSucceeded = SetConsoleWindowInfo(hConsoleOutput, TRUE,
            &Window);
      if(bResizeSucceeded
         && (ExpandedSize.X != Size.X || ExpandedSize.Y != Size.Y))
      {
         bResizeSucceeded = SetConsoleScreenBufferSize(hConsoleOutput, Size);
      }
   }
   else
   {
      bResizeSucceeded = SetConsoleScreenBufferSize(hConsoleOutput, Size);
      if(bResizeSucceeded)
         bResizeSucceeded = SetConsoleWindowInfo(hConsoleOutput, TRUE,
            &Window);
   }
   if(!bResizeSucceeded)
   {
      if(GetConsoleScreenBufferInfo(hConsoleOutput, &Info))
      {
         nWidth = Info.dwSize.X;
         nHeight = Info.dwSize.Y;
      }
   }
   *pnActualWidth = nWidth;
   *pnActualHeight = nHeight;
}

BOOL ODConsoleWrite(const BYTE *pCells, INT nWidth, INT nHeight,
   INT nCursorColumn, INT nCursorRow, BOOL bCursorOn)
{
   size_t nCells;
   size_t nIndex;
   COORD BufferSize;
   COORD BufferOrigin;
   SMALL_RECT Region;
   CONSOLE_CURSOR_INFO CursorInfo;
   COORD CursorPosition;

   if(!bConsoleActive || pCells == NULL || nWidth < 1 || nHeight < 1)
      return(FALSE);
   nCells = (size_t)nWidth * (size_t)nHeight;
   if(nCells > nConsoleCellCapacity)
   {
      CHAR_INFO *pNewCells = (CHAR_INFO *)realloc(pConsoleCells,
         nCells * sizeof(*pConsoleCells));
      if(pNewCells == NULL)
         return(FALSE);
      pConsoleCells = pNewCells;
      nConsoleCellCapacity = nCells;
   }
   for(nIndex = 0; nIndex < nCells; ++nIndex)
   {
      pConsoleCells[nIndex].Char.AsciiChar = (CHAR)pCells[nIndex * 2];
      pConsoleCells[nIndex].Attributes = pCells[nIndex * 2 + 1];
   }
   BufferSize.X = (SHORT)nWidth;
   BufferSize.Y = (SHORT)nHeight;
   BufferOrigin.X = 0;
   BufferOrigin.Y = 0;
   Region.Left = 0;
   Region.Top = 0;
   Region.Right = (SHORT)(nWidth - 1);
   Region.Bottom = (SHORT)(nHeight - 1);
   if(!WriteConsoleOutputA(hConsoleOutput, pConsoleCells, BufferSize,
      BufferOrigin, &Region))
   {
      return(FALSE);
   }

   CursorInfo.dwSize = SavedCursorInfo.dwSize;
   CursorInfo.bVisible = bCursorOn;
   SetConsoleCursorInfo(hConsoleOutput, &CursorInfo);
   if(nCursorColumn >= 0 && nCursorColumn < nWidth
      && nCursorRow >= 0 && nCursorRow < nHeight)
   {
      CursorPosition.X = (SHORT)nCursorColumn;
      CursorPosition.Y = (SHORT)nCursorRow;
      SetConsoleCursorPosition(hConsoleOutput, CursorPosition);
   }
   return(TRUE);
}

BOOL ODConsoleReadKey(WORD *pwKey, BYTE *pbtShiftStatus)
{
   INPUT_RECORD Record;
   DWORD dwRead;
   KEY_EVENT_RECORD *pKey;
   WORD wVirtualKey;

   if(!bConsoleActive || pwKey == NULL || pbtShiftStatus == NULL)
      return(FALSE);
   if(wRepeatCount != 0)
   {
      --wRepeatCount;
      *pwKey = wRepeatedKey;
      *pbtShiftStatus = btRepeatedShiftStatus;
      return(TRUE);
   }

   while(PeekConsoleInputA(hConsoleInput, &Record, 1, &dwRead) && dwRead != 0)
   {
      if(!ReadConsoleInputA(hConsoleInput, &Record, 1, &dwRead) || dwRead == 0)
         return(FALSE);
      if(Record.EventType != KEY_EVENT || !Record.Event.KeyEvent.bKeyDown)
         continue;
      pKey = &Record.Event.KeyEvent;
      wVirtualKey = pKey->wVirtualKeyCode;
      if(wVirtualKey == VK_SHIFT || wVirtualKey == VK_CONTROL
         || wVirtualKey == VK_MENU || wVirtualKey == VK_CAPITAL
         || wVirtualKey == VK_NUMLOCK || wVirtualKey == VK_SCROLL)
      {
         continue;
      }
      wRepeatedKey = MAKEWORD((BYTE)pKey->uChar.AsciiChar,
         (BYTE)pKey->wVirtualScanCode);
      btRepeatedShiftStatus = ODConsoleShiftStatus(pKey->dwControlKeyState);
      wRepeatCount = pKey->wRepeatCount > 0 ? pKey->wRepeatCount - 1 : 0;
      *pwKey = wRepeatedKey;
      *pbtShiftStatus = btRepeatedShiftStatus;
      return(TRUE);
   }
   return(FALSE);
}

#endif /* ODPLAT_WIN32 */
