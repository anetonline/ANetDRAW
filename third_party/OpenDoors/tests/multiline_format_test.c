#include <stdio.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODInQue.h"
#include "ODKrnl.h"
#include "ODScrn.h"

#define CHECK(condition) do { \
   if(!(condition)) { \
      fprintf(stderr, "multiline format test failed at line %d\n", __LINE__); \
      return(1); \
   } \
} while(0)

int main(void)
{
   char buffer[128] = "\x01MSGID: example\rHello\nworld\x8d!\r";
   tODEditOptions options;
   tODInputEvent input;

   memset(&od_control, 0, sizeof(od_control));
   memset(&options, 0, sizeof(options));
   od_control.user_ansi = TRUE;
   od_control.od_cur_attrib = 0x07;
   od_control.od_disable = DIS_TIMEOUT;

   CHECK(ODScrnInitialize() == kODRCSuccess);
   ODScrnSetBoundary(1, 1, 80, 25);
   CHECK(ODInQueueAlloc(&hODInputQueue, 2) == kODRCSuccess);

   input.EventType = EVENT_CHARACTER;
   input.bFromRemote = FALSE;
   input.chKeyPress = 27;
   CHECK(ODInQueueAddEvent(hODInputQueue, &input) == kODRCSuccess);

   options.nAreaLeft = 1;
   options.nAreaTop = 1;
   options.nAreaRight = 80;
   options.nAreaBottom = 23;
   options.TextFormat = FORMAT_FTSC_MESSAGE;

   bODInitialized = TRUE;
   ODTimerStart(&RunKernelTimer, 60000);
   CHECK(od_multiline_edit(buffer, sizeof(buffer), &options)
      == OD_MULTIEDIT_SUCCESS);
   CHECK(strcmp(buffer, "\x01MSGID: example\rHelloworld!\r") == 0);
   CHECK(options.pszFinalBuffer == buffer);

   bODInitialized = FALSE;
   ODInQueueFree(hODInputQueue);
   ODScrnShutdown();
   return(0);
}
