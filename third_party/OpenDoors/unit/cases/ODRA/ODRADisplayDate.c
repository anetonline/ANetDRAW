#define UT_CUSTOM_MOCK_ODScrnDisplayChar
#define UT_CUSTOM_MOCK_ODScrnDisplayString
#define UT_CUSTOM_MOCK_atoi
#define UT_CUSTOM_MOCK_strlen

static char ut_output[32];
static unsigned ut_output_length;

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

int utm_atoi(const char *text)
{
   int value = 0;
   while(*text >= '0' && *text <= '9')
   {
      value = value * 10 + (*text - '0');
      ++text;
   }
   return value;
}

void ODCALL utm_ODScrnDisplayChar(unsigned char character)
{
   UT_ASSERT(ut_output_length + 1 < sizeof(ut_output));
   ut_output[ut_output_length++] = (char)character;
   ut_output[ut_output_length] = '\0';
}

void ODCALL utm_ODScrnDisplayString(const char *text)
{
   while(*text != '\0') utm_ODScrnDisplayChar((unsigned char)*text++);
}

static void reset_output(void)
{
   ut_output_length = 0;
   ut_output[0] = '\0';
   od_control.od_month[0] = "Jan";
   od_control.od_month[11] = "Dec";
}

static void rejects_missing_or_malformed_dates(void)
{
   reset_output();
   utt_ODRADisplayDate(NULL);
   utt_ODRADisplayDate("1-01-80");
   utt_ODRADisplayDate("00-01-80");
   utt_ODRADisplayDate("13-01-80");
   utt_ODRADisplayDate("01-00-80");
   utt_ODRADisplayDate("01-32-80");
   utt_ODRADisplayDate("01-01-/0");
   utt_ODRADisplayDate("01-01-:0");
   utt_ODRADisplayDate("01-01-0/");
   utt_ODRADisplayDate("01-01-0:");
   UT_ASSERT_EQ_UINT(0, ut_output_length);
}

static void displays_valid_dates_using_the_configured_month(void)
{
   reset_output();
   utt_ODRADisplayDate("01-02-34");
   UT_ASSERT(strcmp("02-Jan-34", ut_output) == 0);

   reset_output();
   utt_ODRADisplayDate("12-31-99");
   UT_ASSERT(strcmp("31-Dec-99", ut_output) == 0);
}

static const UTTestCase ut_cases[] = {
   {"invalid dates", rejects_missing_or_malformed_dates},
   {"valid dates", displays_valid_dates_using_the_configured_month}
};
