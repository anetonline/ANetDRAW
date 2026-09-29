#include <limits.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODInQue.h"
#include "ODKrnl.h"
#include "ODPlat.h"
#include "ODScrn.h"
#include "ODVScrn.h"

#define LOCAL_WIDTH 80
#define LOCAL_HEIGHT 25
#define SESSION_WIDTH 132
#define SESSION_HEIGHT 50

#define CHECK(condition) do { \
   if(!(condition)) { \
      nResult = __LINE__; \
      goto cleanup; \
   } \
} while(0)

static tODResult AddEnter(void)
{
   tODInputEvent Event;

   memset(&Event, 0, sizeof(Event));
   Event.EventType = EVENT_CHARACTER;
   Event.chKeyPress = '\r';
   Event.bFromRemote = FALSE;
   return(ODInQueueAddEvent(hODInputQueue, &Event));
}

int main(void)
{
   static const BYTE abtSentinel[2] = {'S', 0x07};
   BYTE abtCell[2];
   char szInput[3];
   INT nResult = 0;


   memset(&od_control, 0, sizeof(od_control));
   od_control.od_disable = DIS_TIMEOUT;
   od_control.od_cur_attrib = 0x07;
   od_control.user_ansi = TRUE;
   CHECK(ODScrnInitialize() == kODRCSuccess);
   ODScrnSetBoundary(1, 1, LOCAL_WIDTH, LOCAL_HEIGHT);
   CHECK(ODInQueueAlloc(&hODInputQueue, 4) == kODRCSuccess);
   ODTimerStart(&RunKernelTimer, 60000);
   bODInitialized = TRUE;

   CHECK(ODScrnPutText(1, 1, 1, 1, (void *)abtSentinel));
   szInput[0] = '\0';
   CHECK(AddEnter() == kODRCSuccess);
   CHECK(od_edit_str(szInput, "**", LOCAL_HEIGHT, LOCAL_WIDTH - 3,
      0x07, 0x07, '_', EDIT_FLAG_NORMAL) == EDIT_RETURN_ACCEPT);
   CHECK(ODScrnGetText(1, 1, 1, 1, abtCell));
   CHECK(abtCell[0] == abtSentinel[0]);

   strcpy(szInput, "XY");
   od_control.od_error = ERR_NONE;
   CHECK(od_edit_str(szInput, "**", LOCAL_HEIGHT, LOCAL_WIDTH - 2,
      0x07, 0x07, '_', EDIT_FLAG_EDIT_STRING | EDIT_FLAG_SHOW_SIZE
      | EDIT_FLAG_NO_REDRAW) == EDIT_RETURN_ERROR);
   CHECK(od_control.od_error == ERR_PARAMETER);
   CHECK(strcmp(szInput, "XY") == 0);

   strcpy(szInput, "XY");
   od_control.od_error = ERR_NONE;
   CHECK(od_edit_str(szInput, "**", LOCAL_HEIGHT + 1, 1,
      0x07, 0x07, '_', EDIT_FLAG_EDIT_STRING) == EDIT_RETURN_ERROR);
   CHECK(od_control.od_error == ERR_PARAMETER);
   CHECK(strcmp(szInput, "XY") == 0);

   od_control.baud = 1;
   ODSessionScreenInitialize(SESSION_WIDTH, SESSION_HEIGHT);
   CHECK(ODSessionScreenAvailable());
   od_control.baud = 0;
   szInput[0] = '\0';
   CHECK(AddEnter() == kODRCSuccess);
   CHECK(od_edit_str(szInput, "**", SESSION_HEIGHT, SESSION_WIDTH - 3,
      0x07, 0x07, '_', EDIT_FLAG_NORMAL) == EDIT_RETURN_ACCEPT);

   strcpy(szInput, "XY");
   od_control.od_error = ERR_NONE;
   CHECK(od_edit_str(szInput, "**", SESSION_HEIGHT, SESSION_WIDTH - 2,
      0x07, 0x07, '_', EDIT_FLAG_EDIT_STRING) == EDIT_RETURN_ERROR);
   CHECK(od_control.od_error == ERR_PARAMETER);
   CHECK(strcmp(szInput, "XY") == 0);

cleanup:
   if(ODSessionScreenAvailable())
      ODSessionScreenShutdown();
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
