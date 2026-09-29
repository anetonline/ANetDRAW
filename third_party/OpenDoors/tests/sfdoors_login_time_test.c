#include <stdio.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODCore.h"

#define DROP_FILENAME "SFDOORS.DAT"

static int WriteDropFile(void)
{
   static const char *const apszLines[] = {
      "1", "Caller Name", "password", "unused", "0", "0", "60",
      "unused", "unused", "TRUE", "10", "0", "0", "unused", "754",
      "unused", "FALSE", "unused", "unused", "unused", "TRUE", "1",
      "1", "1", "unused", "unused", "unused", "0", "0", "0",
      "555-1212", "Somewhere", "unused", "FALSE", "FALSE", "unused",
      "0", "0", "unused"
   };
   FILE *pFile;
   size_t nLine;
   int nResult = 1;

   pFile = fopen(DROP_FILENAME, "w");
   if(pFile == NULL)
      return(0);
   for(nLine = 0; nLine < sizeof(apszLines) / sizeof(apszLines[0]); ++nLine)
   {
      if(fprintf(pFile, "%s\n", apszLines[nLine]) < 0)
         nResult = 0;
   }
   if(fclose(pFile) != 0)
      nResult = 0;
   return(nResult);
}

int main(void)
{
   int nResult = 0;

#define CHECK(condition) do { if(!(condition)) { nResult = __LINE__; goto cleanup; } } while(0)

   remove(DROP_FILENAME);
   CHECK(WriteDropFile());

   memset(&od_control, 0, sizeof(od_control));
   strcpy(od_control.info_path, DROP_FILENAME);
   od_control.od_disable = DIS_NAME_PROMPT | DIS_BPS_SETTING;
   od_control.od_silent_mode = TRUE;
   od_control.od_noexit = TRUE;

   od_init();
   CHECK(od_control.od_info_type == SFDOORSDAT);
   CHECK(strcmp(od_control.user_logintime, "12:34") == 0);

cleanup:
   if(bODInitialized)
      od_exit(0, FALSE);
   remove(DROP_FILENAME);
   return(nResult);
}
