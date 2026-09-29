#include <stddef.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODUtil.h"

#define CHECK(condition) do { if(!(condition)) return(__LINE__); } while(0)

int main(void)
{
   char line[16];
   BOOL lineComplete;

   strcpy(line, "\n");
   CHECK(ODStringNormalizeLine(line, &lineComplete) == 0);
   CHECK(lineComplete && line[0] == '\0');

   strcpy(line, "\r\n");
   CHECK(ODStringNormalizeLine(line, &lineComplete) == 0);
   CHECK(lineComplete && line[0] == '\0');

   strcpy(line, "entry\r\n");
   CHECK(ODStringNormalizeLine(line, &lineComplete) == 5);
   CHECK(lineComplete && strcmp(line, "entry") == 0);

   strcpy(line, "fragment");
   CHECK(ODStringNormalizeLine(line, &lineComplete) == 8);
   CHECK(!lineComplete && strcmp(line, "fragment") == 0);

   return(0);
}
