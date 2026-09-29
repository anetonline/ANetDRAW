#include <winsock2.h>
#include <windows.h>

#include <OpenDoor.h>

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous,
   LPSTR command_line, int show_command)
{
   char *arguments[] = { "door" };

   (void)instance;
   (void)previous;
   (void)command_line;
   (void)show_command;
   od_parse_cmd_line_cons(1, arguments);
   return(0);
}
