#define UT_CUSTOM_MOCK_ODScrnDisplayChar

static char ut_output[9];
static unsigned ut_output_length;

void ODCALL utm_ODScrnDisplayChar(unsigned char character)
{
   UT_ASSERT(ut_output_length < 8);
   ut_output[ut_output_length++] = (char)character;
   ut_output[ut_output_length] = '\0';
}

static void expect_flags(BYTE flags, const char *expected)
{
   ut_output_length = 0;
   ut_output[0] = '\0';
   utt_ODRADisplayFlags(flags);
   UT_ASSERT_EQ_UINT(8, ut_output_length);
   UT_ASSERT(strcmp(expected, ut_output) == 0);
}

static void displays_each_bit_from_low_to_high(void)
{
   expect_flags(0x00, "--------");
   expect_flags(0xff, "XXXXXXXX");
   expect_flags(0x55, "X-X-X-X-");
   expect_flags(0x80, "-------X");
}

static const UTTestCase ut_cases[] = {
   {"flag bits", displays_each_bit_from_low_to_high}
};
