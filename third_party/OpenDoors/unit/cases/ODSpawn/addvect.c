#if defined(ODPLAT_DOS)
#define UT_CUSTOM_MOCK___get_errno_ptr
#define __get_errno_ptr utm___get_errno_ptr
#define UT_CUSTOM_MOCK__getvect

static int ut_errno_value;
static unsigned ut_getvect_calls;

int *utm___get_errno_ptr(void)
{
   return(&ut_errno_value);
}

void ODSWAPCALL utm__getvect(int number, unsigned int *segment,
   unsigned int *offset)
{
   ++ut_getvect_calls;
   *segment = (unsigned int)(0x1000 + number);
   *offset = (unsigned int)(0x2000 + number);
}

static void rejects_invalid_arguments_independently(void)
{
   errno = 0;
   UT_ASSERT_EQ_INT(-1, utt_addvect(-1, CURRENT));
   UT_ASSERT_EQ_INT(EINVAL, errno);
   errno = 0;
   UT_ASSERT_EQ_INT(-1, utt_addvect(256, CURRENT));
   UT_ASSERT_EQ_INT(EINVAL, errno);
   errno = 0;
   UT_ASSERT_EQ_INT(-1, utt_addvect(10, 2));
   UT_ASSERT_EQ_INT(EINVAL, errno);
   errno = 0;
   UT_ASSERT_EQ_INT(-1, utt_addvect(10, -1));
   UT_ASSERT_EQ_INT(EINVAL, errno);
   UT_ASSERT_EQ_UINT(0, ut_getvect_calls);
}

static void replaces_an_existing_vector_record(void)
{
   ut_getvect_calls = 0;
   UT_ASSERT_EQ_INT(0, utt_addvect(0, IRET));
   UT_ASSERT_EQ_INT(IRET, vectab1[0].flag);
   UT_ASSERT_EQ_UINT(0, ut_getvect_calls);

   UT_ASSERT_EQ_INT(0, utt_addvect(0, CURRENT));
   UT_ASSERT_EQ_INT(CURRENT, vectab1[0].flag);
   UT_ASSERT_EQ_UINT(1, ut_getvect_calls);
   UT_ASSERT_EQ_UINT(0x1000, vectab1[0].vseg);
   UT_ASSERT_EQ_UINT(0x2000, vectab1[0].voff);
}

static void uses_free_records_then_reports_a_full_table(void)
{
   int number;
   ut_getvect_calls = 0;
   for(number = 10; number < 24; ++number)
      UT_ASSERT_EQ_INT(0, utt_addvect(number, CURRENT));
   UT_ASSERT_EQ_UINT(14, ut_getvect_calls);
   errno = 0;
   UT_ASSERT_EQ_INT(-1, utt_addvect(24, IRET));
   UT_ASSERT_EQ_INT(ENOMEM, errno);
}

static const UTTestCase ut_cases[] = {
   {"invalid arguments", rejects_invalid_arguments_independently},
   {"existing vector", replaces_an_existing_vector_record},
   {"free/full table", uses_free_records_then_reports_a_full_table}
};
#endif
