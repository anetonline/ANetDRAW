#include <limits.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODGen.h"
#include "ODKrnl.h"
#include "ODPlat.h"

#define CHECK(condition) do { if(!(condition)) return(__LINE__); } while(0)

int main(void)
{
   char colorDescription[] = "blue`red";
   char markerAndAttribute[3];
   char markerOnly[2];


   memset(&od_control, 0, sizeof(od_control));
   strcpy(od_control.od_color_names[1], "BLUE");
   od_control.od_color_delimiter = '`';
   od_control.od_color_char = 1;
   bODInitialized = TRUE;

   chColorCheck = 0;
   od_printf("`red`");
   CHECK(chColorCheck == 0);
   CHECK(od_color_config(colorDescription) == 0x07);

   markerAndAttribute[0] = 1;
   markerAndAttribute[1] = 0x17;
   markerAndAttribute[2] = '\0';
   chColorCheck = 0;
   od_printf("%s", markerAndAttribute);
   CHECK(chColorCheck == 0);

   markerOnly[0] = 1;
   markerOnly[1] = '\0';
   chColorCheck = 0;
   od_printf("%s", markerOnly);
   CHECK(chColorCheck == 0);

   bODInitialized = FALSE;


   return(0);
}
