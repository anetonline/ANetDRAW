#define UT_CUSTOM_MOCK_ODSizeAdd

static BOOL ut_add_succeeds;
static unsigned ut_add_calls;

int utm_ODSizeAdd(size_t left, size_t right, size_t *result)
{
   ++ut_add_calls;
   if(!ut_add_succeeds)
      return FALSE;
   *result = left + right;
   return TRUE;
}

static void reset_length(void)
{
   ut_add_succeeds = TRUE;
   ut_add_calls = 0;
}

static void rejects_each_invalid_argument_independently(void)
{
   BYTE byte = 'A';
   size_t length = 99;
   reset_length();
   UT_ASSERT_EQ_INT(FALSE, utt_ODComCP437ToUnicodeLen(NULL, 1, &length));
   UT_ASSERT_EQ_INT(FALSE, utt_ODComCP437ToUnicodeLen(&byte, 1, NULL));
   UT_ASSERT_EQ_INT(FALSE, utt_ODComCP437ToUnicodeLen(&byte, -1, &length));
   UT_ASSERT_EQ_UINT(0, ut_add_calls);
   UT_ASSERT_EQ_UINT(99, length);
}

static void counts_ascii_two_byte_and_three_byte_characters(void)
{
   BYTE input[] = {'A', 0x80, 0xb0};
   BYTE byte;
   size_t length = 99;
   reset_length();
   UT_ASSERT_EQ_INT(TRUE,
      utt_ODComCP437ToUnicodeLen(input, 0, &length));
   UT_ASSERT_EQ_UINT(0, length);
   UT_ASSERT_EQ_UINT(0, ut_add_calls);

   byte = 0x80;
   UT_ASSERT_EQ_INT(TRUE,
      utt_ODComCP437ToUnicodeLen(&byte, 1, &length));
   UT_ASSERT_EQ_UINT(2, length);

   byte = 0xb0;
   UT_ASSERT_EQ_INT(TRUE,
      utt_ODComCP437ToUnicodeLen(&byte, 1, &length));
   UT_ASSERT_EQ_UINT(3, length);

   length = 99;
   UT_ASSERT_EQ_INT(TRUE,
      utt_ODComCP437ToUnicodeLen(input, sizeof(input), &length));
   UT_ASSERT_EQ_UINT(6, length);
   UT_ASSERT_EQ_UINT(5, ut_add_calls);
}

static void propagates_checked_addition_failure(void)
{
   BYTE byte = 'A';
   size_t length = 99;
   reset_length();
   ut_add_succeeds = FALSE;
   UT_ASSERT_EQ_INT(FALSE,
      utt_ODComCP437ToUnicodeLen(&byte, 1, &length));
   UT_ASSERT_EQ_UINT(1, ut_add_calls);
   UT_ASSERT_EQ_UINT(99, length);
}

static const UTTestCase ut_cases[] = {
   {"invalid arguments", rejects_each_invalid_argument_independently},
   {"encoded lengths", counts_ascii_two_byte_and_three_byte_characters},
   {"overflow", propagates_checked_addition_failure}
};
