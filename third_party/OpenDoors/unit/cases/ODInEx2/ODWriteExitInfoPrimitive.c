#define UT_CUSTOM_MOCK_ODStringCToPascal
#define UT_CUSTOM_MOCK_ODGetElapsedMinutes
#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_time
#define UT_CUSTOM_MOCK_fwrite
#define UT_CUSTOM_MOCK_ODExitInfoPrimitiveEndian
#undef user_credit

static tExitInfoRecord ut_record;
static BOOL ut_elapsed_valid;
static DWORD ut_elapsed_minutes;
static size_t ut_write_result;
static unsigned ut_elapsed_calls;
static unsigned ut_write_calls;
static unsigned ut_endian_calls;

void *utm_memcpy(void *destination, const void *source, size_t size)
{
   BYTE *out = destination;
   const BYTE *in = source;
   size_t index;
   for(index = 0; index < size; ++index) out[index] = in[index];
   return(destination);
}

char *utm_ODStringCToPascal(char *destination, BYTE maximum, char *source)
{
   size_t length = strlen(source);
   if(length > maximum) length = maximum;
   destination[0] = (char)length;
   utm_memcpy(destination + 1, source, length);
   return(destination);
}

BOOL ODCALL utm_ODGetElapsedMinutes(DWORD *minutes, time_t start, time_t end)
{
   ++ut_elapsed_calls;
   UT_ASSERT_NOT_NULL(minutes);
   UT_ASSERT(start == (time_t)1000);
   UT_ASSERT(end == (time_t)5000);
   if(ut_elapsed_valid) *minutes = ut_elapsed_minutes;
   return(ut_elapsed_valid);
}

time_t utm_time(time_t *storage)
{
   UT_ASSERT_NULL(storage);
   return((time_t)5000);
}

size_t utm_fwrite(const void *buffer, size_t size, size_t count, FILE *stream)
{
   ++ut_write_calls;
   UT_ASSERT_EQ_PTR(&ut_record, buffer);
   UT_ASSERT_EQ_UINT(1, size);
   UT_ASSERT_EQ_UINT(sizeof(ut_record), count);
   UT_ASSERT_EQ_PTR((FILE *)&ut_record, stream);
   return(ut_write_result);
}

void utm_ODExitInfoPrimitiveEndian(tExitInfoRecord *record,
   BOOL from_little_endian)
{
   UT_ASSERT_EQ_PTR(&ut_record, record);
   UT_ASSERT_EQ_INT(ut_endian_calls != 0, from_little_endian);
   ++ut_endian_calls;
}

static BOOL pascal_equals(const char *field, const char *text)
{
   size_t length = strlen(text);
   return((BYTE)field[0] == length && memcmp(field + 1, text, length) == 0);
}

static void initialize_control(void)
{
   unsigned index;
   memset(&ut_record, 0, sizeof(ut_record));
   memset(&od_control, 0, sizeof(od_control));
   pExitInfoRecord = &ut_record;
   nStartupUnixTime = (time_t)1000;
   nInitialRemaining = 30;
   ut_record.timelimit = 50;
   od_control.system_calls = 1234;
   strcpy(od_control.system_last_caller, "Last Caller");
   strcpy(od_control.timelog_start_date, "01-02-03");
   for(index = 0; index < 24; ++index) od_control.timelog_busyperhour[index] = (INT16)(100 + index);
   for(index = 0; index < 7; ++index) od_control.timelog_busyperday[index] = (INT16)(200 + index);
   strcpy(od_control.user_name, "Test User");
   strcpy(od_control.user_location, "Detroit");
   strcpy(od_control.user_password, "secret");
   strcpy(od_control.user_dataphone, "555-0101");
   strcpy(od_control.user_homephone, "555-0102");
   strcpy(od_control.user_lasttime, "10:20");
   strcpy(od_control.user_lastdate, "06-07-08");
   od_control.user_attribute = 8;
   od_control.user_flags[0] = 31; od_control.user_flags[1] = 32;
   od_control.user_flags[2] = 33; od_control.user_flags[3] = 34;
   od_control.user_net_credit = 11; od_control.user_pending = 12;
   od_control.user_messages = 13; od_control.user_lastread = 14;
   od_control.user_security = 15; od_control.user_numcalls = 16;
   od_control.user_uploads = 17; od_control.user_downloads = 18;
   od_control.user_upk = 19; od_control.user_downk = 20;
   od_control.user_todayk = 21; od_control.user_group = 22;
   od_control.user_xi_record = 23; od_control.event_status = 24;
   od_control.user_time_used = 25; od_control.user_screen_length = 26;
   od_control.user_last_pwdchange = 27; od_control.user_attrib2 = 2;
   strcpy(od_control.event_starttime, "12:34");
   od_control.event_errorlevel = 28; od_control.event_days = 29;
   od_control.event_force = 30;
   strcpy(od_control.event_last_run, "09-10-11");
   od_control.user_netmailentered = 35; od_control.user_echomailentered = 36;
   strcpy(od_control.user_logintime, "11:22");
   strcpy(od_control.user_logindate, "03-04-05");
   od_control.user_loginsec = 37; od_control.user_credit = 38;
   od_control.user_num = 39; od_control.user_readthru = 40;
   od_control.user_numpages = 41; od_control.user_downlimit = 42;
   od_control.user_timelimit = 20;
   ut_elapsed_valid = FALSE; ut_elapsed_minutes = 0;
   ut_write_result = 0; ut_elapsed_calls = ut_write_calls = 0;
   ut_endian_calls = 0;
}

static void verify_fields(void)
{
   unsigned index;
   UT_ASSERT(ut_record.num_calls == 1234);
   UT_ASSERT(pascal_equals(ut_record.last_caller, "Last Caller"));
   UT_ASSERT(pascal_equals(ut_record.start_date, "01-02-03"));
   for(index = 0; index < 24; ++index) UT_ASSERT_EQ_INT(100 + index, ut_record.busyperhour[index]);
   for(index = 0; index < 7; ++index) UT_ASSERT_EQ_INT(200 + index, ut_record.busyperday[index]);
   UT_ASSERT(pascal_equals(ut_record.uname, "Test User"));
   UT_ASSERT(pascal_equals(ut_record.uloc, "Detroit"));
   UT_ASSERT(pascal_equals(ut_record.password, "secret"));
   UT_ASSERT(pascal_equals(ut_record.dataphone, "555-0101"));
   UT_ASSERT(pascal_equals(ut_record.homephone, "555-0102"));
   UT_ASSERT(pascal_equals(ut_record.lasttime, "10:20"));
   UT_ASSERT(pascal_equals(ut_record.lastdate, "06-07-08"));
   UT_ASSERT_EQ_INT(8, ut_record.attrib);
   UT_ASSERT_EQ_INT(31, ut_record.flags[0]); UT_ASSERT_EQ_INT(32, ut_record.flags[1]);
   UT_ASSERT_EQ_INT(33, ut_record.flags[2]); UT_ASSERT_EQ_INT(34, ut_record.flags[3]);
   UT_ASSERT_EQ_INT(11, ut_record.credit); UT_ASSERT_EQ_INT(12, ut_record.pending);
   UT_ASSERT_EQ_INT(13, ut_record.posted); UT_ASSERT_EQ_INT(14, ut_record.lastread);
   UT_ASSERT_EQ_INT(15, ut_record.sec); UT_ASSERT_EQ_INT(16, ut_record.nocalls);
   UT_ASSERT_EQ_INT(17, ut_record.ups); UT_ASSERT_EQ_INT(18, ut_record.downs);
   UT_ASSERT_EQ_INT(19, ut_record.upk); UT_ASSERT_EQ_INT(20, ut_record.downk);
   UT_ASSERT_EQ_INT(21, ut_record.todayk); UT_ASSERT_EQ_INT(25, ut_record.elapsed);
   UT_ASSERT_EQ_INT(26, ut_record.screenlen); UT_ASSERT_EQ_INT(27, ut_record.lastpwdchange);
   UT_ASSERT_EQ_INT(2, ut_record.attrib2); UT_ASSERT_EQ_INT(22, ut_record.group);
   UT_ASSERT_EQ_INT(23, ut_record.xirecord); UT_ASSERT_EQ_INT(24, ut_record.status);
   UT_ASSERT(pascal_equals(ut_record.starttime, "12:34"));
   UT_ASSERT_EQ_INT(28, ut_record.errorlevel); UT_ASSERT_EQ_INT(29, ut_record.days);
   UT_ASSERT_EQ_INT(30, ut_record.forced);
   UT_ASSERT(pascal_equals(ut_record.lasttimerun, "09-10-11"));
   UT_ASSERT_EQ_INT(35, ut_record.netmailentered);
   UT_ASSERT_EQ_INT(36, ut_record.echomailentered);
   UT_ASSERT(pascal_equals(ut_record.logintime, "11:22"));
   UT_ASSERT(pascal_equals(ut_record.logindate, "03-04-05"));
   UT_ASSERT(ut_record.loginsec == 37); UT_ASSERT(ut_record.net_credit == 38);
   UT_ASSERT_EQ_INT(39, ut_record.userrecord); UT_ASSERT_EQ_INT(40, ut_record.readthru);
   UT_ASSERT_EQ_INT(41, ut_record.numberpages); UT_ASSERT_EQ_INT(42, ut_record.downloadlimint);
}

static void preserves_time_when_elapsed_time_is_invalid(void)
{
   initialize_control();
   UT_ASSERT(!utt_ODWriteExitInfoPrimitive((FILE *)&ut_record, sizeof(ut_record)));
   UT_ASSERT_EQ_UINT(1, ut_elapsed_calls); UT_ASSERT_EQ_UINT(1, ut_write_calls);
   UT_ASSERT_EQ_UINT(2, ut_endian_calls);
   UT_ASSERT_EQ_INT(50, ut_record.timelimit);
   verify_fields();
}

static void applies_time_adjustment_and_reports_complete_write(void)
{
   initialize_control(); ut_elapsed_valid = TRUE; ut_elapsed_minutes = 4;
   ut_write_result = sizeof(ut_record);
   UT_ASSERT(utt_ODWriteExitInfoPrimitive((FILE *)&ut_record, sizeof(ut_record)));
   UT_ASSERT_EQ_UINT(1, ut_elapsed_calls); UT_ASSERT_EQ_UINT(1, ut_write_calls);
   UT_ASSERT_EQ_UINT(2, ut_endian_calls);
   UT_ASSERT_EQ_INT(44, ut_record.timelimit);
   verify_fields();
}

static const UTTestCase ut_cases[] = {
   {"invalid elapsed time and short write", preserves_time_when_elapsed_time_is_invalid},
   {"time adjustment and complete write", applies_time_adjustment_and_reports_complete_write}
};
