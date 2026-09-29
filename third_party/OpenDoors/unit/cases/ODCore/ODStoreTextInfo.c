#define UT_CUSTOM_MOCK_ODScrnGetTextInfo

static unsigned ut_get_calls;

void utm_ODScrnGetTextInfo(tODScrnTextInfo *info)
{
   UT_ASSERT_EQ_PTR(&ODTextInfo, info);
   ++ut_get_calls;
   info->curx = 17;
}

static void stores_text_information_in_the_shared_record(void)
{
   ODTextInfo.curx = 0;
   ut_get_calls = 0;
   utt_ODStoreTextInfo();
   UT_ASSERT_EQ_UINT(1, ut_get_calls);
   UT_ASSERT_EQ_UINT(17, ODTextInfo.curx);
}

static const UTTestCase ut_cases[] = {
   {"store text info", stores_text_information_in_the_shared_record}
};
