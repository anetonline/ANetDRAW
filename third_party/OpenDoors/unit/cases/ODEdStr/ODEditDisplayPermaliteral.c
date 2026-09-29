#define UT_CUSTOM_MOCK_od_putch
#define UT_CUSTOM_MOCK_od_repeat
#define UT_CUSTOM_MOCK_strlen

static char ut_calls[32];
static BYTE ut_values[32];
static unsigned ut_call_count;

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

void ODCALL utm_od_putch(char value)
{
   ut_calls[ut_call_count] = 'P';
   ut_values[ut_call_count++] = (BYTE)value;
}

void ODCALL utm_od_repeat(char value, BYTE count)
{
   UT_ASSERT_EQ_INT(chCurrentBlank, value);
   ut_calls[ut_call_count] = 'R';
   ut_values[ut_call_count++] = count;
}

static void reset_display(const char *input, char *format, INT length)
{
   INT index;
   pszCurrentInput = (char *)input;
   pszCurrentFormat = format;
   nCurrentStringLength = (unsigned char)length;
   chCurrentBlank = '_';
   ut_call_count = 0;
   for(index = 0; index < length; ++index)
   {
      anCurrentFormatOffset[index] = index;
      abCurrentFormatLiteral[index] = FALSE;
   }
}

static void batches_blanks_around_literals_and_size_cell(void)
{
   char format[] = "A-B!";
   reset_display("", format, 4);
   abCurrentFormatLiteral[1] = TRUE;
   abCurrentFormatLiteral[3] = TRUE;
   utt_ODEditDisplayPermaliteral(0);
   UT_ASSERT_EQ_INT(5, ut_call_count);
   UT_ASSERT_EQ_INT('R', ut_calls[0]);
   UT_ASSERT_EQ_INT(1, ut_values[0]);
   UT_ASSERT_EQ_INT('P', ut_calls[1]);
   UT_ASSERT_EQ_INT('-', ut_values[1]);
   UT_ASSERT_EQ_INT('R', ut_calls[2]);
   UT_ASSERT_EQ_INT(1, ut_values[2]);
   UT_ASSERT_EQ_INT('P', ut_calls[3]);
   UT_ASSERT_EQ_INT('!', ut_values[3]);
   UT_ASSERT_EQ_INT('R', ut_calls[4]);
   UT_ASSERT_EQ_INT(1, ut_values[4]);
}

static void supports_literal_first_and_suppressed_size_cell(void)
{
   char format[] = "!";
   reset_display("", format, 1);
   abCurrentFormatLiteral[0] = TRUE;
   utt_ODEditDisplayPermaliteral(EDIT_FLAG_SHOW_SIZE);
   UT_ASSERT_EQ_INT(1, ut_call_count);
   UT_ASSERT_EQ_INT('P', ut_calls[0]);
   UT_ASSERT_EQ_INT('!', ut_values[0]);
}

static void starts_after_existing_input_and_flushes_final_blanks(void)
{
   char format[] = "AAAA";
   reset_display("aa", format, 4);
   utt_ODEditDisplayPermaliteral(0);
   UT_ASSERT_EQ_INT(1, ut_call_count);
   UT_ASSERT_EQ_INT('R', ut_calls[0]);
   UT_ASSERT_EQ_INT(3, ut_values[0]);
}

static const UTTestCase ut_cases[] = {
   {"permaliteral batching", batches_blanks_around_literals_and_size_cell},
   {"literal first", supports_literal_first_and_suppressed_size_cell},
   {"existing input", starts_after_existing_input_and_flushes_final_blanks}
};
