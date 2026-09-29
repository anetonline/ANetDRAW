#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "OpenDoor.h"
#include "ODPlat.h"

#define CHECK(condition) do { if(!(condition)) { result = __LINE__; goto cleanup; } } while(0)

int main(void)
{
   char socketName[64];
   struct sockaddr_un address;
   INT socketHandle = -1;
   tODDirHandle directory;
   tODDirEntry entry;
   BOOL socketExists = FALSE;
   BOOL directoryOpen = FALSE;
   INT result = 0;

   sprintf(socketName, "ODT%lu.SOC", (unsigned long)getpid());
   memset(&address, 0, sizeof(address));
   address.sun_family = AF_UNIX;
   strcpy(address.sun_path, socketName);

   socketHandle = socket(AF_UNIX, SOCK_STREAM, 0);
   CHECK(socketHandle != -1);
   CHECK(bind(socketHandle, (struct sockaddr *)&address, sizeof(address)) == 0);
   socketExists = TRUE;

   CHECK(ODDirOpen(socketName, DIR_ATTRIB_NORMAL, &directory)
      == kODRCSuccess);
   directoryOpen = TRUE;
   CHECK(ODDirRead(directory, &entry) == kODRCSuccess);
   CHECK((entry.wAttributes & DIR_ATTRIB_DIREC) == 0);

cleanup:
   if(directoryOpen)
      ODDirClose(directory);
   if(socketHandle != -1)
      close(socketHandle);
   if(socketExists)
      unlink(socketName);
   return(result);
}
