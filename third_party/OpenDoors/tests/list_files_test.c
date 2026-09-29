#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "OpenDoor.h"
#ifdef ODPLAT_NIX
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "ODCore.h"
#include "ODGen.h"
#include "ODKrnl.h"
#include "ODPlat.h"
#include "ODScrn.h"

#define CHECK(condition) do { \
   if(!(condition)) { \
      fprintf(stderr, "list files test failed at line %d (od_error %d)\n", \
         __LINE__, od_control.od_error); \
      return(__LINE__); \
   } \
} while(0)

int main(void)
{
   const char *fixtureName = "ODBLANK.BBS";
   char acceptedSpec[100];
   char rejectedSpec[101];
   FILE *fixture;
   INT index;
#ifdef ODPLAT_NIX
   char colonDirectory[64];
   char colonIndex[96];
   char colonFile[96];
   char secondFile[96];
   char longDirectory[91];
   char longIndex[112];
   BYTE screenText[80];
#endif

   fixture = fopen(fixtureName, "wb");
   CHECK(fixture != NULL);
   CHECK(fputc('\n', fixture) != EOF);
   CHECK(fclose(fixture) == 0);


   memset(&od_control, 0, sizeof(od_control));
   od_control.od_cur_attrib = 0x07;
   od_control.od_offline = "[OFFLINE] ";
   od_control.user_screen_length = 25;

   CHECK(ODScrnInitialize() == kODRCSuccess);
   ODScrnSetBoundary(1, 1, 80, 25);
   ODTimerStart(&RunKernelTimer, 60000);
   bODInitialized = TRUE;

   CHECK(od_list_files((char *)fixtureName));

   fixture = fopen(fixtureName, "wb");
   CHECK(fixture != NULL);
   for(index = 0; index < 80; ++index)
      CHECK(fputc('A', fixture) != EOF);
   CHECK(fputs(" description\n", fixture) >= 0);
   CHECK(fclose(fixture) == 0);
   CHECK(!od_list_files((char *)fixtureName));
   CHECK(od_control.od_error == ERR_LIMIT);

   fixture = fopen(fixtureName, "wb");
   CHECK(fixture != NULL);
   CHECK(fputs("ABCDEFGH.TXT description\n", fixture) >= 0);
   CHECK(fclose(fixture) == 0);
   CHECK(od_list_files((char *)fixtureName));

   fixture = fopen(fixtureName, "wb");
   CHECK(fixture != NULL);
   for(index = 0; index < 69; ++index)
      CHECK(fputc('D', fixture) != EOF);
   CHECK(fputc(DIRSEP, fixture) != EOF);
   CHECK(fputs("A.TXT description\n", fixture) >= 0);
   CHECK(fclose(fixture) == 0);
   CHECK(!od_list_files((char *)fixtureName));
   CHECK(od_control.od_error == ERR_LIMIT);

#ifdef ODPLAT_NIX
   sprintf(colonDirectory, "ODC%lu.DIR", (unsigned long)getpid());
   sprintf(colonIndex, "%s%cFILES.BBS", colonDirectory, DIRSEP);
   sprintf(colonFile, "%s%cNAME:ONE.TXT", colonDirectory, DIRSEP);
   sprintf(secondFile, "%s%cSECOND.TXT", colonDirectory, DIRSEP);
   CHECK(mkdir(colonDirectory, 0700) == 0);
   fixture = fopen(colonFile, "wb");
   CHECK(fixture != NULL);
   CHECK(fputs("1234567", fixture) >= 0);
   CHECK(fclose(fixture) == 0);
   fixture = fopen(secondFile, "wb");
   CHECK(fixture != NULL);
   CHECK(fputs("12345678", fixture) >= 0);
   CHECK(fclose(fixture) == 0);
   fixture = fopen(colonIndex, "wb");
   CHECK(fixture != NULL);
   CHECK(fputs("NAME:ONE.TXT description\n", fixture) >= 0);
   CHECK(fputs("SECOND.TXT description\n", fixture) >= 0);
   CHECK(fclose(fixture) == 0);
   od_clr_scr();
   CHECK(od_list_files(colonIndex));
   CHECK(ODScrnGetText(1, 1, 20, 2, screenText));
   CHECK(screenText[28] == '7');
   CHECK(screenText[68] == '8');
   od_clr_scr();
   CHECK(od_list_files(colonDirectory));
   CHECK(ODScrnGetText(1, 1, 20, 2, screenText));
   CHECK(screenText[28] == '7');
   CHECK(screenText[68] == '8');
   CHECK(remove(colonIndex) == 0);
   CHECK(remove(colonFile) == 0);
   CHECK(remove(secondFile) == 0);
   CHECK(rmdir(colonDirectory) == 0);

   sprintf(longDirectory, "ODL%lu", (unsigned long)getpid());
   index = strlen(longDirectory);
   memset(longDirectory + index, 'L', 90 - index);
   longDirectory[90] = '\0';
   sprintf(longIndex, "%s%cFILES.BBS", longDirectory, DIRSEP);
   CHECK(mkdir(longDirectory, 0700) == 0);
   fixture = fopen(longIndex, "wb");
   CHECK(fixture != NULL);
   CHECK(fputs("ABCDEFGH.TXT description\n", fixture) >= 0);
   CHECK(fclose(fixture) == 0);
   CHECK(!od_list_files(longDirectory));
   CHECK(od_control.od_error == ERR_LIMIT);
   CHECK(remove(longIndex) == 0);
   CHECK(rmdir(longDirectory) == 0);
#endif

   memset(acceptedSpec, 'A', sizeof(acceptedSpec) - 1);
   acceptedSpec[sizeof(acceptedSpec) - 1] = '\0';
   CHECK(!od_list_files(acceptedSpec));
   CHECK(od_control.od_error == ERR_FILEOPEN);

   memset(rejectedSpec, 'A', sizeof(rejectedSpec) - 1);
   rejectedSpec[sizeof(rejectedSpec) - 1] = '\0';
   CHECK(!od_list_files(rejectedSpec));
   CHECK(od_control.od_error == ERR_LIMIT);

   bODInitialized = FALSE;
   ODScrnShutdown();


   CHECK(remove(fixtureName) == 0);
   return(0);
}
