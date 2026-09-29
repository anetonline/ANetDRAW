#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "OpenDoor.h"
#include "ODInEx.h"

#define EXITINFO_RECORD_SIZE 902
#define EXITINFO_NUM_CALLS_OFFSET 2
#define EXITINFO_BUSY_HOUR_OFFSET 179
#define EXITINFO_BUSY_DAY_OFFSET 227
#define EXITINFO_TIMELIMIT_OFFSET 435

static void PutLE16(unsigned char *bytes, size_t offset, WORD value)
{
   bytes[offset] = (unsigned char)value;
   bytes[offset + 1] = (unsigned char)(value >> 8);
}

static void PutLE32(unsigned char *bytes, size_t offset, DWORD value)
{
   bytes[offset] = (unsigned char)value;
   bytes[offset + 1] = (unsigned char)(value >> 8);
   bytes[offset + 2] = (unsigned char)(value >> 16);
   bytes[offset + 3] = (unsigned char)(value >> 24);
}

static WORD GetLE16(const unsigned char *bytes, size_t offset)
{
   return((WORD)((WORD)bytes[offset] | ((WORD)bytes[offset + 1] << 8)));
}

int main(void)
{
   FILE *pFile = NULL;
   unsigned char InputRecord[EXITINFO_RECORD_SIZE];
   unsigned char OutputRecord[EXITINFO_RECORD_SIZE];
   DWORD dwMinutes;
   int nHour;
   int nDay;
   int nResult = 0;

#define CHECK(condition) do { if(!(condition)) { nResult = __LINE__; goto cleanup; } } while(0)

   CHECK(sizeof(tExitInfoRecord) == EXITINFO_RECORD_SIZE);
   CHECK(offsetof(tExitInfoRecord, num_calls) == EXITINFO_NUM_CALLS_OFFSET);
   CHECK(offsetof(tExitInfoRecord, busyperhour) == EXITINFO_BUSY_HOUR_OFFSET);
   CHECK(offsetof(tExitInfoRecord, busyperday) == EXITINFO_BUSY_DAY_OFFSET);
   CHECK(offsetof(tExitInfoRecord, timelimit) == EXITINFO_TIMELIMIT_OFFSET);
   CHECK(offsetof(tExitInfoRecord, bbs) == 453);
   CHECK(sizeof(tExtendedExitInfo) == 1017);
   CHECK(offsetof(tExtendedExitInfo, menustack) == 2);
   CHECK(sizeof(tRA2ExitInfoRecord) == 2363);
   CHECK(offsetof(tRA2ExitInfoRecord, num_calls) == 2);
   CHECK(offsetof(tRA2ExitInfoRecord, combinedrecord) == 728);
   CHECK(offsetof(tRA2ExitInfoRecord, timelimit) == 1293);

   CHECK(!ODGetElapsedMinutes(&dwMinutes, (time_t)-1, (time_t)100));
   CHECK(!ODGetElapsedMinutes(&dwMinutes, (time_t)100, (time_t)-1));
   CHECK(!ODGetElapsedMinutes(&dwMinutes, (time_t)101, (time_t)100));
   CHECK(ODGetElapsedMinutes(&dwMinutes, (time_t)100, (time_t)220));
   CHECK(dwMinutes == 2);

   memset(InputRecord, 0, sizeof(InputRecord));
   PutLE32(InputRecord, EXITINFO_NUM_CALLS_OFFSET, 0x12345678UL);
   PutLE16(InputRecord, EXITINFO_TIMELIMIT_OFFSET, 100);
   for(nHour = 0; nHour < 24; ++nHour)
   {
      PutLE16(InputRecord, EXITINFO_BUSY_HOUR_OFFSET + (size_t)nHour * 2,
         (WORD)(0x100 + nHour));
   }
   for(nDay = 0; nDay < 7; ++nDay)
   {
      PutLE16(InputRecord, EXITINFO_BUSY_DAY_OFFSET + (size_t)nDay * 2,
         (WORD)(0x200 + nDay));
   }

   pFile = tmpfile();
   CHECK(pFile != NULL);
   CHECK(fwrite(InputRecord, 1, sizeof(InputRecord) - 1, pFile)
      == sizeof(InputRecord) - 1);
   rewind(pFile);

   CHECK(!ODReadExitInfoPrimitive(pFile, (INT)sizeof(InputRecord)));
   CHECK(pExitInfoRecord == NULL);

   fclose(pFile);
   pFile = NULL;

   pFile = tmpfile();
   CHECK(pFile != NULL);
   CHECK(fwrite(InputRecord, 1, sizeof(InputRecord), pFile)
      == sizeof(InputRecord));
   rewind(pFile);

   CHECK(ODReadExitInfoPrimitive(pFile, (INT)sizeof(InputRecord)));
   CHECK(od_control.system_calls == 0x12345678UL);
   for(nHour = 0; nHour < 24; ++nHour)
   {
      CHECK(od_control.timelog_busyperhour[nHour]
         == (INT16)(0x100 + nHour));
      od_control.timelog_busyperhour[nHour] = (INT16)(0x300 + nHour);
   }
   for(nDay = 0; nDay < 7; ++nDay)
   {
      CHECK(od_control.timelog_busyperday[nDay] == (INT16)(0x200 + nDay));
      od_control.timelog_busyperday[nDay] = (INT16)(0x400 + nDay);
   }

   nStartupUnixTime = (time_t)-1;
   nInitialRemaining = 60;
   od_control.user_timelimit = 55;
   rewind(pFile);
   CHECK(ODWriteExitInfoPrimitive(pFile, (INT)sizeof(OutputRecord)));
   rewind(pFile);
   CHECK(fread(OutputRecord, 1, sizeof(OutputRecord), pFile)
      == sizeof(OutputRecord));
   CHECK(GetLE16(OutputRecord, EXITINFO_TIMELIMIT_OFFSET) == 100);

   nStartupUnixTime = time(NULL);
   CHECK(nStartupUnixTime != (time_t)-1);
   rewind(pFile);
   CHECK(ODWriteExitInfoPrimitive(pFile, (INT)sizeof(OutputRecord)));
   rewind(pFile);
   CHECK(fread(OutputRecord, 1, sizeof(OutputRecord), pFile)
      == sizeof(OutputRecord));
   CHECK(GetLE16(OutputRecord, EXITINFO_TIMELIMIT_OFFSET) == 95);
   CHECK(OutputRecord[EXITINFO_NUM_CALLS_OFFSET] == 0x78);
   CHECK(OutputRecord[EXITINFO_NUM_CALLS_OFFSET + 1] == 0x56);
   CHECK(OutputRecord[EXITINFO_NUM_CALLS_OFFSET + 2] == 0x34);
   CHECK(OutputRecord[EXITINFO_NUM_CALLS_OFFSET + 3] == 0x12);

   for(nHour = 0; nHour < 24; ++nHour)
   {
      CHECK(GetLE16(OutputRecord,
         EXITINFO_BUSY_HOUR_OFFSET + (size_t)nHour * 2)
         == (WORD)(0x300 + nHour));
   }
   for(nDay = 0; nDay < 7; ++nDay)
   {
      CHECK(GetLE16(OutputRecord,
         EXITINFO_BUSY_DAY_OFFSET + (size_t)nDay * 2)
         == (WORD)(0x400 + nDay));
   }

cleanup:
   if(pFile != NULL)
      fclose(pFile);
   free(pExitInfoRecord);
   pExitInfoRecord = NULL;
   return(nResult);
}
