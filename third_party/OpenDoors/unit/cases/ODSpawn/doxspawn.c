#ifdef ODPLAT_DOS
#define UT_CUSTOM_MOCK___get_errno_ptr
#define __get_errno_ptr utm___get_errno_ptr
#define UT_CUSTOM_MOCK_cmdenv
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK__chkems
#define UT_CUSTOM_MOCK__xsize
#define UT_CUSTOM_MOCK_ODDWordShiftLeft
#define UT_CUSTOM_MOCK_ODDWordShiftRight
#define UT_CUSTOM_MOCK__savemap
#define UT_CUSTOM_MOCK__getems
#define UT_CUSTOM_MOCK_tempfile
#define UT_CUSTOM_MOCK_savevect
#define UT_CUSTOM_MOCK__xspawn
#define UT_CUSTOM_MOCK__setvect
#define UT_CUSTOM_MOCK__getrc
#define UT_CUSTOM_MOCK__restmap

static int ut_errno_value;
static char ut_environment_memory[32];
static char ut_environment[32];
static char ut_map_memory[32];
static int ut_cmdenv_result;
static BOOL ut_map_malloc_failure;
static int ut_chkems_result;
static int ut_xsize_result;
static long ut_total_size;
static int ut_savemap_result;
static int ut_getems_result;
static int ut_tempfile_result;
static int ut_xspawn_result;
static int ut_child_result;
static int ut_restmap_result;
static const char *ut_xspawn_file;
static unsigned ut_malloc_calls, ut_free_environment_calls, ut_free_map_calls;
static unsigned ut_chkems_calls, ut_tempfile_calls, ut_savevect_calls;
static unsigned ut_xspawn_calls, ut_setvect_calls, ut_getrc_calls;
static unsigned ut_savemap_calls, ut_getems_calls, ut_restmap_calls;
static const char *const ut_arguments[] = {"program", NULL};
static const char *const ut_environment_vector[] = {"NAME=value", NULL};

int *utm___get_errno_ptr(void) { return(&ut_errno_value); }

static int utm_cmdenv(const char *const arguments[],
   const char *const environment_vector[], char *command, char **environment,
   char **memory)
{
   UT_ASSERT_EQ_PTR(ut_arguments, arguments);
   UT_ASSERT_EQ_PTR(ut_environment_vector, environment_vector);
   command[0] = 0; command[1] = '\r';
   *environment = ut_environment; *memory = ut_environment_memory;
   return(ut_cmdenv_result);
}

void *utm_malloc(size_t size)
{
   ++ut_malloc_calls; UT_ASSERT_EQ_UINT((unsigned)mapsize, (unsigned)size);
   return(ut_map_malloc_failure ? NULL : ut_map_memory);
}

void utm_free(void *memory)
{
   if(memory == ut_environment_memory) ++ut_free_environment_calls;
   else if(memory == ut_map_memory) ++ut_free_map_calls;
   else UT_ASSERT(FALSE);
}

int ODSWAPCALL utm__chkems(char *name, int *size)
{
   ++ut_chkems_calls; UT_ASSERT(strcmp(name, "EMMXXXX0") == 0);
   *size = 16; return(ut_chkems_result);
}

int ODSWAPCALL utm__xsize(unsigned int psp, long *swap_size, long *total_size)
{
   UT_ASSERT_EQ_UINT(_psp, psp); *swap_size = swapsize;
   *total_size = ut_total_size; return(ut_xsize_result);
}

DWORD utm_ODDWordShiftLeft(DWORD value, BYTE distance)
{ return((DWORD)(value << distance)); }
DWORD utm_ODDWordShiftRight(DWORD value, BYTE distance)
{ return((DWORD)(value >> distance)); }

int ODSWAPCALL utm__savemap(char *map)
{ ++ut_savemap_calls; UT_ASSERT_EQ_PTR(ut_map_memory, map); return(ut_savemap_result); }
int ODSWAPCALL utm__getems(int pages, int *handle)
{ ++ut_getems_calls; UT_ASSERT(pages >= 1); *handle = 41; return(ut_getems_result); }

static int utm_tempfile(char *file, int *handle)
{
   ++ut_tempfile_calls; strcpy(file, "C:\\SWAP.$$$"); *handle = 42;
   return(ut_tempfile_result);
}

static void utm_savevect(void) { ++ut_savevect_calls; }

int ODSWAPCALL utm__xspawn(char *path, char *command, char *environment,
   VECTOR *vectors, int no_swap, int environment_length, char *file,
   int handle)
{
   ++ut_xspawn_calls;
   UT_ASSERT(strcmp(path, "PROGRAM.EXE") == 0);
   UT_ASSERT_EQ_INT(0, command[0]); UT_ASSERT_EQ_PTR(ut_environment, environment);
   UT_ASSERT_EQ_PTR(vectab1, vectors); UT_ASSERT_EQ_INT(ut_cmdenv_result, environment_length);
   UT_ASSERT(no_swap == 0 || no_swap == 1); (void)handle;
   if(ut_xspawn_file != NULL) strcpy(file, ut_xspawn_file);
   return(ut_xspawn_result);
}

void ODSWAPCALL utm__setvect(VECTOR *vectors)
{ ++ut_setvect_calls; UT_ASSERT_EQ_PTR(vectab2, vectors); }
int ODSWAPCALL utm__getrc(void) { ++ut_getrc_calls; return(ut_child_result); }
int ODSWAPCALL utm__restmap(char *map)
{ ++ut_restmap_calls; UT_ASSERT_EQ_PTR(ut_map_memory, map); return(ut_restmap_result); }

static void reset_doxspawn(void)
{
   _swap = 1; _useems = 0; _required = 0; ems = 2; mapsize = 16;
   swapsize = 16384; ut_total_size = 100000;
   ut_cmdenv_result = 16; ut_map_malloc_failure = FALSE;
   ut_chkems_result = 1; ut_xsize_result = 0;
   ut_savemap_result = ut_getems_result = ut_tempfile_result = 0;
   ut_xspawn_result = 0; ut_child_result = 7; ut_restmap_result = 0;
   ut_xspawn_file = NULL;
   ut_malloc_calls = ut_free_environment_calls = ut_free_map_calls = 0;
   ut_chkems_calls = ut_tempfile_calls = ut_savevect_calls = 0;
   ut_xspawn_calls = ut_setvect_calls = ut_getrc_calls = 0;
   ut_savemap_calls = ut_getems_calls = ut_restmap_calls = 0;
   errno = 0;
}

static int call_doxspawn(void)
{
   return(utt_doxspawn("PROGRAM.EXE", ut_arguments, ut_environment_vector));
}

static void reports_environment_and_map_allocation_failures(void)
{
   reset_doxspawn(); ut_cmdenv_result = -1;
   UT_ASSERT_EQ_INT(-1, call_doxspawn());
   UT_ASSERT_EQ_UINT(0, ut_free_environment_calls);

   reset_doxspawn(); _swap = 0; _useems = 0; ut_chkems_result = 0;
   ut_map_malloc_failure = TRUE;
   UT_ASSERT_EQ_INT(-1, call_doxspawn()); UT_ASSERT_EQ_INT(ENOMEM, errno);
   UT_ASSERT_EQ_UINT(1, ut_free_environment_calls);
}

static void executes_without_swapping_when_disabled_or_unnecessary(void)
{
   reset_doxspawn();
   ut_xspawn_file = "";
   UT_ASSERT_EQ_INT(7, call_doxspawn());
   UT_ASSERT_EQ_UINT(1, ut_xspawn_calls); UT_ASSERT_EQ_UINT(1, ut_getrc_calls);
   UT_ASSERT_EQ_UINT(0, ut_restmap_calls); UT_ASSERT_EQ_UINT(0, ut_savemap_calls);

   reset_doxspawn(); _swap = 0; ems = 1; _required = 1;
   ut_total_size = swapsize + 272 + 2048;
   UT_ASSERT_EQ_INT(7, call_doxspawn());
   UT_ASSERT_EQ_UINT(0, ut_tempfile_calls); UT_ASSERT_EQ_UINT(0, ut_savemap_calls);

   reset_doxspawn(); _swap = 0; ems = 1; _required = 1;
   ut_total_size = swapsize + 272;
   UT_ASSERT_EQ_INT(7, call_doxspawn());
   UT_ASSERT_EQ_UINT(1, ut_tempfile_calls); UT_ASSERT_EQ_UINT(0, ut_savemap_calls);
}

static void selects_ems_and_rounds_the_page_count(void)
{
   reset_doxspawn(); _swap = 0; ut_chkems_result = 0; swapsize = 16384;
   UT_ASSERT_EQ_INT(7, call_doxspawn());
   UT_ASSERT_EQ_UINT(1, ut_savemap_calls); UT_ASSERT_EQ_UINT(1, ut_getems_calls);
   UT_ASSERT_EQ_UINT(1, ut_restmap_calls); UT_ASSERT_EQ_UINT(1, ut_free_map_calls);

   reset_doxspawn(); _swap = 0; ut_chkems_result = 0; swapsize = 16385;
   UT_ASSERT_EQ_INT(7, call_doxspawn());
   UT_ASSERT_EQ_UINT(1, ut_getems_calls);

   reset_doxspawn(); _swap = 0; ut_chkems_result = 0;
   ut_savemap_result = 1;
   UT_ASSERT_EQ_INT(7, call_doxspawn());
   UT_ASSERT_EQ_UINT(0, ut_getems_calls); UT_ASSERT_EQ_UINT(1, ut_tempfile_calls);

   reset_doxspawn(); _swap = 0; ut_chkems_result = 0;
   ut_getems_result = 1;
   UT_ASSERT_EQ_INT(7, call_doxspawn());
   UT_ASSERT_EQ_UINT(1, ut_getems_calls); UT_ASSERT_EQ_UINT(1, ut_tempfile_calls);

   reset_doxspawn(); _swap = 0; ut_chkems_result = 0;
   ut_getems_result = 1; ut_tempfile_result = 1;
   UT_ASSERT_EQ_INT(-1, call_doxspawn());
   UT_ASSERT_EQ_UINT(1, ut_tempfile_calls); UT_ASSERT_EQ_UINT(0, ut_xspawn_calls);
}

static void handles_swap_file_and_size_failures(void)
{
   reset_doxspawn(); _swap = 0; _useems = 1; ems = 0;
   ut_tempfile_result = 1;
   UT_ASSERT_EQ_INT(-1, call_doxspawn()); UT_ASSERT_EQ_UINT(0, ut_xspawn_calls);

   reset_doxspawn(); _swap = 0; ems = 1; ut_tempfile_result = 1;
   UT_ASSERT_EQ_INT(-1, call_doxspawn());

   reset_doxspawn(); _swap = 0; ut_xsize_result = 5;
   UT_ASSERT_EQ_INT(-1, call_doxspawn()); UT_ASSERT_EQ_INT(EACCES, errno);
}

static void translates_spawn_and_ems_restore_results(void)
{
   reset_doxspawn(); ut_xspawn_result = 5;
   UT_ASSERT_EQ_INT(-1, call_doxspawn()); UT_ASSERT_EQ_INT(EACCES, errno);
   UT_ASSERT_EQ_UINT(0, ut_getrc_calls); UT_ASSERT_EQ_UINT(1, ut_setvect_calls);

   reset_doxspawn(); _swap = 0; ut_chkems_result = 0;
   ut_restmap_result = 1;
   UT_ASSERT_EQ_INT(-1, call_doxspawn()); UT_ASSERT_EQ_INT(EACCES, errno);

   reset_doxspawn(); _swap = 0; ut_chkems_result = 0;
   ut_savemap_result = 1; ut_xspawn_file = "C:\\SWAP.$$$";
   UT_ASSERT_EQ_INT(7, call_doxspawn());
   UT_ASSERT_EQ_UINT(0, ut_restmap_calls);
}

static const UTTestCase ut_cases[] = {
   {"setup failures", reports_environment_and_map_allocation_failures},
   {"no swap", executes_without_swapping_when_disabled_or_unnecessary},
   {"EMS", selects_ems_and_rounds_the_page_count},
   {"swap failures", handles_swap_file_and_size_failures},
   {"result handling", translates_spawn_and_ems_restore_results}
};
#endif
