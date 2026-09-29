#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODInQue.h"
#include "ODKrnl.h"
#include "ODPlat.h"
#include "ODScrn.h"

#define CHECK(condition) do { \
   if(!(condition)) { \
      fprintf(stderr, "multiline growth test failed at line %d\n", __LINE__); \
      return(1); \
   } \
} while(0)

static UINT unRequestedSize;
static void *pReturnedBuffer;

static void *GrowBuffer(void *pOriginalBuffer, UINT unNewSize)
{
   void *pNewBuffer = realloc(pOriginalBuffer, unNewSize);

   if(pNewBuffer != NULL)
   {
      unRequestedSize = unNewSize;
      pReturnedBuffer = pNewBuffer;
   }
   return(pNewBuffer);
}

static tODResult AddCharacter(char chKey)
{
   tODInputEvent Input;

   memset(&Input, 0, sizeof(Input));
   Input.EventType = EVENT_CHARACTER;
   Input.bFromRemote = FALSE;
   Input.chKeyPress = chKey;
   return(ODInQueueAddEvent(hODInputQueue, &Input));
}

int main(void)
{
   static const char szEmptyText[] = "";
   tODEditOptions Options;
   char *pszInitialBuffer;
   UINT unInitialSize = (UINT)sizeof(szEmptyText);


   pszInitialBuffer = (char *)malloc(sizeof(szEmptyText));
   CHECK(pszInitialBuffer != NULL);
   memcpy(pszInitialBuffer, szEmptyText, sizeof(szEmptyText));

   memset(&od_control, 0, sizeof(od_control));
   memset(&Options, 0, sizeof(Options));
   od_control.user_ansi = TRUE;
   od_control.od_cur_attrib = 0x07;
   od_control.od_disable = DIS_TIMEOUT;
   Options.pfBufferRealloc = GrowBuffer;

   CHECK(ODScrnInitialize() == kODRCSuccess);
   ODScrnSetBoundary(1, 1, 80, 25);
   CHECK(ODInQueueAlloc(&hODInputQueue, 3) == kODRCSuccess);
   CHECK(AddCharacter('A') == kODRCSuccess);
   CHECK(AddCharacter(27) == kODRCSuccess);

   bODInitialized = TRUE;
   ODTimerStart(&RunKernelTimer, 60000);
   CHECK(od_multiline_edit(pszInitialBuffer, unInitialSize, &Options)
      == OD_MULTIEDIT_SUCCESS);
   CHECK(unRequestedSize > unInitialSize);
   CHECK(Options.pszFinalBuffer == pReturnedBuffer);
   CHECK(Options.unFinalBufferSize == unRequestedSize);
   CHECK(strcmp(Options.pszFinalBuffer, "A") == 0);

   free(Options.pszFinalBuffer);
   bODInitialized = FALSE;
   ODInQueueFree(hODInputQueue);
   hODInputQueue = NULL;
   ODScrnShutdown();


   return(0);
}
