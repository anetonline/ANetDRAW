#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODInQue.h"
#include "ODKrnl.h"
#include "ODPlat.h"
#include "ODScrn.h"

#define DISPLAY_FILE "ODAUTOF.asc"
#define SECTION_FILE "ODAUTOS.asc"
#define RIP_FILE "ODCLASS.RiP"
#define LONG_RIP_FILE "ODCLASS.ripple"
#define SECTION_RIP_FILE "ODSECT.RiP"
#define SECTION_FOLLOWED_RIP_FILE "ODSECT.rip.txt"
#define SECTION_LIMIT_FILE "ODSECTL.asc"

static const char szTestSectionMarker[] = "@#";

#define CHECK(condition) do { \
   if(!(condition)) { \
      nResult = __LINE__; \
      goto cleanup; \
   } \
} while(0)

static BOOL WriteFixture(const char *pszName, const char *pszContents)
{
   FILE *pFile;
   BOOL bSuccess;

   pFile = fopen(pszName, "wb");
   if(pFile == NULL)
      return(FALSE);
   bSuccess = fputs(pszContents, pFile) >= 0;
   if(fclose(pFile) != 0)
      bSuccess = FALSE;
   return(bSuccess);
}

static BOOL WriteSectionFixture(const char *pszName,
   const char *pszSectionName)
{
   FILE *pFile;
   BOOL bSuccess = TRUE;

   pFile = fopen(pszName, "wb");
   if(pFile == NULL)
      return(FALSE);
   if(fputs(szTestSectionMarker, pFile) < 0
      || fputs(pszSectionName, pFile) < 0
      || fputs("\nlong section display\n@#END\n", pFile) < 0)
   {
      bSuccess = FALSE;
   }
   if(fclose(pFile) != 0)
      bSuccess = FALSE;
   return(bSuccess);
}

static BOOL FirstScreenCharacterIs(unsigned char chExpected)
{
   BYTE abtCell[2];

   return(ODScrnGetText(1, 1, 1, 1, abtCell)
      && abtCell[0] == chExpected);
}

int main(void)
{
   char szLongBase[sizeof(szODWorkString)];
   char szLongExplicit[sizeof(szODWorkString)];
   char szLongRIPPrefix[sizeof(szODWorkString) + 1];
   char szMaxSectionName[UCHAR_MAX - sizeof(szTestSectionMarker) + 2];
   char szTooLongSectionName[sizeof(szMaxSectionName) + 1];
   INT nResult = 0;

   remove(DISPLAY_FILE);
   remove(SECTION_FILE);
   remove(RIP_FILE);
   remove(LONG_RIP_FILE);
   remove(SECTION_RIP_FILE);
   remove(SECTION_FOLLOWED_RIP_FILE);
   remove(SECTION_LIMIT_FILE);

   memset(szMaxSectionName, 'M', sizeof(szMaxSectionName) - 1);
   szMaxSectionName[sizeof(szMaxSectionName) - 1] = '\0';
   memset(szTooLongSectionName, 'L', sizeof(szTooLongSectionName) - 1);
   szTooLongSectionName[sizeof(szTooLongSectionName) - 1] = '\0';

   CHECK(WriteFixture(DISPLAY_FILE, "automatic display\n"));
   CHECK(WriteFixture(SECTION_FILE,
      "@#MAIN\nsection display\n@#END\n"));
   CHECK(WriteFixture(RIP_FILE, "R\n"));
   CHECK(WriteFixture(LONG_RIP_FILE, "L\n"));
   CHECK(WriteFixture(SECTION_RIP_FILE, "@#MAIN\nR\n@#END\n"));
   CHECK(WriteFixture(SECTION_FOLLOWED_RIP_FILE,
      "@#MAIN\nF\n@#END\n"));
   CHECK(WriteSectionFixture(SECTION_LIMIT_FILE, szMaxSectionName));


   memset(&od_control, 0, sizeof(od_control));
   od_control.od_cur_attrib = 0x07;
   od_control.od_disable = DIS_TIMEOUT;
   od_control.od_sending_rip = "Sending RIP file: ";
   CHECK(ODScrnInitialize() == kODRCSuccess);
   ODScrnSetBoundary(1, 1, 80, 25);
   CHECK(ODInQueueAlloc(&hODInputQueue, 2) == kODRCSuccess);
   bODInitialized = TRUE;
   ODTimerStart(&RunKernelTimer, 60000);

   CHECK(od_send_file("./ODAUTOF"));
   CHECK(od_send_file_section("./ODAUTOS", "MAIN"));
   CHECK(od_send_file_section(SECTION_LIMIT_FILE, szMaxSectionName));
   od_control.od_error = ERR_NONE;
   CHECK(!od_send_file_section(SECTION_LIMIT_FILE, szTooLongSectionName));
   CHECK(od_control.od_error == ERR_LIMIT);

   memset(szLongBase, 'B', sizeof(szLongBase) - 1);
   szLongBase[sizeof(szLongBase) - 1] = '\0';
   od_control.od_error = ERR_NONE;
   CHECK(!od_send_file(szLongBase));
   CHECK(od_control.od_error == ERR_LIMIT);
   od_control.od_error = ERR_NONE;
   CHECK(!od_send_file_section(szLongBase, "MAIN"));
   CHECK(od_control.od_error == ERR_LIMIT);

   memset(szLongExplicit, 'E', sizeof(szLongExplicit) - 1);
   memcpy(szLongExplicit + sizeof(szLongExplicit) - sizeof(".txt"), ".txt",
      sizeof(".txt"));
   od_control.od_error = ERR_NONE;
   CHECK(!od_send_file(szLongExplicit));
   CHECK(od_control.od_error == ERR_FILEOPEN);

   memset(szLongRIPPrefix, 'P', sizeof(szLongRIPPrefix) - 1);
   szLongRIPPrefix[sizeof(szLongRIPPrefix) - 1] = '\0';
   od_control.od_sending_rip = szLongRIPPrefix;

   ODScrnClear();
   CHECK(od_send_file(RIP_FILE));
   CHECK(FirstScreenCharacterIs(' '));

   ODScrnClear();
   CHECK(od_send_file(LONG_RIP_FILE));
   CHECK(FirstScreenCharacterIs('L'));

   ODScrnClear();
   CHECK(od_send_file_section(SECTION_RIP_FILE, "MAIN"));
   CHECK(FirstScreenCharacterIs(' '));

   ODScrnClear();
   CHECK(od_send_file_section(SECTION_FOLLOWED_RIP_FILE, "MAIN"));
   CHECK(FirstScreenCharacterIs('F'));

cleanup:
   if(bODInitialized)
   {
      bODInitialized = FALSE;
      ODScrnShutdown();
   }
   if(hODInputQueue != NULL)
   {
      ODInQueueFree(hODInputQueue);
      hODInputQueue = NULL;
   }


   remove(DISPLAY_FILE);
   remove(SECTION_FILE);
   remove(RIP_FILE);
   remove(LONG_RIP_FILE);
   remove(SECTION_RIP_FILE);
   remove(SECTION_FOLLOWED_RIP_FILE);
   remove(SECTION_LIMIT_FILE);
   return(nResult);
}
