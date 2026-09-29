#include <errno.h>
#include <stdlib.h>

#include "test_support.h"

static BOOL FileExists(const char *name)
{
   FILE *file = fopen(name, "r");
   if(file == NULL)
      return(FALSE);
   return(fclose(file) == 0);
}

static BOOL WaitForFile(const char *name)
{
   int attempt;
   for(attempt = 0; attempt < 60; ++attempt)
   {
      if(FileExists(name))
         return(TRUE);
      od_sleep(50);
   }
   return(FALSE);
}

static int SetEnvironment(char *assignment)
{
#ifdef ODPLAT_WIN32
   return(_putenv(assignment));
#else
   return(putenv(assignment));
#endif
}

static void SpawnStage(const char *name)
{
   fprintf(stderr, "spawn acceptance stage: %s\n", name);
   fflush(stderr);
}

int main(int argc, char **argv)
{
   const char *child_args[9];
   const char *child_environment[2];
   const char *child_name;
   const char *child_separator;
   const char *other_separator;
   const char *old_path;
   FILE *file;
   INT16 result;
   char command[512];
   char path_assignment[4096];
   static char inherited_assignment[] =
      "OD_INHERITED_VALUE=inherited value";
   size_t child_directory_length;
   size_t path_length;

   OD_TEST_CHECK(argc == 2);
   remove("ODCHILD.OK");
   remove("ODSIMPLE.OK");
   remove("ODEMPTY.OK");
   remove("ODNZERO.OK");
   remove("ODINHER.OK");
   remove("ODPATH.OK");
   remove("ODNOWAIT.OK");
   ODTestConfigureLocal();
   od_init();
   OD_TEST_CHECK(SetEnvironment(inherited_assignment) == 0);

   child_args[0] = argv[1];
   child_args[1] = "argument one";
   child_args[2] = "";
   child_args[3] = "quote\"inside";
   child_args[4] = "slashes\\\\\"quote";
   child_args[5] = "ends with slash\\";
   child_args[6] = "tab\targument";
   child_args[7] = "argument-two";
   child_args[8] = NULL;
   child_environment[0] = "OD_ACCEPT_VALUE=environment value";
   child_environment[1] = NULL;
   SpawnStage("explicit environment");
   result = od_spawnvpe(P_WAIT, argv[1], child_args, child_environment);
   if(result != 23)
      fprintf(stderr, "od_spawnvpe returned %d (errno %d)\n",
         (int)result, errno);
   OD_TEST_CHECK(result == 23);
   file = fopen("ODCHILD.OK", "r");
   OD_TEST_CHECK(file != NULL);
   OD_TEST_CHECK(fclose(file) == 0);
   remove("ODCHILD.OK");

   OD_TEST_CHECK(strlen(argv[1]) + 1 < sizeof(command));
   strcpy(command, argv[1]);
   SpawnStage("empty command tail");
   OD_TEST_CHECK(od_spawn(command));
   OD_TEST_CHECK(FileExists("ODEMPTY.OK"));
   remove("ODEMPTY.OK");

   OD_TEST_CHECK(strlen(argv[1]) + sizeof(" --simple") < sizeof(command));
   strcpy(command, argv[1]);
   strcat(command, " --simple");
   SpawnStage("simple command tail");
   OD_TEST_CHECK(od_spawn(command));
   file = fopen("ODSIMPLE.OK", "r");
   OD_TEST_CHECK(file != NULL);
   OD_TEST_CHECK(fclose(file) == 0);
   remove("ODSIMPLE.OK");

   OD_TEST_CHECK(strlen(argv[1]) + sizeof(" --nonzero") < sizeof(command));
   strcpy(command, argv[1]);
   strcat(command, " --nonzero");
   SpawnStage("nonzero command tail");
   OD_TEST_CHECK(od_spawn(command));
   OD_TEST_CHECK(FileExists("ODNZERO.OK"));
   remove("ODNZERO.OK");

   child_args[0] = argv[1];
   child_args[1] = "--inherit";
   child_args[2] = NULL;
   SpawnStage("inherited environment");
   result = od_spawnvpe(P_WAIT, argv[1], child_args, NULL);
   OD_TEST_CHECK(result == 31);
   OD_TEST_CHECK(FileExists("ODINHER.OK"));
   remove("ODINHER.OK");

   child_separator = strrchr(argv[1], '/');
   other_separator = strrchr(argv[1], '\\');
   if(other_separator != NULL && (child_separator == NULL
      || other_separator > child_separator))
   {
      child_separator = other_separator;
   }
   if(child_separator == NULL)
   {
      child_name = argv[1];
      child_directory_length = 1;
   }
   else
   {
      child_name = child_separator + 1;
      child_directory_length = (size_t)(child_separator - argv[1]);
   }
   old_path = getenv("PATH");
   path_length = old_path == NULL ? 0 : strlen(old_path);
   OD_TEST_CHECK(sizeof("PATH=.") + child_directory_length + path_length
      < sizeof(path_assignment));
   strcpy(path_assignment, "PATH=");
   if(child_separator == NULL)
      path_assignment[5] = '.';
   else
      memcpy(path_assignment + 5, argv[1], child_directory_length);
   path_assignment[5 + child_directory_length] =
#ifdef ODPLAT_WIN32
      ';';
#else
      ':';
#endif
   if(old_path == NULL)
      path_assignment[6 + child_directory_length] = '\0';
   else
      strcpy(path_assignment + 6 + child_directory_length, old_path);
   OD_TEST_CHECK(SetEnvironment(path_assignment) == 0);
   child_args[0] = child_name;
   child_args[1] = "--path";
   child_args[2] = NULL;
   SpawnStage("PATH search");
   result = od_spawnvpe(P_WAIT, child_name, child_args, NULL);
   OD_TEST_CHECK(result == 29);
   OD_TEST_CHECK(FileExists("ODPATH.OK"));
   remove("ODPATH.OK");

   child_args[0] = argv[1];
   child_args[1] = "--nowait";
   child_args[2] = NULL;
   SpawnStage("no-wait child");
   result = od_spawnvpe(P_NOWAIT, argv[1], child_args, NULL);
   OD_TEST_CHECK(result != -1);
   OD_TEST_CHECK(WaitForFile("ODNOWAIT.OK"));
   remove("ODNOWAIT.OK");

   SpawnStage("missing command");
   OD_TEST_CHECK(!od_spawn("opendoors-acceptance-missing-program"));

   errno = 0;
   SpawnStage("missing wait child");
   result = od_spawnvpe(P_WAIT, "opendoors-acceptance-missing-program",
      child_args, child_environment);
   OD_TEST_CHECK(result == -1);
   SpawnStage("missing no-wait child");
   result = od_spawnvpe(P_NOWAIT, "opendoors-acceptance-missing-program",
      child_args, child_environment);
   OD_TEST_CHECK(result == -1);

#ifdef ODPLAT_WIN32
   od_control.od_error = ERR_NONE;
   SpawnStage("null argument vector");
   result = od_spawnvpe(P_WAIT, argv[1], NULL, NULL);
   OD_TEST_CHECK(result == -1);
   OD_TEST_CHECK(od_control.od_error == ERR_PARAMETER);
   child_args[0] = NULL;
   od_control.od_error = ERR_NONE;
   SpawnStage("null argv zero");
   result = od_spawnvpe(P_WAIT, argv[1], child_args, NULL);
   OD_TEST_CHECK(result == -1);
   OD_TEST_CHECK(od_control.od_error == ERR_PARAMETER);
#endif

   SpawnStage("exit");
   od_exit(0, FALSE);
   return(0);
}
