#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "OpenDoor.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef ODPLAT_NIX
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>
#endif
#include "ODCore.h"
#include "ODGen.h"
#include "ODInEx.h"
#include "ODKrnl.h"

BOOL ODCALL ODLogTimeRecordSucceeded(time_t nUnixTime,
   const struct tm *ptmTimeRecord);

static char szOversizedTemplate[OD_GLOBAL_WORK_STRING_SIZE + 32];

static int CountText(const char *pszText, const char *pszNeedle)
{
   int nCount = 0;
   size_t nNeedleLength = strlen(pszNeedle);

   while((pszText = strstr(pszText, pszNeedle)) != NULL)
   {
      ++nCount;
      pszText += nNeedleLength;
   }
   return(nCount);
}

#define CHECK(condition) do { if(!(condition)) { nResult = __LINE__; goto cleanup; } } while(0)

int main(void)
{
   char szLogName[64];
   char szContents[4096];
   FILE *pLog = NULL;
   size_t nLength;
   INT nIndex;
   INT nResult = 0;
   struct tm ValidTimeRecord;
#ifdef ODPLAT_NIX
   struct rlimit OriginalLimit;
   struct rlimit Limited;
   struct sigaction OriginalXfszAction;
   struct sigaction IgnoreXfszAction;
   struct stat LogStatus;
   int bLimitActive = 0;
   int bXfszActionSaved = 0;
#endif

#ifdef _WIN32
   sprintf(szLogName, "ODLOG%lu.TMP", (unsigned long)GetCurrentProcessId());
#else
   sprintf(szLogName, "ODLOG%lu.TMP", (unsigned long)getpid());
#endif
   remove(szLogName);

   memset(&ValidTimeRecord, 0, sizeof(ValidTimeRecord));
   CHECK(ODLogTimeRecordSucceeded((time_t)0, &ValidTimeRecord));
   CHECK(!ODLogTimeRecordSucceeded((time_t)-1, &ValidTimeRecord));
   CHECK(!ODLogTimeRecordSucceeded((time_t)0, NULL));


   memset(&od_control, 0, sizeof(od_control));
   strcpy(od_control.od_logfile_name, szLogName);
   strcpy(od_control.od_prog_name, "Log Test");
   strcpy(od_control.user_name, "Test User");
   for(nIndex = 0; nIndex < 7; ++nIndex)
      od_control.od_day[nIndex] = "Day";
   for(nIndex = 0; nIndex < 12; ++nIndex)
      od_control.od_month[nIndex] = "Month";
   od_control.od_logfile_messages[11] = "%s entering";
   od_control.od_logfile_messages[13] = "Test complete";
   od_control.od_logfile_messages[0] = "Standard event";
   od_control.od_logfile_messages[5] = "Exit status %d";
   od_control.od_logfile_messages[8] = "Paging sysop";
   od_control.od_logfile_messages[12] = "Reason: %s";
   bPreOrExit = TRUE;
   bODInitialized = TRUE;

   memset(szOversizedTemplate, 'X', sizeof(szOversizedTemplate) - 1);
   szOversizedTemplate[sizeof(szOversizedTemplate) - 1] = '\0';
   od_control.od_logfile_messages[11] = szOversizedTemplate;
   CHECK(!od_log_open());
   CHECK(od_control.od_error == ERR_LIMIT);
   CHECK(pfLogWrite == NULL);
   CHECK(pfLogClose == NULL);
   CHECK(remove(szLogName) == 0);
   od_control.od_logfile_messages[11] = "%s entering";
   od_control.od_error = ERR_NONE;

   CHECK(od_log_open());
   CHECK(od_log_open());
   CHECK(fflush(NULL) == 0);
   CHECK(pfLogClose != NULL);
   CHECK(pfLogClose(0));

   pLog = fopen(szLogName, "rb");
   CHECK(pLog != NULL);
   nLength = fread(szContents, 1, sizeof(szContents) - 1, pLog);
   CHECK(!ferror(pLog));
   szContents[nLength] = '\0';
   CHECK(fclose(pLog) == 0);
   pLog = NULL;

   CHECK(CountText(szContents, "----------") == 1);
   CHECK(CountText(szContents, "Test User entering") == 1);

   CHECK(od_log_open());
   od_control.od_logfile_messages[12] = szOversizedTemplate;
   CHECK(!pfLogWrite(8));
   CHECK(od_control.od_error == ERR_LIMIT);
   od_control.od_logfile_messages[12] = "Reason: %s";
   od_control.od_logfile_messages[5] = szOversizedTemplate;
   bPreOrExit = FALSE;
   btExitReason = 0;
   CHECK(!pfLogClose(0));
   CHECK(od_control.od_error == ERR_LIMIT);
   CHECK(pfLogWrite == NULL);
   CHECK(pfLogClose == NULL);
   od_control.od_logfile_messages[5] = "Exit status %d";
   bPreOrExit = TRUE;

   CHECK(od_log_open());
   CHECK(pfLogWrite != NULL);
   CHECK(pfLogClose != NULL);
   od_control.od_logfile_disable = TRUE;
   CHECK(pfLogClose(0));
   CHECK(pfLogWrite == NULL);
   CHECK(pfLogClose == NULL);

#ifdef ODPLAT_NIX
   od_control.od_logfile_disable = FALSE;
   CHECK(sigaction(SIGXFSZ, NULL, &OriginalXfszAction) == 0);
   bXfszActionSaved = 1;
   memset(&IgnoreXfszAction, 0, sizeof(IgnoreXfszAction));
   IgnoreXfszAction.sa_handler = SIG_IGN;
   CHECK(sigemptyset(&IgnoreXfszAction.sa_mask) == 0);
   CHECK(sigaction(SIGXFSZ, &IgnoreXfszAction, NULL) == 0);
   CHECK(getrlimit(RLIMIT_FSIZE, &OriginalLimit) == 0);

   CHECK(od_log_open());
   CHECK(stat(szLogName, &LogStatus) == 0);
   Limited = OriginalLimit;
   Limited.rlim_cur = (rlim_t)LogStatus.st_size;
   CHECK(setrlimit(RLIMIT_FSIZE, &Limited) == 0);
   bLimitActive = 1;
   CHECK(pfLogWrite != NULL);
   CHECK(!pfLogWrite(0));
   CHECK(setrlimit(RLIMIT_FSIZE, &OriginalLimit) == 0);
   bLimitActive = 0;
   od_control.od_logfile_disable = TRUE;
   (void)pfLogClose(0);

   od_control.od_logfile_disable = FALSE;
   CHECK(stat(szLogName, &LogStatus) == 0);
   Limited = OriginalLimit;
   Limited.rlim_cur = (rlim_t)LogStatus.st_size;
   CHECK(setrlimit(RLIMIT_FSIZE, &Limited) == 0);
   bLimitActive = 1;
   CHECK(!od_log_open());
   CHECK(pfLogWrite == NULL);
   CHECK(pfLogClose == NULL);
   CHECK(setrlimit(RLIMIT_FSIZE, &OriginalLimit) == 0);
   bLimitActive = 0;

   CHECK(od_log_open());
   CHECK(stat(szLogName, &LogStatus) == 0);
   Limited = OriginalLimit;
   Limited.rlim_cur = (rlim_t)LogStatus.st_size;
   CHECK(setrlimit(RLIMIT_FSIZE, &Limited) == 0);
   bLimitActive = 1;
   od_control.od_error = ERR_NONE;
   CHECK(!pfLogClose(0));
   CHECK(pfLogWrite == NULL);
   CHECK(pfLogClose == NULL);
   CHECK(od_control.od_error == ERR_GENERALFAILURE);
   CHECK(setrlimit(RLIMIT_FSIZE, &OriginalLimit) == 0);
   bLimitActive = 0;

   CHECK(sigaction(SIGXFSZ, &OriginalXfszAction, NULL) == 0);
   bXfszActionSaved = 0;
#endif

cleanup:
#ifdef ODPLAT_NIX
   if(bLimitActive)
      setrlimit(RLIMIT_FSIZE, &OriginalLimit);
   if(bXfszActionSaved)
      sigaction(SIGXFSZ, &OriginalXfszAction, NULL);
#endif
   if(pLog != NULL)
      fclose(pLog);
   if(pfLogClose != NULL)
      (void)pfLogClose(0);
   od_control.od_logfile_disable = FALSE;
   remove(szLogName);
   bODInitialized = FALSE;
   return(nResult);
}
