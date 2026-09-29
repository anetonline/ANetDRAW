#include <stdio.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODKrnl.h"
#include "ODScrn.h"

#define CHECK(condition) do { \
   if(!(condition)) { \
      fprintf(stderr, "RA substitution test failed at line %d\n", __LINE__); \
      nResult = __LINE__; \
      goto cleanup; \
   } \
} while(0)

static const char szSubstitutions[] = "\006Q,\006R,\006S,\006T";
static const char szExpected[] =
   "4000000001,3000000002,2000000003,1000000004";
static const char szOtherSubstitutions[] =
   "\006L,\006N,\006P,\0069,\006:";
static const char szOtherExpected[] =
   "3900000001,2900000002,1900000003,4000000001:2000000003,"
   "3000000002:1000000004";
static const char szSystemCallsSubstitution[] = "\013A";
static const char szSystemCallsExpected[] = "1800000004";

int main(void)
{
   BYTE abtScreen[(sizeof(szExpected) - 1) * 2];
   BYTE abtOtherScreen[(sizeof(szOtherExpected) - 1) * 2];
   BYTE abtSystemCallsScreen[(sizeof(szSystemCallsExpected) - 1) * 2];
   size_t nCharacter;
   INT nResult = 0;

   memset(&od_control, 0, sizeof(od_control));
   od_control.od_cur_attrib = 0x07;
   od_control.od_disable = DIS_TIMEOUT;
   od_control.user_uploads = 4000000001UL;
   od_control.user_upk = 3000000002UL;
   od_control.user_downloads = 2000000003UL;
   od_control.user_downk = 1000000004UL;
   od_control.user_net_credit = 3900000001UL;
   od_control.user_lastread = 2900000002UL;
   od_control.user_numcalls = 1900000003UL;
   od_control.system_calls = 1800000004L;

   CHECK(ODScrnInitialize() == kODRCSuccess);
   ODScrnSetBoundary(1, 1, 80, 25);
   bODInitialized = TRUE;
   ODTimerStart(&RunKernelTimer, 60000);

   od_disp_emu(szSubstitutions, FALSE);
   CHECK(ODScrnGetText(1, 1, (BYTE)(sizeof(szExpected) - 1), 1,
      abtScreen));
   for(nCharacter = 0; nCharacter < sizeof(szExpected) - 1; ++nCharacter)
      CHECK(abtScreen[nCharacter * 2] == (BYTE)szExpected[nCharacter]);

   ODScrnClear();
   ODScrnSetCursorPos(1, 1);
   od_disp_emu(szOtherSubstitutions, FALSE);
   CHECK(ODScrnGetText(1, 1, (BYTE)(sizeof(szOtherExpected) - 1), 1,
      abtOtherScreen));
   for(nCharacter = 0; nCharacter < sizeof(szOtherExpected) - 1;
      ++nCharacter)
   {
      CHECK(abtOtherScreen[nCharacter * 2]
         == (BYTE)szOtherExpected[nCharacter]);
   }

   ODScrnClear();
   ODScrnSetCursorPos(1, 1);
   od_disp_emu(szSystemCallsSubstitution, FALSE);
   CHECK(ODScrnGetText(1, 1,
      (BYTE)(sizeof(szSystemCallsExpected) - 1), 1,
      abtSystemCallsScreen));
   for(nCharacter = 0; nCharacter < sizeof(szSystemCallsExpected) - 1;
      ++nCharacter)
   {
      CHECK(abtSystemCallsScreen[nCharacter * 2]
         == (BYTE)szSystemCallsExpected[nCharacter]);
   }

cleanup:
   if(bODInitialized)
   {
      bODInitialized = FALSE;
      ODScrnShutdown();
   }
   return(nResult);
}
