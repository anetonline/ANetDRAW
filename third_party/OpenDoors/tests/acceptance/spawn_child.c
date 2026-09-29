#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

static int WriteSentinel(const char *name, const char *message,
   int return_code)
{
   FILE *file = fopen(name, "w");
   if(file == NULL)
      return(20);
   if(fputs(message, file) < 0 || fclose(file) != 0)
      return(21);
   return(return_code);
}

int main(int argc, char **argv)
{
   static const char *expected[] = {
      "argument one",
      "",
      "quote\"inside",
      "slashes\\\\\"quote",
      "ends with slash\\",
      "tab\targument",
      "argument-two"
   };
   const char *value = getenv("OD_ACCEPT_VALUE");
   FILE *file;
   int index;
   if(argc == 1)
      return(WriteSentinel("ODEMPTY.OK", "no-tail spawn passed\n", 0));
   if(argc == 2 && strcmp(argv[1], "--simple") == 0)
      return(WriteSentinel("ODSIMPLE.OK", "simple spawn passed\n", 0));
   if(argc == 2 && strcmp(argv[1], "--nonzero") == 0)
      return(WriteSentinel("ODNZERO.OK", "nonzero spawn passed\n", 7));
   if(argc == 2 && strcmp(argv[1], "--inherit") == 0)
   {
      value = getenv("OD_INHERITED_VALUE");
      if(value == NULL || strcmp(value, "inherited value") != 0)
         return(22);
      return(WriteSentinel("ODINHER.OK", "inherited environment passed\n",
         31));
   }
   if(argc == 2 && strcmp(argv[1], "--path") == 0)
      return(WriteSentinel("ODPATH.OK", "path search passed\n", 29));
   if(argc == 2 && strcmp(argv[1], "--nowait") == 0)
   {
#ifdef _WIN32
      Sleep(1000);
#else
      sleep(1);
#endif
      return(WriteSentinel("ODNOWAIT.OK", "asynchronous spawn passed\n",
         0));
   }
   if(argc != 8)
   {
      fprintf(stderr, "spawn child received %d arguments, expected 8\n", argc);
      for(index = 0; index < argc; ++index)
         fprintf(stderr, "spawn child argv[%d] is '%s'\n", index, argv[index]);
      return(17);
   }
   for(index = 0; index < 7; ++index)
   {
      if(strcmp(argv[index + 1], expected[index]) != 0)
      {
         fprintf(stderr, "spawn child argv[%d] is '%s', expected '%s'\n",
            index + 1, argv[index + 1], expected[index]);
         return(17);
      }
   }
   if(value == NULL || strcmp(value, "environment value") != 0)
   {
      fprintf(stderr, "spawn child environment value is '%s'\n",
         value == NULL ? "(unset)" : value);
      return(17);
   }
   if(getenv("OD_INHERITED_VALUE") != NULL)
   {
      fprintf(stderr, "replacement environment inherited parent value\n");
      return(17);
   }
   file = fopen("ODCHILD.OK", "w");
   if(file == NULL)
      return(18);
   if(fputs("spawn child passed\n", file) < 0 || fclose(file) != 0)
      return(19);
   return(23);
}
