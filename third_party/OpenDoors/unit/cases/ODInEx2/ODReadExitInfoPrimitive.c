#define UT_CUSTOM_MOCK_calloc
#define UT_CUSTOM_MOCK_fread
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_ODExitInfoPrimitiveEndian
#define UT_CUSTOM_MOCK_ODStringPascalToC
#define UT_CUSTOM_MOCK_memcpy
#undef user_credit
static tExitInfoRecord ut_record;
static BOOL ut_malloc_fails;
static BOOL ut_read_fails;
static unsigned ut_free_calls;
static unsigned ut_endian_calls;
void *utm_memcpy(void *destination, const void *source, size_t size)
{
   BYTE *out = destination; const BYTE *in = source; size_t index;
   for(index = 0; index < size; ++index) out[index] = in[index];
   return(destination);
}
void *utm_calloc(size_t count, size_t size)
{
   UT_ASSERT_EQ_UINT(1, count);
   UT_ASSERT_EQ_UINT(sizeof(tExitInfoRecord), size);
   return(ut_malloc_fails ? NULL : &ut_record);
}
void utm_ODExitInfoPrimitiveEndian(tExitInfoRecord *record,
   BOOL from_little_endian)
{
   UT_ASSERT_EQ_PTR(&ut_record, record);
   UT_ASSERT_EQ_INT(TRUE, from_little_endian);
   ++ut_endian_calls;
}
size_t utm_fread(void *buffer, size_t size, size_t count, FILE *stream)
{
   UT_ASSERT_EQ_PTR(&ut_record, buffer); UT_ASSERT_EQ_UINT(1, size);
   UT_ASSERT_EQ_PTR((FILE *)&ut_record, stream);
   return(ut_read_fails ? count - 1 : count);
}
void utm_free(void *memory)
{
   UT_ASSERT_EQ_PTR(&ut_record, memory); ++ut_free_calls;
}
char *utm_ODStringPascalToC(char *destination, char *source, BYTE maximum)
{
   BYTE length = (BYTE)source[0];
   if(length > maximum) length = maximum;
   utm_memcpy(destination, source + 1, length); destination[length] = 0;
   return(destination);
}
static void set_pascal(char *field, const char *text)
{
   size_t length = strlen(text); field[0] = (char)length;
   utm_memcpy(field + 1, text, length);
}
static void reset_read(void)
{
   memset(&ut_record, 0, sizeof(ut_record)); memset(&od_control, 0, sizeof(od_control));
   pExitInfoRecord = NULL; ut_malloc_fails = ut_read_fails = FALSE;
   ut_free_calls = ut_endian_calls = 0;
}
static void reports_allocation_and_read_failures(void)
{
   reset_read(); ut_malloc_fails = TRUE;
   UT_ASSERT(!utt_ODReadExitInfoPrimitive((FILE *)&ut_record, sizeof(ut_record)));
   UT_ASSERT_NULL(pExitInfoRecord); UT_ASSERT_EQ_UINT(0, ut_free_calls);
   UT_ASSERT_EQ_UINT(0, ut_endian_calls);
   reset_read(); ut_read_fails = TRUE;
   UT_ASSERT(!utt_ODReadExitInfoPrimitive((FILE *)&ut_record, sizeof(ut_record)));
   UT_ASSERT_NULL(pExitInfoRecord); UT_ASSERT_EQ_UINT(1, ut_free_calls);
   UT_ASSERT_EQ_UINT(0, ut_endian_calls);
}
static void imports_exitinfo_fields(void)
{
   unsigned index;
   reset_read(); ut_record.baud = 57600; ut_record.num_calls = 1234;
   set_pascal(ut_record.last_caller, "Last Caller");
   set_pascal(ut_record.start_date, "01-02-03");
   set_pascal(ut_record.uname, "Test User"); set_pascal(ut_record.uloc, "Detroit");
   set_pascal(ut_record.password, "secret"); set_pascal(ut_record.starttime, "12:34");
   set_pascal(ut_record.logintime, "11:22"); set_pascal(ut_record.logindate, "03-04-05");
   set_pascal(ut_record.dataphone, "555-0101");
   set_pascal(ut_record.homephone, "555-0102");
   set_pascal(ut_record.lasttime, "10:20");
   set_pascal(ut_record.lastdate, "06-07-08");
   set_pascal(ut_record.lasttimerun, "09-10-11");
   for(index = 0; index < 24; ++index) ut_record.busyperhour[index] = (WORD)(100 + index);
   for(index = 0; index < 7; ++index) ut_record.busyperday[index] = (WORD)(200 + index);
   ut_record.attrib = 8; ut_record.flags[0] = 31; ut_record.flags[1] = 32;
   ut_record.flags[2] = 33; ut_record.flags[3] = 34; ut_record.attrib2 = 2;
   ut_record.credit = 11;
   ut_record.pending = 12; ut_record.posted = 13; ut_record.lastread = 14;
   ut_record.sec = 15; ut_record.nocalls = 16; ut_record.ups = 17;
   ut_record.downs = 18; ut_record.upk = 19; ut_record.downk = 20;
   ut_record.todayk = 21; ut_record.group = 22; ut_record.xirecord = 23;
   ut_record.status = 24; ut_record.elapsed = 25; ut_record.screenlen = 26;
   ut_record.lastpwdchange = 27; ut_record.errorlevel = 28; ut_record.days = 29;
   ut_record.forced = 30; ut_record.netmailentered = 35;
   ut_record.echomailentered = 36; ut_record.loginsec = 37;
   ut_record.net_credit = 38; ut_record.userrecord = 39;
   ut_record.readthru = 40; ut_record.numberpages = 41;
   ut_record.downloadlimint = 42;
   UT_ASSERT(utt_ODReadExitInfoPrimitive((FILE *)&ut_record, sizeof(ut_record)));
   UT_ASSERT_EQ_UINT(1, ut_endian_calls);
   UT_ASSERT(od_control.baud == 57600); UT_ASSERT(od_control.system_calls == 1234);
   UT_ASSERT(strcmp(od_control.system_last_caller, "Last Caller") == 0);
   UT_ASSERT(strcmp(od_control.timelog_start_date, "01-02-03") == 0);
   for(index = 0; index < 24; ++index)
      UT_ASSERT_EQ_INT(100 + index, od_control.timelog_busyperhour[index]);
   for(index = 0; index < 7; ++index)
      UT_ASSERT_EQ_INT(200 + index, od_control.timelog_busyperday[index]);
   UT_ASSERT(strcmp(od_control.user_name, "Test User") == 0);
   UT_ASSERT(strcmp(od_control.user_location, "Detroit") == 0);
   UT_ASSERT(strcmp(od_control.user_password, "secret") == 0);
   UT_ASSERT(strcmp(od_control.user_dataphone, "555-0101") == 0);
   UT_ASSERT(strcmp(od_control.user_homephone, "555-0102") == 0);
   UT_ASSERT(strcmp(od_control.user_lasttime, "10:20") == 0);
   UT_ASSERT(strcmp(od_control.user_lastdate, "06-07-08") == 0);
   UT_ASSERT_EQ_INT(8, od_control.user_attribute);
   UT_ASSERT_EQ_INT(31, od_control.user_flags[0]);
   UT_ASSERT_EQ_INT(32, od_control.user_flags[1]);
   UT_ASSERT_EQ_INT(33, od_control.user_flags[2]);
   UT_ASSERT_EQ_INT(34, od_control.user_flags[3]);
   UT_ASSERT(od_control.user_net_credit == 11);
   UT_ASSERT(od_control.user_pending == 12);
   UT_ASSERT(od_control.user_messages == 13);
   UT_ASSERT(od_control.user_lastread == 14);
   UT_ASSERT(od_control.user_security == 15);
   UT_ASSERT(od_control.user_numcalls == 16);
   UT_ASSERT(od_control.user_uploads == 17);
   UT_ASSERT(od_control.user_downloads == 18);
   UT_ASSERT(od_control.user_upk == 19);
   UT_ASSERT(od_control.user_downk == 20);
   UT_ASSERT(od_control.user_todayk == 21);
   UT_ASSERT_EQ_INT(25, od_control.user_time_used);
   UT_ASSERT_EQ_INT(26, od_control.user_screen_length);
   UT_ASSERT_EQ_INT(27, od_control.user_last_pwdchange);
   UT_ASSERT_EQ_INT(2, od_control.user_attrib2);
   UT_ASSERT(od_control.user_group == 22);
   UT_ASSERT(od_control.user_xi_record == 23);
   UT_ASSERT_EQ_INT(24, od_control.event_status);
   UT_ASSERT(strcmp(od_control.event_starttime, "12:34") == 0);
   UT_ASSERT_EQ_INT(28, od_control.event_errorlevel);
   UT_ASSERT_EQ_INT(29, od_control.event_days);
   UT_ASSERT_EQ_INT(30, od_control.event_force);
   UT_ASSERT(strcmp(od_control.event_last_run, "09-10-11") == 0);
   UT_ASSERT_EQ_INT(35, od_control.user_netmailentered);
   UT_ASSERT_EQ_INT(36, od_control.user_echomailentered);
   UT_ASSERT(strcmp(od_control.user_logintime, "11:22") == 0);
   UT_ASSERT(strcmp(od_control.user_logindate, "03-04-05") == 0);
   UT_ASSERT_EQ_INT(37, od_control.user_loginsec);
   UT_ASSERT_EQ_INT(38, od_control.user_credit);
   UT_ASSERT_EQ_INT(39, od_control.user_num);
   UT_ASSERT_EQ_INT(40, od_control.user_readthru);
   UT_ASSERT_EQ_INT(41, od_control.user_numpages);
   UT_ASSERT_EQ_INT(42, od_control.user_downlimit);
   UT_ASSERT(od_control.user_ansi != 0); UT_ASSERT(od_control.user_avatar != 0);
}
static const UTTestCase ut_cases[] = {
   {"failures", reports_allocation_and_read_failures},
   {"field import", imports_exitinfo_fields}
};
