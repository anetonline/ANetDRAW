#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_ODSessionScreenDisplayBuffer

static unsigned ut_length_calls;
static unsigned ut_display_calls;
static const char *ut_text;
static INT ut_length;

size_t utm_strlen(const char *text)
{
   ++ut_length_calls;
   UT_ASSERT_EQ_PTR(ut_text, text);
   return (size_t)ut_length;
}

void utm_ODSessionScreenDisplayBuffer(const char *text, INT length)
{
   ++ut_display_calls;
   UT_ASSERT_EQ_PTR(ut_text, text);
   UT_ASSERT_EQ_INT(ut_length, length);
}

static void ignores_null_and_forwards_string_length(void)
{
   ut_length_calls = ut_display_calls = 0;
   utt_ODSessionScreenDisplayString(NULL);
   UT_ASSERT_EQ_UINT(0, ut_length_calls);
   UT_ASSERT_EQ_UINT(0, ut_display_calls);
   ut_text = "screen";
   ut_length = 6;
   utt_ODSessionScreenDisplayString(ut_text);
   UT_ASSERT_EQ_UINT(1, ut_length_calls);
   UT_ASSERT_EQ_UINT(1, ut_display_calls);
}

static const UTTestCase ut_cases[] = {
   {"display string", ignores_null_and_forwards_string_length}
};
