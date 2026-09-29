#include <winsock2.h>
#include <windows.h>

#include "test_support.h"

typedef struct
{
   HANDLE hPublished;
   BOOL bFoundWindow;
   BOOL bPainted;
   BOOL bPostedKey;
} tScreenTestContext;

static BOOL ODTestCreateSocketPair(SOCKET *pDoorSocket, SOCKET *pPeerSocket)
{
   WSADATA SocketData;
   SOCKET hListener = INVALID_SOCKET;
   SOCKET hDoorSocket = INVALID_SOCKET;
   SOCKET hPeerSocket = INVALID_SOCKET;
   struct sockaddr_in Address;
   int nAddressSize = sizeof(Address);

   if(WSAStartup(MAKEWORD(2, 2), &SocketData) != 0)
      return(FALSE);
   hListener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if(hListener == INVALID_SOCKET)
      goto failure;

   memset(&Address, 0, sizeof(Address));
   Address.sin_family = AF_INET;
   Address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   if(bind(hListener, (struct sockaddr *)&Address, sizeof(Address)) != 0
      || listen(hListener, 1) != 0
      || getsockname(hListener, (struct sockaddr *)&Address,
         &nAddressSize) != 0)
   {
      goto failure;
   }

   hPeerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if(hPeerSocket == INVALID_SOCKET
      || connect(hPeerSocket, (struct sockaddr *)&Address,
         sizeof(Address)) != 0)
   {
      goto failure;
   }
   hDoorSocket = accept(hListener, NULL, NULL);
   if(hDoorSocket == INVALID_SOCKET)
      goto failure;

   closesocket(hListener);
   *pDoorSocket = hDoorSocket;
   *pPeerSocket = hPeerSocket;
   return(TRUE);

failure:
   if(hListener != INVALID_SOCKET) closesocket(hListener);
   if(hDoorSocket != INVALID_SOCKET) closesocket(hDoorSocket);
   if(hPeerSocket != INVALID_SOCKET) closesocket(hPeerSocket);
   WSACleanup();
   return(FALSE);
}

static DWORD WINAPI ODTestScreenThread(void *pParameter)
{
   tScreenTestContext *pContext = (tScreenTestContext *)pParameter;
   HWND hwndFrame = NULL;
   HWND hwndScreen = NULL;
   DWORD dwDeadline = GetTickCount() + 10000UL;

   while(hwndScreen == NULL && (LONG)(dwDeadline - GetTickCount()) > 0)
   {
      hwndFrame = FindWindowA("ODFrame", NULL);
      if(hwndFrame != NULL)
         hwndScreen = FindWindowExA(hwndFrame, NULL, "ODScreen", NULL);
      if(hwndScreen == NULL) Sleep(10);
   }
   if(hwndScreen == NULL)
      return(1);
   pContext->bFoundWindow = TRUE;

   if(WaitForSingleObject(pContext->hPublished, 10000) != WAIT_OBJECT_0)
      return(2);
   if(!InvalidateRect(hwndScreen, NULL, FALSE))
      return(3);
   pContext->bPainted = UpdateWindow(hwndScreen);
   pContext->bPostedKey = PostMessage(hwndScreen, WM_CHAR, 'Z',
      (LPARAM)((0x2cUL << 16) | 1UL));
   return(pContext->bPainted && pContext->bPostedKey ? 0 : 4);
}

static int ODTestMain(void)
{
   SOCKET hDoorSocket;
   SOCKET hPeerSocket;
   HANDLE hThread;
   HANDLE hPublished;
   DWORD dwThreadResult;
   tScreenTestContext Context;
   tODInputEvent Event;

   OD_TEST_CHECK(ODTestCreateSocketPair(&hDoorSocket, &hPeerSocket));
   hPublished = CreateEvent(NULL, TRUE, FALSE, NULL);
   OD_TEST_CHECK(hPublished != NULL);
   memset(&Context, 0, sizeof(Context));
   Context.hPublished = hPublished;
   hThread = CreateThread(NULL, 0, ODTestScreenThread, &Context, 0, NULL);
   OD_TEST_CHECK(hThread != NULL);

   memset(&od_control, 0, sizeof(od_control));
   od_control.baud = 38400;
   od_control.od_connect_speed = 38400;
   od_control.od_use_socket = TRUE;
   od_control.od_open_handle = (DWORD_PTR)hDoorSocket;
   od_control.od_force_local = TRUE;
   od_control.od_disable = DIS_INFOFILE | DIS_NAME_PROMPT |
      DIS_CARRIERDETECT | DIS_TIMEOUT | DIS_SYSOP_KEYS;
   od_control.od_nocopyright = TRUE;
   od_control.od_noexit = TRUE;
   od_init();

   od_disp("published screen", 16, FALSE);
   OD_TEST_CHECK(SetEvent(hPublished));
   OD_TEST_CHECK(od_get_input(&Event, 10000, GETIN_RAW));
   OD_TEST_CHECK(Event.EventType == EVENT_CHARACTER);
   OD_TEST_CHECK(!Event.bFromRemote);
   OD_TEST_CHECK(Event.chKeyPress == 'Z');
   OD_TEST_CHECK(WaitForSingleObject(hThread, 10000) == WAIT_OBJECT_0);
   OD_TEST_CHECK(GetExitCodeThread(hThread, &dwThreadResult));
   OD_TEST_CHECK(dwThreadResult == 0);
   OD_TEST_CHECK(Context.bFoundWindow && Context.bPainted && Context.bPostedKey);

   od_exit(0, FALSE);
   CloseHandle(hThread);
   CloseHandle(hPublished);
   closesocket(hPeerSocket);
   WSACleanup();
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
