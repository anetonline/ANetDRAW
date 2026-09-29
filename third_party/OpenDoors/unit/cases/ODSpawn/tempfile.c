#ifdef ODPLAT_DOS
#define UT_CUSTOM_MOCK___get_errno_ptr
#define __get_errno_ptr utm___get_errno_ptr
#define UT_CUSTOM_MOCK__getdrv
#define UT_CUSTOM_MOCK_testfile
#define UT_CUSTOM_MOCK_memmove

static int ut_errno_value;
static int ut_test_results[8];
static unsigned ut_test_calls;
static char ut_test_paths[8][79];

int *utm___get_errno_ptr(void) { return(&ut_errno_value); }
int ODSWAPCALL utm__getdrv(void) { return(2); }

static int utm_testfile(char *end, char *file, int *handle)
{
   unsigned call = ut_test_calls++;
   size_t length = (size_t)(end - file);
   UT_ASSERT(length < sizeof(ut_test_paths[call]));
   memcpy(ut_test_paths[call], file, length);
   ut_test_paths[call][length] = '\0';
   if(ut_test_results[call] == 0) *handle = 31;
   return(ut_test_results[call]);
}

void *utm_memmove(void *destination, const void *source, size_t size)
{
   unsigned char *out = destination;
   const unsigned char *in = source;
   size_t index;
   if(out < in)
      for(index = 0; index < size; ++index) out[index] = in[index];
   else
      for(index = size; index != 0; --index) out[index-1] = in[index-1];
   return(destination);
}

static void reset_tempfile(void)
{
   unsigned index;
   for(index = 0; index < 8; ++index)
   {
      ut_test_results[index] = 1;
      ut_test_paths[index][0] = '\0';
   }
   ut_test_calls = 0; errno = 0;
}

static void accepts_absolute_and_drive_relative_paths(void)
{
   char file[79]; int handle;
   reset_tempfile(); _swappath = "C:\\SWAP"; ut_test_results[0] = 0;
   UT_ASSERT_EQ_INT(0, utt_tempfile(file, &handle));
   UT_ASSERT(strcmp("C:\\SWAP", ut_test_paths[0]) == 0);
   UT_ASSERT_EQ_INT(31, handle);

   reset_tempfile(); _swappath = "C:"; ut_test_results[0] = 0;
   UT_ASSERT_EQ_INT(0, utt_tempfile(file, &handle));
   UT_ASSERT(strcmp("C:", ut_test_paths[0]) == 0);
}

static void supplies_the_current_drive_for_relative_paths(void)
{
   char file[79]; int handle;
   reset_tempfile(); memset(file, 0, sizeof(file)); file[1] = ':';
   _swappath = "X"; ut_test_results[0] = 0;
   UT_ASSERT_EQ_INT(0, utt_tempfile(file, &handle));
   UT_ASSERT(strcmp("c:X", ut_test_paths[0]) == 0);

   reset_tempfile(); _swappath = "AB"; ut_test_results[0] = 0;
   UT_ASSERT_EQ_INT(0, utt_tempfile(file, &handle));
   UT_ASSERT(strcmp("c:AB", ut_test_paths[0]) == 0);

   reset_tempfile(); _swappath = "DIR"; ut_test_results[0] = 0;
   UT_ASSERT_EQ_INT(0, utt_tempfile(file, &handle));
   UT_ASSERT(strcmp("c:DIR", ut_test_paths[0]) == 0);
}

static void searches_each_nonempty_configured_path(void)
{
   char file[79]; int handle;
   reset_tempfile(); _swappath = ";ONE;;D:\\TWO;";
   ut_test_results[0] = 1; ut_test_results[1] = 0;
   UT_ASSERT_EQ_INT(0, utt_tempfile(file, &handle));
   UT_ASSERT_EQ_UINT(2, ut_test_calls);
   UT_ASSERT(strcmp("c:ONE", ut_test_paths[0]) == 0);
   UT_ASSERT(strcmp("D:\\TWO", ut_test_paths[1]) == 0);

   reset_tempfile(); _swappath = ";ONE;;";
   UT_ASSERT_EQ_INT(1, utt_tempfile(file, &handle));
   UT_ASSERT_EQ_UINT(1, ut_test_calls);
   UT_ASSERT_EQ_INT(EACCES, errno);
}

static void uses_the_current_directory_without_a_swap_path(void)
{
   char file[79]; int handle;
   reset_tempfile(); _swappath = NULL; ut_test_results[0] = 0;
   UT_ASSERT_EQ_INT(0, utt_tempfile(file, &handle));
   UT_ASSERT(strcmp("c:", ut_test_paths[0]) == 0);

   reset_tempfile(); _swappath = NULL;
   UT_ASSERT_EQ_INT(1, utt_tempfile(file, &handle));
   UT_ASSERT_EQ_INT(EACCES, errno);
}

static const UTTestCase ut_cases[] = {
   {"absolute paths", accepts_absolute_and_drive_relative_paths},
   {"relative paths", supplies_the_current_drive_for_relative_paths},
   {"path list", searches_each_nonempty_configured_path},
   {"current directory", uses_the_current_directory_without_a_swap_path}
};
#endif
