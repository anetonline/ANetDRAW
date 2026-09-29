#include <stdio.h>

int main(void)
{
   FILE *sentinel = fopen("D32CHILD.OK", "w");

   if(sentinel == NULL)
      return(1);
   if(fputs("OpenDoors DOS32 spawn child ran\n", sentinel) < 0)
      return(2);
   return(fclose(sentinel) == 0 ? 0 : 3);
}
