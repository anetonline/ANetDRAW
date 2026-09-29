#ifdef ODPLAT_DOS
#include <dos.h>
#ifdef __WATCOMC__
#include <i86.h>
#endif
#define UT_CUSTOM_MOCK___get_errno_ptr
#define __get_errno_ptr utm___get_errno_ptr
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_strchr
#define UT_CUSTOM_MOCK_strcpy

static int ut_errno_value;
static char ut_memory[8192];
static BOOL ut_malloc_failure;
static BOOL ut_oversized_string;
static unsigned ut_malloc_calls;
static unsigned ut_free_calls;

int *utm___get_errno_ptr(void) { return(&ut_errno_value); }

void *utm_malloc(size_t size)
{
   ++ut_malloc_calls;
   UT_ASSERT(size <= sizeof(ut_memory));
   return(ut_malloc_failure ? NULL : ut_memory);
}

void utm_free(void *memory)
{
   UT_ASSERT_EQ_PTR(ut_memory, memory);
   ++ut_free_calls;
}

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   if(ut_oversized_string) return(32766U);
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

static void reset_cmdenv(void)
{
   memset(ut_memory, 0x55, sizeof(ut_memory));
   ut_malloc_failure = FALSE;
   ut_oversized_string = FALSE;
   ut_malloc_calls = ut_free_calls = 0;
   errno = 0;
}

static int call_cmdenv(const char *const arguments[],
   const char *const environment[], char *command, char **env, char **memory)
{
   return(utt_cmdenv(arguments, environment, command, env, memory));
}

static void copies_an_explicit_environment(void)
{
   static const char *const environment[] = {"ONE=1", "TWO=second", NULL};
   static const char *const empty_environment[] = {NULL};
   static const char *const arguments[] = {"program", NULL};
   char command[128]; char *env; char *memory; int length;

   reset_cmdenv();
   length = call_cmdenv(arguments, environment, command, &env, &memory);
   UT_ASSERT_EQ_INT(18, length);
   UT_ASSERT_EQ_PTR(ut_memory, memory);
   UT_ASSERT_EQ_UINT(0, ((unsigned int)env) & 15U);
   UT_ASSERT(strcmp(env, "ONE=1") == 0);
   env += 6; UT_ASSERT(strcmp(env, "TWO=second") == 0);
   env += 11; UT_ASSERT_EQ_INT(0, *env);
   UT_ASSERT_EQ_INT(0, command[0]); UT_ASSERT_EQ_INT('\r', command[1]);

   reset_cmdenv();
   length = call_cmdenv(arguments, empty_environment, command, &env, &memory);
   UT_ASSERT_EQ_INT(1, length); UT_ASSERT_EQ_INT(0, env[0]);
   UT_ASSERT_EQ_UINT(1, ut_malloc_calls);
}

static void copies_the_process_environment(void)
{
   static const char *const arguments[] = {"program", NULL};
   char command[128]; char *env; char *memory; int length; int index;
   reset_cmdenv();
   length = call_cmdenv(arguments, NULL, command, &env, &memory);
   UT_ASSERT(length >= 2);
   UT_ASSERT_EQ_PTR(ut_memory, memory);
   index = length - 1;
   UT_ASSERT_EQ_INT(0, env[index]);
   UT_ASSERT_EQ_INT(0, env[index - 1]);
}

static void reports_environment_size_and_allocation_failures(void)
{
   static const char *const environment[] = {"TOO-LARGE", NULL};
   static const char *const arguments[] = {"program", NULL};
   char command[128]; char *env; char *memory;
   reset_cmdenv(); ut_oversized_string = TRUE;
   UT_ASSERT_EQ_INT(-1, call_cmdenv(arguments, environment, command, &env, &memory));
   UT_ASSERT_EQ_INT(E2BIG, errno); UT_ASSERT_EQ_UINT(0, ut_malloc_calls);

   reset_cmdenv(); ut_malloc_failure = TRUE;
   UT_ASSERT_EQ_INT(-1, call_cmdenv(arguments, environment, command, &env, &memory));
   UT_ASSERT_EQ_INT(ENOMEM, errno); UT_ASSERT_EQ_UINT(1, ut_malloc_calls);

   reset_cmdenv(); ut_malloc_failure = TRUE;
   UT_ASSERT_EQ_INT(-1, call_cmdenv(arguments, NULL, command, &env, &memory));
   UT_ASSERT_EQ_INT(ENOMEM, errno); UT_ASSERT_EQ_UINT(1, ut_malloc_calls);
}

static void rejects_an_oversized_process_environment(void)
{
   static const char *const arguments[] = {"program", NULL};
   unsigned segment;
   unsigned original_segment;
   unsigned far *psp_environment;
   char far *large_environment;
   unsigned long index;
   char command[128]; char *env; char *memory; int result;

   reset_cmdenv();
#ifdef __TURBOC__
   UT_ASSERT_EQ_INT(-1, allocmem(0x0801, &segment));
#else
   UT_ASSERT_EQ_INT(0, _dos_allocmem(0x0801, &segment));
#endif
   large_environment = (char far *)MK_FP(segment, 0);
   for(index = 0; index < 32766UL; ++index)
      large_environment[index] = 'A';
   large_environment[32766UL] = '\0';
   large_environment[32767UL] = '\0';
   psp_environment = (unsigned far *)MK_FP(_psp, 0x2c);
   original_segment = *psp_environment;
   *psp_environment = segment;
   result = call_cmdenv(arguments, NULL, command, &env, &memory);
   *psp_environment = original_segment;
#ifdef __TURBOC__
   UT_ASSERT_EQ_INT(0, freemem(segment));
#else
   UT_ASSERT_EQ_INT(0, _dos_freemem(segment));
#endif
   UT_ASSERT_EQ_INT(-1, result);
   UT_ASSERT_EQ_INT(E2BIG, errno);
   UT_ASSERT_EQ_UINT(0, ut_malloc_calls);
}

static void constructs_the_dos_command_tail(void)
{
   static const char *const no_program[] = {NULL};
   static const char *const arguments[] = {"program", "one", "two words", NULL};
   static const char *const long_arguments[] = {"program",
      "1234567890123456789012345678901234567890123456789012345678901234",
      "12345678901234567890123456789012345678901234567890123456789012", NULL};
   static const char *const environment[] = {NULL};
   char command[128]; char *env; char *memory;

   reset_cmdenv();
   UT_ASSERT_EQ_INT(1, call_cmdenv(NULL, environment, command, &env, &memory));
   UT_ASSERT_EQ_INT(0, command[0]); UT_ASSERT_EQ_INT('\r', command[1]);
   reset_cmdenv();
   UT_ASSERT_EQ_INT(1, call_cmdenv(no_program, environment, command, &env, &memory));
   UT_ASSERT_EQ_INT(0, command[0]);

   reset_cmdenv();
   UT_ASSERT_EQ_INT(1, call_cmdenv(arguments, environment, command, &env, &memory));
   UT_ASSERT_EQ_INT(14, command[0]);
   command[15] = '\0'; UT_ASSERT(strcmp(command + 1, " one two words") == 0);

   reset_cmdenv();
   UT_ASSERT_EQ_INT(-1, call_cmdenv(long_arguments, environment,
      command, &env, &memory));
   UT_ASSERT_EQ_INT(E2BIG, errno); UT_ASSERT_EQ_UINT(1, ut_free_calls);
}

static const UTTestCase ut_cases[] = {
   {"explicit environment", copies_an_explicit_environment},
   {"process environment", copies_the_process_environment},
   {"environment failures", reports_environment_size_and_allocation_failures},
   {"large process environment", rejects_an_oversized_process_environment},
   {"command tail", constructs_the_dos_command_tail}
};
#endif
