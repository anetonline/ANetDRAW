#define UT_CUSTOM_MOCK_strcpy

#ifdef ODPLAT_NIX
#include <setjmp.h>
static int ut_errno_value;
#define UT_ERRNO_STORAGE ut_errno_value
#include "../unix_errno_mock.h"
#define UT_CUSTOM_MOCK_strchr
#define UT_CUSTOM_MOCK_getenv
#define UT_CUSTOM_MOCK_confstr
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_pipe
#define UT_CUSTOM_MOCK_fcntl
#define UT_CUSTOM_MOCK_close
#define UT_CUSTOM_MOCK_fork
#define UT_CUSTOM_MOCK_ODUnixReportSpawnError
#define UT_CUSTOM_MOCK_ODUnixExecProgram
#define UT_CUSTOM_MOCK__exit
#define UT_CUSTOM_MOCK_ODUnixWaitForChild
#define UT_CUSTOM_MOCK_ODUnixReadSpawnError

static const char *ut_arguments[] = {"program", NULL};
static const char *ut_environment[] = {"NAME=value", NULL};
static char ut_default_path[32], ut_path_buffer[128];
static const char *ut_path_value;
static size_t ut_confstr_size, ut_confstr_fill_result;
static void *ut_malloc_results[2];
static int ut_pipe_result, ut_fcntl_get_result, ut_fcntl_set_result;
static pid_t ut_fork_results[2];
static int ut_wait_result, ut_read_result, ut_child_error, ut_status;
static int ut_exec_error;
static BOOL ut_overflow_length;
static unsigned ut_confstr_calls, ut_malloc_calls, ut_free_calls;
static unsigned ut_fcntl_calls, ut_close_calls, ut_fork_calls;
static unsigned ut_exec_calls, ut_report_calls, ut_wait_calls, ut_read_calls;
static jmp_buf ut_child_target;

char *utm_strchr(const char *text, int character)
{ while(*text != '\0') { if(*text == character) return((char *)text); ++text; }
  return(character == 0 ? (char *)text : NULL); }
char *utm_getenv(const char *name)
{ UT_ASSERT(strcmp(name, "PATH") == 0); return((char *)ut_path_value); }
char *utm_strcpy(char *destination, const char *source)
{
   char *result = destination;
   while((*destination++ = *source++) != '\0') { }
   return(result);
}
size_t utm_confstr(int name, char *buffer, size_t size)
{
   ++ut_confstr_calls; UT_ASSERT_EQ_INT(_CS_PATH, name);
   if(buffer == NULL) { UT_ASSERT_EQ_UINT(0, size); return(ut_confstr_size); }
   UT_ASSERT_EQ_PTR(ut_default_path, buffer); UT_ASSERT_EQ_UINT(ut_confstr_size, size);
   if(ut_confstr_fill_result != 0) utm_strcpy(buffer, "/bin");
   return(ut_confstr_fill_result);
}
void *utm_malloc(size_t size)
{ (void)size; return(ut_malloc_results[ut_malloc_calls++]); }
size_t utm_strlen(const char *text)
{
   size_t length = 0; if(ut_overflow_length && text == ut_path_value) return((size_t)-1);
   while(text[length] != '\0') ++length; return(length);
}
void utm_free(void *memory) { (void)memory; ++ut_free_calls; }
int utm_pipe(int files[2])
{ files[0] = 10; files[1] = 11; return(ut_pipe_result); }
int utm_fcntl(int file, int command, ...)
{
   ++ut_fcntl_calls; UT_ASSERT_EQ_INT(11, file);
   if(command == F_GETFD) return(ut_fcntl_get_result);
   UT_ASSERT_EQ_INT(F_SETFD, command); return(ut_fcntl_set_result);
}
int utm_close(int file) { UT_ASSERT(file == 10 || file == 11); ++ut_close_calls; return(0); }
pid_t utm_fork(void) { return(ut_fork_results[ut_fork_calls++]); }
static int utm_ODUnixExecProgram(const char *path, const char *const arguments[],
   char *const environment[], const char *search, char *buffer)
{
   ++ut_exec_calls; UT_ASSERT(strcmp(path, "program") == 0);
   UT_ASSERT_EQ_PTR(ut_arguments, arguments); UT_ASSERT_NOT_NULL(environment);
   (void)search; (void)buffer; return(ut_exec_error);
}
static void utm_ODUnixReportSpawnError(int file, int error)
{
   UT_ASSERT_EQ_INT(11, file); UT_ASSERT_EQ_INT(ut_exec_error, error);
   ++ut_report_calls; longjmp(ut_child_target, 1);
}
void utm__exit(int status)
{ UT_ASSERT_EQ_INT(0, status); longjmp(ut_child_target, 2); }
static int utm_ODUnixWaitForChild(pid_t child, int *status)
{ ++ut_wait_calls; UT_ASSERT(child == (pid_t)42); *status = ut_status; return(ut_wait_result); }
static int utm_ODUnixReadSpawnError(int file, int *error)
{ ++ut_read_calls; UT_ASSERT_EQ_INT(10, file); *error = ut_child_error; return(ut_read_result); }

static void reset_unix_spawn(void)
{
   ut_path_value = "/bin:/usr/bin"; ut_confstr_size = 8; ut_confstr_fill_result = 8;
   ut_malloc_results[0] = ut_path_buffer; ut_malloc_results[1] = ut_path_buffer;
   ut_pipe_result = 0; ut_fcntl_get_result = 0; ut_fcntl_set_result = 0;
   ut_fork_results[0] = 42; ut_fork_results[1] = 0;
   ut_wait_result = 0; ut_read_result = 0; ut_child_error = EACCES;
   ut_status = 3 << 8; ut_exec_error = EACCES; ut_overflow_length = FALSE;
   ut_confstr_calls = ut_malloc_calls = ut_free_calls = ut_fcntl_calls = 0;
   ut_close_calls = ut_fork_calls = ut_exec_calls = ut_report_calls = 0;
   ut_wait_calls = ut_read_calls = 0; errno = 0;
}
static int call_spawn(int mode, const char *path, const char *const environment[])
{ return(utt__spawnvpe(mode, path, ut_arguments, environment)); }

static void selects_environment_and_search_path_sources(void)
{
   reset_unix_spawn(); UT_ASSERT_EQ_INT(3, call_spawn(P_WAIT, "./program", NULL));
   UT_ASSERT_EQ_UINT(0, ut_malloc_calls);
   reset_unix_spawn(); UT_ASSERT_EQ_INT(3, call_spawn(P_WAIT, "program", ut_environment));
   UT_ASSERT_EQ_UINT(1, ut_malloc_calls);
   reset_unix_spawn(); ut_path_value = NULL;
   ut_malloc_results[0] = ut_default_path; ut_malloc_results[1] = ut_path_buffer;
   UT_ASSERT_EQ_INT(3, call_spawn(P_WAIT, "program", NULL));
   UT_ASSERT_EQ_UINT(2, ut_confstr_calls); UT_ASSERT_EQ_UINT(2, ut_malloc_calls);
}
static void reports_default_path_and_allocation_failures(void)
{
   reset_unix_spawn(); ut_path_value = NULL; ut_confstr_size = 0;
   UT_ASSERT_EQ_INT(-1, call_spawn(P_WAIT, "program", NULL));
   reset_unix_spawn(); ut_path_value = NULL; ut_malloc_results[0] = NULL;
   UT_ASSERT_EQ_INT(-1, call_spawn(P_WAIT, "program", NULL));
   reset_unix_spawn(); ut_path_value = NULL; ut_malloc_results[0] = ut_default_path;
   ut_confstr_fill_result = 0; errno = EIO;
   UT_ASSERT_EQ_INT(-1, call_spawn(P_WAIT, "program", NULL)); UT_ASSERT_EQ_INT(EIO, errno);
   reset_unix_spawn(); ut_overflow_length = TRUE;
   UT_ASSERT_EQ_INT(-1, call_spawn(P_WAIT, "program", NULL)); UT_ASSERT_EQ_INT(ENAMETOOLONG, errno);
   reset_unix_spawn(); ut_malloc_results[0] = NULL; errno = ENOMEM;
   UT_ASSERT_EQ_INT(-1, call_spawn(P_WAIT, "program", NULL)); UT_ASSERT_EQ_INT(ENOMEM, errno);
}
static void reports_pipe_fcntl_and_fork_failures(void)
{
   reset_unix_spawn(); ut_pipe_result = -1; errno = EMFILE;
   UT_ASSERT_EQ_INT(-1, call_spawn(P_WAIT, "program", NULL)); UT_ASSERT_EQ_INT(EMFILE, errno);
   reset_unix_spawn(); ut_fcntl_get_result = -1; errno = EIO;
   UT_ASSERT_EQ_INT(-1, call_spawn(P_WAIT, "program", NULL));
   reset_unix_spawn(); ut_fcntl_set_result = -1; errno = EACCES;
   UT_ASSERT_EQ_INT(-1, call_spawn(P_WAIT, "program", NULL));
   reset_unix_spawn(); ut_fork_results[0] = -1; errno = EAGAIN;
   UT_ASSERT_EQ_INT(-1, call_spawn(P_WAIT, "program", NULL)); UT_ASSERT_EQ_INT(EAGAIN, errno);
}
static void covers_child_wait_and_detach_paths(void)
{
   int jump;
   reset_unix_spawn(); ut_fork_results[0] = 0;
   jump = setjmp(ut_child_target); if(jump == 0) (void)call_spawn(P_WAIT, "program", NULL);
   UT_ASSERT_EQ_INT(1, jump); UT_ASSERT_EQ_UINT(1, ut_exec_calls); UT_ASSERT_EQ_UINT(1, ut_report_calls);
   reset_unix_spawn(); ut_fork_results[0] = 0; ut_fork_results[1] = -1;
   errno = ut_exec_error;
   jump = setjmp(ut_child_target); if(jump == 0) (void)call_spawn(P_NOWAIT, "program", NULL);
   UT_ASSERT_EQ_INT(1, jump);
   reset_unix_spawn(); ut_fork_results[0] = 0; ut_fork_results[1] = 43;
   jump = setjmp(ut_child_target); if(jump == 0) (void)call_spawn(P_NOWAIT, "program", NULL);
   UT_ASSERT_EQ_INT(2, jump);
   reset_unix_spawn(); ut_fork_results[0] = 0; ut_fork_results[1] = 0;
   jump = setjmp(ut_child_target); if(jump == 0) (void)call_spawn(P_NOWAIT, "program", NULL);
   UT_ASSERT_EQ_INT(1, jump); UT_ASSERT_EQ_UINT(1, ut_exec_calls);
}
static void interprets_parent_wait_pipe_and_status_results(void)
{
   reset_unix_spawn(); ut_wait_result = -1; errno = ECHILD;
   UT_ASSERT_EQ_INT(-1, call_spawn(P_WAIT, "program", NULL));
   reset_unix_spawn(); ut_read_result = 1; ut_child_error = EACCES;
   UT_ASSERT_EQ_INT(-1, call_spawn(P_WAIT, "program", NULL)); UT_ASSERT_EQ_INT(EACCES, errno);
   reset_unix_spawn(); ut_read_result = -1; errno = EIO;
   UT_ASSERT_EQ_INT(-1, call_spawn(P_WAIT, "program", NULL)); UT_ASSERT_EQ_INT(EIO, errno);
   reset_unix_spawn(); ut_status = 9;
   UT_ASSERT_EQ_INT(-1, call_spawn(P_WAIT, "program", NULL)); UT_ASSERT_EQ_INT(ECHILD, errno);
   reset_unix_spawn(); ut_status = 4 << 8;
   UT_ASSERT_EQ_INT(4, call_spawn(P_WAIT, "program", NULL));
   reset_unix_spawn(); ut_status = 1 << 8;
   UT_ASSERT_EQ_INT(-1, call_spawn(P_NOWAIT, "program", NULL)); UT_ASSERT_EQ_INT(ECHILD, errno);
   reset_unix_spawn(); ut_status = 0;
   UT_ASSERT_EQ_INT(0, call_spawn(P_NOWAIT, "program", NULL));
}
static const UTTestCase ut_cases[] = {
   {"environment/PATH", selects_environment_and_search_path_sources},
   {"PATH failures", reports_default_path_and_allocation_failures},
   {"setup failures", reports_pipe_fcntl_and_fork_failures},
   {"child paths", covers_child_wait_and_detach_paths},
   {"parent results", interprets_parent_wait_pipe_and_status_results}
};
#endif

#ifdef ODPLAT_DOS
#define UT_CUSTOM_MOCK___get_errno_ptr
#define __get_errno_ptr utm___get_errno_ptr
#define UT_CUSTOM_MOCK_getenv
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_strchr
#define UT_CUSTOM_MOCK__spawnve

static int ut_errno_value;
static const char *ut_path_environment;
static int ut_spawn_results[8];
static int ut_spawn_errors[8];
static unsigned ut_spawn_calls;
static char ut_spawn_paths[8][80];
static const char *const ut_dos_arguments[] = {"door", NULL};
static const char *const ut_dos_environment[] = {"NAME=value", NULL};

int *utm___get_errno_ptr(void) { return(&ut_errno_value); }

char *utm_getenv(const char *name)
{
   UT_ASSERT(strcmp(name, "PATH") == 0);
   return((char *)ut_path_environment);
}

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return(length);
}

char *utm_strchr(const char *text, int character)
{
   do
   {
      if(*text == character) return((char *)text);
   } while(*text++ != '\0');
   return(NULL);
}

char *utm_strcpy(char *destination, const char *source)
{
   char *result = destination;
   while((*destination++ = *source++) != '\0') { }
   return(result);
}

int utm__spawnve(int mode, const char *path,
   const char *const arguments[], const char *const environment[])
{
   unsigned call = ut_spawn_calls++;
   UT_ASSERT_EQ_INT(P_WAIT, mode);
   UT_ASSERT_EQ_PTR(ut_dos_arguments, arguments);
   UT_ASSERT_EQ_PTR(ut_dos_environment, environment);
   utm_strcpy(ut_spawn_paths[call], path);
   errno = ut_spawn_errors[call];
   return(ut_spawn_results[call]);
}

static void reset_dos_spawnvpe(void)
{
   unsigned index;
   ut_path_environment = "C:\\BIN";
   ut_spawn_calls = 0;
   for(index = 0; index < 8; ++index)
   {
      ut_spawn_results[index] = -1;
      ut_spawn_errors[index] = ENOENT;
      ut_spawn_paths[index][0] = '\0';
   }
   utm_strcpy(od_control.od_swapping_path, "D:\\SWAP");
   od_control.od_swapping_noems = TRUE;
   od_control.od_swapping_disable = TRUE;
   _swappath = NULL; _useems = 0; _swap = 0;
}

static int call_dos_spawnvpe(const char *path)
{
   return(utt__spawnvpe(P_WAIT, path, ut_dos_arguments, ut_dos_environment));
}

static void configures_swapping_and_honors_direct_results(void)
{
   static const char embedded_colon[] = {'\0', ':', '\0'};
   reset_dos_spawnvpe(); ut_spawn_results[0] = 4;
   UT_ASSERT_EQ_INT(4, call_dos_spawnvpe("door"));
   UT_ASSERT_EQ_PTR(od_control.od_swapping_path, _swappath);
   UT_ASSERT_EQ_INT(TRUE, _useems); UT_ASSERT_EQ_INT(TRUE, _swap);

   reset_dos_spawnvpe(); ut_spawn_errors[0] = EACCES;
   UT_ASSERT_EQ_INT(-1, call_dos_spawnvpe("door"));
   UT_ASSERT_EQ_UINT(1, ut_spawn_calls);

   reset_dos_spawnvpe();
   UT_ASSERT_EQ_INT(-1, call_dos_spawnvpe("\\door"));
   reset_dos_spawnvpe();
   UT_ASSERT_EQ_INT(-1, call_dos_spawnvpe("/door"));
   reset_dos_spawnvpe();
   UT_ASSERT_EQ_INT(-1, call_dos_spawnvpe("C:door"));

   reset_dos_spawnvpe(); ut_path_environment = NULL;
   UT_ASSERT_EQ_INT(-1, call_dos_spawnvpe("door"));
   reset_dos_spawnvpe(); ut_path_environment = NULL;
   UT_ASSERT_EQ_INT(-1, call_dos_spawnvpe(""));
   reset_dos_spawnvpe();
   UT_ASSERT_EQ_INT(-1, call_dos_spawnvpe(embedded_colon));

   reset_dos_spawnvpe(); od_control.od_swapping_path[0] = '\0';
   ut_spawn_results[0] = 1;
   UT_ASSERT_EQ_INT(1, call_dos_spawnvpe("door"));
   UT_ASSERT_NULL(_swappath);
}

static void skips_oversized_search_components(void)
{
   static char path[76];
   unsigned index;
   reset_dos_spawnvpe();
   for(index = 0; index < 67; ++index) path[index] = 'A';
   utm_strcpy(path + 67, ";B");
   ut_path_environment = path; ut_spawn_results[1] = 6;
   UT_ASSERT_EQ_INT(6, call_dos_spawnvpe("door"));
   UT_ASSERT(strcmp("B\\door", ut_spawn_paths[1]) == 0);

   reset_dos_spawnvpe();
   for(index = 0; index < 67; ++index) path[index] = 'A';
   path[67] = '\0'; ut_path_environment = path;
   UT_ASSERT_EQ_INT(-1, call_dos_spawnvpe("door"));
   UT_ASSERT_EQ_UINT(1, ut_spawn_calls);
}

static void searches_path_components_and_preserves_separators(void)
{
   reset_dos_spawnvpe();
   ut_path_environment = ";C:\\BIN;D:/BIN/;E:\\LAST";
   ut_spawn_results[3] = 9;
   UT_ASSERT_EQ_INT(9, call_dos_spawnvpe("door"));
   UT_ASSERT_EQ_UINT(4, ut_spawn_calls);
   UT_ASSERT(strcmp("C:\\BIN\\door", ut_spawn_paths[1]) == 0);
   UT_ASSERT(strcmp("D:/BIN/door", ut_spawn_paths[2]) == 0);
   UT_ASSERT(strcmp("E:\\LAST\\door", ut_spawn_paths[3]) == 0);

   reset_dos_spawnvpe(); ut_path_environment = "C:\\BIN\\";
   ut_spawn_results[1] = 8;
   UT_ASSERT_EQ_INT(8, call_dos_spawnvpe("door"));
   UT_ASSERT(strcmp("C:\\BIN\\door", ut_spawn_paths[1]) == 0);

   reset_dos_spawnvpe(); ut_path_environment = "C:\\BIN";
   ut_spawn_errors[1] = EACCES;
   UT_ASSERT_EQ_INT(-1, call_dos_spawnvpe("door"));
   UT_ASSERT_EQ_INT(EACCES, errno);

   reset_dos_spawnvpe(); ut_path_environment = ";";
   UT_ASSERT_EQ_INT(-1, call_dos_spawnvpe("door"));
   UT_ASSERT_EQ_UINT(1, ut_spawn_calls);
}

static const UTTestCase ut_cases[] = {
   {"direct result", configures_swapping_and_honors_direct_results},
   {"long PATH", skips_oversized_search_components},
   {"PATH search", searches_path_components_and_preserves_separators}
};
#endif
