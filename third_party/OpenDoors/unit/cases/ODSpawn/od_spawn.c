#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK___get_errno_ptr
#define __get_errno_ptr utm___get_errno_ptr
static int ut_errno_value;
int *utm___get_errno_ptr(void) { return(&ut_errno_value); }
#elif defined(ODPLAT_NIX)
static int ut_errno_value;
#define UT_ERRNO_STORAGE ut_errno_value
#include "../unix_errno_mock.h"
#endif

#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed
static BOOL ut_public_call_allowed = TRUE;
tODControl od_control;
BOOL utm_ODSyncPublicCallAllowed(void)
{
   if(!ut_public_call_allowed)
      od_control.od_error = ERR_GENERALFAILURE;
   return ut_public_call_allowed;
}

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
#define UT_CUSTOM_MOCK_getenv
#define UT_CUSTOM_MOCK_od_spawnvpe
static const char *ut_comspec;
static INT16 ut_spawn_results[2];
static int ut_spawn_errors[2];
static unsigned ut_spawn_calls;
char *utm_getenv(const char *name)
{
   UT_ASSERT(strcmp(name, "COMSPEC") == 0);
   return((char *)ut_comspec);
}
INT16 ODCALL utm_od_spawnvpe(INT16 mode, const char *path,
   const char *const arguments[], const char *const environment[])
{
   unsigned call = ut_spawn_calls++;
   UT_ASSERT_EQ_INT(P_WAIT, mode); UT_ASSERT_NULL(environment);
   UT_ASSERT_EQ_PTR(path, arguments[0]);
   UT_ASSERT(strcmp(arguments[1], "/c") == 0);
   UT_ASSERT(strcmp(arguments[2], "door arg") == 0);
   UT_ASSERT_NULL(arguments[3]);
   errno = ut_spawn_errors[call];
   return(ut_spawn_results[call]);
}
static void reset_dos_spawn(void)
{
   ut_comspec = "C:\\COMMAND.COM"; ut_spawn_calls = 0;
   ut_spawn_results[0] = ut_spawn_results[1] = 0;
   ut_spawn_errors[0] = ut_spawn_errors[1] = 0;
}
static void uses_comspec_when_available(void)
{
   reset_dos_spawn(); ut_spawn_results[0] = 3;
   UT_ASSERT(utt_od_spawn("door arg")); UT_ASSERT_EQ_UINT(1, ut_spawn_calls);
   reset_dos_spawn(); ut_spawn_results[0] = -1; ut_spawn_errors[0] = EACCES;
   UT_ASSERT(!utt_od_spawn("door arg")); UT_ASSERT_EQ_UINT(1, ut_spawn_calls);
}
static void falls_back_only_when_comspec_is_missing(void)
{
   reset_dos_spawn(); ut_comspec = NULL; ut_spawn_results[0] = 4;
   UT_ASSERT(utt_od_spawn("door arg")); UT_ASSERT_EQ_UINT(1, ut_spawn_calls);
   reset_dos_spawn(); ut_spawn_results[0] = -1; ut_spawn_errors[0] = ENOENT;
   ut_spawn_results[1] = 5;
   UT_ASSERT(utt_od_spawn("door arg")); UT_ASSERT_EQ_UINT(2, ut_spawn_calls);
   reset_dos_spawn(); ut_spawn_results[0] = -1; ut_spawn_errors[0] = ENOENT;
   ut_spawn_results[1] = -1;
   UT_ASSERT(!utt_od_spawn("door arg")); UT_ASSERT_EQ_UINT(2, ut_spawn_calls);
}
#endif

#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_strchr
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_ODSpawnVPEInternal
static char ut_program[64];
static BOOL ut_malloc_fails;
static INT16 ut_spawn_result;
static unsigned ut_free_calls;
char *utm_strchr(const char *text, int character)
{
   while(*text != '\0') { if(*text == character) return((char *)text); ++text; }
   return(character == 0 ? (char *)text : NULL);
}
size_t utm_strlen(const char *text)
{ size_t length = 0; while(text[length] != '\0') ++length; return(length); }
void *utm_malloc(size_t size)
{
   UT_ASSERT(size <= sizeof(ut_program));
   return(ut_malloc_fails ? NULL : ut_program);
}
void *utm_memcpy(void *destination, const void *source, size_t size)
{
   char *out = destination; const char *in = source; size_t index;
   for(index = 0; index < size; ++index) out[index] = in[index];
   return(destination);
}
void utm_free(void *memory)
{ UT_ASSERT_EQ_PTR(ut_program, memory); ++ut_free_calls; }
INT16 utm_ODSpawnVPEInternal(INT16 mode, const char *path,
   const char *const arguments[], const char *const environment[],
   BOOL quote_arguments)
{
   UT_ASSERT_EQ_INT(P_WAIT, mode); UT_ASSERT_EQ_PTR(ut_program, path);
   UT_ASSERT_EQ_PTR(path, arguments[0]); UT_ASSERT_NULL(environment);
   UT_ASSERT(!quote_arguments);
   if(strcmp(path, "program") == 0)
   { UT_ASSERT(strcmp(arguments[1], "one two") == 0); UT_ASSERT_NULL(arguments[2]); }
   else
   { UT_ASSERT(strcmp(path, "solo") == 0); UT_ASSERT_NULL(arguments[1]); }
   return(ut_spawn_result);
}
static void reset_windows_spawn(void)
{
   memset(ut_program, 0, sizeof(ut_program)); ut_malloc_fails = FALSE;
   ut_spawn_result = 0; ut_free_calls = 0;
}
static void reports_program_name_allocation_failure(void)
{
   reset_windows_spawn(); ut_malloc_fails = TRUE;
   UT_ASSERT(!utt_od_spawn("solo")); UT_ASSERT_EQ_UINT(0, ut_free_calls);
}
static void splits_optional_argument_tail_and_reports_spawn_result(void)
{
   reset_windows_spawn(); ut_spawn_result = 7;
   UT_ASSERT(utt_od_spawn("solo")); UT_ASSERT_EQ_UINT(1, ut_free_calls);
   reset_windows_spawn(); ut_spawn_result = 0;
   UT_ASSERT(utt_od_spawn("program one two")); UT_ASSERT_EQ_UINT(1, ut_free_calls);
   reset_windows_spawn(); ut_spawn_result = -1;
   UT_ASSERT(!utt_od_spawn("program one two")); UT_ASSERT_EQ_UINT(1, ut_free_calls);
}
#endif

#ifdef ODPLAT_NIX
#define UT_CUSTOM_MOCK_sigemptyset
#define UT_CUSTOM_MOCK_sigaddset
#define UT_CUSTOM_MOCK_sigprocmask
#define UT_CUSTOM_MOCK_system
static int ut_empty_result, ut_add_result;
static int ut_mask_results[2];
static int ut_system_result, ut_system_error;
static unsigned ut_empty_calls, ut_add_calls, ut_mask_calls, ut_system_calls;
int utm_sigemptyset(sigset_t *set)
{ (void)set; ++ut_empty_calls; return(ut_empty_result); }
int utm_sigaddset(sigset_t *set, int signal_number)
{ (void)set; UT_ASSERT_EQ_INT(SIGALRM, signal_number); ++ut_add_calls; return(ut_add_result); }
int utm_sigprocmask(int operation, const sigset_t *set, sigset_t *old_set)
{
   unsigned call = ut_mask_calls++;
   (void)set;
   if(call == 0) { UT_ASSERT_EQ_INT(SIG_BLOCK, operation); UT_ASSERT_NOT_NULL(old_set); }
   else { UT_ASSERT_EQ_INT(SIG_SETMASK, operation); UT_ASSERT_NULL(old_set); }
   return(ut_mask_results[call]);
}
int utm_system(const char *command)
{
   UT_ASSERT(strcmp(command, "door arg") == 0); ++ut_system_calls;
   errno = ut_system_error; return(ut_system_result);
}
static void reset_unix_spawn(void)
{
   ut_empty_result = ut_add_result = 0;
   ut_mask_results[0] = ut_mask_results[1] = 0;
   ut_system_result = 0; ut_system_error = 0;
   ut_empty_calls = ut_add_calls = ut_mask_calls = ut_system_calls = 0;
}
static void reports_signal_mask_setup_failures(void)
{
   reset_unix_spawn(); ut_empty_result = -1;
   UT_ASSERT(!utt_od_spawn("door arg"));
   UT_ASSERT_EQ_UINT(1, ut_empty_calls); UT_ASSERT_EQ_UINT(0, ut_add_calls);
   reset_unix_spawn(); ut_add_result = -1;
   UT_ASSERT(!utt_od_spawn("door arg"));
   UT_ASSERT_EQ_UINT(1, ut_add_calls); UT_ASSERT_EQ_UINT(0, ut_mask_calls);
   reset_unix_spawn(); ut_mask_results[0] = -1;
   UT_ASSERT(!utt_od_spawn("door arg"));
   UT_ASSERT_EQ_UINT(1, ut_mask_calls); UT_ASSERT_EQ_UINT(0, ut_system_calls);
}
static void restores_mask_and_interprets_system_status(void)
{
   reset_unix_spawn(); ut_system_result = -1; ut_system_error = EACCES;
   UT_ASSERT(!utt_od_spawn("door arg")); UT_ASSERT_EQ_INT(EACCES, errno);
   reset_unix_spawn(); ut_mask_results[1] = -1;
   UT_ASSERT(!utt_od_spawn("door arg")); UT_ASSERT_EQ_UINT(2, ut_mask_calls);
   reset_unix_spawn(); ut_system_result = 127 << 8;
   UT_ASSERT(!utt_od_spawn("door arg"));
   reset_unix_spawn(); ut_system_result = 9;
   UT_ASSERT(utt_od_spawn("door arg"));
   reset_unix_spawn(); ut_system_result = 3 << 8;
   UT_ASSERT(utt_od_spawn("door arg"));
}
#endif

static void rejects_a_terminal_session(void)
{
   ut_public_call_allowed = FALSE;
   UT_ASSERT(!utt_od_spawn("door arg"));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   ut_public_call_allowed = TRUE;
}

static const UTTestCase ut_cases[] = {
   {"terminal session", rejects_a_terminal_session},
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   {"COMSPEC", uses_comspec_when_available},
   {"command.com fallback", falls_back_only_when_comspec_is_missing}
#elif defined(ODPLAT_WIN32)
   {"allocation failure", reports_program_name_allocation_failure},
   {"argument splitting", splits_optional_argument_tail_and_reports_spawn_result}
#else
   {"signal-mask setup", reports_signal_mask_setup_failures},
   {"system status", restores_mask_and_interprets_system_status}
#endif
};
