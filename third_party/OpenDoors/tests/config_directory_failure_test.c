#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODInEx.h"

#define CONFIG_FILENAME "ODCFGDFL.TMP"
#define DIRECTORY_LEVELS 5
#define DIRECTORY_COMPONENT_CHARACTERS 100

int main(void)
{
   FILE *pConfigFile = NULL;
   char aszDirectory[DIRECTORY_LEVELS][1024];
   char szComponent[DIRECTORY_COMPONENT_CHARACTERS + 1];
   char szConfigPath[1024];
   char szCurrentDirectory[1024];
   char szOriginalDirectory[1024];
   const char *pszParent;
   int nCharacters;
   int nCreatedDirectories = 0;
   int nDirectory;
   int nResult = 0;

#define CHECK(condition) do { if(!(condition)) { nResult = __LINE__; goto cleanup; } } while(0)

   szConfigPath[0] = '\0';
   szOriginalDirectory[0] = '\0';
   ODDirGetCurrent(szOriginalDirectory, sizeof(szOriginalDirectory));
   CHECK(szOriginalDirectory[0] != '\0');
   nCharacters = snprintf(szConfigPath, sizeof(szConfigPath), "%s%s%s",
      szOriginalDirectory, DIRSEP_STR, CONFIG_FILENAME);
   CHECK(nCharacters > 0 && (size_t)nCharacters < sizeof(szConfigPath));
   if(szConfigPath[0] != '\0')
      remove(szConfigPath);

   pConfigFile = fopen(szConfigPath, "w");
   CHECK(pConfigFile != NULL);
   CHECK(fprintf(pConfigFile, "DoorDir %s\n", szOriginalDirectory) > 0);
   CHECK(fclose(pConfigFile) == 0);
   pConfigFile = NULL;

   memset(szComponent, 'D', sizeof(szComponent) - 1);
   szComponent[sizeof(szComponent) - 1] = '\0';
   for(nDirectory = 0; nDirectory < DIRECTORY_LEVELS; ++nDirectory)
   {
      szComponent[0] = (char)('A' + nDirectory);
      pszParent = nDirectory == 0
         ? szOriginalDirectory : aszDirectory[nDirectory - 1];
      nCharacters = snprintf(aszDirectory[nDirectory],
         sizeof(aszDirectory[nDirectory]), "%s%s%s", pszParent,
         DIRSEP_STR, szComponent);
      CHECK(nCharacters > 0
         && (size_t)nCharacters < sizeof(aszDirectory[nDirectory]));
      CHECK(mkdir(aszDirectory[nDirectory], 0700) == 0);
      ++nCreatedDirectories;
   }

   ODDirChangeCurrent(aszDirectory[DIRECTORY_LEVELS - 1]);
   ODDirGetCurrent(szCurrentDirectory, sizeof(szCurrentDirectory));
   CHECK(strcmp(szCurrentDirectory,
      aszDirectory[DIRECTORY_LEVELS - 1]) == 0);

   memset(&od_control, 0, sizeof(od_control));
   od_control.od_config_file = INCLUDE_CONFIG_FILE;
   od_control.od_config_filename = szConfigPath;
   od_control.od_force_local = TRUE;
   od_control.od_disable = DIS_NAME_PROMPT;
   od_control.od_silent_mode = TRUE;
   od_control.od_noexit = TRUE;

   od_init();
   CHECK(szOriginalDir == NULL);
   ODDirGetCurrent(szCurrentDirectory, sizeof(szCurrentDirectory));
   CHECK(strcmp(szCurrentDirectory,
      aszDirectory[DIRECTORY_LEVELS - 1]) == 0);

cleanup:
   if(pConfigFile != NULL)
      fclose(pConfigFile);
   if(bODInitialized)
      od_exit(0, FALSE);
   if(szOriginalDirectory[0] != '\0')
      ODDirChangeCurrent(szOriginalDirectory);
   if(szConfigPath[0] != '\0')
      remove(szConfigPath);
   while(nCreatedDirectories > 0)
   {
      --nCreatedDirectories;
      rmdir(aszDirectory[nCreatedDirectories]);
   }
   return(nResult);
}
