#define UT_CUSTOM_MOCK_ODFileSize
#define UT_CUSTOM_MOCK_ODMakeFilename
#define UT_CUSTOM_MOCK_ODReadExitInfoPrimitive
#define UT_CUSTOM_MOCK_ODExitInfoExtendedEndian
#define UT_CUSTOM_MOCK_ODExitInfoRA2Endian
#define UT_CUSTOM_MOCK_ODStringPascalToC
#define UT_CUSTOM_MOCK_fclose
#define UT_CUSTOM_MOCK_fopen
#define UT_CUSTOM_MOCK_fread
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_memcpy

static int ut_binary_file_token;
static FILE *ut_binary_file = (FILE *)&ut_binary_file_token;
static tRA2ExitInfoRecord ut_ra2_record;
static tExtendedExitInfo ut_extended_record;
static tExitInfoRecord ut_primitive_record;
static DWORD ut_file_size;
static BOOL ut_open_fails;
static BOOL ut_malloc_fails;
static BOOL ut_read_succeeds;
static BOOL ut_primitive_succeeds;
static unsigned ut_close_count;
static unsigned ut_free_count;
static unsigned ut_pascal_count;
static INT ut_primitive_count;
static unsigned ut_ra2_endian_count;
static unsigned ut_extended_endian_count;

void utm_ODExitInfoRA2Endian(tRA2ExitInfoRecord *record,
   BOOL from_little_endian)
{
   UT_ASSERT_EQ_PTR(&ut_ra2_record, record);
   UT_ASSERT_EQ_INT(TRUE, from_little_endian);
   ++ut_ra2_endian_count;
}

void utm_ODExitInfoExtendedEndian(tExtendedExitInfo *record,
   BOOL from_little_endian)
{
   UT_ASSERT_EQ_PTR(&ut_extended_record, record);
   UT_ASSERT_EQ_INT(TRUE, from_little_endian);
   ++ut_extended_endian_count;
}

static void ut_pascal(char *destination, const char *text)
{
   unsigned length = 0;
   unsigned index;
   while(text[length] != '\0')
      ++length;
   destination[0] = (char)length;
   for(index = 0; index < length; ++index)
      destination[index + 1] = text[index];
}

tODResult utm_ODMakeFilename(char *destination, const char *directory,
   const char *name, INT size)
{
   unsigned index = 0;
   (void)directory;
   UT_ASSERT(strcmp("exitinfo.bbs", name) == 0);
   UT_ASSERT_EQ_UINT(sizeof(szExitinfoBBSPath), size);
   while(index + 1 < (unsigned)size && name[index] != '\0')
   {
      destination[index] = name[index];
      ++index;
   }
   destination[index] = '\0';
   return kODRCSuccess;
}

FILE *utm_fopen(const char *path, const char *mode)
{
   UT_ASSERT(strcmp("exitinfo.bbs", path) == 0);
   UT_ASSERT(strcmp("rb", mode) == 0);
   return ut_open_fails ? NULL : ut_binary_file;
}

DWORD utm_ODFileSize(FILE *stream)
{
   UT_ASSERT_EQ_PTR(ut_binary_file, stream);
   return ut_file_size;
}

void *utm_malloc(size_t size)
{
   if(ut_malloc_fails)
      return NULL;
   if(size == sizeof(tRA2ExitInfoRecord))
      return &ut_ra2_record;
   UT_ASSERT_EQ_UINT(sizeof(tExtendedExitInfo), size);
   return &ut_extended_record;
}

void utm_free(void *memory)
{
   UT_ASSERT(memory == &ut_ra2_record || memory == &ut_extended_record);
   ++ut_free_count;
}

size_t utm_fread(void *destination, size_t size, size_t count, FILE *stream)
{
   UT_ASSERT_EQ_PTR(ut_binary_file, stream);
   UT_ASSERT_EQ_UINT(1, size);
   if(destination == &ut_ra2_record)
      UT_ASSERT_EQ_UINT(2363, count);
   else
   {
      UT_ASSERT_EQ_PTR(&ut_extended_record, destination);
      UT_ASSERT_EQ_UINT(sizeof(tExtendedExitInfo), count);
   }
   return ut_read_succeeds ? count : count - 1;
}

BOOL utm_ODReadExitInfoPrimitive(FILE *stream, INT count)
{
   UT_ASSERT_EQ_PTR(ut_binary_file, stream);
   ut_primitive_count = count;
   pExitInfoRecord = &ut_primitive_record;
   return ut_primitive_succeeds;
}

char *utm_ODStringPascalToC(char *destination, char *source, BYTE maximum)
{
   unsigned length = (unsigned char)source[0];
   unsigned index;
   ++ut_pascal_count;
   if(length > (unsigned)maximum)
      length = (unsigned)maximum;
   for(index = 0; index < length; ++index)
      destination[index] = source[index + 1];
   destination[length] = '\0';
   return destination;
}

void *utm_memcpy(void *destination, const void *source, size_t size)
{
   unsigned char *to = (unsigned char *)destination;
   const unsigned char *from = (const unsigned char *)source;
   size_t index;
   for(index = 0; index < size; ++index)
      to[index] = from[index];
   return destination;
}

int utm_fclose(FILE *stream)
{
   UT_ASSERT_EQ_PTR(ut_binary_file, stream);
   ++ut_close_count;
   return 0;
}

static void reset_exitinfo_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
   memset(&ut_ra2_record, 0, sizeof(ut_ra2_record));
   memset(&ut_extended_record, 0, sizeof(ut_extended_record));
   memset(&ut_primitive_record, 0, sizeof(ut_primitive_record));
   ut_file_size = 0;
   ut_open_fails = FALSE;
   ut_malloc_fails = FALSE;
   ut_read_succeeds = TRUE;
   ut_primitive_succeeds = TRUE;
   ut_close_count = 0;
   ut_free_count = 0;
   ut_pascal_count = 0;
   ut_primitive_count = -1;
   ut_ra2_endian_count = 0;
   ut_extended_endian_count = 0;
   pRA2ExitInfoRecord = NULL;
   pExtendedExitInfo = NULL;
   pExitInfoRecord = NULL;
   bRAStatus = FALSE;
   btRAStatusToSet = 0;
   nInitialElapsed = 0;
}

static void ignores_a_missing_file(void)
{
   reset_exitinfo_fixture();
   od_control.od_extended_info = TRUE;
   od_control.od_ra_info = TRUE;
   ut_open_fails = TRUE;
   utt_ODInitReadExitInfo();
   UT_ASSERT_EQ_INT(FALSE, od_control.od_extended_info);
   UT_ASSERT_EQ_INT(FALSE, od_control.od_ra_info);
   UT_ASSERT_EQ_UINT(0, ut_close_count);
}

static void closes_an_unrecognized_short_record(void)
{
   reset_exitinfo_fixture();
   ut_file_size = 451;
   od_control.user_attribute = 4;
   utt_ODInitReadExitInfo();
   UT_ASSERT_EQ_INT(FALSE, od_control.od_extended_info);
   UT_ASSERT_EQ_INT(4, od_control.od_page_pausing);
   UT_ASSERT_EQ_UINT(1, ut_close_count);
   UT_ASSERT_EQ_INT(-1, ut_primitive_count);
}

static void handles_ra2_allocation_and_read_failures(void)
{
   reset_exitinfo_fixture();
   ut_file_size = 2363;
   ut_malloc_fails = TRUE;
   utt_ODInitReadExitInfo();
   UT_ASSERT_NULL(pRA2ExitInfoRecord);
   UT_ASSERT_EQ_UINT(0, ut_free_count);
   UT_ASSERT_EQ_UINT(1, ut_close_count);

   reset_exitinfo_fixture();
   ut_file_size = 2363;
   ut_read_succeeds = FALSE;
   utt_ODInitReadExitInfo();
   UT_ASSERT_NULL(pRA2ExitInfoRecord);
   UT_ASSERT_EQ_UINT(1, ut_free_count);
   UT_ASSERT_EQ_INT(FALSE, od_control.od_extended_info);
}

static void fill_ra2_record(BYTE sex)
{
   ut_ra2_record.baud = 38400;
   ut_ra2_record.num_calls = 1234;
   ut_pascal(ut_ra2_record.name, "RA2 User");
   ut_pascal(ut_ra2_record.address[0], "Address 1");
   ut_pascal(ut_ra2_record.address[2], "Address 3");
   ut_ra2_record.attrib = 12;
   ut_ra2_record.attrib2 = 2;
   ut_ra2_record.sex = sex;
   ut_ra2_record.sec = 90;
   ut_ra2_record.wantchat = 1;
   ut_ra2_record.menustackpointer = 49;
   ut_pascal(ut_ra2_record.menustack[0], "FIRST");
   ut_pascal(ut_ra2_record.menustack[49], "LAST");
   ut_ra2_record.status_line = 3;
   ut_ra2_record.menu_cost_per_min = 17;
   ut_ra2_record.has_rip = 1;
   ut_ra2_record.btRIPVersion = 4;
}

static void imports_an_ra2_record_and_both_sex_values(void)
{
   reset_exitinfo_fixture();
   ut_file_size = 2363;
   fill_ra2_record(1);
   utt_ODInitReadExitInfo();
   UT_ASSERT_EQ_PTR(&ut_ra2_record, pRA2ExitInfoRecord);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_ra_info);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_extended_info);
   UT_ASSERT_EQ_INT(RA2EXITINFO, od_control.od_info_type);
   UT_ASSERT_EQ_INT(38400, od_control.baud);
   UT_ASSERT_EQ_INT(1234, od_control.system_calls);
   UT_ASSERT(strcmp("RA2 User", od_control.user_name) == 0);
   UT_ASSERT(strcmp("Address 1", od_control.user_address[0]) == 0);
   UT_ASSERT(strcmp("Address 3", od_control.user_address[2]) == 0);
   UT_ASSERT_EQ_INT(90, od_control.user_security);
   UT_ASSERT_EQ_INT('M', od_control.user_sex);
   UT_ASSERT(od_control.user_ansi != 0);
   UT_ASSERT(od_control.user_avatar != 0);
   UT_ASSERT(strcmp("FIRST", od_control.user_menustack[0]) == 0);
   UT_ASSERT(strcmp("LAST", od_control.user_menustack[49]) == 0);
   UT_ASSERT_EQ_INT(TRUE, bRAStatus);
   UT_ASSERT_EQ_INT(2, btRAStatusToSet);
   UT_ASSERT_EQ_INT(17, od_control.user_menu_cost);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_rip);
   UT_ASSERT_EQ_INT(4, od_control.user_rip_ver);
   UT_ASSERT_EQ_INT(4, od_control.od_page_pausing);
   UT_ASSERT_EQ_UINT(1, ut_close_count);
   UT_ASSERT_EQ_UINT(1, ut_ra2_endian_count);

   reset_exitinfo_fixture();
   ut_file_size = 2363;
   fill_ra2_record(2);
   utt_ODInitReadExitInfo();
   UT_ASSERT_EQ_INT('F', od_control.user_sex);
}

static void handles_ra1_primitive_and_allocation_failures(void)
{
   reset_exitinfo_fixture();
   ut_file_size = 1493;
   ut_primitive_succeeds = FALSE;
   utt_ODInitReadExitInfo();
   UT_ASSERT_EQ_INT(476, ut_primitive_count);
   UT_ASSERT_NULL(pExtendedExitInfo);

   reset_exitinfo_fixture();
   ut_file_size = 1493;
   ut_malloc_fails = TRUE;
   utt_ODInitReadExitInfo();
   UT_ASSERT_EQ_INT(476, ut_primitive_count);
   UT_ASSERT_NULL(pExtendedExitInfo);

   reset_exitinfo_fixture();
   ut_file_size = 1493;
   ut_read_succeeds = FALSE;
   utt_ODInitReadExitInfo();
   UT_ASSERT_NULL(pExtendedExitInfo);
   UT_ASSERT_EQ_UINT(1, ut_free_count);
}

static void imports_an_ra1_extended_record(void)
{
   reset_exitinfo_fixture();
   ut_file_size = 1493;
   ut_pascal(ut_primitive_record.bbs.ra.timeofcreation, "10:00");
   ut_pascal(ut_primitive_record.bbs.ra.logonpassword, "logon");
   ut_primitive_record.bbs.ra.wantchat = 1;
   ut_extended_record.deducted_time = 12;
   ut_pascal(ut_extended_record.menustack[0], "FIRST");
   ut_pascal(ut_extended_record.menustack[49], "LAST");
   ut_extended_record.menustackpointer = 49;
   ut_pascal(ut_extended_record.userhandle, "Handle");
   ut_extended_record.screenwidth = 100;
   ut_extended_record.msgarea = 7;
   ut_extended_record.filearea = 8;
   ut_extended_record.error_free = 1;
   utt_ODInitReadExitInfo();
   UT_ASSERT_EQ_PTR(&ut_extended_record, pExtendedExitInfo);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_ra_info);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_extended_info);
   UT_ASSERT_EQ_INT(RA1EXITINFO, od_control.od_info_type);
   UT_ASSERT(strcmp("10:00", od_control.user_timeofcreation) == 0);
   UT_ASSERT(strcmp("logon", od_control.user_logonpassword) == 0);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_wantchat);
   UT_ASSERT_EQ_INT(12, od_control.user_deducted_time);
   UT_ASSERT(strcmp("FIRST", od_control.user_menustack[0]) == 0);
   UT_ASSERT(strcmp("LAST", od_control.user_menustack[49]) == 0);
   UT_ASSERT(strcmp("Handle", od_control.user_handle) == 0);
   UT_ASSERT_EQ_INT(100, od_control.user_screenwidth);
   UT_ASSERT_EQ_INT(7, od_control.user_msg_area);
   UT_ASSERT_EQ_INT(8, od_control.user_file_area);
   UT_ASSERT(od_control.user_error_free != 0);
   UT_ASSERT_EQ_UINT(1, ut_extended_endian_count);
}

static void handles_qbbs_primitive_failure_and_size_cap(void)
{
   reset_exitinfo_fixture();
   ut_file_size = 477;
   ut_primitive_succeeds = FALSE;
   utt_ODInitReadExitInfo();
   UT_ASSERT_EQ_INT(477, ut_primitive_count);

   reset_exitinfo_fixture();
   ut_file_size = 1492;
   ut_primitive_succeeds = FALSE;
   utt_ODInitReadExitInfo();
   UT_ASSERT_EQ_UINT(sizeof(tExitInfoRecord), ut_primitive_count);
}

static void imports_a_qbbs_extended_record(void)
{
   reset_exitinfo_fixture();
   ut_file_size = 1492;
   ut_primitive_record.bbs.qbbs.qwantchat = 1;
   ut_primitive_record.bbs.qbbs.gosublevel = 2;
   ut_pascal(ut_primitive_record.bbs.qbbs.menustack[0], "ONE");
   ut_pascal(ut_primitive_record.bbs.qbbs.menustack[1], "TWO");
   ut_pascal(ut_primitive_record.bbs.qbbs.menu, "CURRENT");
   ut_primitive_record.elapsed = 33;
   ut_primitive_record.bbs.qbbs.ripactive = 1;
   utt_ODInitReadExitInfo();
   UT_ASSERT_EQ_INT(TRUE, od_control.user_wantchat);
   UT_ASSERT(strcmp("ONE", od_control.user_menustack[0]) == 0);
   UT_ASSERT(strcmp("TWO", od_control.user_menustack[1]) == 0);
   UT_ASSERT(strcmp("CURRENT", od_control.user_menustack[2]) == 0);
   UT_ASSERT_EQ_INT(2, od_control.user_menustackpointer);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_extended_info);
   UT_ASSERT_EQ_INT(QBBS275EXITINFO, od_control.od_info_type);
   UT_ASSERT_EQ_INT(33, nInitialElapsed);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_rip);
}

static void handles_base_ra_primitive_failure(void)
{
   reset_exitinfo_fixture();
   ut_file_size = 452;
   ut_primitive_succeeds = FALSE;
   utt_ODInitReadExitInfo();
   UT_ASSERT_EQ_INT(452, ut_primitive_count);
   UT_ASSERT_EQ_INT(FALSE, od_control.od_extended_info);
}

static void imports_a_base_ra_record(void)
{
   reset_exitinfo_fixture();
   ut_file_size = 476;
   ut_pascal(ut_primitive_record.bbs.ra.timeofcreation, "09:30");
   ut_pascal(ut_primitive_record.bbs.ra.logonpassword, "base-pass");
   ut_primitive_record.bbs.ra.wantchat = 1;
   utt_ODInitReadExitInfo();
   UT_ASSERT_EQ_INT(476, ut_primitive_count);
   UT_ASSERT(strcmp("09:30", od_control.user_timeofcreation) == 0);
   UT_ASSERT(strcmp("base-pass", od_control.user_logonpassword) == 0);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_wantchat);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_extended_info);
   UT_ASSERT_EQ_INT(EXITINFO, od_control.od_info_type);
}

static const UTTestCase ut_cases[] = {
   {"missing file", ignores_a_missing_file},
   {"short record", closes_an_unrecognized_short_record},
   {"RA2 failures", handles_ra2_allocation_and_read_failures},
   {"RA2 record", imports_an_ra2_record_and_both_sex_values},
   {"RA1 failures", handles_ra1_primitive_and_allocation_failures},
   {"RA1 record", imports_an_ra1_extended_record},
   {"QBBS sizing", handles_qbbs_primitive_failure_and_size_cap},
   {"QBBS record", imports_a_qbbs_extended_record},
   {"base RA failure", handles_base_ra_primitive_failure},
   {"base RA record", imports_a_base_ra_record}
};
