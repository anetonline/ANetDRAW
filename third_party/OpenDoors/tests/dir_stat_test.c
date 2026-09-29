#include <stdio.h>
#include <unistd.h>

#include "OpenDoor.h"
#include "ODPlat.h"

#define CHECK(condition) do { if(!(condition)) { result = __LINE__; goto cleanup; } } while(0)

int main(void)
{
   char fixtureName[64];
   FILE *fixture = NULL;
   tODDirHandle directory;
   tODDirEntry entry;
   BOOL directoryOpen = FALSE;
   BOOL fixtureExists = FALSE;
   INT result = 0;

   sprintf(fixtureName, "ODS%lu.TMP", (unsigned long)getpid());
   fixture = fopen(fixtureName, "wb");
   CHECK(fixture != NULL);
   CHECK(fclose(fixture) == 0);
   fixture = NULL;
   fixtureExists = TRUE;

   CHECK(ODDirOpen(fixtureName, DIR_ATTRIB_NORMAL, &directory)
      == kODRCSuccess);
   directoryOpen = TRUE;
   CHECK(remove(fixtureName) == 0);
   fixtureExists = FALSE;

   CHECK(ODDirRead(directory, &entry) == kODRCEndOfFile);
   CHECK(ODDirRead(directory, &entry) == kODRCEndOfFile);

cleanup:
   if(directoryOpen)
      ODDirClose(directory);
   if(fixture != NULL)
      fclose(fixture);
   if(fixtureExists)
      remove(fixtureName);
   return(result);
}
