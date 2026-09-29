#include <winsock2.h>
#include <windows.h>

#include "test_support.h"

typedef struct
{
   CRITICAL_SECTION *pLock;
   BOOL bExit;
   BOOL bSucceeded;
} tThreadHandoffContext;

static DWORD WINAPI ODTestHandoffThread(void *pParameter)
{
   tThreadHandoffContext *pContext = (tThreadHandoffContext *)pParameter;

   EnterCriticalSection(pContext->pLock);
   if(pContext->bExit)
   {
      od_exit(0, FALSE);
      pContext->bSucceeded = od_control_get() == NULL;
   }
   else
   {
      od_disp_str("worker thread");
      pContext->bSucceeded = od_control_get() == &od_control;
   }
   LeaveCriticalSection(pContext->pLock);
   return(0);
}

static BOOL ODTestRunHandoff(CRITICAL_SECTION *pLock, BOOL bExit)
{
   HANDLE hThread;
   tThreadHandoffContext Context;

   memset(&Context, 0, sizeof(Context));
   Context.pLock = pLock;
   Context.bExit = bExit;
   hThread = CreateThread(NULL, 0, ODTestHandoffThread, &Context, 0, NULL);
   if(hThread == NULL)
      return(FALSE);
   if(WaitForSingleObject(hThread, 10000) != WAIT_OBJECT_0)
   {
      CloseHandle(hThread);
      return(FALSE);
   }
   CloseHandle(hThread);
   return(Context.bSucceeded);
}

static int ODTestMain(void)
{
   CRITICAL_SECTION Lock;
   BOOL bNormalCallSucceeded;
   BOOL bExitSucceeded;

   InitializeCriticalSection(&Lock);
   EnterCriticalSection(&Lock);
   ODTestConfigureLocal();
   od_init();
   LeaveCriticalSection(&Lock);

   bNormalCallSucceeded = ODTestRunHandoff(&Lock, FALSE);

   EnterCriticalSection(&Lock);
   od_disp_str("main thread");
   LeaveCriticalSection(&Lock);

   bExitSucceeded = ODTestRunHandoff(&Lock, TRUE);
   if(!bExitSucceeded)
   {
      /* The old lifetime-owner implementation leaves the request queued. */
      EnterCriticalSection(&Lock);
      od_kernel();
      LeaveCriticalSection(&Lock);
   }
   DeleteCriticalSection(&Lock);

   OD_TEST_CHECK(bNormalCallSucceeded);
   OD_TEST_CHECK(bExitSucceeded);
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
