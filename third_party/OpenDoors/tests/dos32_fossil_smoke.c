#include <stdio.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODCom.h"

#ifndef ODPLAT_DOS32
#error This test requires ODPLAT_DOS32
#endif

static int Fail(const char *name, int line)
{
   FILE *failure = fopen(name, "w");

   if(failure != NULL)
   {
      fprintf(failure, "DOS32 FOSSIL test failed at line %d\n", line);
      fclose(failure);
   }
   return(line);
}

#define CHECK(file, condition) \
   do { if(!(condition)) return(Fail(file, __LINE__)); } while(0)

static int NoFossilTest(void)
{
   tPortHandle port;

   CHECK("NOFSFAIL.TXT", ODComAlloc(&port) == kODRCSuccess);
   CHECK("NOFSFAIL.TXT", ODComSetPort(port, 0) == kODRCSuccess);
   CHECK("NOFSFAIL.TXT",
      ODComSetPreferredMethod(port, kComMethodFOSSIL) == kODRCSuccess);
   CHECK("NOFSFAIL.TXT", ODComOpen(port) != kODRCSuccess);
   CHECK("NOFSFAIL.TXT",
      ODComSetPreferredMethod(port, kComMethodUART) == kODRCSuccess);
   CHECK("NOFSFAIL.TXT", ODComOpen(port) == kODRCSuccess);
   CHECK("NOFSFAIL.TXT", ODComClose(port) == kODRCSuccess);
   CHECK("NOFSFAIL.TXT", ODComFree(port) == kODRCSuccess);
   {
      FILE *sentinel = fopen("NOFSPASS.OK", "w");
      CHECK("NOFSFAIL.TXT", sentinel != NULL);
      CHECK("NOFSFAIL.TXT",
         fputs("OpenDoors DOS32 absent-FOSSIL test passed\n", sentinel) >= 0);
      CHECK("NOFSFAIL.TXT", fclose(sentinel) == 0);
   }
   return(0);
}

static int FossilTest(void)
{
   static BYTE block[] = "block-transfer";
   static BYTE fallback[] = "byte-fallback";
   BYTE received[32];
   char byte;
   int waiting;
   int count;
   BOOL carrier;
   tComMethod method;
   tPortHandle port;

   CHECK("FOSSFAIL.TXT", ODComAlloc(&port) == kODRCSuccess);
   CHECK("FOSSFAIL.TXT", ODComSetPort(port, 0) == kODRCSuccess);
   CHECK("FOSSFAIL.TXT",
      ODComSetPreferredMethod(port, kComMethodFOSSIL) == kODRCSuccess);
   CHECK("FOSSFAIL.TXT", ODComOpen(port) == kODRCSuccess);
   CHECK("FOSSFAIL.TXT", ODComGetMethod(port, &method) == kODRCSuccess);
   CHECK("FOSSFAIL.TXT", method == kComMethodFOSSIL);
   CHECK("FOSSFAIL.TXT", ODComCarrier(port, &carrier) == kODRCSuccess);
   CHECK("FOSSFAIL.TXT", carrier);
   CHECK("FOSSFAIL.TXT", ODComSetDTR(port, FALSE) == kODRCSuccess);
   CHECK("FOSSFAIL.TXT", ODComSetDTR(port, TRUE) == kODRCSuccess);

   CHECK("FOSSFAIL.TXT", ODComSendByte(port, 'A') == kODRCSuccess);
   CHECK("FOSSFAIL.TXT", ODComInbound(port, &waiting) == kODRCSuccess);
   CHECK("FOSSFAIL.TXT", waiting != 0);
   CHECK("FOSSFAIL.TXT", ODComGetByte(port, &byte, FALSE) == kODRCSuccess);
   CHECK("FOSSFAIL.TXT", byte == 'A');

   CHECK("FOSSFAIL.TXT",
      ODComSendBuffer(port, block, sizeof(block)) == kODRCSuccess);
   memset(received, 0, sizeof(received));
   CHECK("FOSSFAIL.TXT",
      ODComGetBuffer(port, received, sizeof(received), &count)
         == kODRCSuccess);
   CHECK("FOSSFAIL.TXT", count == sizeof(block));
   CHECK("FOSSFAIL.TXT", memcmp(received, block, sizeof(block)) == 0);

   CHECK("FOSSFAIL.TXT",
      ODComDOS32DisableFossilBlockIO(port) == kODRCSuccess);
   CHECK("FOSSFAIL.TXT",
      ODComSendBuffer(port, fallback, sizeof(fallback)) == kODRCSuccess);
   memset(received, 0, sizeof(received));
   CHECK("FOSSFAIL.TXT",
      ODComGetBuffer(port, received, sizeof(received), &count)
         == kODRCSuccess);
   CHECK("FOSSFAIL.TXT", count == sizeof(fallback));
   CHECK("FOSSFAIL.TXT", memcmp(received, fallback, sizeof(fallback)) == 0);

   CHECK("FOSSFAIL.TXT", ODComClearInbound(port) == kODRCSuccess);
   CHECK("FOSSFAIL.TXT", ODComClearOutbound(port) == kODRCSuccess);
   CHECK("FOSSFAIL.TXT", ODComClose(port) == kODRCSuccess);
   CHECK("FOSSFAIL.TXT", ODComFree(port) == kODRCSuccess);
   {
      FILE *sentinel = fopen("FOSSPASS.OK", "w");
      CHECK("FOSSFAIL.TXT", sentinel != NULL);
      CHECK("FOSSFAIL.TXT",
         fputs("OpenDoors DOS32 FOSSIL tests passed\n", sentinel) >= 0);
      CHECK("FOSSFAIL.TXT", fclose(sentinel) == 0);
   }
   return(0);
}

int main(int argc, char **argv)
{
   if(argc == 2 && strcmp(argv[1], "nofossil") == 0)
      return(NoFossilTest());
   return(FossilTest());
}
