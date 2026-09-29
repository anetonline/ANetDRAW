#include <stdarg.h>

#define UT_CUSTOM_MOCK_sscanf
#define UT_CUSTOM_MOCK_toupper

#define MOCK_SCAN 302

static int ut_scan_result;
static WORD ut_scan_value;

static int ut_upper(int value)
{
   if(value >= 'a' && value <= 'z')
      return value - ('a' - 'A');
   return value;
}

int utm_toupper(int value)
{
   UT_ASSERT_EQ_INT((int)(unsigned char)value, value);
   return ut_upper(value);
}

int utm_sscanf(const char *text, const char *format, ...)
{
   va_list arguments;
   WORD *result;
   (void)text;
   (void)format;
   ut_mock_called(MOCK_SCAN);
   va_start(arguments, format);
   result = va_arg(arguments, WORD *);
   va_end(arguments);
   if(ut_scan_result == 1)
      *result = ut_scan_value;
   return ut_scan_result;
}

static void reset_fixture(void)
{
   ut_scan_result = 1;
   ut_scan_value = 0x12ab;
}

static void hexadecimal_word_finds_first_digit(void)
{
   char text[] = "/:Gx123";
   WORD value = 0;
   reset_fixture();
   UT_ASSERT_EQ_INT(TRUE, utt_ODCfgGetWordHex(text, &value));
   UT_ASSERT_EQ_UINT(0x12ab, value);
}

static void hexadecimal_word_finds_letters_regardless_of_case(void)
{
   char upper[] = "A";
   char lower[] = "f";
   WORD value = 0;
   reset_fixture();
   UT_ASSERT_EQ_INT(TRUE, utt_ODCfgGetWordHex(upper, &value));
   UT_ASSERT_EQ_INT(TRUE, utt_ODCfgGetWordHex(lower, &value));
}

static void hexadecimal_word_rejects_missing_number(void)
{
   char text[] = "/:Gx";
   WORD value = 99;
   reset_fixture();
   UT_ASSERT_EQ_INT(FALSE, utt_ODCfgGetWordHex(text, &value));
   UT_ASSERT_EQ_UINT(99, value);
}

static void hexadecimal_word_skips_high_bit_bytes(void)
{
   char text[] = {(char)0x80, '1', '\0'};
   WORD value = 0;
   reset_fixture();
   UT_ASSERT_EQ_INT(TRUE, utt_ODCfgGetWordHex(text, &value));
   UT_ASSERT_EQ_UINT(0x12ab, value);
}

static void hexadecimal_word_reports_conversion_failure(void)
{
   char text[] = "1";
   WORD value = 99;
   reset_fixture();
   ut_scan_result = 0;
   UT_ASSERT_EQ_INT(FALSE, utt_ODCfgGetWordHex(text, &value));
   UT_ASSERT_EQ_UINT(99, value);
}

static const UTTestCase ut_cases[] = {
   {"skip non-hexadecimal", hexadecimal_word_finds_first_digit},
   {"hexadecimal letters", hexadecimal_word_finds_letters_regardless_of_case},
   {"high-bit prefix", hexadecimal_word_skips_high_bit_bytes},
   {"no hexadecimal value", hexadecimal_word_rejects_missing_number},
   {"conversion failure", hexadecimal_word_reports_conversion_failure}
};
