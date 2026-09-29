#include <winsock2.h>
#include <windows.h>

#include "test_support.h"

static unsigned personalityCalls;
static unsigned mpsCalls;
static unsigned kernelExecCalls;
static BYTE personalityOperation;
static BOOL writerResult;
static char promptTitle[40];
static char expectedName[40];

static DWORD WINAPI ODTestWriteConsoleName(LPVOID unused)
{
   HANDLE input;
   HANDLE output;
   INPUT_RECORD records[80];
   CONSOLE_SCREEN_BUFFER_INFO info;
   COORD origin;
   char cells[8192];
   DWORD cellCount;
   DWORD cellsRead;
   DWORD written;
   DWORD firstWriteCount;
   DWORD remainingWritten;
   DWORD recordCount;
   DWORD index;
   unsigned attempt;
   BOOL foundPrompt;

   (void)unused;
   input = output = INVALID_HANDLE_VALUE;
   for(attempt = 0; attempt < 500
      && (input == INVALID_HANDLE_VALUE || output == INVALID_HANDLE_VALUE);
      ++attempt)
   {
      if(input == INVALID_HANDLE_VALUE)
         input = CreateFileA("CONIN$", GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
      if(output == INVALID_HANDLE_VALUE)
         output = CreateFileA("CONOUT$", GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
      if(input == INVALID_HANDLE_VALUE || output == INVALID_HANDLE_VALUE)
         Sleep(10);
   }
   if(input == INVALID_HANDLE_VALUE || output == INVALID_HANDLE_VALUE)
   {
      fputs("console helper could not open CONIN$/CONOUT$\n", stderr);
      fflush(stderr);
      return(1);
   }
   FlushConsoleInputBuffer(input);
   origin.X = origin.Y = 0;
   for(attempt = 0; attempt < 500; ++attempt)
   {
      if(GetConsoleScreenBufferInfo(output, &info))
      {
         cellCount = (DWORD)info.dwSize.X * (DWORD)info.dwSize.Y;
         if(cellCount >= sizeof(cells)) cellCount = sizeof(cells) - 1;
         if(ReadConsoleOutputCharacterA(output, cells, cellCount, origin,
            &cellsRead))
         {
            cells[cellsRead] = '\0';
            if(strstr(cells, promptTitle) != NULL)
               break;
         }
      }
      Sleep(10);
   }
   foundPrompt = attempt != 500;
   if(!foundPrompt)
   {
      fprintf(stderr, "console helper did not find prompt marker %s\n",
         promptTitle);
      fflush(stderr);
   }
   /* od_init() may replace an inherited pseudo-console while this helper is
    * waiting for the first published frame. Reopen CONIN$ after observing
    * that frame so input is injected into the console OpenDoors now owns. */
   CloseHandle(input);
   input = CreateFileA("CONIN$", GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
   if(input == INVALID_HANDLE_VALUE)
   {
      CloseHandle(output);
      return(3);
   }
   FlushConsoleInputBuffer(input);
   recordCount = 35 + (DWORD)strlen(expectedName) + 1;
   memset(records, 0, sizeof(records));
   for(index = 0; index < 35; ++index)
   {
      records[index].EventType = KEY_EVENT;
      records[index].Event.KeyEvent.bKeyDown = TRUE;
      records[index].Event.KeyEvent.wRepeatCount = 1;
      records[index].Event.KeyEvent.wVirtualKeyCode = VK_BACK;
      records[index].Event.KeyEvent.uChar.AsciiChar = '\b';
   }
   for(; index + 1 < recordCount; ++index)
   {
      DWORD nameIndex = index - 35;
      records[index].EventType = KEY_EVENT;
      records[index].Event.KeyEvent.bKeyDown = TRUE;
      records[index].Event.KeyEvent.wRepeatCount = 1;
      records[index].Event.KeyEvent.wVirtualKeyCode =
         (WORD)(BYTE)expectedName[nameIndex];
      records[index].Event.KeyEvent.uChar.AsciiChar = expectedName[nameIndex];
   }
   records[index].EventType = KEY_EVENT;
   records[index].Event.KeyEvent.bKeyDown = TRUE;
   records[index].Event.KeyEvent.wRepeatCount = 1;
   records[index].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
   records[index].Event.KeyEvent.uChar.AsciiChar = '\r';
   /* Leave a normal human-sized gap after the first typed character. During
    * od_init(), an empty cooperative wait must remain pending rather than
    * reusing that character while the lifecycle state is Initializing. */
   firstWriteCount = 36;
   writerResult = foundPrompt
      && WriteConsoleInputA(input, records, firstWriteCount, &written)
      && written == firstWriteCount;
   Sleep(100);
   writerResult = writerResult
      && WriteConsoleInputA(input, records + firstWriteCount,
         recordCount - firstWriteCount, &remainingWritten)
      && remainingWritten == recordCount - firstWriteCount;
   if(!foundPrompt)
      (void)WriteConsoleInputA(input, records, recordCount, &written);
   CloseHandle(output);
   CloseHandle(input);
   return(writerResult ? 0 : 1);
}

static void ODTestPersonality(BYTE operation)
{
   ++personalityCalls;
   personalityOperation = operation;
}

static void ODTestMPSComponent(void)
{
   ++mpsCalls;
   ODMPSEnable();
}

static void ODTestKernelExec(void)
{
   ++kernelExecCalls;
}

int main(int argc, char **argv)
{
   HANDLE output;
   HANDLE writer;
   CONSOLE_SCREEN_BUFFER_INFO info;
   FILETIME marker;
   COORD origin;
   char cells[8192];
   char screenMarker[40];
   char statusRow[80];
   char extraRow[80];
   DWORD cellCount;
   DWORD cellsRead;
   DWORD statusRead;
   DWORD extraRead;
   DWORD index;
   BOOL statusNonblank;
   BOOL extraBlank;

   od_parse_cmd_line(argc, argv);
   ODTestConfigureLocal();
   od_control.od_silent_mode = FALSE;
   od_control.od_disable &= ~(DIS_NAME_PROMPT | DIS_LOCAL_INPUT);
   GetSystemTimeAsFileTime(&marker);
   sprintf(promptTitle, "OD-%08lx%08lx", (unsigned long)marker.dwHighDateTime,
      (unsigned long)marker.dwLowDateTime);
   sprintf(expectedName, "P%08lx%08lx",
      (unsigned long)marker.dwHighDateTime,
      (unsigned long)marker.dwLowDateTime);
   strcpy(od_control.od_prog_name, promptTitle);
   mpsCalls = 0;
   kernelExecCalls = 0;
   od_control.od_mps = (OD_COMPONENT *)ODTestMPSComponent;
   od_control.od_ker_exec = ODTestKernelExec;
   od_control.user_screenwidth = 100;
   od_control.user_screen_length = 30;
   writerResult = FALSE;
   writer = CreateThread(NULL, 0, ODTestWriteConsoleName, NULL, 0, NULL);
   OD_TEST_CHECK(writer != NULL);
   od_init();
   OD_TEST_CHECK(WaitForSingleObject(writer, 5000) == WAIT_OBJECT_0);
   CloseHandle(writer);
   OD_TEST_CHECK(writerResult);
   OD_TEST_CHECK(strcmp(od_control.user_name, expectedName) == 0);
   OD_TEST_CHECK(mpsCalls == 1);
   OD_TEST_CHECK(kernelExecCalls == 0);
   od_kernel();
   OD_TEST_CHECK(kernelExecCalls == 1);

   OD_TEST_CHECK(FindWindowA("ODFrame", NULL) == NULL);
   output = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
   OD_TEST_CHECK(!od_control.od_silent_mode);
   OD_TEST_CHECK(output != INVALID_HANDLE_VALUE);
   OD_TEST_CHECK(GetConsoleScreenBufferInfo(output, &info));
   if(info.dwSize.X < 100 || info.dwSize.Y < 32)
   {
      fprintf(stderr,
         "console canvas %dx%d, window (%d,%d)-(%d,%d), expected at least 100x32\n",
         (int)info.dwSize.X, (int)info.dwSize.Y,
         (int)info.srWindow.Left, (int)info.srWindow.Top,
         (int)info.srWindow.Right, (int)info.srWindow.Bottom);
      fflush(stderr);
   }
   /* Windows may enforce a larger host-dependent minimum buffer size. The
    * console frontend must preserve at least the requested logical canvas
    * and compose correctly across the actual buffer it reports. */
   OD_TEST_CHECK(info.dwSize.X >= 100);
   OD_TEST_CHECK(info.dwSize.Y >= 32);
   origin.X = 0;
   origin.Y = 24;
   OD_TEST_CHECK(ReadConsoleOutputCharacterA(output, statusRow,
      sizeof(statusRow), origin, &statusRead));
   origin.Y = info.dwSize.Y - 1;
   OD_TEST_CHECK(ReadConsoleOutputCharacterA(output, extraRow,
      sizeof(extraRow), origin, &extraRead));
   OD_TEST_CHECK(statusRead == sizeof(statusRow));
   OD_TEST_CHECK(extraRead == sizeof(extraRow));
   statusNonblank = FALSE;
   extraBlank = TRUE;
   for(index = 0; index < sizeof(statusRow); ++index)
   {
      if(statusRow[index] != ' ')
         statusNonblank = TRUE;
      if(extraRow[index] != ' ')
         extraBlank = FALSE;
   }
   OD_TEST_CHECK(statusNonblank);
   OD_TEST_CHECK(extraBlank);

   OD_TEST_CHECK(od_set_personality("OD_ONEROW"));
   OD_TEST_CHECK(od_control.key_status[6] == 0x4200);
   OD_TEST_CHECK(od_control.key_status[7] == 0x4300);
   OD_TEST_CHECK(od_control.key_status[8] == 0x4400);
   origin.X = info.dwSize.X > 80
      ? (SHORT)((info.dwSize.X - 80) / 2) : 0;
   origin.Y = 24;
   OD_TEST_CHECK(ReadConsoleOutputCharacterA(output, statusRow,
      sizeof(statusRow), origin, &statusRead));
   OD_TEST_CHECK(statusRead == sizeof(statusRow));
   OD_TEST_CHECK(memcmp(statusRow + 75, "F8/9", 4) == 0);

   OD_TEST_CHECK(od_add_personality("TEST", 1, 23, ODTestPersonality));
   OD_TEST_CHECK(od_set_personality("TEST"));
   OD_TEST_CHECK(personalityCalls == 2);
   OD_TEST_CHECK(personalityOperation == STATUS_NORMAL);
   GetSystemTimeAsFileTime(&marker);
   sprintf(screenMarker, "SCREEN-%08lx%08lx",
      (unsigned long)marker.dwHighDateTime,
      (unsigned long)marker.dwLowDateTime);
   od_disp_str(screenMarker);
   OD_TEST_CHECK(GetConsoleScreenBufferInfo(output, &info));
   origin.X = origin.Y = 0;
   cellCount = (DWORD)info.dwSize.X * (DWORD)info.dwSize.Y;
   if(cellCount >= sizeof(cells)) cellCount = sizeof(cells) - 1;
   OD_TEST_CHECK(ReadConsoleOutputCharacterA(output, cells, cellCount,
      origin, &cellsRead));
   cells[cellsRead] = '\0';
   OD_TEST_CHECK(strstr(cells, screenMarker) != NULL);

   od_exit(0, FALSE);
   if(output != INVALID_HANDLE_VALUE) CloseHandle(output);
   fputs("OPENDOORS_WINDOWS_CONSOLE_ACCEPTANCE_OK\n", stderr);
   fflush(stderr);
   return(0);
}
