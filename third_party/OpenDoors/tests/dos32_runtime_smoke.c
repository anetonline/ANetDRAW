#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODPlat.h"
#include "ODVScrn.h"

#ifndef ODPLAT_DOS32
#error This test requires ODPLAT_DOS32
#endif

static int Fail(int line)
{
   FILE *failure;

   failure = fopen("D32FAIL.TXT", "w");
   if(failure != NULL)
   {
      fprintf(failure, "DOS32 runtime test failed at line %d\n", line);
      fclose(failure);
   }
   return(line);
}

#define CHECK(condition) do { if(!(condition)) return(Fail(__LINE__)); } while(0)

int main(void)
{
   const char *spawn_arguments[] = {"OD32CHLD.EXE", NULL};
   char current_directory[260];
   BYTE *large_block;
   BYTE *screen_snapshot;
   DWORD screen_snapshot_size;
   tODDirEntry directory_entry;
   tODDirHandle directory;
   tODTimer timer;
   FILE *sentinel;

   od_control.od_force_local = TRUE;
   od_control.od_silent_mode = TRUE;
   od_control.od_disable |= DIS_NAME_PROMPT;
   od_control.od_noexit = TRUE;
   CHECK(sizeof(tODEditTextFormat) == sizeof(int));
   CHECK(sizeof(tODEditMenuResult) == sizeof(int));
   CHECK(sizeof(tODInputEventType) == sizeof(int));

   large_block = (BYTE *)malloc(256U * 1024U);
   CHECK(large_block != NULL);
   memset(large_block, 0x5a, 256U * 1024U);
   CHECK(large_block[0] == 0x5a);
   CHECK(large_block[256U * 1024U - 1U] == 0x5a);
   free(large_block);

   ODTimerStart(&timer, 20);
   ODTimerWaitForElapse(&timer);
   CHECK(ODTimerElapsed(&timer));
   od_sleep(0);

   memset(current_directory, 0, sizeof(current_directory));
   ODDirGetCurrent(current_directory, sizeof(current_directory));
   CHECK(current_directory[0] != '\0');
   CHECK(current_directory[1] == ':' && current_directory[2] == '\\');
   CHECK(strchr(current_directory + 2, ':') == NULL);
   ODDirChangeCurrent(current_directory);
   sentinel = fopen("D32DIR.TMP", "w");
   CHECK(sentinel != NULL);
   CHECK(fclose(sentinel) == 0);
   CHECK(ODDirOpen("D32DIR.TMP", 0, &directory) == kODRCSuccess);
   CHECK(ODDirRead(directory, &directory_entry) == kODRCSuccess);
   ODDirClose(directory);
   CHECK(ODFileDelete("D32DIR.TMP") == kODRCSuccess);

   od_control.baud = 1;
   od_control.user_screenwidth = 255;
   od_control.user_screen_length = 129;
   ODSessionScreenInitialize(80, 25);
   CHECK(ODSessionScreenAvailable());
   CHECK(ODSessionScreenWidth() == 255);
   CHECK(ODSessionScreenHeight() == 129);
   screen_snapshot_size = ODSessionScreenSnapshotSize();
   CHECK(screen_snapshot_size == 48UL + 255UL * 129UL * 2UL);
   screen_snapshot = (BYTE *)malloc((size_t)screen_snapshot_size);
   CHECK(screen_snapshot != NULL);
   CHECK(ODSessionScreenSave(screen_snapshot, screen_snapshot_size));
   free(screen_snapshot);
   ODSessionScreenShutdown();
   od_control.baud = 0;

   CHECK(od_spawnvpe(P_WAIT, "OD32CHLD.EXE", spawn_arguments, NULL) == 0);
   sentinel = fopen("D32CHILD.OK", "r");
   CHECK(sentinel != NULL);
   CHECK(fclose(sentinel) == 0);
   CHECK(ODFileDelete("D32CHILD.OK") == kODRCSuccess);
   od_exit(0, FALSE);

   sentinel = fopen("D32PASS.OK", "w");
   CHECK(sentinel != NULL);
   CHECK(fputs("OpenDoors DOS32 runtime tests passed\n", sentinel) >= 0);
   CHECK(fclose(sentinel) == 0);
   return(0);
}
