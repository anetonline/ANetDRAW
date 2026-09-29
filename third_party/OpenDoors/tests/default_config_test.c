#include <stdio.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODCore.h"

#define DEFAULT_CONFIG_FILENAME "door.cfg"

int main(void)
{
   FILE *pConfigFile = NULL;
   int nResult = 0;
   char szTimeLeft[16];

#define CHECK(condition) do { if(!(condition)) { nResult = __LINE__; goto cleanup; } } while(0)

   remove(DEFAULT_CONFIG_FILENAME);
   pConfigFile = fopen(DEFAULT_CONFIG_FILENAME, "w");
   CHECK(pConfigFile != NULL);
   CHECK(fputs("MaximumDoorTime 37\n", pConfigFile) >= 0);
   CHECK(fclose(pConfigFile) == 0);
   pConfigFile = NULL;

   memset(&od_control, 0, sizeof(od_control));
   od_control.od_config_file = INCLUDE_CONFIG_FILE;
   od_control.od_force_local = TRUE;
   od_control.od_disable = DIS_NAME_PROMPT;
   od_control.od_silent_mode = TRUE;
   od_control.od_noexit = TRUE;

   od_init();
   CHECK(od_control.od_config_filename != NULL);
   CHECK(strcmp(od_control.od_config_filename, DEFAULT_CONFIG_FILENAME) == 0);
   CHECK(od_control.od_maxtime == 37);
   CHECK(strcmp(od_control.od_time_left, "%4d mins  ") == 0);
   CHECK(sprintf(szTimeLeft, od_control.od_time_left, 0) == 11);
   CHECK(strcmp(szTimeLeft, "   0 mins  ") == 0);
   CHECK(sprintf(szTimeLeft, od_control.od_time_left, 99) == 11);
   CHECK(strcmp(szTimeLeft, "  99 mins  ") == 0);
   CHECK(sprintf(szTimeLeft, od_control.od_time_left, 999) == 11);
   CHECK(strcmp(szTimeLeft, " 999 mins  ") == 0);
   CHECK(sprintf(szTimeLeft, od_control.od_time_left, 1440) == 11);
   CHECK(strcmp(szTimeLeft, "1440 mins  ") == 0);

cleanup:
   if(pConfigFile != NULL)
      fclose(pConfigFile);
   if(bODInitialized)
      od_exit(0, FALSE);
   remove(DEFAULT_CONFIG_FILENAME);
   return(nResult);
}
