#include <windows.h>

#if defined(_MSC_VER) && defined(_M_IX86)
#pragma comment(linker, "/export:DoorInitialize=_DoorInitialize@0")
#pragma comment(linker, "/export:DoorShutdown=_DoorShutdown@0")
#pragma comment(linker, "/export:DoorWrite=_DoorWrite@8")
#pragma comment(linker, "/export:DoorRead=_DoorRead@8")
#pragma comment(linker, "/export:DoorGetAvailableEventHandle=_DoorGetAvailableEventHandle@0")
#pragma comment(linker, "/export:DoorGetOfflineEventHandle=_DoorGetOfflineEventHandle@0")
#endif

__declspec(dllexport) BOOL WINAPI DoorInitialize(void)
{
   char value[2];
   return(GetEnvironmentVariableA("OPENDOORS_UNIT_DOOR32_INIT_FAIL", value,
      sizeof(value)) == 0);
}

__declspec(dllexport) BOOL WINAPI DoorShutdown(void)
{
   return(TRUE);
}

__declspec(dllexport) BOOL WINAPI DoorWrite(const unsigned char *data,
   DWORD size)
{
   (void)data;
   (void)size;
   return(TRUE);
}

__declspec(dllexport) DWORD WINAPI DoorRead(unsigned char *data, DWORD size)
{
   (void)data;
   (void)size;
   return(0);
}

__declspec(dllexport) HANDLE WINAPI DoorGetAvailableEventHandle(void)
{
   return((HANDLE)(UINT_PTR)1);
}

__declspec(dllexport) HANDLE WINAPI DoorGetOfflineEventHandle(void)
{
   return((HANDLE)(UINT_PTR)1);
}
