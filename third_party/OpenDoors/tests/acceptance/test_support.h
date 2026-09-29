#ifndef OPENDOORS_ACCEPTANCE_TEST_SUPPORT_H
#define OPENDOORS_ACCEPTANCE_TEST_SUPPORT_H

#include <stdio.h>
#include <string.h>

#include <OpenDoor.h>

static int ODTestCheck(int condition, const char *file, int line,
   const char *expression)
{
   if(condition)
      return(1);
   fprintf(stderr, "%s:%d: check failed: %s\n", file, line, expression);
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   {
      FILE *failure = fopen("ODFAIL.TXT", "w");
      if(failure != NULL)
      {
         fprintf(failure, "%s:%d: check failed: %s\n",
            file, line, expression);
         fclose(failure);
      }
   }
#endif
   return(0);
}

#define OD_TEST_CHECK(condition) do { \
   if(!ODTestCheck((condition), __FILE__, __LINE__, #condition)) \
      return(__LINE__); \
} while(0)

#ifndef OD_ACCEPTANCE_NO_LOCAL_CONFIG
static void ODTestConfigureLocal(void)
{
   memset(&od_control, 0, sizeof(od_control));
   od_control.od_force_local = TRUE;
   od_control.od_disable = DIS_INFOFILE | DIS_NAME_PROMPT |
      DIS_CARRIERDETECT | DIS_TIMEOUT | DIS_LOCAL_INPUT;
   od_control.od_silent_mode = TRUE;
   od_control.od_nocopyright = TRUE;
   od_control.od_noexit = TRUE;
   od_control.od_errorlevel[ERRORLEVEL_CRITICAL] = 97;
}
#endif

#endif
