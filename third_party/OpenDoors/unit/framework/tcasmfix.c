/* Normalize the one TASM segment-override spelling emitted by Turbo C 2.01. */
#include <stdio.h>
#include <string.h>

#define LINE_SIZE 512

static const char duplicate_group[] = "DGROUP:DGROUP:";
static const char single_group[] = "DGROUP:";
static char line[LINE_SIZE];

int main(int argc, char **argv)
{
   FILE *input;
   FILE *output;
   char *current;

   if(argc != 2)
      return 1;
   input = fopen(argv[1], "r");
   if(input == NULL)
      return 1;
   output = fopen("FIXED.ASM", "w");
   if(output == NULL)
   {
      fclose(input);
      return 1;
   }
   while(fgets(line, sizeof(line), input) != NULL)
   {
      current = line;
      for(;;)
      {
         char *match = strstr(current, duplicate_group);
         if(match == NULL)
         {
            fputs(current, output);
            break;
         }
         while(current < match)
            fputc(*current++, output);
         fputs(single_group, output);
         current += sizeof(duplicate_group) - 1;
      }
   }
   if(ferror(input) || fclose(input) != 0 || fclose(output) != 0)
      return 1;
   if(remove(argv[1]) != 0 || rename("FIXED.ASM", argv[1]) != 0)
      return 1;
   return 0;
}
