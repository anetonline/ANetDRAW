static int ut_errno_value;
#define UT_ERRNO_STORAGE ut_errno_value
#include "../unix_errno_mock.h"
#define UT_CUSTOM_MOCK_execve
#define UT_CUSTOM_MOCK_strchr
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_strcpy
static int ut_exec_errors[5];
static char ut_exec_paths[5][64];
static unsigned ut_exec_count;
size_t utm_strlen(const char *text);
void *utm_memcpy(void *destination, const void *source, size_t size);
int utm_execve(const char *path, char *const arguments[], char *const environment[])
{
   size_t length = utm_strlen(path); unsigned call = ut_exec_count++;
   UT_ASSERT(call < 5); UT_ASSERT_NOT_NULL(arguments); UT_ASSERT_NOT_NULL(environment);
   utm_memcpy(ut_exec_paths[call], path, length + 1); errno = ut_exec_errors[call];
   return(-1);
}
char *utm_strchr(const char *text, int character)
{ while(*text != '\0') { if(*text == character) return((char *)text); ++text; }
  return(character == 0 ? (char *)text : NULL); }
size_t utm_strlen(const char *text)
{ size_t length = 0; while(text[length] != '\0') ++length; return(length); }
void *utm_memcpy(void *destination, const void *source, size_t size)
{ char *out = destination; const char *in = source; size_t index;
  for(index = 0; index < size; ++index) out[index] = in[index]; return(destination); }
char *utm_strcpy(char *destination, const char *source)
{ utm_memcpy(destination, source, utm_strlen(source) + 1); return(destination); }
static void reset_exec(void)
{ memset(ut_exec_errors, 0, sizeof(ut_exec_errors));
  memset(ut_exec_paths, 0, sizeof(ut_exec_paths)); ut_exec_count = 0; }
static int call_exec(const char *path, const char *search)
{
   static const char *arguments[] = {"program", NULL};
   static char *environment[] = {"NAME=value", NULL};
   char buffer[128];
   return(utt_ODUnixExecProgram(path, arguments, environment, search, buffer));
}
static void returns_direct_path_and_unsearchable_errors(void)
{
   reset_exec(); ut_exec_errors[0] = EACCES;
   UT_ASSERT_EQ_INT(EACCES, call_exec("./program", "one"));
   UT_ASSERT_EQ_UINT(1, ut_exec_count);
   reset_exec(); ut_exec_errors[0] = EIO;
   UT_ASSERT_EQ_INT(EIO, call_exec("program", "one"));
   UT_ASSERT_EQ_UINT(1, ut_exec_count);
}
static void searches_each_nonempty_path_component(void)
{
   reset_exec(); ut_exec_errors[0] = ENOENT; ut_exec_errors[1] = EACCES;
   ut_exec_errors[2] = ENOTDIR;
   UT_ASSERT_EQ_INT(EACCES, call_exec("program", "one::two/"));
   UT_ASSERT_EQ_UINT(3, ut_exec_count);
   UT_ASSERT(strcmp(ut_exec_paths[1], "one/program") == 0);
   UT_ASSERT(strcmp(ut_exec_paths[2], "two/program") == 0);
   reset_exec(); ut_exec_errors[0] = ENOENT; ut_exec_errors[1] = EIO;
   UT_ASSERT_EQ_INT(EIO, call_exec("program", "one:two"));
   UT_ASSERT_EQ_UINT(2, ut_exec_count);
   reset_exec(); ut_exec_errors[0] = ENOTDIR; ut_exec_errors[1] = ENOENT;
   UT_ASSERT_EQ_INT(ENOENT, call_exec("program", "one"));
}
static void retains_initial_access_error_without_candidates(void)
{
   reset_exec(); ut_exec_errors[0] = EACCES;
   UT_ASSERT_EQ_INT(EACCES, call_exec("program", ":"));
   UT_ASSERT_EQ_UINT(1, ut_exec_count);
}
static const UTTestCase ut_cases[] = {
   {"direct path/error", returns_direct_path_and_unsearchable_errors},
   {"PATH search", searches_each_nonempty_path_component},
   {"empty PATH", retains_initial_access_error_without_candidates}
};
