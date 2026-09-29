#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "OpenDoor.h"

int _spawnvpe(int nModeFlag, const char *pszPath,
   const char *const papszArgs[], const char *const papszEnviron[]);

static void HandleChildSignal(int nSignal)
{
   (void)nSignal;
}

static int HasTestSignalHandler(void)
{
   struct sigaction Action;

   if(sigaction(SIGCHLD, NULL, &Action) == -1)
   {
      return(0);
   }
   return(Action.sa_handler == HandleChildSignal);
}

#define CHECK(condition) do { \
   if(!(condition)) { \
      fprintf(stderr, "Unix spawn test failed at line %d: %s\n", \
         __LINE__, strerror(errno)); \
      nResult = __LINE__; \
      goto cleanup; \
   } \
} while(0)

int main(void)
{
   const char *const apszEnvironment[] = {
      "PATH=/bin:/usr/bin",
      NULL
   };
   const char *const apszWaitArgs[] = { "sh", "-c", "exit 7", NULL };
   const char *const apszPathArgs[] = {
      "sh", "-c", "test -n \"$PATH\"", NULL
   };
   const char *const apszMissingArgs[] = { "missing", NULL };
   char szMarkerName[64];
   char szMarkerCommand[128];
   const char *apszNowaitArgs[4];
   struct sigaction OriginalAction;
   struct sigaction TestAction;
   sigset_t CurrentMask;
   sigset_t OriginalMask;
   sigset_t TestMask;
   FILE *pMarker = NULL;
   int bMarkerExists = 0;
   int bSignalMaskSaved = 0;
   int bSignalActionSaved = 0;
   int nAttempt;
   int nResult = 0;

   sprintf(szMarkerName, "ODSPAWN%lu.TMP", (unsigned long)getpid());
   sprintf(szMarkerCommand, "printf done > %s", szMarkerName);
   remove(szMarkerName);
   apszNowaitArgs[0] = "sh";
   apszNowaitArgs[1] = "-c";
   apszNowaitArgs[2] = szMarkerCommand;
   apszNowaitArgs[3] = NULL;

   CHECK(sigprocmask(SIG_BLOCK, NULL, &OriginalMask) == 0);
   bSignalMaskSaved = 1;
   CHECK(sigaction(SIGCHLD, NULL, &OriginalAction) == 0);
   bSignalActionSaved = 1;
   memset(&TestAction, 0, sizeof(TestAction));
   TestAction.sa_handler = HandleChildSignal;
   CHECK(sigemptyset(&TestAction.sa_mask) == 0);
   CHECK(sigaction(SIGCHLD, &TestAction, NULL) == 0);

   CHECK(_spawnvpe(P_WAIT, "/bin/sh", apszWaitArgs,
      apszEnvironment) == 7);
   CHECK(HasTestSignalHandler());

   CHECK(od_spawn("exit 0"));
   CHECK(od_spawn("exit 7"));
   CHECK(!od_spawn("exit 127"));
   CHECK(HasTestSignalHandler());

   TestMask = OriginalMask;
   CHECK(sigaddset(&TestMask, SIGALRM) == 0);
   CHECK(sigprocmask(SIG_SETMASK, &TestMask, NULL) == 0);
   CHECK(od_spawn("exit 0"));
   CHECK(sigprocmask(SIG_BLOCK, NULL, &CurrentMask) == 0);
   CHECK(sigismember(&CurrentMask, SIGALRM) == 1);

   TestMask = OriginalMask;
   CHECK(sigdelset(&TestMask, SIGALRM) == 0);
   CHECK(sigprocmask(SIG_SETMASK, &TestMask, NULL) == 0);
   CHECK(od_spawn("exit 0"));
   CHECK(sigprocmask(SIG_BLOCK, NULL, &CurrentMask) == 0);
   CHECK(sigismember(&CurrentMask, SIGALRM) == 0);
   CHECK(sigprocmask(SIG_SETMASK, &OriginalMask, NULL) == 0);

   CHECK(getenv("PATH") != NULL);
   CHECK(_spawnvpe(P_WAIT, "sh", apszPathArgs, NULL) == 0);
   CHECK(HasTestSignalHandler());

   errno = 0;
   CHECK(_spawnvpe(P_WAIT, "/definitely/not/an/opendoors-program",
      apszMissingArgs, apszEnvironment) == -1);
   CHECK(errno == ENOENT);
   CHECK(HasTestSignalHandler());

   errno = 0;
   CHECK(_spawnvpe(P_NOWAIT, "/definitely/not/an/opendoors-program",
      apszMissingArgs, apszEnvironment) == -1);
   CHECK(errno == ENOENT);
   CHECK(HasTestSignalHandler());

   CHECK(_spawnvpe(P_NOWAIT, "/bin/sh", apszNowaitArgs,
      apszEnvironment) == 0);
   CHECK(HasTestSignalHandler());

   for(nAttempt = 0; nAttempt < 200; ++nAttempt)
   {
      pMarker = fopen(szMarkerName, "rb");
      if(pMarker != NULL)
      {
         char szContents[5];
         size_t nLength;

         nLength = fread(szContents, 1, 4, pMarker);
         fclose(pMarker);
         pMarker = NULL;
         if(nLength == 4)
         {
            szContents[4] = '\0';
            CHECK(strcmp(szContents, "done") == 0);
            bMarkerExists = 1;
            break;
         }
      }
      usleep(10000);
   }
   CHECK(bMarkerExists);

cleanup:
   if(pMarker != NULL)
      fclose(pMarker);
   remove(szMarkerName);
   if(bSignalMaskSaved)
      sigprocmask(SIG_SETMASK, &OriginalMask, NULL);
   if(bSignalActionSaved)
      sigaction(SIGCHLD, &OriginalAction, NULL);
   return(nResult);
}
