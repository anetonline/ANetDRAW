#define UT_CUSTOM_MOCK_free

static tPortInfo ut_port;
static unsigned ut_free_calls;

void utm_free(void *memory)
{
   ++ut_free_calls;
   UT_ASSERT_EQ_PTR(&ut_port, memory);
}

static void releases_a_closed_port(void)
{
   memset(&ut_port, 0, sizeof(ut_port));
   ut_free_calls = 0;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODComFree(ODPTR2HANDLE(&ut_port, tPortInfo)));
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
}

static const UTTestCase ut_cases[] = {
   {"closed port", releases_a_closed_port}
};
