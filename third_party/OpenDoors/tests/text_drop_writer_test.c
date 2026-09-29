#include <stdio.h>
#include <string.h>

#include "OpenDoor.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef ODPLAT_NIX
#include <signal.h>
#include <sys/resource.h>
#endif

typedef struct tODDropFileWriter
{
   FILE *pFile;
   INT nErrorCode;
   int nRuntimeError;
} tODDropFileWriter;

BOOL ODCALL ODDropFileOpen(tODDropFileWriter *pWriter,
   const char *pszPath, const char *pszMode);
BOOL ODCALL ODDropFileWrite(tODDropFileWriter *pWriter,
   const void *pData, size_t nSize);
void ODVCALL ODTextDropFileWrite(tODDropFileWriter *pWriter,
   const char *pszFormat, ...);
BOOL ODCALL ODDropFileClose(tODDropFileWriter *pWriter);

#define CHECK(condition) do { if(!(condition)) { \
   fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
      #condition); \
   nResult = __LINE__; \
   goto cleanup; \
} } while(0)

int main(void)
{
   char szDropFileName[64];
   char szMissingPath[96];
   char szContents[64];
   const unsigned char abBinaryData[] = { 0x00, 0x52, 0xff, 0x0a };
   FILE *pFile = NULL;
   size_t nLength;
   INT nResult = 0;
   tODDropFileWriter Writer;
#ifdef ODPLAT_NIX
   struct rlimit OriginalLimit;
   struct rlimit Limited;
   struct sigaction OriginalXfszAction;
   struct sigaction IgnoreXfszAction;
   int bLimitActive = 0;
   int bXfszActionSaved = 0;
#endif

#ifdef _WIN32
   sprintf(szDropFileName, "ODDROP%lu.TMP",
      (unsigned long)GetCurrentProcessId());
   sprintf(szMissingPath, "ODMISS%lu.DIR/FILE.TMP",
      (unsigned long)GetCurrentProcessId());
#else
   sprintf(szDropFileName, "ODDROP%lu.TMP", (unsigned long)getpid());
   sprintf(szMissingPath, "ODMISS%lu.DIR/FILE.TMP", (unsigned long)getpid());
#endif
   remove(szDropFileName);

   memset(&od_control, 0, sizeof(od_control));
   CHECK(ODDropFileOpen(&Writer, szDropFileName, "w"));
   ODTextDropFileWrite(&Writer, "%s\n", "caller");
   ODTextDropFileWrite(&Writer, "%u\n", 17U);
   CHECK(ODDropFileClose(&Writer));

   pFile = fopen(szDropFileName, "rb");
   CHECK(pFile != NULL);
   nLength = fread(szContents, 1, sizeof(szContents) - 1, pFile);
   CHECK(!ferror(pFile));
   szContents[nLength] = '\0';
   CHECK(fclose(pFile) == 0);
   pFile = NULL;
#ifdef _WIN32
   CHECK(strcmp(szContents, "caller\r\n17\r\n") == 0);
#else
   CHECK(strcmp(szContents, "caller\n17\n") == 0);
#endif

   CHECK(ODDropFileOpen(&Writer, szDropFileName, "wb"));
   CHECK(ODDropFileWrite(&Writer, abBinaryData, sizeof(abBinaryData)));
   CHECK(ODDropFileClose(&Writer));
   pFile = fopen(szDropFileName, "rb");
   CHECK(pFile != NULL);
   nLength = fread(szContents, 1, sizeof(szContents), pFile);
   CHECK(!ferror(pFile));
   CHECK(fclose(pFile) == 0);
   pFile = NULL;
   CHECK(nLength == sizeof(abBinaryData));
   CHECK(memcmp(szContents, abBinaryData, sizeof(abBinaryData)) == 0);

   od_control.od_error = ERR_NONE;
   CHECK(!ODDropFileOpen(&Writer, szMissingPath, "r+b"));
   CHECK(Writer.pFile == NULL);
   CHECK(od_control.od_error == ERR_FILEOPEN);

#ifdef ODPLAT_NIX
   CHECK(sigaction(SIGXFSZ, NULL, &OriginalXfszAction) == 0);
   bXfszActionSaved = 1;
   memset(&IgnoreXfszAction, 0, sizeof(IgnoreXfszAction));
   IgnoreXfszAction.sa_handler = SIG_IGN;
   CHECK(sigemptyset(&IgnoreXfszAction.sa_mask) == 0);
   CHECK(sigaction(SIGXFSZ, &IgnoreXfszAction, NULL) == 0);
   CHECK(getrlimit(RLIMIT_FSIZE, &OriginalLimit) == 0);

   CHECK(ODDropFileOpen(&Writer, szDropFileName, "wb"));
   CHECK(setvbuf(Writer.pFile, NULL, _IONBF, 0) == 0);
   Limited = OriginalLimit;
   Limited.rlim_cur = 0;
   CHECK(setrlimit(RLIMIT_FSIZE, &Limited) == 0);
   bLimitActive = 1;
   od_control.od_error = ERR_NONE;
   CHECK(!ODDropFileWrite(&Writer, abBinaryData, sizeof(abBinaryData)));
   CHECK(Writer.nErrorCode == ERR_GENERALFAILURE);
   CHECK(setrlimit(RLIMIT_FSIZE, &OriginalLimit) == 0);
   bLimitActive = 0;
   ODTextDropFileWrite(&Writer, "second write\n");
   CHECK(!ODDropFileClose(&Writer));
   CHECK(Writer.pFile == NULL);
   CHECK(od_control.od_error == ERR_GENERALFAILURE);
   pFile = fopen(szDropFileName, "rb");
   CHECK(pFile != NULL);
   CHECK(fread(szContents, 1, sizeof(szContents), pFile) == 0);
   CHECK(!ferror(pFile));
   CHECK(fclose(pFile) == 0);
   pFile = NULL;

   CHECK(ODDropFileOpen(&Writer, szDropFileName, "wb"));
   CHECK(setvbuf(Writer.pFile, szContents, _IOFBF, sizeof(szContents)) == 0);
   Limited = OriginalLimit;
   Limited.rlim_cur = 0;
   CHECK(setrlimit(RLIMIT_FSIZE, &Limited) == 0);
   bLimitActive = 1;
   od_control.od_error = ERR_NONE;
   (void)ODDropFileWrite(&Writer, abBinaryData, sizeof(abBinaryData));
   CHECK(!ODDropFileClose(&Writer));
   CHECK(Writer.pFile == NULL);
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
   if(pFile != NULL)
      fclose(pFile);
   if(Writer.pFile != NULL)
      (void)ODDropFileClose(&Writer);
   remove(szDropFileName);
   return(nResult);
}
