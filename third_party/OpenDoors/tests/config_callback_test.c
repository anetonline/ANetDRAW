#include <stdio.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODInEx.h"

#define CONFIG_FILENAME "ODCFGCB.TMP"
#define CONFIG_PATH "ODCFGMIS" DIRSEP_STR CONFIG_FILENAME

static int nValuelessCount;
static int nValueCount;
static int nBuiltInCount;

static void ConfigCallback(char *pszKeyword, char *pszOptions)
{
   if(strcmp(pszKeyword, "DISPLAYWINNERS") == 0
      && strcmp(pszOptions, "") == 0)
   {
      ++nValuelessCount;
   }
   else if(strcmp(pszKeyword, "CUSTOMTITLE") == 0
      && strcmp(pszOptions, "Value With Spaces") == 0)
   {
      ++nValueCount;
   }
   else if(strcmp(pszKeyword, "DOORDIR") == 0
      && strcmp(pszOptions, "") == 0)
   {
      ++nBuiltInCount;
   }
}

int main(void)
{
   FILE *pConfigFile = NULL;
   char szCurrentDirectory[512];
   char szOriginalDirectory[512];
   int nResult = 0;

#define CHECK(condition) do { if(!(condition)) { nResult = __LINE__; goto cleanup; } } while(0)

   remove(CONFIG_FILENAME);
   ODDirGetCurrent(szOriginalDirectory, sizeof(szOriginalDirectory));
   CHECK(szOriginalDirectory[0] != '\0');
   pConfigFile = fopen(CONFIG_FILENAME, "w");
   CHECK(pConfigFile != NULL);
   CHECK(fputs("DoorDir    ; empty values are ignored\n", pConfigFile) >= 0);
   CHECK(fputs("DisplayWinners   ; no option text\n", pConfigFile) >= 0);
   CHECK(fputs("CustomTitle Value With Spaces \t; trailing whitespace\n",
      pConfigFile) >= 0);
   CHECK(fputs("PortAddress 2F8\n", pConfigFile) >= 0);
   CHECK(fputs("PortAddress XYZ\n", pConfigFile) >= 0);
   CHECK(fclose(pConfigFile) == 0);
   pConfigFile = NULL;

   memset(&od_control, 0, sizeof(od_control));
   od_control.od_config_file = INCLUDE_CONFIG_FILE;
   od_control.od_config_filename = CONFIG_PATH;
   od_control.od_config_function = ConfigCallback;
   od_control.od_force_local = TRUE;
   od_control.od_disable = DIS_NAME_PROMPT;
   od_control.od_silent_mode = TRUE;
   od_control.od_noexit = TRUE;

   od_init();
   CHECK(nValuelessCount == 1);
   CHECK(nValueCount == 1);
   CHECK(nBuiltInCount == 1);
   CHECK((WORD)od_control.od_com_address == 0x02f8);
   CHECK(szOriginalDir == NULL);
   ODDirGetCurrent(szCurrentDirectory, sizeof(szCurrentDirectory));
   CHECK(strcmp(szCurrentDirectory, szOriginalDirectory) == 0);

cleanup:
   if(pConfigFile != NULL)
      fclose(pConfigFile);
   if(bODInitialized)
      od_exit(0, FALSE);
   remove(CONFIG_FILENAME);
   return(nResult);
}
