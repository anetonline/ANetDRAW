#include <limits.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODInQue.h"
#include "ODKrnl.h"
#include "ODPlat.h"

#define CHECK(condition) do { if(!(condition)) return(__LINE__); } while(0)

static void SetCharacterEvent(tODInputEvent *pEvent, char chKey,
   BOOL bFromRemote)
{
   memset(pEvent, 0, sizeof(*pEvent));
   pEvent->EventType = EVENT_CHARACTER;
   pEvent->chKeyPress = chKey;
   pEvent->bFromRemote = bFromRemote;
}

int main(void)
{
   tODInputEvent Event;


   memset(&od_control, 0, sizeof(od_control));
   od_control.od_disable = DIS_TIMEOUT;
   CHECK(ODInQueueAlloc(&hODInputQueue, 4) == kODRCSuccess);
   ODTimerStart(&RunKernelTimer, 60000);
   bODInitialized = TRUE;

   SetCharacterEvent(&Event, '\n', FALSE);
   CHECK(ODInQueueAddEvent(hODInputQueue, &Event) == kODRCSuccess);
   od_control.od_last_input = 7;
   CHECK(od_get_key(FALSE) == 0);
   CHECK(od_control.od_last_input == 1);
   CHECK(!ODInQueueWaiting(hODInputQueue));

   SetCharacterEvent(&Event, '\n', FALSE);
   CHECK(ODInQueueAddEvent(hODInputQueue, &Event) == kODRCSuccess);
   SetCharacterEvent(&Event, 'X', TRUE);
   CHECK(ODInQueueAddEvent(hODInputQueue, &Event) == kODRCSuccess);
   CHECK(od_get_key(FALSE) == 'X');
   CHECK(od_control.od_last_input == 0);
   CHECK(!ODInQueueWaiting(hODInputQueue));

   bODInitialized = FALSE;
   ODInQueueFree(hODInputQueue);
   hODInputQueue = NULL;


   return(0);
}
