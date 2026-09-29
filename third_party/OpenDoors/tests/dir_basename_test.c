#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "OpenDoor.h"
#include "ODPlat.h"

#define CHECK(condition) do { if(!(condition)) { result = __LINE__; goto cleanup; } } while(0)

int main(void)
{
   const char *entryName = "ENTRY.TMP";
   char directoryName[64];
   char entryPath[96];
   char pattern[96];
   FILE *fixture = NULL;
   tODDirHandle directory;
   tODDirEntry entry;
   BOOL directoryOpen = FALSE;
   BOOL directoryCreated = FALSE;
   BOOL entryCreated = FALSE;
   INT result = 0;

   sprintf(directoryName, "ODB%lu.DIR", (unsigned long)getpid());
   sprintf(entryPath, "%s%c%s", directoryName, DIRSEP, entryName);
   sprintf(pattern, "%s%c*", directoryName, DIRSEP);

   CHECK(mkdir(directoryName, 0700) == 0);
   directoryCreated = TRUE;
   fixture = fopen(entryPath, "wb");
   CHECK(fixture != NULL);
   CHECK(fclose(fixture) == 0);
   fixture = NULL;
   entryCreated = TRUE;

   CHECK(ODDirOpen(pattern, DIR_ATTRIB_NORMAL, &directory) == kODRCSuccess);
   directoryOpen = TRUE;
   CHECK(ODDirRead(directory, &entry) == kODRCSuccess);
   CHECK(strcmp(entry.szFileName, entryName) == 0);

cleanup:
   if(directoryOpen)
      ODDirClose(directory);
   if(fixture != NULL)
      fclose(fixture);
   if(entryCreated)
      remove(entryPath);
   if(directoryCreated)
      rmdir(directoryName);
   return(result);
}
