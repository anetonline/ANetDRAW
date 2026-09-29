#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODInEx.h"

#define CONFIG_FILENAME "ODCFGPTH.TMP"
#define DOOR_DIRECTORY_CHARACTERS 79

int main(void)
{
   FILE *pConfigFile = NULL;
   char szCurrentDirectory[512];
   char szDoorDirectory[DOOR_DIRECTORY_CHARACTERS + 1];
   char szLongBBSDir[sizeof(od_control.info_path) + 32];
   char szLongDoorDirectory[sizeof(szDoorDirectory) + 1];
   char szLongLogFileName[sizeof(od_control.od_logfile_name) + 32];
   char szOriginalDirectory[512];
   size_t nOriginalLength;
   int nResult = 0;

#define CHECK(condition) do { if(!(condition)) { nResult = __LINE__; goto cleanup; } } while(0)

   memset(szDoorDirectory, 'D', sizeof(szDoorDirectory) - 1);
   szDoorDirectory[sizeof(szDoorDirectory) - 1] = '\0';
   memcpy(szLongDoorDirectory, szDoorDirectory, sizeof(szDoorDirectory) - 1);
   szLongDoorDirectory[sizeof(szDoorDirectory) - 1] = 'X';
   szLongDoorDirectory[sizeof(szDoorDirectory)] = '\0';

   memset(szLongBBSDir, 'B', sizeof(szLongBBSDir) - 1);
   szLongBBSDir[sizeof(szLongBBSDir) - 1] = '\0';
   memset(szLongLogFileName, 'L', sizeof(szLongLogFileName) - 1);
   szLongLogFileName[sizeof(szLongLogFileName) - 1] = '\0';

   remove(CONFIG_FILENAME);
   rmdir(szDoorDirectory);
   CHECK(mkdir(szDoorDirectory, 0700) == 0);
   ODDirGetCurrent(szOriginalDirectory, sizeof(szOriginalDirectory));
   CHECK(szOriginalDirectory[0] != '\0');

   pConfigFile = fopen(CONFIG_FILENAME, "w");
   CHECK(pConfigFile != NULL);
   CHECK(fprintf(pConfigFile, "BBSDir %s\n", szLongBBSDir) > 0);
   CHECK(fprintf(pConfigFile, "LogFileName %s\n", szLongLogFileName) > 0);
   CHECK(fputs("DoorDir .\n", pConfigFile) >= 0);
   CHECK(fprintf(pConfigFile, "DoorDir %s\n", szLongDoorDirectory) > 0);
   CHECK(fclose(pConfigFile) == 0);
   pConfigFile = NULL;

   memset(&od_control, 0, sizeof(od_control));
   od_control.od_config_file = INCLUDE_CONFIG_FILE;
   od_control.od_config_filename = CONFIG_FILENAME;
   od_control.od_force_local = TRUE;
   od_control.od_disable = DIS_NAME_PROMPT;
   od_control.od_silent_mode = TRUE;
   od_control.od_noexit = TRUE;

   od_init();
   CHECK(strlen(od_control.info_path) == sizeof(od_control.info_path) - 1);
   CHECK(strncmp(od_control.info_path, szLongBBSDir,
      sizeof(od_control.info_path) - 1) == 0);
   CHECK(strlen(od_control.od_logfile_name)
      == sizeof(od_control.od_logfile_name) - 1);
   CHECK(strncmp(od_control.od_logfile_name, szLongLogFileName,
      sizeof(od_control.od_logfile_name) - 1) == 0);

   ODDirGetCurrent(szCurrentDirectory, sizeof(szCurrentDirectory));
   nOriginalLength = strlen(szOriginalDirectory);
   CHECK(strlen(szCurrentDirectory) > nOriginalLength);
   CHECK(strncmp(szCurrentDirectory, szOriginalDirectory,
      nOriginalLength) == 0);
   CHECK(szCurrentDirectory[nOriginalLength] == DIRSEP);
   CHECK(strcmp(szCurrentDirectory + nOriginalLength + 1,
      szDoorDirectory) == 0);

   od_exit(0, FALSE);
   ODDirGetCurrent(szCurrentDirectory, sizeof(szCurrentDirectory));
   CHECK(strcmp(szCurrentDirectory, szOriginalDirectory) == 0);

cleanup:
   if(pConfigFile != NULL)
      fclose(pConfigFile);
   if(bODInitialized)
      od_exit(0, FALSE);
   remove(CONFIG_FILENAME);
   rmdir(szDoorDirectory);
   return(nResult);
}
