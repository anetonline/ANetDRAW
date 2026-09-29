#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODInQue.h"
#include "ODKrnl.h"
#include "ODPlat.h"
#include "ODScrn.h"
#include "ODVScrn.h"

#define CHECK(condition) do { \
   if(!(condition)) { \
      fprintf(stderr, "multiline bounds test failed at line %d\n", __LINE__); \
      return(1); \
   } \
} while(0)

static BOOL RejectRectangle(INT nLeft, INT nTop, INT nRight, INT nBottom)
{
   char szBuffer[1] = "";
   tODEditOptions Options;

   memset(&Options, 0, sizeof(Options));
   Options.nAreaLeft = nLeft;
   Options.nAreaTop = nTop;
   Options.nAreaRight = nRight;
   Options.nAreaBottom = nBottom;
   od_control.od_error = ERR_NONE;

   return(od_multiline_edit(szBuffer, sizeof(szBuffer), &Options)
      == OD_MULTIEDIT_ERROR && od_control.od_error == ERR_PARAMETER);
}

static tODResult AddEscape(void)
{
   tODInputEvent Input;

   memset(&Input, 0, sizeof(Input));
   Input.EventType = EVENT_CHARACTER;
   Input.bFromRemote = FALSE;
   Input.chKeyPress = 27;
   return(ODInQueueAddEvent(hODInputQueue, &Input));
}

int main(void)
{
   char szBuffer[1] = "";
   tODEditOptions Options;


   memset(&od_control, 0, sizeof(od_control));
   od_control.user_ansi = TRUE;
   od_control.od_cur_attrib = 0x07;
   od_control.od_disable = DIS_TIMEOUT;
   CHECK(ODScrnInitialize() == kODRCSuccess);
   ODScrnSetBoundary(1, 1, 80, 25);
   bODInitialized = TRUE;
   ODTimerStart(&RunKernelTimer, 60000);

   CHECK(RejectRectangle(-1, 1, 10, 10));
   CHECK(RejectRectangle(10, 1, 9, 10));
   CHECK(RejectRectangle(10, 1, 10, 10));
   CHECK(RejectRectangle(1, 10, 10, 9));
   CHECK(RejectRectangle(1, 10, 10, 10));
   CHECK(RejectRectangle(1, 1, 81, 25));
   CHECK(RejectRectangle(1, 1, 80, 26));

   od_control.baud = 1;
   od_control.user_screenwidth = 100;
   od_control.user_screen_length = 40;
   ODSessionScreenInitialize(80, 25);
   CHECK(ODSessionScreenAvailable());
   CHECK(ODSessionScreenWidth() == 100);
   CHECK(ODSessionScreenHeight() == 40);
   od_control.baud = 0;

   CHECK(RejectRectangle(1, 1, 101, 40));
   CHECK(RejectRectangle(1, 1, 100, 41));

   CHECK(ODInQueueAlloc(&hODInputQueue, 2) == kODRCSuccess);
   CHECK(AddEscape() == kODRCSuccess);
   memset(&Options, 0, sizeof(Options));
   Options.nAreaLeft = 1;
   Options.nAreaTop = 1;
   Options.nAreaRight = 100;
   Options.nAreaBottom = 40;
   CHECK(od_multiline_edit(szBuffer, sizeof(szBuffer), &Options)
      == OD_MULTIEDIT_SUCCESS);
   ODInQueueFree(hODInputQueue);
   hODInputQueue = NULL;

   ODSessionScreenShutdown();
   od_control.baud = 1;
   od_control.user_ansi = FALSE;
   od_control.user_avatar = TRUE;
   od_control.user_screenwidth = 80;
   od_control.user_screen_length = 300;
   ODSessionScreenInitialize(80, 25);
   CHECK(ODSessionScreenAvailable());
   od_control.baud = 0;
   CHECK(RejectRectangle(1, 1, 80, 256));

   ODSessionScreenShutdown();
   bODInitialized = FALSE;
   ODScrnShutdown();


   return(0);
}
