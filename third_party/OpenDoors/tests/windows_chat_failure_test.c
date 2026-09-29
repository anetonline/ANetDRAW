#include <stdio.h>
#include <string.h>
#include <time.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODGen.h"
#include "ODInQue.h"
#include "ODKrnl.h"
#include "ODSync.h"

#define CHECK(condition) do { if(!(condition)) { \
   fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
      #condition); \
   return(__LINE__); \
} } while(0)

static DWORD dwOwnerThread;
static BOOL bTimerCallbackOnOwner;
static unsigned nKernelCallbackCount;

static void KernelCallback(void)
{
   ++nKernelCallbackCount;
}

static void TimeMessage(char *pszMessage)
{
   (void)pszMessage;
   bTimerCallbackOnOwner = GetCurrentThreadId() == dwOwnerThread
      && od_control_get() == &od_control;
}

static int ODTestMain(void)
{
   memset(&od_control, 0, sizeof(od_control));
   dwOwnerThread = GetCurrentThreadId();
   CHECK(ODSyncSessionInitialize() == kODRCSuccess);
   CHECK(ODInQueueAlloc(&hODInputQueue, 4) == kODRCSuccess);
   CHECK(ODKrnlInitialize() == kODRCSuccess);
   bODInitialized = TRUE;
   eODLifecycleState = kODLifecycleActive;
   od_control.user_timelimit = 10;
   od_control.od_chat_active = TRUE;
   CHECK(ODKrnlRefreshUIState());

   od_control.od_ker_exec = KernelCallback;
   od_kernel();
   od_kernel();
   CHECK(nKernelCallbackCount == 2);

   ODKrnlRequestChatToggle();
   ODKrnlDispatchPending(TRUE);
   CHECK(!od_control.od_chat_active);

   /* Ordered non-modal requests leave state unchanged after two toggles. */
   od_control.od_user_keyboard_on = TRUE;
   CHECK(ODKrnlRefreshUIState());
   ODKrnlRequestKeyboardToggle();
   ODKrnlRequestKeyboardToggle();
   ODKrnlDispatchPending(TRUE);
   CHECK(od_control.od_user_keyboard_on);

   od_control.od_chat_active = FALSE;
   od_control.od_disable_inactivity = TRUE;
   od_control.od_time_msg_func = TimeMessage;
   od_control.od_time_warning = (char *)"%d";
   od_control.user_timelimit = 2;
   nNextTimeDeductTime = time(NULL);
   od_control.od_ker_exec = NULL;
   od_kernel();
   CHECK(bTimerCallbackOnOwner);
   CHECK(od_control.user_timelimit == 1);

   bODInitialized = FALSE;
   ODKrnlShutdown();
   ODInQueueFree(hODInputQueue);
   hODInputQueue = NULL;
   ODSyncSessionShutdown();
   return(0);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInstance,
   LPSTR pszCommandLine, int nShowCommand)
{
   (void)hInstance;
   (void)hPreviousInstance;
   (void)pszCommandLine;
   (void)nShowCommand;
   return(ODTestMain());
}
