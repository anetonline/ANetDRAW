#include <stdio.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODPlat.h"

#define CHECK(condition) do { if(!(condition)) { result = __LINE__; goto cleanup; } } while(0)

int main(void)
{
   char firstCreated[MAX_PATH];
   char secondCreated[MAX_PATH];
   char pattern[MAX_PATH];
   char firstFound[MAX_PATH];
   char secondFound[MAX_PATH];
   DWORD uniquePart1;
   DWORD uniquePart2;
   HANDLE fileHandle;
   HANDLE findHandle = INVALID_HANDLE_VALUE;
   WIN32_FIND_DATAA findData;
   tODDirHandle directory;
   tODDirEntry entry;
   BOOL directoryOpen = FALSE;
   BOOL firstExists = FALSE;
   BOOL secondExists = FALSE;
   INT result = 0;

   uniquePart1 = GetCurrentProcessId() & 0x0fff;
   uniquePart2 = GetTickCount() & 0x0fff;
   sprintf(pattern, "D%03lX%03lX?.TMP", (unsigned long)uniquePart1,
      (unsigned long)uniquePart2);
   sprintf(firstCreated, "D%03lX%03lXA.TMP", (unsigned long)uniquePart1,
      (unsigned long)uniquePart2);
   sprintf(secondCreated, "D%03lX%03lXB.TMP", (unsigned long)uniquePart1,
      (unsigned long)uniquePart2);

   fileHandle = CreateFileA(firstCreated, GENERIC_WRITE, 0, NULL, CREATE_NEW,
      FILE_ATTRIBUTE_NORMAL, NULL);
   CHECK(fileHandle != INVALID_HANDLE_VALUE);
   firstExists = TRUE;
   CHECK(CloseHandle(fileHandle));

   fileHandle = CreateFileA(secondCreated, GENERIC_WRITE, 0, NULL, CREATE_NEW,
      FILE_ATTRIBUTE_NORMAL, NULL);
   CHECK(fileHandle != INVALID_HANDLE_VALUE);
   secondExists = TRUE;
   CHECK(CloseHandle(fileHandle));

   /* Use Windows' actual enumeration order rather than assuming an order. */
   findHandle = FindFirstFileA(pattern, &findData);
   CHECK(findHandle != INVALID_HANDLE_VALUE);
   strcpy(firstFound, findData.cFileName);
   CHECK(FindNextFileA(findHandle, &findData));
   strcpy(secondFound, findData.cFileName);
   CHECK(!FindNextFileA(findHandle, &findData));
   CHECK(GetLastError() == ERROR_NO_MORE_FILES);
   CHECK(FindClose(findHandle));
   findHandle = INVALID_HANDLE_VALUE;

   CHECK(SetFileAttributesA(secondFound, FILE_ATTRIBUTE_HIDDEN));
   CHECK(ODDirOpen(pattern, DIR_ATTRIB_NORMAL, &directory) == kODRCSuccess);
   directoryOpen = TRUE;
   CHECK(ODDirRead(directory, &entry) == kODRCSuccess);
   CHECK(_stricmp(entry.szFileName, firstFound) == 0);
   CHECK(ODDirRead(directory, &entry) == kODRCEndOfFile);

cleanup:
   if(directoryOpen)
   {
      ODDirClose(directory);
   }
   if(findHandle != INVALID_HANDLE_VALUE)
   {
      FindClose(findHandle);
   }
   if(firstExists)
   {
      SetFileAttributesA(firstCreated, FILE_ATTRIBUTE_NORMAL);
      DeleteFileA(firstCreated);
   }
   if(secondExists)
   {
      SetFileAttributesA(secondCreated, FILE_ATTRIBUTE_NORMAL);
      DeleteFileA(secondCreated);
   }
   return(result);
}
