#include <stdio.h>

#include "OpenDoor.h"
#include "ODPlat.h"

#define CHECK(condition) do { if(!(condition)) return(__LINE__); } while(0)

int main(void)
{
   const char *fixtureName = "ODATTR.TMP";
   FILE *fixture;
   tODDirHandle directory;
   tODDirEntry entry;
#ifdef ODPLAT_WIN32
   const char *hiddenFixtureName = "ODHIDE.TMP";
   DWORD handlesBefore;
   DWORD handlesAfter;
   INT index;
#endif

   CHECK(ODDirAttributesMatch(DIR_ATTRIB_NORMAL, DIR_ATTRIB_NORMAL));
   CHECK(ODDirAttributesMatch(DIR_ATTRIB_ARCH | DIR_ATTRIB_RDONLY,
      DIR_ATTRIB_NORMAL));
   CHECK(!ODDirAttributesMatch(DIR_ATTRIB_HIDDEN, DIR_ATTRIB_NORMAL));
   CHECK(ODDirAttributesMatch(DIR_ATTRIB_HIDDEN, DIR_ATTRIB_HIDDEN));
   CHECK(!ODDirAttributesMatch(DIR_ATTRIB_LABEL, DIR_ATTRIB_NORMAL));
   CHECK(ODDirAttributesMatch(DIR_ATTRIB_LABEL, DIR_ATTRIB_LABEL));
   CHECK(!ODDirAttributesMatch(DIR_ATTRIB_SYSTEM | DIR_ATTRIB_DIREC,
      DIR_ATTRIB_DIREC));
   CHECK(ODDirAttributesMatch(DIR_ATTRIB_SYSTEM | DIR_ATTRIB_DIREC,
      DIR_ATTRIB_SYSTEM | DIR_ATTRIB_DIREC));

   fixture = fopen(fixtureName, "wb");
   CHECK(fixture != NULL);
   CHECK(fclose(fixture) == 0);

   CHECK(ODDirOpen(fixtureName, DIR_ATTRIB_DIREC, &directory)
      == kODRCSuccess);
   CHECK(ODDirRead(directory, &entry) == kODRCSuccess);
   ODDirClose(directory);

   CHECK(remove(fixtureName) == 0);
   CHECK(ODDirOpen(fixtureName, DIR_ATTRIB_DIREC, &directory)
      == kODRCNoMatch);

#ifdef ODPLAT_WIN32
   fixture = fopen(hiddenFixtureName, "wb");
   CHECK(fixture != NULL);
   CHECK(fclose(fixture) == 0);
   CHECK(SetFileAttributesA(hiddenFixtureName, FILE_ATTRIBUTE_HIDDEN));
   CHECK(GetProcessHandleCount(GetCurrentProcess(), &handlesBefore));
   for(index = 0; index < 32; ++index)
   {
      CHECK(ODDirOpen(hiddenFixtureName, DIR_ATTRIB_NORMAL, &directory)
         == kODRCNoMatch);
   }
   CHECK(GetProcessHandleCount(GetCurrentProcess(), &handlesAfter));
   CHECK(handlesAfter == handlesBefore);
   CHECK(SetFileAttributesA(hiddenFixtureName, FILE_ATTRIBUTE_NORMAL));
   CHECK(remove(hiddenFixtureName) == 0);
#endif

   return(0);
}
