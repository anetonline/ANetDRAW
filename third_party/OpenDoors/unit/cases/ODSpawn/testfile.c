#ifdef ODPLAT_DOS
#define UT_CUSTOM_MOCK___get_errno_ptr
#define __get_errno_ptr utm___get_errno_ptr
#define UT_CUSTOM_MOCK__getcd
#define UT_CUSTOM_MOCK_ODFileAccessMode
#define UT_CUSTOM_MOCK__dskspace
#define UT_CUSTOM_MOCK_ODDWordDivide
#define UT_CUSTOM_MOCK_ltoa
#define UT_CUSTOM_MOCK__create
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_strcpy

static int ut_errno_value;
static int ut_getcd_result;
static int ut_dskspace_result;
static unsigned ut_cluster_bytes;
static unsigned ut_free_clusters;
static DWORD ut_quotient;
static DWORD ut_remainder;
static int ut_access_results[8];
static int ut_create_results[4];
static unsigned ut_access_calls;
static unsigned ut_create_calls;
static unsigned ut_ltoa_calls;
static BOOL ut_force_number_cycle;
static unsigned ut_cycle_start;

char *utm_strcpy(char *destination, const char *source);

int *utm___get_errno_ptr(void) { return(&ut_errno_value); }

int ODSWAPCALL utm__getcd(int drive, char *directory)
{
   UT_ASSERT_EQ_INT(3, drive);
   if(ut_getcd_result == 0) utm_strcpy(directory, "DIR");
   return(ut_getcd_result);
}

BOOL utm_ODFileAccessMode(const char *path, int mode)
{
   (void)path; UT_ASSERT_EQ_INT(0, mode);
   return((BOOL)ut_access_results[ut_access_calls++]);
}

int ODSWAPCALL utm__dskspace(int drive, unsigned int *bytes,
   unsigned int *clusters)
{
   UT_ASSERT_EQ_INT(3, drive);
   *bytes = ut_cluster_bytes; *clusters = ut_free_clusters;
   return(ut_dskspace_result);
}

BOOL utm_ODDWordDivide(DWORD *quotient, DWORD *remainder,
   DWORD dividend, DWORD divisor)
{
   UT_ASSERT(swapsize == (long)dividend);
   UT_ASSERT((DWORD)ut_cluster_bytes == divisor);
   *quotient = ut_quotient; *remainder = ut_remainder;
   return(TRUE);
}

char *utm_ltoa(long value, char *text, int radix)
{
   unsigned long number = (unsigned long)value;
   char reversed[12]; unsigned count = 0; unsigned index;
   UT_ASSERT_EQ_INT(10, radix); ++ut_ltoa_calls;
   do { reversed[count++] = (char)('0' + number % 10UL); number /= 10UL; }
   while(number != 0UL);
   for(index = 0; index < count; ++index) text[index] = reversed[count-index-1];
   text[count] = '\0';
   if(ut_force_number_cycle && ut_ltoa_calls == 1)
      tempno = (unsigned int)(ut_cycle_start - 1U);
   return(text);
}

int ODSWAPCALL utm__create(char *path, int *handle)
{
   (void)path; *handle = 27;
   return(ut_create_results[ut_create_calls++]);
}

size_t utm_strlen(const char *text)
{
   size_t length = 0; while(text[length] != '\0') ++length; return(length);
}

char *utm_strcpy(char *destination, const char *source)
{
   char *result = destination;
   while((*destination++ = *source++) != '\0') { }
   return(result);
}

static void reset_testfile(void)
{
   unsigned index;
   ut_getcd_result = ut_dskspace_result = 0;
   ut_cluster_bytes = 1024; ut_free_clusters = 100;
   ut_quotient = 2; ut_remainder = 0; swapsize = 2048;
   for(index = 0; index < 8; ++index) ut_access_results[index] = 1;
   for(index = 0; index < 4; ++index) ut_create_results[index] = 0;
   ut_access_calls = ut_create_calls = ut_ltoa_calls = 0;
   ut_force_number_cycle = FALSE; ut_cycle_start = 0;
   tempno = 1; errno = 0;
}

static int call_testfile(char *file, int *handle)
{
   return(utt_testfile(file + utm_strlen(file), file, handle));
}

static void rejects_invalid_locations_and_insufficient_space(void)
{
   char file[79]; int handle;
   reset_testfile(); utm_strcpy(file, "C:"); ut_getcd_result = 1;
   UT_ASSERT_EQ_INT(1, call_testfile(file, &handle));

   reset_testfile(); utm_strcpy(file, "C:\\MISSING"); ut_access_results[0] = 1;
   UT_ASSERT_EQ_INT(1, call_testfile(file, &handle));

   reset_testfile(); utm_strcpy(file, "C:\\DIR"); ut_access_results[0] = 0;
   ut_dskspace_result = 1;
   UT_ASSERT_EQ_INT(1, call_testfile(file, &handle));

   reset_testfile(); utm_strcpy(file, "C:\\DIR"); ut_access_results[0] = 0;
   ut_free_clusters = 2;
   UT_ASSERT_EQ_INT(1, call_testfile(file, &handle));
}

static void forms_root_current_and_subdirectory_names(void)
{
   char file[79]; int handle;
   reset_testfile(); utm_strcpy(file, "C:");
   UT_ASSERT_EQ_INT(0, call_testfile(file, &handle));
   UT_ASSERT(strcmp(file, "C:\\DIR\\swp2") == 0);
   UT_ASSERT_EQ_INT(27, handle);

   reset_testfile(); utm_strcpy(file, "C:\\"); ut_remainder = 1;
   ut_access_results[0] = 0;
   UT_ASSERT_EQ_INT(0, call_testfile(file, &handle));
   UT_ASSERT(strcmp(file, "C:\\swp2") == 0);

   reset_testfile(); utm_strcpy(file, "C:/");
   ut_access_results[0] = 0;
   UT_ASSERT_EQ_INT(0, call_testfile(file, &handle));
   UT_ASSERT(strcmp(file, "C:/swp2") == 0);

   reset_testfile(); utm_strcpy(file, "C:\\DIR"); ut_access_results[0] = 0;
   UT_ASSERT_EQ_INT(0, call_testfile(file, &handle));
   UT_ASSERT(strcmp(file, "C:\\DIR\\swp2") == 0);
}

static void retries_name_collisions_and_create_races(void)
{
   char file[79]; int handle;
   reset_testfile(); utm_strcpy(file, "C:\\");
   ut_access_results[0] = 0; ut_access_results[1] = 0;
   ut_access_results[2] = 1;
   UT_ASSERT_EQ_INT(0, call_testfile(file, &handle));
   UT_ASSERT_EQ_UINT(2, ut_ltoa_calls);

   reset_testfile(); utm_strcpy(file, "C:\\");
   ut_access_results[0] = 0;
   ut_create_results[0] = 80; ut_create_results[1] = EACCES;
   UT_ASSERT_EQ_INT(EACCES, call_testfile(file, &handle));
   UT_ASSERT_EQ_UINT(2, ut_create_calls);

   reset_testfile(); utm_strcpy(file, "C:\\"); tempno = 0xffffU;
   ut_access_results[0] = 0;
   UT_ASSERT_EQ_INT(0, call_testfile(file, &handle));
   UT_ASSERT_EQ_UINT(1, tempno);
}

static void detects_exhaustion_of_the_temporary_number_space(void)
{
   char file[79]; int handle;
   reset_testfile(); utm_strcpy(file, "C:\\");
   ut_access_results[0] = 0; ut_access_results[1] = 0;
   ut_force_number_cycle = TRUE; ut_cycle_start = tempno;
   UT_ASSERT_EQ_INT(1, call_testfile(file, &handle));
   UT_ASSERT_EQ_UINT(1, ut_ltoa_calls);
   UT_ASSERT_EQ_UINT(0, ut_create_calls);
}

static const UTTestCase ut_cases[] = {
   {"location failures", rejects_invalid_locations_and_insufficient_space},
   {"path forms", forms_root_current_and_subdirectory_names},
   {"name retries", retries_name_collisions_and_create_races},
   {"number exhaustion", detects_exhaustion_of_the_temporary_number_space}
};
#endif
