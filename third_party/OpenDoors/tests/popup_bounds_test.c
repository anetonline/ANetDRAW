#include <limits.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODGen.h"
#include "ODKrnl.h"
#include "ODPlat.h"

#define CHECK(condition) do { if(!(condition)) return(__LINE__); } while(0)

int main(void)
{

   bODInitialized = TRUE;

   od_control.od_error = ERR_NONE;
   CHECK(od_popup_menu(NULL, NULL, 1, 1, -1, MENU_NORMAL) == POPUP_ERROR);
   CHECK(od_control.od_error == ERR_LIMIT);

   od_control.od_error = ERR_NONE;
   CHECK(od_popup_menu(NULL, NULL, 1, 1, 10, MENU_NORMAL) == POPUP_ERROR);
   CHECK(od_control.od_error == ERR_PARAMETER);

   od_control.od_error = ERR_NONE;
   CHECK(od_popup_menu(NULL, NULL, 1, 1, 11, MENU_NORMAL) == POPUP_ERROR);
   CHECK(od_control.od_error == ERR_LIMIT);

   od_control.od_error = ERR_NONE;
   CHECK(od_popup_menu(NULL, "", 1, 1, 0, MENU_NORMAL) == POPUP_ERROR);
   CHECK(od_control.od_error == ERR_PARAMETER);
   CHECK(od_popup_menu(NULL, "", 1, 1, 0, MENU_NORMAL) == POPUP_ERROR);
   CHECK(od_control.od_error == ERR_PARAMETER);

   od_control.user_ansi = FALSE;
   od_control.user_avatar = FALSE;
   od_control.od_error = ERR_NONE;
   CHECK(od_popup_menu(NULL, "One", 1, 1, 0, MENU_NORMAL) == POPUP_ERROR);
   CHECK(od_control.od_error == ERR_NOGRAPHICS);
   CHECK(od_popup_menu(NULL, "One", 1, 1, 0, MENU_NORMAL) == POPUP_ERROR);
   CHECK(od_control.od_error == ERR_NOGRAPHICS);

   bODInitialized = FALSE;


   return(0);
}
