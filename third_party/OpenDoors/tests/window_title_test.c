#include <limits.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODKrnl.h"
#include "ODPlat.h"
#include "ODScrn.h"

#define CHECK(condition) do { if(!(condition)) return(__LINE__); } while(0)

static int CheckWindow(INT nTop, INT nWidth, const char *pszExpected)
{
   BYTE abtText[16];
   char szActual[8];
   void *pWindow;
   INT nColumn;
   int nMatches;

   pWindow = od_window_create(10, nTop, 10 + nWidth - 1, nTop + 2,
      "ABCDE", 0x07, 0x07, 0x07, 0);
   if(pWindow == NULL)
      return(0);
   if(!ODScrnGetText(10, (BYTE)nTop, (BYTE)(10 + nWidth - 1),
      (BYTE)nTop, abtText))
   {
      od_window_remove(pWindow);
      return(0);
   }
   for(nColumn = 0; nColumn < nWidth; ++nColumn)
      szActual[nColumn] = (char)abtText[nColumn * 2];
   szActual[nWidth] = '\0';
   nMatches = strcmp(szActual, pszExpected) == 0;
   if(!od_window_remove(pWindow))
      return(0);
   return(nMatches);
}

int main(void)
{

   memset(&od_control, 0, sizeof(od_control));
   od_control.user_ansi = TRUE;
   od_control.od_box_chars[BOX_UPPERLEFT] = '+';
   od_control.od_box_chars[BOX_TOP] = '-';
   od_control.od_box_chars[BOX_UPPERRIGHT] = '+';
   od_control.od_box_chars[BOX_LEFT] = '|';
   od_control.od_box_chars[BOX_LOWERLEFT] = '+';
   od_control.od_box_chars[BOX_LOWERRIGHT] = '+';
   od_control.od_box_chars[BOX_BOTTOM] = '-';
   od_control.od_box_chars[BOX_RIGHT] = '|';

   CHECK(ODScrnInitialize() == kODRCSuccess);
   ODScrnSetBoundary(1, 1, 80, 25);
   ODTimerStart(&RunKernelTimer, 60000);
   bODInitialized = TRUE;

   CHECK(CheckWindow(1, 3, "+-+"));
   CHECK(CheckWindow(5, 4, "+--+"));
   CHECK(CheckWindow(9, 5, "+---+"));
   CHECK(CheckWindow(13, 6, "+----+"));
   CHECK(CheckWindow(17, 7, "+- A -+"));

   bODInitialized = FALSE;
   ODScrnShutdown();


   return(0);
}
