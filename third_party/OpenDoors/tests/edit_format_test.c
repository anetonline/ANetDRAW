#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODInQue.h"
#include "ODKrnl.h"
#include "ODPlat.h"
#include "ODScrn.h"

#define CHECK(condition) do { \
   if(!(condition)) { \
      fprintf(stderr, "edit format test failed at line %d\n", __LINE__); \
      nResult = __LINE__; \
      goto cleanup; \
   } \
} while(0)

static tODResult AddCharacter(unsigned char chKey)
{
   tODInputEvent Event;

   memset(&Event, 0, sizeof(Event));
   Event.EventType = EVENT_CHARACTER;
   Event.chKeyPress = (char)chKey;
   Event.bFromRemote = FALSE;
   return(ODInQueueAddEvent(hODInputQueue, &Event));
}

static BOOL QueueString(const char *pszText)
{
   while(*pszText != '\0')
   {
      if(AddCharacter((unsigned char)*pszText++) != kODRCSuccess)
         return(FALSE);
   }
   return(AddCharacter('\r') == kODRCSuccess);
}

int main(void)
{
   char szExpected[] = "Az2:.X?*#$&'(>-@_!{}~";
   char szInput[sizeof(szExpected)];
   char szFormat[sizeof(szExpected)];
   char szUppercaseInput[sizeof("Az2.*")];
   INT nResult = 0;


   memset(&od_control, 0, sizeof(od_control));
   od_control.od_disable = DIS_TIMEOUT;
   od_control.od_cur_attrib = 0x07;
   od_control.user_ansi = TRUE;
   CHECK(ODScrnInitialize() == kODRCSuccess);
   ODScrnSetBoundary(1, 1, 80, 25);
   CHECK(ODInQueueAlloc(&hODInputQueue,
      (INT)(sizeof(szExpected) + sizeof(szUppercaseInput))) == kODRCSuccess);
   ODTimerStart(&RunKernelTimer, 60000);
   bODInitialized = TRUE;

   szExpected[5] = DIRSEP;
   memset(szFormat, 'W', sizeof(szFormat) - 1);
   szFormat[sizeof(szFormat) - 1] = '\0';
   szInput[0] = '\0';
   CHECK(QueueString(szExpected));
   CHECK(od_edit_str(szInput, szFormat, 1, 1, 0x07, 0x07, '_',
      EDIT_FLAG_NORMAL) == EDIT_RETURN_ACCEPT);
   if(strcmp(szInput, szExpected) != 0)
      fprintf(stderr, "expected [%s], received [%s]\n", szExpected, szInput);
   CHECK(strcmp(szInput, szExpected) == 0);

   szUppercaseInput[0] = '\0';
   CHECK(QueueString("Az2.*"));
   CHECK(od_edit_str(szUppercaseInput, "FFFFF", 2, 1, 0x07, 0x07, '_',
      EDIT_FLAG_NORMAL) == EDIT_RETURN_ACCEPT);
   CHECK(strcmp(szUppercaseInput, "AZ2.*") == 0);

cleanup:
   if(bODInitialized)
   {
      bODInitialized = FALSE;
      ODScrnShutdown();
   }
   if(hODInputQueue != NULL)
   {
      ODInQueueFree(hODInputQueue);
      hODInputQueue = NULL;
   }


   return(nResult);
}
