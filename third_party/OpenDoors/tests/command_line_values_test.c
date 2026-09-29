#include <stdio.h>
#include <string.h>

#include "OpenDoor.h"

#define CHECK(condition) do { if(!(condition)) return(__LINE__); } while(0)

static char szCustomKeyword[32];
static char szCustomOptions[80];
static INT nCustomCalls;

static void CustomOptionHandler(char *pszKeyword, char *pszOptions)
{
   strncpy(szCustomKeyword, pszKeyword, sizeof(szCustomKeyword) - 1);
   szCustomKeyword[sizeof(szCustomKeyword) - 1] = '\0';
   strncpy(szCustomOptions, pszOptions, sizeof(szCustomOptions) - 1);
   szCustomOptions[sizeof(szCustomOptions) - 1] = '\0';
   ++nCustomCalls;
}

static void ParseTestCommandLine(void)
{
   char *papszArguments[] = {
      "door",
      "-USERNAME", "012345678901234567890123456789", "ABCDEFGHIJ",
      "unused-user-word",
      "-LOCATION", "01234567890123456789", "abcdefghij",
      "unused-location-word",
      "-BBSNAME", "012345678901234567890123456789",
      "abcdefghijklmnop", "unused-bbs-word",
      "-CUSTOM", "111111111111111111111111111111",
      "222222222222222222222222222222",
      "333333333333333333333333333333", "unused-custom-word",
      "-NODE", "7"
   };

#ifdef ODPLAT_WIN32
   od_parse_cmd_line_cons(
#else
   od_parse_cmd_line(
#endif
      (INT)(sizeof(papszArguments) / sizeof(papszArguments[0])),
      papszArguments);
}

static int TestSplitter(void)
{
   INT nArgCount;
   char **papszArguments;

   papszArguments = od_split_cmd_line("-NAME \"Jane Smith\" -LOCAL",
      &nArgCount);
   CHECK(papszArguments != NULL);
#ifdef ODPLAT_WIN32
   CHECK(nArgCount == 4);
   CHECK(strcmp(papszArguments[1], "-NAME") == 0);
   CHECK(strcmp(papszArguments[2], "Jane Smith") == 0);
   CHECK(strcmp(papszArguments[3], "-LOCAL") == 0);
   CHECK(papszArguments[4] == NULL);
#else
   CHECK(nArgCount == 5);
   CHECK(strcmp(papszArguments[1], "-NAME") == 0);
   CHECK(strcmp(papszArguments[2], "\"Jane") == 0);
   CHECK(strcmp(papszArguments[3], "Smith\"") == 0);
   CHECK(strcmp(papszArguments[4], "-LOCAL") == 0);
   CHECK(papszArguments[5] == NULL);
#endif
   od_free_split_cmd_line(papszArguments);
   return(0);
}

int main(void)
{
   int nResult;

   od_control.od_cmd_line_handler = CustomOptionHandler;
   ParseTestCommandLine();

   CHECK(strlen(od_control.user_name) == sizeof(od_control.user_name) - 1);
   CHECK(strcmp(od_control.user_name,
      "012345678901234567890123456789 ABCD") == 0);
   CHECK(strlen(od_control.user_location)
      == sizeof(od_control.user_location) - 1);
   CHECK(strcmp(od_control.user_location,
      "01234567890123456789 abcd") == 0);
   CHECK(strlen(od_control.system_name) == sizeof(od_control.system_name) - 1);
   CHECK(strcmp(od_control.system_name,
      "012345678901234567890123456789 abcdefgh") == 0);
   CHECK(nCustomCalls == 1);
   CHECK(strcmp(szCustomKeyword, "-CUSTOM") == 0);
   CHECK(strlen(szCustomOptions) == sizeof(szCustomOptions) - 1);
   CHECK(strcmp(szCustomOptions,
      "111111111111111111111111111111 222222222222222222222222222222 "
      "33333333333333333") == 0);
   CHECK(od_control.od_node == 7);

   nResult = TestSplitter();
   CHECK(nResult == 0);
   return(0);
}
