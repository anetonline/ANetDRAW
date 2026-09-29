#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODKrnl.h"
#include "ODScrn.h"
#include "ODVScrn.h"

#define CHECK(condition) do { \
   if(!(condition)) { \
      fprintf(stderr, "virtual screen test failed at line %d\n", __LINE__); \
      return(1); \
   } \
} while(0)

int main(void)
{
   BYTE block[12];
   BYTE result[12];
   BYTE localCell[2];
   BYTE localWrite[2];
   BYTE *snapshot;
   BYTE *legacySnapshot;
   BYTE *guardedSnapshot;
   DWORD snapshotSize;
   tODVScreenInfo info;
   tODScrnTextInfo textInfo;
   INT index;
   INT row;
   INT column;

   memset(&od_control, 0, sizeof(od_control));
   od_control.baud = 1;
   od_control.user_ansi = TRUE;
   od_control.user_screenwidth = 132;
   od_control.user_screen_length = 50;

   CHECK(ODScrnInitialize() == kODRCSuccess);
   ODScrnSetBoundary(1, 1, 80, 25);
   ODSessionScreenInitialize(80, 25);
   CHECK(ODSessionScreenAvailable());
   CHECK(ODSessionScreenWidth() == 132);
   CHECK(ODSessionScreenHeight() == 50);

   ODSessionScreenSetCursorPos(80, 1);
   ODSessionScreenDisplayString("ABC");
   CHECK(ODSessionScreenGetText(80, 1, 82, 1, result));
   CHECK(result[0] == 'A' && result[2] == 'B' && result[4] == 'C');
   ODSessionScreenGetInfo(&info);
   CHECK(info.curx == 83 && info.cury == 1);

   ODSessionScreenPresent();
   CHECK(ODScrnGetText(80, 1, 80, 1, localCell));
   CHECK(localCell[0] == 'A');

   localWrite[0] = 'X';
   localWrite[1] = 0x1f;
   CHECK(ODScrnPutText(1, 1, 1, 1, localWrite));
   CHECK(ODSessionScreenGetText(1, 1, 1, 1, result));
   CHECK(result[0] == ' ' && result[1] == 0x07);

   for(index = 0; index < 6; ++index)
   {
      block[index * 2] = (BYTE)('a' + index);
      block[index * 2 + 1] = (BYTE)(0x10 + index);
   }
   bODInitialized = TRUE;
   ODTimerStart(&RunKernelTimer, 60000);
   od_control.baud = 0;

   od_set_cursor(10, 20);
   ODSessionScreenGetInfo(&info);
   CHECK(info.curx == 20 && info.cury == 10);
   od_control.od_error = ERR_NONE;
   od_set_cursor(51, 20);
   CHECK(od_control.od_error == ERR_PARAMETER);
   ODSessionScreenGetInfo(&info);
   CHECK(info.curx == 20 && info.cury == 10);
   od_control.od_error = ERR_NONE;
   od_set_cursor(10, 133);
   CHECK(od_control.od_error == ERR_PARAMETER);
   ODSessionScreenGetInfo(&info);
   CHECK(info.curx == 20 && info.cury == 10);

   memset(result, 0xa5, sizeof(result));
   od_control.od_error = ERR_NONE;
   CHECK(!od_gettext(2, 1, 1, 1, result));
   CHECK(od_control.od_error == ERR_PARAMETER);
   for(index = 0; index < (INT)sizeof(result); ++index)
      CHECK(result[index] == 0xa5);
   od_control.od_error = ERR_NONE;
   CHECK(!od_gettext(1, 2, 1, 1, result));
   CHECK(od_control.od_error == ERR_PARAMETER);
   for(index = 0; index < (INT)sizeof(result); ++index)
      CHECK(result[index] == 0xa5);

   ODScrnSetCursorPos(1, 3);
   od_disp_emu("L", FALSE);
   CHECK(ODScrnGetText(1, 3, 1, 3, localCell));
   CHECK(localCell[0] == 'L');
   CHECK(ODSessionScreenGetText(1, 3, 1, 3, result));
   CHECK(result[0] == ' ');
   legacySnapshot = (BYTE *)malloc(4004U);
   CHECK(legacySnapshot != NULL);
   CHECK(od_save_screen(legacySnapshot));
   CHECK(legacySnapshot[4 + 2 * 80 * 2] == 'L');
   free(legacySnapshot);

   ODSessionScreenSetCursorPos(80, 2);
   od_disp_emu("ABC", TRUE);
   CHECK(ODSessionScreenGetText(80, 2, 82, 2, result));
   CHECK(result[0] == 'A' && result[2] == 'B' && result[4] == 'C');
   CHECK(ODScrnGetText(1, 1, 1, 1, localCell));
   CHECK(localCell[0] == 'X');

   CHECK(od_puttext(100, 39, 105, 39, block));
   memset(result, 0, sizeof(result));
   CHECK(od_gettext(100, 39, 105, 39, result));
   CHECK(memcmp(block, result, sizeof(block)) == 0);

   CHECK(od_scroll(100, 39, 105, 40, -1, SCROLL_NO_CLEAR));
   memset(result, 0, sizeof(result));
   CHECK(od_gettext(100, 40, 105, 40, result));
   CHECK(memcmp(block, result, sizeof(block)) == 0);

   snapshotSize = od_save_screen_size();
   CHECK(snapshotSize == 48UL + 132UL * 50UL * 2UL);
   snapshot = (BYTE *)malloc((size_t)snapshotSize);
   CHECK(snapshot != NULL);
   CHECK(od_save_screen_ex(snapshot, snapshotSize));
   CHECK(!od_save_screen_ex(snapshot, snapshotSize - 1));
   CHECK(od_control.od_error == ERR_PARAMETER);

   memset(block, 0, sizeof(block));
   CHECK(od_puttext(100, 40, 105, 40, block));
   CHECK(od_restore_screen_ex(snapshot, snapshotSize));
   memset(result, 0, sizeof(result));
   CHECK(od_gettext(100, 40, 105, 40, result));
   CHECK(result[0] == 'a' && result[10] == 'f');

   snapshot[0] = 'X';
   CHECK(!od_restore_screen_ex(snapshot, snapshotSize));
   CHECK(od_control.od_error == ERR_PARAMETER);

   snapshot[0] = 'O';
   ODSessionScreenShutdown();
   od_control.baud = 1;
   od_control.user_screenwidth = 100;
   ODSessionScreenInitialize(80, 25);
   CHECK(ODSessionScreenAvailable());
   od_control.baud = 0;
   CHECK(!od_restore_screen_ex(snapshot, snapshotSize));
   CHECK(od_control.od_error == ERR_PARAMETER);

   ODSessionScreenShutdown();
   od_control.baud = 1;
   od_control.user_screenwidth = 132;
   od_control.user_screen_length = 300;
   ODSessionScreenInitialize(80, 25);
   CHECK(ODSessionScreenAvailable());
   od_control.baud = 0;
   od_control.user_ansi = TRUE;
   od_control.user_avatar = FALSE;
   od_set_cursor(300, 131);
   ODSessionScreenGetInfo(&info);
   CHECK(info.curx == 131 && info.cury == 300);
   od_control.user_avatar = TRUE;
   od_set_cursor(255, 132);
   ODSessionScreenGetInfo(&info);
   CHECK(info.curx == 132 && info.cury == 255);
   od_control.od_error = ERR_NONE;
   od_set_cursor(256, 132);
   CHECK(od_control.od_error == ERR_PARAMETER);
   ODSessionScreenGetInfo(&info);
   CHECK(info.curx == 132 && info.cury == 255);
   od_control.user_avatar = FALSE;
   od_disp_emu("\033[300;131HZ", TRUE);
   CHECK(ODSessionScreenGetText(131, 300, 131, 300, result));
   CHECK(result[0] == 'Z');

   free(snapshot);
   ODSessionScreenShutdown();

   guardedSnapshot = (BYTE *)malloc(4004U + 320U);
   CHECK(guardedSnapshot != NULL);
   memset(guardedSnapshot, 'G', 4004U + 320U);
   legacySnapshot = guardedSnapshot + 320;
   memset(legacySnapshot, 0, 4004U);
   legacySnapshot[0] = 2;
   legacySnapshot[1] = 4;
   legacySnapshot[2] = 0x1f;
   legacySnapshot[3] = 4;
   for(row = 0; row < 4; ++row)
   {
      for(column = 0; column < 80; ++column)
      {
         legacySnapshot[4 + row * 160 + column * 2] = ' ';
         legacySnapshot[4 + row * 160 + column * 2 + 1] = 0x07;
      }
      legacySnapshot[4 + row * 160] = (BYTE)('A' + row);
   }

   ODScrnSetBoundary(1, 1, 80, 2);
   od_control.user_ansi = TRUE;
   od_control.user_avatar = FALSE;
   CHECK(od_restore_screen(legacySnapshot));
   CHECK(ODScrnGetText(1, 1, 1, 2, result));
   CHECK(result[0] == 'C' && result[2] == 'D');
   ODScrnGetTextInfo(&textInfo);
   CHECK(textInfo.curx == 2 && textInfo.cury == 2);
   CHECK(textInfo.attribute == 0x1f);

   od_control.user_ansi = FALSE;
   CHECK(od_restore_screen(legacySnapshot));
   CHECK(ODScrnGetText(1, 1, 1, 2, result));
   CHECK(result[0] == 'C' && result[2] == 'D');
   ODScrnGetTextInfo(&textInfo);
   CHECK(textInfo.curx == 2 && textInfo.cury == 2);

   memset(legacySnapshot + 4, 0, 320U);
   legacySnapshot[0] = 2;
   legacySnapshot[1] = 2;
   legacySnapshot[2] = 0x07;
   legacySnapshot[3] = 2;
   for(column = 0; column < 80; ++column)
   {
      legacySnapshot[4 + column * 2] = 'X';
      legacySnapshot[4 + column * 2 + 1] = 0x07;
      legacySnapshot[164 + column * 2] = ' ';
      legacySnapshot[164 + column * 2 + 1] = 0x07;
   }
   legacySnapshot[164] = 'Y';

   CHECK(od_restore_screen(legacySnapshot));
   CHECK(ODScrnGetText(1, 1, 1, 1, localCell));
   CHECK(localCell[0] == 'X');
   CHECK(ODScrnGetText(80, 1, 80, 1, localCell));
   CHECK(localCell[0] == 'X');
   CHECK(ODScrnGetText(1, 2, 1, 2, localCell));
   CHECK(localCell[0] == 'Y');
   ODScrnGetTextInfo(&textInfo);
   CHECK(textInfo.curx == 2 && textInfo.cury == 2);

   legacySnapshot[0] = 1;
   legacySnapshot[164] = 'Z';
   CHECK(od_restore_screen(legacySnapshot));
   CHECK(ODScrnGetText(1, 2, 1, 2, localCell));
   CHECK(localCell[0] == ' ');
   ODScrnGetTextInfo(&textInfo);
   CHECK(textInfo.curx == 1 && textInfo.cury == 2);

   legacySnapshot[0] = 2;
   legacySnapshot[164] = 'Y';
   od_control.baud = 1;
   od_control.user_screenwidth = 132;
   od_control.user_screen_length = 50;
   ODSessionScreenInitialize(80, 25);
   CHECK(ODSessionScreenAvailable());
   od_control.baud = 0;
   CHECK(od_restore_screen(legacySnapshot));
   CHECK(ODSessionScreenGetText(1, 1, 1, 1, result));
   CHECK(result[0] == 'X');
   CHECK(ODSessionScreenGetText(80, 1, 80, 1, result));
   CHECK(result[0] == 'X');
   CHECK(ODSessionScreenGetText(1, 2, 1, 2, result));
   CHECK(result[0] == 'Y');
   ODSessionScreenGetInfo(&info);
   CHECK(info.curx == 2 && info.cury == 2);
   ODSessionScreenShutdown();

   free(guardedSnapshot);

   ODScrnSetBoundary(1, 1, 80, 25);
   od_control.user_ansi = TRUE;
   od_control.user_avatar = FALSE;
   ODScrnSetCursorPos(20, 10);
   od_control.od_error = ERR_NONE;
   od_set_cursor(26, 20);
   CHECK(od_control.od_error == ERR_PARAMETER);
   ODScrnGetTextInfo(&textInfo);
   CHECK(textInfo.curx == 20 && textInfo.cury == 10);
   od_control.od_error = ERR_NONE;
   od_set_cursor(10, 81);
   CHECK(od_control.od_error == ERR_PARAMETER);
   ODScrnGetTextInfo(&textInfo);
   CHECK(textInfo.curx == 20 && textInfo.cury == 10);
   memset(result, 0x5a, sizeof(result));
   od_control.od_error = ERR_NONE;
   CHECK(!od_gettext(2, 1, 1, 1, result));
   CHECK(od_control.od_error == ERR_PARAMETER);
   for(index = 0; index < (INT)sizeof(result); ++index)
      CHECK(result[index] == 0x5a);
   od_control.od_error = ERR_NONE;
   CHECK(!od_gettext(1, 2, 1, 1, result));
   CHECK(od_control.od_error == ERR_PARAMETER);
   for(index = 0; index < (INT)sizeof(result); ++index)
      CHECK(result[index] == 0x5a);
   ODScrnShutdown();
   bODInitialized = FALSE;
   return(0);
}
