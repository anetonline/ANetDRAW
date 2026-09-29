#include <limits.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODInQue.h"
#include "ODKrnl.h"
#include "ODPlat.h"
#include "ODScrn.h"

#define CHECK(condition) do { if(!(condition)) return(__LINE__); } while(0)

static tODResult AddCharacter(unsigned char chKey)
{
   tODInputEvent Event;

   memset(&Event, 0, sizeof(Event));
   Event.EventType = EVENT_CHARACTER;
   Event.chKeyPress = (char)chKey;
   Event.bFromRemote = FALSE;
   return(ODInQueueAddEvent(hODInputQueue, &Event));
}

int main(void)
{
   char Input[3];


   memset(&od_control, 0, sizeof(od_control));
   memset(Input, 0x55, sizeof(Input));
   od_control.od_disable = DIS_TIMEOUT;
   od_control.od_cur_attrib = 0x07;
   CHECK(ODScrnInitialize() == kODRCSuccess);
   ODScrnSetBoundary(1, 1, 80, 25);
   CHECK(ODInQueueAlloc(&hODInputQueue, 5) == kODRCSuccess);
   ODTimerStart(&RunKernelTimer, 60000);
   bODInitialized = TRUE;

   CHECK(AddCharacter(0x7f) == kODRCSuccess);
   CHECK(AddCharacter(0x80) == kODRCSuccess);
   CHECK(AddCharacter(0xff) == kODRCSuccess);
   CHECK(AddCharacter('\r') == kODRCSuccess);

   od_input_str(Input, 2, 0x80, 0xff);
   CHECK((unsigned char)Input[0] == 0x80);
   CHECK((unsigned char)Input[1] == 0xff);
   CHECK(Input[2] == '\0');
   CHECK(!ODInQueueWaiting(hODInputQueue));

   bODInitialized = FALSE;
   ODInQueueFree(hODInputQueue);
   hODInputQueue = NULL;
   ODScrnShutdown();


   return(0);
}
