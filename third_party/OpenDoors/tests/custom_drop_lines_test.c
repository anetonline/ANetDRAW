#include <stdio.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODCore.h"

#define CONFIG_FILENAME "ODCFGLIN.TMP"
#define DROP_FILENAME "ODDRPLIN.TMP"

int main(void)
{
   static const char acDropContents[] = "\n\r\nCaller Name\n";
   FILE *pFile = NULL;
   int nResult = 0;

#define CHECK(condition) do { if(!(condition)) { nResult = __LINE__; goto cleanup; } } while(0)

   remove(CONFIG_FILENAME);
   remove(DROP_FILENAME);

   pFile = fopen(DROP_FILENAME, "wb");
   CHECK(pFile != NULL);
   CHECK(fwrite(acDropContents, 1, sizeof(acDropContents) - 1, pFile)
      == sizeof(acDropContents) - 1);
   CHECK(fclose(pFile) == 0);
   pFile = NULL;

   pFile = fopen(CONFIG_FILENAME, "w");
   CHECK(pFile != NULL);
   CHECK(fputs("CustomFileName " DROP_FILENAME "\n", pFile) >= 0);
   CHECK(fputs("CustomFileLine Ignore\n", pFile) >= 0);
   CHECK(fputs("CustomFileLine Ignore\n", pFile) >= 0);
   CHECK(fputs("CustomFileLine UserName\n", pFile) >= 0);
   CHECK(fclose(pFile) == 0);
   pFile = NULL;

   memset(&od_control, 0, sizeof(od_control));
   od_control.od_config_file = INCLUDE_CONFIG_FILE;
   od_control.od_config_filename = CONFIG_FILENAME;
   od_control.od_disable = DIS_NAME_PROMPT;
   od_control.od_silent_mode = TRUE;
   od_control.od_noexit = TRUE;

   od_init();
   CHECK(od_control.od_info_type == CUSTOM);
   CHECK(strcmp(od_control.user_name, "Caller Name") == 0);

cleanup:
   if(pFile != NULL)
      fclose(pFile);
   if(bODInitialized)
      od_exit(0, FALSE);
   remove(CONFIG_FILENAME);
   remove(DROP_FILENAME);
   return(nResult);
}
