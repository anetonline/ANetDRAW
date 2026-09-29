#ifdef __TURBOC__
#define UT_CUSTOM_MOCK_dostounix
long utm_dostounix(struct date *date_value, struct time *time_value)
{
   UT_ASSERT_NOT_NULL(date_value);
   UT_ASSERT_NOT_NULL(time_value);
   UT_ASSERT_EQ_INT(15, date_value->da_day);
   UT_ASSERT_EQ_INT(6, date_value->da_mon);
   UT_ASSERT_EQ_INT(2024, date_value->da_year);
   UT_ASSERT_EQ_INT(13, time_value->ti_hour);
   UT_ASSERT_EQ_INT(42, time_value->ti_min);
   UT_ASSERT_EQ_INT(26, time_value->ti_sec);
   UT_ASSERT_EQ_INT(0, time_value->ti_hund);
   return(12345L);
}
#else
#define UT_CUSTOM_MOCK_mktime
time_t utm_mktime(struct tm *time_value)
{
   UT_ASSERT_NOT_NULL(time_value);
   UT_ASSERT_EQ_INT(26, time_value->tm_sec);
   UT_ASSERT_EQ_INT(42, time_value->tm_min);
   UT_ASSERT_EQ_INT(13, time_value->tm_hour);
   UT_ASSERT_EQ_INT(15, time_value->tm_mday);
   UT_ASSERT_EQ_INT(5, time_value->tm_mon);
   UT_ASSERT_EQ_INT(124, time_value->tm_year);
   UT_ASSERT_EQ_INT(0, time_value->tm_wday);
   UT_ASSERT_EQ_INT(0, time_value->tm_yday);
   UT_ASSERT_EQ_INT(-1, time_value->tm_isdst);
   return((time_t)12345);
}
#endif

static void converts_dos_date_and_time_fields(void)
{
   WORD date_value = (WORD)((44U << 9) | (6U << 5) | 15U);
   WORD time_value = (WORD)((13U << 11) | (42U << 5) | 13U);
   UT_ASSERT_EQ_INT(12345, utt_DOSToCTime(date_value, time_value));
}

static const UTTestCase ut_cases[] = {
   {"DOS date and time", converts_dos_date_and_time_fields}
};
