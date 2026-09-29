static const char ut_city_formats[] = "Cc";
static const char ut_alpha_formats[] = "AaLlMmUu";
static const char ut_date_formats[] = "Dd";
static const char ut_hex_formats[] = "Hh";
static const char ut_phone_formats[] = "Tt";
static const char ut_text_formats[] = "Xx";
static const char ut_filename_formats[] = "FfWw";
static const char ut_filename_symbols[] = ":.?*#$&'(>-@_!{}~";
static const char ut_yes_no_formats[] = "Yy";
static const char ut_yes_no_values[] = "ynYN";

static BOOL valid_for(char format, char entered)
{
   char format_string[2];
   format_string[0] = format;
   format_string[1] = '\0';
   pszCurrentFormat = format_string;
   anCurrentFormatOffset[0] = 0;
   abCurrentFormatLiteral[0] = FALSE;
   return utt_ODEditIsCharValidForPos(entered, 0);
}

static void handles_literal_positions(void)
{
   char format[] = "!";
   pszCurrentFormat = format;
   anCurrentFormatOffset[0] = 0;
   abCurrentFormatLiteral[0] = TRUE;
   UT_ASSERT(utt_ODEditIsCharValidForPos('!', 0));
   UT_ASSERT(!utt_ODEditIsCharValidForPos('?', 0));
}

static void handles_numeric_printable_and_float_fields(void)
{
   UT_ASSERT(valid_for('#', '0'));
   UT_ASSERT(!valid_for('#', '/'));
   UT_ASSERT(!valid_for('#', ':'));

   UT_ASSERT(valid_for('%', '5'));
   UT_ASSERT(valid_for('%', ' '));
   UT_ASSERT(!valid_for('%', '/'));
   UT_ASSERT(!valid_for('%', ':'));

   UT_ASSERT(valid_for('9', '5'));
   UT_ASSERT(valid_for('9', '.'));
   UT_ASSERT(valid_for('9', '+'));
   UT_ASSERT(valid_for('9', '-'));
   UT_ASSERT(!valid_for('9', '/'));
   UT_ASSERT(!valid_for('9', ':'));
   UT_ASSERT(!valid_for('9', 'x'));

   UT_ASSERT(valid_for('*', ' '));
   UT_ASSERT(!valid_for('*', 31));
}

static void handles_city_and_alpha_fields(void)
{
   unsigned index;
   for(index = 0; ut_city_formats[index] != '\0'; ++index)
      UT_ASSERT(valid_for(ut_city_formats[index], 'A'));
   UT_ASSERT(valid_for('C', 'Z'));
   UT_ASSERT(valid_for('C', 'a'));
   UT_ASSERT(valid_for('C', 'z'));
   UT_ASSERT(valid_for('C', ' '));
   UT_ASSERT(valid_for('C', ','));
   UT_ASSERT(valid_for('C', '.'));
   UT_ASSERT(valid_for('C', '*'));
   UT_ASSERT(valid_for('C', '?'));
   UT_ASSERT(!valid_for('C', '@'));
   UT_ASSERT(!valid_for('C', '['));
   UT_ASSERT(!valid_for('C', '`'));
   UT_ASSERT(!valid_for('C', '{'));

   for(index = 0; ut_alpha_formats[index] != '\0'; ++index)
      UT_ASSERT(valid_for(ut_alpha_formats[index], 'A'));
   UT_ASSERT(valid_for('A', 'Z'));
   UT_ASSERT(valid_for('A', 'a'));
   UT_ASSERT(valid_for('A', 'z'));
   UT_ASSERT(valid_for('A', ' '));
   UT_ASSERT(!valid_for('A', '@'));
   UT_ASSERT(!valid_for('A', '['));
   UT_ASSERT(!valid_for('A', '`'));
   UT_ASSERT(!valid_for('A', '{'));
}

static void handles_date_hex_phone_and_alphanumeric_fields(void)
{
   unsigned index;
   for(index = 0; ut_date_formats[index] != '\0'; ++index)
      UT_ASSERT(valid_for(ut_date_formats[index], '0'));
   UT_ASSERT(valid_for('D', '9'));
   UT_ASSERT(valid_for('D', '-'));
   UT_ASSERT(valid_for('D', '/'));
   UT_ASSERT(!valid_for('D', '.'));
   UT_ASSERT(!valid_for('D', ':'));
   UT_ASSERT(!valid_for('D', 'x'));

   for(index = 0; ut_hex_formats[index] != '\0'; ++index)
      UT_ASSERT(valid_for(ut_hex_formats[index], '0'));
   UT_ASSERT(valid_for('H', '9'));
   UT_ASSERT(valid_for('H', 'A'));
   UT_ASSERT(valid_for('H', 'F'));
   UT_ASSERT(valid_for('H', 'a'));
   UT_ASSERT(valid_for('H', 'f'));
   UT_ASSERT(!valid_for('H', '/'));
   UT_ASSERT(!valid_for('H', ':'));
   UT_ASSERT(!valid_for('H', '@'));
   UT_ASSERT(!valid_for('H', 'G'));
   UT_ASSERT(!valid_for('H', '`'));
   UT_ASSERT(!valid_for('H', 'g'));

   for(index = 0; ut_phone_formats[index] != '\0'; ++index)
      UT_ASSERT(valid_for(ut_phone_formats[index], '0'));
   UT_ASSERT(valid_for('T', '9'));
   UT_ASSERT(valid_for('T', '-'));
   UT_ASSERT(valid_for('T', '('));
   UT_ASSERT(valid_for('T', ')'));
   UT_ASSERT(valid_for('T', ' '));
   UT_ASSERT(valid_for('T', '+'));
   UT_ASSERT(!valid_for('T', '/'));
   UT_ASSERT(!valid_for('T', ':'));
   UT_ASSERT(!valid_for('T', 'x'));

   for(index = 0; ut_text_formats[index] != '\0'; ++index)
      UT_ASSERT(valid_for(ut_text_formats[index], 'A'));
   UT_ASSERT(valid_for('X', 'Z'));
   UT_ASSERT(valid_for('X', 'a'));
   UT_ASSERT(valid_for('X', 'z'));
   UT_ASSERT(valid_for('X', '0'));
   UT_ASSERT(valid_for('X', '9'));
   UT_ASSERT(valid_for('X', ' '));
   UT_ASSERT(!valid_for('X', '@'));
   UT_ASSERT(!valid_for('X', '['));
   UT_ASSERT(!valid_for('X', '`'));
   UT_ASSERT(!valid_for('X', '{'));
}

static void handles_filename_fields_and_each_symbol(void)
{
   unsigned index;
   for(index = 0; ut_filename_formats[index] != '\0'; ++index)
      UT_ASSERT(valid_for(ut_filename_formats[index], 'A'));
   UT_ASSERT(valid_for('F', 'Z'));
   UT_ASSERT(valid_for('F', '0'));
   UT_ASSERT(valid_for('F', '9'));
   UT_ASSERT(valid_for('F', 'a'));
   UT_ASSERT(valid_for('F', 'z'));
   for(index = 0; ut_filename_symbols[index] != '\0'; ++index)
      UT_ASSERT(valid_for('F', ut_filename_symbols[index]));
   UT_ASSERT(valid_for('F', DIRSEP));
   UT_ASSERT(!valid_for('F', '^'));
   UT_ASSERT(!valid_for('F', '['));
#ifdef ODPLAT_NIX
   UT_ASSERT(!valid_for('F', '\\'));
#else
   UT_ASSERT(!valid_for('F', '/'));
#endif
   UT_ASSERT(!valid_for('F', '`'));
   UT_ASSERT(!valid_for('F', '|'));
   UT_ASSERT(!valid_for('F', ' '));
}

static void handles_yes_no_and_unrecognized_formats(void)
{
   unsigned index;
   unsigned value;
   for(index = 0; ut_yes_no_formats[index] != '\0'; ++index)
      for(value = 0; ut_yes_no_values[value] != '\0'; ++value)
         UT_ASSERT(valid_for(ut_yes_no_formats[index],
            ut_yes_no_values[value]));
   UT_ASSERT(!valid_for('Y', 'x'));
   UT_ASSERT(valid_for('!', 1));
}

static const UTTestCase ut_cases[] = {
   {"literal positions", handles_literal_positions},
   {"numeric and printable formats", handles_numeric_printable_and_float_fields},
   {"city and alpha formats", handles_city_and_alpha_fields},
   {"date hex phone and text formats", handles_date_hex_phone_and_alphanumeric_fields},
   {"filename formats", handles_filename_fields_and_each_symbol},
   {"yes-no and default formats", handles_yes_no_and_unrecognized_formats}
};
